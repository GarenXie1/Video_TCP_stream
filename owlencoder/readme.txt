1.文件介绍：
lib	修改的库
test	示例源码,在小机上编译

2.示例演示：
a.将lib目录下*.so库copy到小机的/usr/lib下；
b.将test拷贝到小机任意目录，执行make,生成omx_vce_test；
c.拷贝一个640*480的yuv420sp的数据到小机，并改名为bus_vga.yuv；
d.执行即omx_vce_test可得到一个out.avc的h264码流文件；

3.注意事项
a.码流文件out.avc可以用eseye播放；
b.因为videoEncTest.h中定义了：
#define  Test_Frame_Nums               30      /*测试帧数*/
所以最多只能编30帧，可以自己修改；
c.因为测试程序读入的是yuv420sp的文件，所以数据源格式要对应。
在videoEncTest.c中的main函数里有：
paraConf.szInFile = "bus_vga.yuv";	//input file name
paraConf.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;	//input format, such as YUV420SP
如果输入源格式为yuv420p，请修改为：
paraConf.eColorFormat = OMX_COLOR_FormatYUV420Planar;