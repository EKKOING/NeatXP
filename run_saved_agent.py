from time import sleep
import pymongo
import json
import pickle
from neat import nn
from GANet import GANet

try:
    with open('creds.json') as f:
        creds = json.load(f)
except FileNotFoundError:
    print('creds.json not found!')
    exit()

db_string = creds['mongodb']
client = pymongo.MongoClient(db_string)
db = client.NEAT
collection = db.genomes

generation = input("Generation Number: ")
try:
    generation = int(generation)
except ValueError:
    print('Generation must be an integer!')
    exit()

algorithm = input("Algorithm (GA/NEAT): ")
if algorithm == 'GA' or algorithm == 'ga':
    algorithm = 'ga'
elif algorithm == 'NEAT' or algorithm == 'neat':
    algorithm = 'neat'
else:
    print('Invalid algorithm!')
    exit()

num_agents = collection.count_documents({'generation': generation, 'algo': algorithm, 'genome': {'$ne': None}})
while num_agents == 0:
    generation += 1
    print(f'Searching for {algorithm}-{generation}!')
    num_agents = collection.count_documents({'generation': generation, 'algo': algorithm, 'genome': {'$ne': None}})

print(f'{num_agents} agents found in generation {generation}!')

from shellbot import ShellBot
print('Starting Bot!')
sb = ShellBot()
sb.start()

print('==== RUN START ====')
while True:
    try:
        for genome in collection.find({'generation': generation, 'algo': algorithm, 'genome': {'$ne': None}}).sort('fitness', pymongo.DESCENDING):
            print(f'Using {algorithm} {genome["generation"]}-{genome["individual_num"]}!')
            sb.net = pickle.loads(genome['genome'])
            sleep(60)
    except KeyboardInterrupt:
        print('==== Exit Request Received! ====')
        break
