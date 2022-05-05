from shellbot import ShellBot
from time import sleep
import pymongo
import json
from bson.binary import Binary
import pickle
from neat import nn
from random import randint

eval_length = 15

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

print('Starting Bot!')
sb = ShellBot()
sb.start()

while True:
    try:
        genome = collection.find_one({'started_eval': True, 'finished_eval': True}).sort([('fitness', pymongo.DESCENDING), ('bonus', pymongo.DESCENDING)])

        net: nn.FeedForwardNetwork = pickle.loads(genome['genome'])
        generation = genome['generation']
        individual_num = genome['individual_num']
        sb.nn = net
        print(
            f'Generation {generation} number {individual_num} loaded!')
        sleep(15)
    except Exception as e:
        print(f'Error: {e}')
        print('==================')
    sleep(1)