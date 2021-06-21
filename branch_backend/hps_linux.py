import socket

run = True
value = ""
valread = ""

s = socket.socket()

s.connect(("localhost", 8080))

print("Sending hello")
s.send("HÆLLO HOUSEKEEPINHG".encode())

valread = s.recv(1024)
print("Hello message recieved: ", valread)

while(run):
    value = input("skriv\n")
    s.send(value.encode())
    if value == "exit":
        run = False
    if value[0] == "a":
        valread = s.recv(1024)
        print(str(valread, "utf-8"))


s.close()

##123