1.�ļ����ܣ�
lib	�޸ĵĿ�
test	ʾ��Դ��,��С���ϱ���

2.ʾ����ʾ��
a.��libĿ¼��*.so��copy��С����/usr/lib�£�
b.��test������С������Ŀ¼��ִ��make,����omx_vce_test��
c.����һ��640*480��yuv420sp�����ݵ�С����������Ϊbus_vga.yuv��
d.ִ�м�omx_vce_test�ɵõ�һ��out.avc��h264�����ļ���

3.ע������
a.�����ļ�out.avc������eseye���ţ�
b.��ΪvideoEncTest.h�ж����ˣ�
#define  Test_Frame_Nums               30      /*����֡��*/
�������ֻ�ܱ�30֡�������Լ��޸ģ�
c.��Ϊ���Գ���������yuv420sp���ļ�����������Դ��ʽҪ��Ӧ��
��videoEncTest.c�е�main�������У�
paraConf.szInFile = "bus_vga.yuv";	//input file name
paraConf.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;	//input format, such as YUV420SP
�������Դ��ʽΪyuv420p�����޸�Ϊ��
paraConf.eColorFormat = OMX_COLOR_FormatYUV420Planar;