
import time
import os
import socket
import binascii

# IP8 as TCP client , 是无法与电脑通信，因为无法连接上 PC ，但ping 是正常的.
'''
address = ('192.168.8.127', 8899)
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print("socket.socket ...")
s.connect(address)
'''
address = ('', 8899)
s=socket.socket(socket.AF_INET,socket.SOCK_STREAM,0)
s.bind(address)
s.listen(128)
newSocket,clientAddress = s.accept()
print(clientAddress)

'''
# 简单测试 TCP 接收，发送是否成功.
while True:
    newSocket.sendto(b"I am client", clientAddress)
    data=newSocket.recv(10)
    print("address -> %s data -> %s , len -> %d" %(clientAddress,data,len(data)))
    time.sleep(2)
'''
data_open=os.open("out.avc",os.O_RDONLY)
count=0
while True:
    data=os.read(data_open,1024)
    if len(data)>0:
        newSocket.sendto(data, clientAddress)
    print(len(data))
    if len(data)==0:
        break



