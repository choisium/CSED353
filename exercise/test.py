from socket import *
import sys, pdb

serverName = 'tomahawk.postech.ac.kr'
serverPort = int(50001)

clientSocket = socket(AF_INET, SOCK_DGRAM)
sentence = '20160169'
clientSocket.sendto(sentence.encode(), (serverName, serverPort))
modifiedSentence, serverAddress = clientSocket.recvfrom(2048)

print('From Server: ', modifiedSentence.decode())

clientSocket.close()

# from socket import *
# import sys, pdb

# serverName = 'tomahawk.postech.ac.kr'
# serverPort = int(50001)

# clientSocket = socket(AF_INET, SOCK_STREAM)
# clientSocket.connect((serverName,serverPort))

# sentence = '20160169'
# # breakpoint() # bp2
# clientSocket.send(sentence.encode())
# modifiedSentence = clientSocket.recv(1024)

# print('From Server: ', modifiedSentence.decode())
# # breakpoint() # bp4 or bp5
# clientSocket.close()