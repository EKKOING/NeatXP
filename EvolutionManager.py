from shellbot import ShellBot
import json
import pickle
import pprint as pp
from os import walk, mkdir
from time import sleep, time

import numpy as np

## Script Settings
pop_size = 9
mutation_chance = 0.04
starting_file = 'fully_trained.pickle'
exp_dir = './ga7'
game_length = 30
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
    weights_flat = genome[:-5]
    biases_flat = genome[-5:]
    weights = weights_flat.reshape(15, 5)
    biases = biases_flat.reshape(1, 5)
    network = {'weights': weights, 'biases': biases}
    return network

def pad_generation(gen_num: int) -> str:
    if gen_num < 10:
        return '00' + str(gen_num)
    elif gen_num < 100:
        return '0' + str(gen_num)
    return str(gen_num)

def get_variance_multiplier() -> float:
    return 1.0 / (1 + (generation / 500.0))

## Learning Functions
def mutate(individual: np.ndarray, mutation_chance: float = 0.05, variance: float = 1):
    for idx, num in enumerate(individual):
        if np.random.random() < mutation_chance:
            individual[idx] = (np.random.randn() * variance) + num
        individual[idx] = min(max(individual[idx], -50), 50)
    return individual

def crossover(individual1: np.ndarray, individual2: np.ndarray):
    crossover_point = np.random.randint(0, len(individual1))
    return np.concatenate((individual1[:crossover_point], individual2[crossover_point:])), np.concatenate((individual2[:crossover_point], individual1[crossover_point:]))

def evaluate_individual(individual: np.ndarray, name: str) -> float:
    network = genome_to_network(individual)
    with open(exp_dir + '/' + name, 'w+b') as f:
        pickle.dump(network, f)
    bot.reset()
    bot.load_pickle(exp_dir + '/' + name)
    while bot.reset_now == True and not bot.isAlive:
        sleep(0.05)
    sleep(game_length)
    return bot.get_score()
    ##return (np.max(individual) - np.min(individual))

def evaluate_population(population: list):
    scores = []
    for idx, individual in enumerate(population):
        #if idx == 0 and generation != 1:
            #print('Skipping King!')
            #scores.append(king_score)
            #continue
        print(f'Evaluating Individual {idx}')
        score = evaluate_individual(individual, f'{pad_generation(generation)}_{idx}.pickle')
        print(f'Score: {round(score, 1)}')
        scores.append(score)
    avg = np.average(scores)
    scores_range = np.max(scores) - np.min(scores)
    return scores, avg, scores_range

def crossover_population(population: list, scores: list) -> list:
    new_population = []
    total_scores = np.sum(scores)
    while len(new_population) < pop_size - 1:
        rand_one = np.random.rand()
        running_total = 0.0
        parent1 = 0
        for idx, individual in enumerate(population):
            if rand_one < scores[idx] / total_scores + running_total:
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

variance_mod = get_variance_multiplier()

population = []
load_failed = True
try:
    print('Loading Population from Saved Files')
    temp_pop = []
    for filename in current_saves:
        if not filename.endswith('.pickle'):
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
    print('Generating new population from provided base file!')
    with open(starting_file, 'rb') as f:
        base_genome = pickle.load(f)
        base_genome = network_to_genome(base_genome)
        population = generate_population_from_base(base_genome, pop_size)
    print('Population generated!')
    ##pp.pprint(population[0])
pp.pprint(genome_to_network(population[0])['biases'])
pp.pprint(genome_to_network(mutate(population[0], 0.5, 0.01))['biases'])

print('Starting Bot!')
bot = ShellBot("GA1")
bot.load_pickle(starting_file)
bot.start()

## Give Us Enough Time to Type Password In
for idx in range(0, 10):
    print(f'Grant Operator Perms in Next {10 - idx} Seconds!!!')
    sleep(1)


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



