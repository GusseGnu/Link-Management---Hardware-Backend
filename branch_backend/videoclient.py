import socket

msgFromClient = "EF"

print(msgFromClient)

bytesToSend = str.encode(msgFromClient)

serverAddressPort = ("localhost", 8888)

bufferSize = 1024

# Create a UDP socket at client side

UDPClientSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)

# Send to server using created UDP socket

UDPClientSocket.sendto(bytesToSend, serverAddressPort)

msgFromServer = UDPClientSocket.recvfrom(bufferSize)

msg = "Message from Server {}".format(msgFromServer[0])

print(msg)


bytesToSend = str.encode("99")

UDPClientSocket.sendto(bytesToSend, serverAddressPort)