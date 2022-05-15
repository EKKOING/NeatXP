import json
import os
import pickle
import pprint as pp
import sys
from datetime import datetime, timedelta
from math import ceil, floor
from os import mkdir, walk
from time import sleep, time

import numpy as np
import pymongo
from bson.binary import Binary

import wandb
from GANet import GANet

wandb.init(project="NeatXP", entity="ekkoing", resume=False)
wandb.config = {
    "pop": 100,
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

## Script Settings
pop_size = 99
mutation_chance = 0.05
exp_dir = './ga'
##

generation = 1
average_scores = []
king = None
king_score = 0
king_streak = 0
king_scores = []
score_ranges = []
gen_maxes = []
variance_mod = 1.0

try:
    mkdir(exp_dir)
except FileExistsError:
    pass

log: dict = {}

def save_log():
    log = {'generation': generation, 'king_score': king_score, 'king_scores': king_scores, 'average_scores': average_scores, 'score_ranges': score_ranges, 'gen_maxes': gen_maxes}
    with open(exp_dir + '/log.json', 'w') as f:
        json.dump(log, f)

def load_log():
    with open(exp_dir + '/log.json', 'r') as f:
        log = json.load(f)
    print('Last Generation in Log: ' + str(log['generation']))
    return log

try:
    log = load_log()
    generation = log['generation']
    king_score = log['king_score']
    king_scores = log['king_scores']
    average_scores = log['average_scores']
    score_ranges = log['score_ranges']
    gen_maxes = log['gen_maxes']
    print('Loaded Log from Directory!')
except FileNotFoundError:
    print('No log found in directory')
    print('Generating new log')
    save_log()

current_saves = []
for (dirpath, dirnames, filenames) in walk(exp_dir):
    current_saves.extend(filenames)
    break
current_saves.sort()

## Utility Functions
def generate_population_from_base(base: np.ndarray, pop_size: int):
    population = []
    population.append(base)
    king = base
    for _ in range(pop_size - 1):
        population.append(mutate(base, 1, 1))
    return population

def network_to_genome(network: dict):
    weights = network['weights']
    biases = network['biases']
    genome = np.append(weights.flatten(), biases.flatten())
    return genome

def genome_to_network(genome: np.ndarray):
    weights_flat = genome[:-8]
    biases_flat = genome[-8:]
    weights = weights_flat.reshape(22, 8)
    biases = biases_flat.reshape(1, 8)
    network = {'weights': weights, 'biases': biases}
    return network

def pad_generation(gen_num: int) -> str:
    if gen_num < 10:
        return '00' + str(gen_num)
    elif gen_num < 100:
        return '0' + str(gen_num)
    return str(gen_num)

def pad_individual(ind_num: int) -> str:
    if ind_num < 10:
        return '0' + str(ind_num)
    return str(ind_num)

def get_variance_multiplier() -> float:
    return 1.0 / (1 + (generation / 500.0))

## Learning Functions
def mutate(individual: np.ndarray, mutation_chance: float = 0.05, variance: float = 1):
    for idx, num in enumerate(individual):
        if np.random.random() < mutation_chance:
            individual[idx] = (np.random.randn() * variance) + num
        individual[idx] = min(max(individual[idx], -30), 30)
    return individual

def crossover(individual1: np.ndarray, individual2: np.ndarray):
    crossover_point = np.random.randint(0, len(individual1))
    return np.concatenate((individual1[:crossover_point], individual2[crossover_point:])), np.concatenate((individual2[:crossover_point], individual1[crossover_point:]))

def evaluate_population(population: list):
    num_workers = 0
    gen_start = datetime.now()
    collection = db.genomes

    for idx, individual in enumerate(population):
        net = genome_to_network(individual)
        with open(exp_dir + '/' + f'{pad_generation(generation)}-{pad_individual(idx)}.xpainet', 'w+b') as f:
            pickle.dump(net, f)
        net = GANet(net['weights'], net['biases'])
        net = Binary(pickle.dumps(net))
        db_entry = {
            'genome': net,
            'individual_num': idx,
            'generation': generation,
            'algo': 'ga',
            'fitness': 0,
            'bonus': 0,
            'started_eval': False,
            'started_at': None,
            'finished_eval': False,
        }
        if collection.find_one({'generation': generation, 'individual_num': idx, 'algo': 'ga'}):
            collection.update_one(
                {
                    'generation': generation,
                    'individual_num': idx,
                    'algo': 'ga'
                }, {
                    '$set': {
                        'started_eval': False, 'finished_eval': False, 'fitness': 0, 'bonus': 0, 'genome': net, 'started_at': None
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
        uncompleted_training = collection.count_documents({'generation': generation,
                                                            'finished_eval': False, 'algo': 'ga'})
        started_training = collection.count_documents(
            {'generation': generation, 'started_eval': True, 'finished_eval': False, 'algo': 'ga'})
        finished_training = collection.count_documents(
            {'generation': generation, 'finished_eval': True, 'algo': 'ga'})

        if num_workers < started_training:
            num_workers = started_training

        check_eval_status(collection)

        if uncompleted_training == 0:
            break

        if not first_sleep:
            delete_last_lines(6)
        else:
            first_sleep = False
        print(f'=== {datetime.now().strftime("%H:%M:%S")} ===\n{uncompleted_training} genomes still need to be evaluated\n{started_training} currently being evaluated\n{finished_training} have been evaluated')
        progress_bar(finished_training, started_training, len(population))
        secs_tg = (ceil(uncompleted_training * 60 / (num_workers + 1)))
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
        if num_workers == 0 and secs_passed % 60 == 0 and no_alert:
            wandb.alert(
                title="No Worker Nodes Available",
                text="No workers currently running.",
            )
            no_alert = False
            
    fit_list = []
    bonus_list = []
    score_list = []
    for idx, individual in enumerate(population):
        results = collection.find_one({'individual_num': idx, 'generation': generation, 'algo': 'ga'})
        fit_list.append(results['fitness'] + results['bonus'])
        bonus_list.append(results['bonus'])
        score_list.append(results['fitness'])

    avg = np.average(fit_list)
    scores_range = np.max(fit_list) - np.min(fit_list)
    wandb.log({
        "Generation": generation,
        "Average Fitness": np.mean(fit_list),
        "Best Fitness": max(fit_list),
        "Num Workers": num_workers,
        "Fitness SD": np.std(fit_list),
        "Min Fitness": min(fit_list),
        "Median Fitness": np.median(fit_list),
        "Average Bonus": np.mean(bonus_list),
        "Bonus SD": np.std(bonus_list),
        "Best Bonus": max(bonus_list),
        "Median Bonus": np.median(bonus_list),
        "Min Bonus": min(bonus_list),
        "Average Score": np.mean(score_list),
        "Score SD": np.std(score_list),
        "Best Score": max(score_list),
        "Median Score": np.median(score_list),
        "Min Score": min(score_list),
        "Time Elapsed": (datetime.now() - gen_start).total_seconds(),
        "Time": datetime.now()
    })
    return fit_list, avg, scores_range

def crossover_population(population: list, scores: list) -> list:
    new_population = []
    total_scores = np.sum(scores)
    while len(new_population) < pop_size - 1:
        rand_one = np.random.rand()
        running_total = 0.0
        parent1 = 0
        for idx, individual in enumerate(population):
            if rand_one < (scores[idx] / total_scores) + running_total:
                parent1 = idx
                break
            running_total += scores[idx] / total_scores
        rand_one = np.random.rand()
        running_total = 0.0
        parent2 = 0
        for idx, individual in enumerate(population):
            if rand_one < scores[idx] / total_scores + running_total and idx != parent1:
                parent1 = idx
                break
            running_total += scores[idx] / total_scores
        child1, child2 = crossover(population[parent1], population[parent2])
        new_population.append(child1)
        new_population.append(child2)
    return new_population

def mutate_population(population: list) -> list:
    for idx, individual in enumerate(population):
        population[idx] = mutate(individual, mutation_chance, variance_mod)
    return population

def check_eval_status(collection):
    for genome in collection.find({'generation': generation, 'algo': 'ga',  'started_eval': True, 'finished_eval': False}):
        genome_id = genome['_id']
        individual_num = genome['individual_num']
        started_at = genome['started_at']
        if (datetime.now() - started_at) > timedelta(minutes=5):
            delete_last_lines(5)
            print(
                f'Genome {generation}-{individual_num} has been running for 5 minutes, marking for review!\n\n\n\n\n\n\n')
            collection.update_one({'_id': genome_id}, {
                                    '$set': {'started_eval': False, 'finished_eval': False}})
            wandb.alert(
                title="Evaluation Timeout",
                text=f"Genome {generation}-{individual_num} has been running for 5 minutes, marking for review",
            )
    for genome in collection.find({'generation': generation, 'algo': 'ga', 'started_eval': True, 'finished_eval': True, 'fitness': 0}):
        genome_id = genome['_id']
        individual_num = genome['individual_num']
        delete_last_lines(5)
        print(f'Genome {generation}-{individual_num}  did nothing!\n\n\n\n\n\n\n')
        collection.update_one({'_id': genome_id}, {
                                '$set': {'fitness': -20}})

variance_mod = get_variance_multiplier()

population = []
load_failed = True
try:
    print('Loading Population from Saved Files')
    temp_pop = []
    for filename in current_saves:
        if not filename.endswith('.xpainet'):
            continue
        load_gen, load_idx = filename.split('_')[0], filename.split('_')[1].split('.')[0]
        if load_gen != pad_generation(generation - 1):
            continue
        print(f'Loading {filename}!')
        with(open(exp_dir + '/' + filename, 'rb')) as f:
            individual = pickle.load(f)
            ##individual['weights'][12][1] = 14 * np.random.randn() * 0.5 + 14
            individual = network_to_genome(individual)
            temp_pop.append(individual)
            if load_idx == '0':
                print('Found King!')
                king = individual
    load_failed = False
    if len(temp_pop) == 0:
        print('No Population Found in Directory!')
        load_failed = True
    else:
        print(f'Loaded {len(temp_pop)} individuals from directory')
        ##temp_pop = mutate_population(temp_pop)
        if king is not None:
            temp_pop[0] = king
        population = temp_pop
except FileNotFoundError:
    print('No Saves Found or Load Failed!!')
    load_failed = True

if len(current_saves) == 0 or generation == 1 and load_failed:
    print('Generating new population from scratch!')
    base_genome = {
        'weights': np.random.randn(22, 8) * 0.25,
        'biases': np.random.randn(1, 8) * 0.25,
    }
    base_genome = network_to_genome(base_genome)
    population = generate_population_from_base(base_genome, pop_size)
    print('Population generated!')
    ##pp.pprint(population[0])
##pp.pprint(genome_to_network(population[0])['biases'])
##pp.pprint(genome_to_network(mutate(population[0], 0.5, 0.01))['biases'])

for gen_num in range(generation, 999):
    generation = gen_num
    save_log()
    print(f'Starting Evaluation of Generation: {generation}')
    scores, avg, range = evaluate_population(population)
    print(f'Evaluation Complete!')
    average_scores.append(avg)
    score_ranges.append(range)
    print(f'Average Score: {round(avg, 2)}')
    print(f'Score Range: {round(range, 2)}')
    scores[0] = scores[0] * 1.05
    gen_max = np.max(scores)
    gen_maxes.append(gen_max)
    print(f'Gen Max: {round(gen_max, 2)}')
    king = population[np.argmax(scores)]
    if king_score < gen_max:
        king_score = gen_max
        king_streak = 0
    else:
        king_streak += 1
    king_scores.append(king_score)
    print(f'King Score: {round(king_score, 2)}')
    print(f'King Streak: {king_streak}')
    population = crossover_population(population, scores)
    variance_mod = get_variance_multiplier()
    population = mutate_population(population)
    population.insert(0, king)
    print(f'=========================')



