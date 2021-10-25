
# VLC 串流 .mp4 文件，作为 HTTP server.
# python: 作为 HTTP Client , 从 HTTP 接收到，直接 写入文件，能正常显示.
# Win10 , 通过 VLC 正常显示 test.mp4 文件.

import time
import socket
import http.client

conn=http.client.HTTPConnection("192.168.8.127",8080,timeout=10000)
conn.request("GET","/stream")
response = conn.getresponse()
print(response.status, response.reason)

mp4 = open("test.mp4", "ab")
while True:
    chunk = response.read(10000)
    mp4.write(chunk)
    print(len(chunk))
    if not chunk:
        break
mp4.close()




