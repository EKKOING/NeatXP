import json
from time import sleep

import pymongo

with open('creds.json') as f:
    creds = json.load(f)

from_gen = 0
to_gen = 400
preserve_top = 10

client = pymongo.MongoClient(creds['mongodb'])
db = client.NEAT

for generation in range(from_gen, to_gen):
    print(f'Purging Generation {generation}')
    for genome in db.genomes.find({'generation': generation}).sort('fitness', pymongo.DESCENDING).skip(preserve_top):
        if genome['genome']:
            print(f'Purging {genome["generation"]}-{genome["individual_num"]}')
            db.genomes.update_one({'_id': genome['_id']}, {'$set': {'genome': None}})
    ##sleep(1)

for genome in db.genomes.find({'algo': None}):
    print(f'Fixing {genome["generation"]}-{genome["individual_num"]}')
    db.genomes.update_one({'_id': genome['_id']}, {'$set': {'algo': 'NEAT'}})


