from socket import *
import sys, pdb

serverName  = 'localhost'
serverPort  = int(sys.argv[1])
serverPort2 = int(sys.argv[2])

# clientSocket = socket(AF_INET, SOCK_STREAM)
# clientSocket.connect((serverName,serverPort))  # to be replaced

clientSocket1 = socket(AF_INET,SOCK_STREAM)
clientSocket1.bind(('', 50010))
clientSocket1.connect((serverName,serverPort))

clientSocket2 = socket(AF_INET,SOCK_STREAM)
clientSocket2.setsockopt(SOL_SOCKET, SO_REUSEPORT, 1)
clientSocket2.bind(('', 50010))
clientSocket2.connect((serverName,serverPort2))

sentence = input('Input lowercase sentence:')
clientSocket1.send(sentence.encode())
modifiedSentence = clientSocket1.recv(1024)
print('From Server1: ', modifiedSentence.decode())
clientSocket1.close()

clientSocket2.send(sentence.encode())
modifiedSentence = clientSocket2.recv(1024)
print('From Server2: ', modifiedSentence.decode())
clientSocket2.close()
