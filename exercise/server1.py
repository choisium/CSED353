from socket import *
import sys, pdb
import os

serverPort = int(sys.argv[1])

serverSocket = socket(AF_INET,SOCK_STREAM)
breakpoint() #bp1
serverSocket.bind(('',serverPort))
breakpoint() #bp3
serverSocket.listen(1)
breakpoint() #bp4

print('The server is ready to receive')

while True:
    connectionSocket, addr = serverSocket.accept()
    breakpoint() #bp6
    sentence = connectionSocket.recv(1024).decode()
    capitalizedSentence = sentence.upper()
    connectionSocket.send(capitalizedSentence.encode())
    
    breakpoint() #bp7
    connectionSocket.close()
    breakpoint() #bp9