import os
import neat
import pymongo
import json
from bson.binary import Binary
import pickle
from time import sleep
from datetime import datetime, timedelta

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

##TODO: load generation number from mongodb
class EvolveManager:
    generation = 0

    def __init__(self, config_file, latest=None, log=None):
        self.config_file = config_file
        self.latest = latest
        self.config = neat.Config(neat.DefaultGenome, neat.DefaultReproduction,
                                  neat.DefaultSpeciesSet, neat.DefaultStagnation,
                                  config_file)
        self.p = neat.Checkpointer.restore_checkpoint(latest) if latest else neat.Population(self.config)
        self.p.add_reporter(neat.StdOutReporter(True))
        self.stats = neat.StatisticsReporter()
        self.p.add_reporter(self.stats)
    
    def eval_genomes(self, genomes, config):
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
            collection.insert_one(db_entry)
        sleep(1)
        while True:
            uncompleted_training = collection.count_documents({'generation': self.generation, 
            'finished_eval': False})
            started_training = collection.count_documents({'generation': self.generation, 'started_eval': True, 'finished_eval': False})
            finished_training = collection.count_documents({'generation': self.generation, 'finished_eval': True})
            if uncompleted_training == 0:
                break
            print(f'=== {datetime.now().strftime("%H:%M:%S")} ===')
            print(f'{uncompleted_training} genomes still need to be evaluated')
            print(f'{started_training} currently being evaluated')
            print(f'{finished_training} have been evaluated')
            sleep(30)
        
        for genome_id, genome in genomes:
            key = genome.key
            results = collection.find_one({'key': key})
            genome.fitness = results['fitness']
        self.generation += 1
        
    def run(self, num_gens: int = 0):
        winner = self.p.run(self.eval_genomes, num_gens)

        print(f'Best genome: {winner}')

manager = EvolveManager(config_path, latest=None, log=None)
manager.run(num_gens=3)
