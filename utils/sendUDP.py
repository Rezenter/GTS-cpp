import socket


_s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
#print(_s.sendto((0b00001110000000000000000000000000).to_bytes(4, byteorder='little'), ('192.168.10.56', 8080)))
print(_s.sendto((0b00000000000000000000000000000000).to_bytes(4, byteorder='little'), ('192.168.10.56', 8080)))

print('code OK')
