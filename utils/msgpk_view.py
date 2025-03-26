import json
import msgpack

path = 'd://data//db//debug//raw//00916//ophir.msgpk'
path = 'd://data//db//plasma//ophir//43225.msgpk'

data = []

with open(path, 'rb') as file:
    data.append(msgpack.unpackb(file.read()))

with open('debug.json', 'w') as file:
    json.dump(data, file, indent=2)

print('Code ok')
