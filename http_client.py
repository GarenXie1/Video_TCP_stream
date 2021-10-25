import time
import socket
import http.client
import av
import av.datasets
import cv2

#   提取 .mp4 文件中的 第 100 帧数据
#   参考 pyav demo:
#   https://chenyue.top/2020/03/26/PyAV%E5%BA%93%E4%BD%BF%E7%94%A8%E4%BB%8B%E7%BB%8D/
#   正确 提取 .mp4 文件中的 第 100 帧数据，并使用 YUVplayer 预览正确.
'''
container = av.open("Full_HD_Samsung_LED_Color_Demo_HD_No_sound.mp4")
for frame in container.decode(video=0):
    if  frame.index==100 :
        print("index -> %s , format -> %s" % (frame.index,frame.format))
        yuv = open("YUV420p_1280_720.yuv", "ab")
        yuv.write(frame.to_ndarray())
        yuv.close()
        break
'''


#   播放 mp4 视频:
'''
container = av.open("Full_HD_Samsung_LED_Color_Demo_HD_No_sound.mp4")
while True:
    for frame in container.decode(video=0):
        framergb = frame.to_rgb()
        img = framergb.to_ndarray()
         # img=frame.to_image()
        RGB_img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        cv2.imshow("frame", RGB_img)
        if cv2.waitKey(25) & 0xFF == ord("q"):
            break
    break
'''


#   播放 HTTP 视频流
#
container = av.open("http://192.168.8.127:8080/stream","r")
while True:
    for frame in container.decode(video=0):
        framergb = frame.to_rgb()
        img = framergb.to_ndarray()
         # img=frame.to_image()
        RGB_img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        cv2.imshow("frame", RGB_img)
        if cv2.waitKey(25) & 0xFF == ord("q"):
            break
    break




'''
codec = av.CodecContext.create('h264', 'r')
print(codec)
while True:
    # chunk = fh.read(65536)
    chunk = response.read(200000)
    packets = codec.parse(chunk)
    for packet in packets:
        frames = codec.decode(packet)
        for frame in frames:
            framergb = frame.to_rgb()
            img = framergb.to_ndarray()
            # img=frame.to_image()
            RGB_img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
            cv2.imshow("frame", RGB_img)
            if cv2.waitKey(25) & 0xFF == ord("q"):
                break

    if not chunk:
        break
'''

