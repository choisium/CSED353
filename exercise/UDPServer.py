# from socket import *
# import sys, pdb

# serverPort = int(sys.argv[1])

# serverSocket = socket(AF_INET,SOCK_DGRAM)
# serverSocket.bind(('',serverPort))

# print('The server is ready to receive')

# while True:
#     message, clientAddress = serverSocket.recvfrom(2048)
#     capitalizedSentence = message.decode().upper()
#     breakpoint() # bp2
#     serverSocket.sendto(capitalizedSentence.encode(), clientAddress)

from socket import *
import sys, pdb

serverPort = int(sys.argv[1])

serverSocket = socket(AF_INET,SOCK_STREAM)
serverSocket.bind(('',serverPort))
serverSocket.listen(1)

print('The server is ready to receive')

while True:
    connectionSocket, addr = serverSocket.accept()
    sentence = connectionSocket.recv(1024).decode()
    capitalizedSentence = sentence.upper()
    # breakpoint() # bp3
    connectionSocket.send(capitalizedSentence.encode())
    # breakpoint() # bp4 or bp5
    connectionSocket.close()