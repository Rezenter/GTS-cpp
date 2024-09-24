import socket
import json
import time

_s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)


_timeout = 2  # second before next isAlive request
_start_time = time.time()


class Commands:  # describes all implemented commands
    class Alive:
        _cmd = "alive"
        _args = []

    class Arm:
        _cmd = "arm"
        _args = ["shotn", "isPlasma"]

    class Disarm:
        _cmd = "disarm"
        _args = []

    class Exit:
        _cmd = "exit"
        _args = []

    class Close:
        _cmd = "close"
        _args = []


def send_cmd(cmd, args=None):
    if args is None:
        args = []
    packet = {
        "cmd": cmd._cmd
    }
    if len(cmd._args) != 0:
        if args is None:
            print("No arguments passed, while cmd expects %s." % cmd._args)
            return
        if len(cmd._args) != len(args):
            print("Wrong number of arguments: got %d, expected %d." % (len(args), len(cmd._args)))
            return
        for i in range(len(args)):
            packet[cmd._args[i]] = args[i]
    #print(json.dumps(packet))
    _s.send(bytes(json.dumps(packet), 'UTF8'))


def watchdog():
    global _start_time
    send_cmd(Commands.Alive)
    _start_time = time.time()


def spam():
    while True:
        if (time.time() - _start_time) > _timeout:
            watchdog()
        try:
            data = _s.recv(1024)
            if len(data) > 0:
                converted = json.loads(data)
                print(converted)
        except socket.timeout as msg:
            continue


def connect():
    _s.connect(("localhost", 27015))
    _s.settimeout(0.1)
    print("connected")


def disconnect():
    _s.close()


def read():
    while True:
        try:
            data = _s.recv(1024)
            if len(data) > 0:
                return json.loads(data)
        except socket.timeout:
            continue
    return None
