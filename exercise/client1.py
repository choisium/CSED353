from socket import *
import sys, pdb
import os

serverName = 'localhost'
serverPort = int(sys.argv[1])

clientSocket = socket(AF_INET, SOCK_STREAM)
clientSocket.bind(('',50010))

breakpoint() #bp2
clientSocket.connect((serverName,serverPort))
breakpoint() #bp5

sentence = 'abcde'
clientSocket.send(sentence.encode())
modifiedSentence = clientSocket.recv(1024)

print('From Server: ', modifiedSentence.decode())

breakpoint() #bp8
clientSocket.close()
breakpoint() #bpA