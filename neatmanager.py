import json
import os
import pickle
import sys
from datetime import datetime, timedelta
from math import ceil, floor
from time import sleep

import neat
import numpy as np
import pymongo
from bson.binary import Binary

import wandb

wandb.init(project="NeatXP", entity="ekkoing")
wandb.config = {
    "pop": 50,
    "duration": 60,
}

# Output Utils
CURSOR_UP_ONE = '\x1b[1A'
ERASE_LINE = '\x1b[2K'


def delete_last_lines(n: int = 1) -> None:
    for _ in range(n):
        sys.stdout.write(CURSOR_UP_ONE)
        sys.stdout.write(ERASE_LINE)


def progress_bar(progress: float, in_progress: float, total: float) -> None:
    left = '['
    right = ']'
    bar_length = 30
    fill = '|'
    in_progress_fill = '>'
    percent = progress / total
    in_percent = in_progress / total
    fill_amt = int(round(percent * bar_length))
    fill_str = ''
    for _ in range(fill_amt):
        fill_str += fill
    in_fill_amt = int(round(in_percent * bar_length))
    for _ in range(in_fill_amt):
        fill_str += in_progress_fill
    for _ in range(bar_length - (fill_amt + in_fill_amt)):
        fill_str += ' '
    print(f'{left}{fill_str}{right} {percent:.2%}')


try:
    with open('creds.json') as f:
        creds = json.load(f)
except FileNotFoundError:
    print('creds.json not found!')
    exit()

db_string = creds['mongodb']
client = pymongo.MongoClient(db_string)
db = client.NEAT


local_dir = os.path.dirname(__file__)
config_path = os.path.join(local_dir, 'config')

# TODO: load generation number from mongodb


class EvolveManager:
    generation = 0
    num_workers = 0
    gen_start: datetime

    def __init__(self, config_file: str, latest=None, generation: int = 0):
        self.config_file = config_file
        self.latest = latest
        self.generation = generation
        self.config = neat.Config(neat.DefaultGenome, neat.DefaultReproduction,
                                  neat.DefaultSpeciesSet, neat.DefaultStagnation,
                                  config_file)
        self.p = neat.Checkpointer.restore_checkpoint(
            latest) if latest else neat.Population(self.config)
        self.p.add_reporter(neat.StdOutReporter(False))
        self.checkpointer = neat.Checkpointer(1, filename_prefix='NEAT-')
        self.p.add_reporter(self.checkpointer)

    def eval_genomes(self, genomes, config):
        self.num_workers = 0
        self.gen_start = datetime.now()
        collection = db.genomes
        individual_num = 0
        for genome_id, genome in genomes:
            individual_num += 1
            key = genome.key
            net = neat.nn.FeedForwardNetwork.create(genome, config)
            net = Binary(pickle.dumps(net))
            db_entry = {
                'key': key,
                'genome': net,
                'individual_num': individual_num,
                'generation': self.generation,
                'fitness': 0,
                'started_eval': False,
                'started_at': None,
                'finished_eval': False,
            }
            if collection.find_one({'generation': self.generation, 'individual_num': individual_num}):
                collection.update_one(
                    {
                        'generation': self.generation,
                        'individual_num': individual_num
                    }, {
                        '$set': {
                            'started_eval': False, 'finished_eval': False, 'fitness': 0, 'genome': net, 'started_at': None, 'key': key
                        }
                    }
                )
            else:
                collection.insert_one(db_entry)
        sleep(1)
        first_sleep = True
        secs_passed = 0
        no_alert = True
        secs_since_tg_update = 0
        last_secs_tg = 0
        while True:
            uncompleted_training = collection.count_documents({'generation': self.generation,
                                                               'finished_eval': False})
            started_training = collection.count_documents(
                {'generation': self.generation, 'started_eval': True, 'finished_eval': False})
            finished_training = collection.count_documents(
                {'generation': self.generation, 'finished_eval': True})

            if self.num_workers < started_training:
                self.num_workers = started_training

            self.check_eval_status(collection)

            if uncompleted_training == 0:
                break

            if not first_sleep:
                delete_last_lines(6)
            else:
                first_sleep = False
            print(f'=== {datetime.now().strftime("%H:%M:%S")} ===\n{uncompleted_training} genomes still need to be evaluated\n{started_training} currently being evaluated\n{finished_training} have been evaluated')
            progress_bar(finished_training, started_training, len(genomes))
            secs_tg = (ceil(uncompleted_training * 60 / (self.num_workers + 1)))
            if secs_tg != last_secs_tg:
                secs_since_tg_update = 0
            last_secs_tg = secs_tg
            secs_tg -= secs_since_tg_update
            secs_since_tg_update += 1
            mins_tg = floor(secs_tg / 60)
            secs_tg = round(secs_tg % 60)
            if secs_tg < 10:
                secs_tg = f'0{secs_tg}'
            secs_passed_str = round(secs_passed % 60)
            if secs_passed_str < 10:
                secs_passed_str = f'0{secs_passed_str}'
            print(f'{floor(secs_passed / 60)}:{secs_passed_str} Elapsed - ETA {mins_tg}:{secs_tg} Remaining')
            sleep(1)
            secs_passed += 1
            if self.num_workers == 0 and secs_passed % 60 == 0 and no_alert:
                wandb.alert(
                    title="No Worker Nodes Available",
                    text="No workers currently running.",
                )
                no_alert = False
        fit_list = []
        for genome_id, genome in genomes:
            key = genome.key
            results = collection.find_one({'key': key})
            genome.fitness = results['fitness']
            fit_list.append(results['fitness'])
        self.fit_list = fit_list

    def run(self, num_gens: int = 0):
        winner = self.p.run(self.eval_genomes, num_gens)

        print(f'Best genome: {winner}')

    def check_eval_status(self, collection):
        for genome in collection.find({'generation': self.generation, 'started_eval': True, 'finished_eval': False}):
            genome_id = genome['_id']
            key = genome['key']
            started_at = genome['started_at']
            if (datetime.now() - started_at) > timedelta(minutes=5):
                delete_last_lines(5)
                print(
                    f'Genome {key} has been running for 5 minutes, marking for review!\n\n\n\n\n\n\n')
                collection.update_one({'_id': genome_id}, {
                                      '$set': {'started_eval': False}})
                wandb.alert(
                    title="Evaluation Timeout",
                    text=f"Genome {key} has been running for 5 minutes, marking for review",
                )
        for genome in collection.find({'generation': self.generation, 'started_eval': True, 'finished_eval': True, 'fitness': 0}):
            genome_id = genome['_id']
            key = genome['key']
            delete_last_lines(5)
            print(f'Genome {key} did nothing!\n\n\n\n\n\n\n')
            collection.update_one({'_id': genome_id}, {
                                  '$set': {'fitness': -10}})


manager = EvolveManager(config_path, latest=None, generation=0)
while True:
    try:
        manager.run(num_gens=1)
        wandb.log({
            "Generation": manager.generation,
            "Average Fitness": np.mean(manager.fit_list),
            "Best Fitness": max(manager.fit_list),
            "Num Workers": manager.num_workers,
            "Range": max(manager.fit_list) - min(manager.fit_list),
            "Min": min(manager.fit_list),
            "Median": np.median(manager.fit_list),
            "Time Elapsed": (datetime.now() - manager.gen_start).total_seconds(),
        })
        manager.generation += 1
    except KeyboardInterrupt:
        wandb.alert(
            title="Run Aborted",
            text=f"Run Ended at generation {manager.generation}",
        )
        break
    except Exception as e:
        wandb.alert(
            title="Run Error",
            text=f"Run error at generation {manager.generation} with error {e}",
        )
        print(e)
        raise e
