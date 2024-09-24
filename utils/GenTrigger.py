import pyvisa

class GEN:
    my_instrument = None

    def __init__(self, ip_addr):
        rm = pyvisa.ResourceManager()
        self.my_instrument = rm.open_resource('TCPIP0::%s::INSTR' % ip_addr)
        print(self.my_instrument.query('*IDN?'))

    def trigger(self):
        self.my_instrument.write(':TRIG ')


gen = GEN('192.168.10.106')
gen.trigger()

print('Code ok')
