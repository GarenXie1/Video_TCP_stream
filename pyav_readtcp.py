import os
import av
import av.datasets
import cv2
import time
import socket

'''
# 作为 TCP service 的代码
address = ('', 8899)
s=socket.socket(socket.AF_INET,socket.SOCK_STREAM,0)
s.bind(address)
s.listen(128)
newSocket,clientAddress = s.accept()
#h264_path = 'out.avc'
'''

# 作为 TCP Client 的代码
address = ('192.168.8.149', 8899)
socket_Client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
socket_Client.connect(address)

'''
while True:
    data = socket_Client.recv(10)
    print("data -> %s , len -> %d" %(data , len(data)))
    socket_Client.sendto(b"Hello TCP client",address)
'''

#fh = open(h264_path, 'rb')
codec = av.CodecContext.create('h264', 'r')
while True:
    #chunk = fh.read(65536)
    chunk=socket_Client.recv(1024)
    packets = codec.parse(chunk)
    for packet in packets:
        frames = codec.decode(packet)
        for frame in frames:
            framergb=frame.to_rgb()
            img=framergb.to_ndarray()
            #img=frame.to_image()
            RGB_img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
            cv2.imshow("frame",RGB_img)
            if cv2.waitKey(25) & 0xFF==ord("q"):
                break

            
    if not chunk:
        break


