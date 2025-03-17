import json
import msgpack

#path = 'd:/data/db/plasma/raw/'
path = 'd:/data/db/debug/raw/'
group_count = 8
_ch_count = 2

shotn = 863

shot_folder = '%s%05d_0' % (path, shotn)
FILE_EXT = 'msgpk'

data = []
for board in range(8):
    with open('%s/%s.%s' % (shot_folder, '%s' % board, FILE_EXT), 'rb') as file:
        data.append(msgpack.unpackb(file.read()))
        pass

for event_ind in range(len(data[0])):
    line = '%d ' % data[1][event_ind]['DAC1']
    for board in range(8):
        line += '%.1f ' % (data[board][event_ind]['t'])
        #line += '%d ' % data[board][event_ind]['t_raw']
    print(line)
        #data[event_ind]['t'] = 3.6 + (event_ind - 1)*1000/330
    #with open('%s/%s.%s' % (shot_folder, '%s_new' % board, FILE_EXT), 'wb') as file:
    #    file.write(msgpack.packb(data))

print('Code ok')
