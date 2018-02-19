Here is a simple python client for CS:

import socket
import sys

host = "localhost"
port = 1024
addr = (host,port)

flag = True

print 'username : ',
reponse = sys.stdin.readline().strip().split(':')

username = reponse[0]
bot      = ''
if (len(reponse) > 1):
  bot    = reponse[1]

print 'username : ',username
print 'bot      : ',bot

data_in = ''

while flag:

  client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  client_socket.connect(addr)

  client_socket.sendall((username+chr(0)+bot+chr(0)+data_in[0:-1] +chr(0))) 

  data_out = client_socket.recv(80)

  print data_out
  print '>> ',

  data_in = sys.stdin.readline()

  if (':exit' in data_in):
    break

  client_socket.close() 