from utils import chatter
import time

DB_PATH = 'd:/data/db/'
DEBUG_SHOTS = 'debug/'

shot_filename = "%s%sSHOTN.TXT" % (DB_PATH, DEBUG_SHOTS)
isPlasma = False
with open(shot_filename, 'r') as shotn_file:
    line = shotn_file.readline()
    shotn = int(line)


def increment_shotn():
    global shotn
    with open(shot_filename, 'w') as shotn_file:
        shotn_file.seek(0)
        shotn += 1
        shotn_file.write('%d' % shotn)


print(shotn)

chatter.connect()

increment_shotn()

chatter.send_cmd(chatter.Commands.Arm, [shotn, isPlasma])
print(chatter.read())

time.sleep(0.1)

chatter.disconnect()
