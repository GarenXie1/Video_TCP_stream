/*
 *  OpenCV BSD3 License
 *  videodev2.h BSD License (dual license)
 *  If you raise some license issue here simply don't use it and let us know
 * 
 *  Copyright (C) 2016 the contributors
 *
 *  Luiz Vitor Martinez Cardoso
 *  Jules Thuillier
 *  @lex (avafinger)
 *
 *  gcc cap.c -o cap $(pkg-config --libs --cflags opencv) -lm
 *
 *  gcc -I/usr/src/linux-headers-VERSION/ cap.c -o cap $(pkg-config --libs --cflags opencv) -lm -O3
 *
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>

/* -----------------------------------------------------------------------------
 * BananaPi M64 / Pine64+ (A64) or if you want to control Exposure,Hflip,Vflip
 * -----------------------------------------------------------------------------
 * _V4L2_KERNEL_ should be defined and point to: /usr/src/linux-headers-version
 * 
 * build with: gcc -I/usr/src/linux-headers-3.10.102/ cap.c -o cap $(pkg-config --libs --cflags opencv) -lm -O3
 *
 * 
 * -----------------------------------------------------------------------------
 * OrangePi / BananaPi / NanoPi (H3) / BPI-M3 (A83T - ov5640 & ov8865)
 * -----------------------------------------------------------------------------
 * _V4L2_KERNEL_ should not be defined unless you want Exposure, Hflip and Vflip
 *
 * build with: gcc cap.c -o cap $(pkg-config --libs --cflags opencv) -lm
 *
 * 
*/
//#define _V4L2_KERNEL_	// BananaPi M64 / Pine64+ only or for setting Exposure,Hflip,Vflip

#ifdef _V4L2_KERNEL_
/* --- A64 --- */
#include <linux/videodev2.h>
#else
/* --- H3 / A83T --- */
#include "videodev2.h"
#endif

#ifdef _V4L2_KERNEL_
#define V4L2_MODE_VIDEO				0x0002  /* video capture */
#define V4L2_MODE_IMAGE				0x0003  /* image capture */
#define V4L2_MODE_PREVIEW			0x0004  /* preview capture */
#endif

#define N_BUFFERS 4
#define CAP_OK 0
#define CAP_ERROR -1
#define CAP_ERROR_RET(s) { \
							printf("v4l2: %s\n", s); \
							return CAP_ERROR; \
						 }
#define CAP_CLIP(val, min, max) (((val) > (max)) ? (max) : (((val) < (min)) ? (min) : (val)))

#define CLEAR(x) memset (&(x), 0, sizeof (x))
#define ALIGN_4K(x) (((x) + (4095)) & ~(4095))
#define ALIGN_16B(x) (((x) + (15)) & ~(15))


typedef struct {
    void *start;
    size_t length;
} v4l2_buffer_t;

int width;
int height;
v4l2_buffer_t *buffers = NULL;
int n_buffers = N_BUFFERS;
int sensor_video_mode;
int sensor_exposure;
int sensor_hflip;
int sensor_vflip;
int frame_count = 30;

double get_wall_time()
{
    struct timeval time;
    if (gettimeofday(&time, NULL))
        return 0.;
    return (double) time.tv_sec + (double) time.tv_usec * .000001;
}

static int xioctl(int fd, int request, void *arg)
{
    int r;
    int tries = 3;

    do {
        r = ioctl(fd, request, arg);
    } while (--tries > 0 && -1 == r && EINTR == errno);

    return r;
}

int v4l2_display_sizes_pix_format(int fd)
{
	int ret = 0;
	int fsizeind = 0; /*index for supported sizes*/
	struct v4l2_frmsizeenum fsize;

    printf("V4L2 pixel sizes:\n");

	CLEAR(fsize);
	fsize.index = 0;
	fsize.pixel_format = V4L2_PIX_FMT_YUV420;

	while ((ret = xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &fsize)) == 0)	{
		fsize.index++;
		if (fsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
			printf("( %u x %u ) Pixels\n", fsize.discrete.width, fsize.discrete.height);
			fsizeind++;
		}	
	}
    return fsizeind;
}

int v4l2_display_pix_format(int fd)
{
    struct v4l2_fmtdesc fmt;
    int index;

    printf("V4L2 pixel formats:\n");

    index = 0;
    CLEAR(fmt);
    fmt.index = index;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmt) != -1) {
        printf("%i: [0x%08X] '%c%c%c%c' (%s)\n", index, fmt.pixelformat, fmt.pixelformat >> 0, fmt.pixelformat >> 8, fmt.pixelformat >> 16, fmt.pixelformat >> 24, fmt.description);

        memset(&fmt, 0, sizeof(fmt));
        fmt.index = ++index;
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    }
    // printf("\n");
}

#ifdef _V4L2_KERNEL_
int v4l2_set_exposure(int fd, int exposure)
{
    struct v4l2_queryctrl queryctrl;
    struct v4l2_control control;
    int rc;

    printf("set Exposure: %d\n", exposure);
    rc = 0;
    memset(&control, 0, sizeof(control));
    control.id = V4L2_CID_EXPOSURE;
    rc = xioctl(fd, VIDIOC_G_CTRL, &control);
    printf("rc: %d - get exposure: %d\n", rc, control.value);
    control.value = exposure;
    rc = xioctl(fd, VIDIOC_S_CTRL, &control);
    printf("rc: %d - new exposure: %d\n", rc, exposure);
    return rc;
}

int v4l2_set_hflip(int fd, int hflip)
{
    struct v4l2_queryctrl queryctrl;
    struct v4l2_control control;
    int rc;

    printf("set Hflip: %d\n", hflip);
    rc = 0;
    memset(&control, 0, sizeof(control));
    control.id = V4L2_CID_HFLIP;
    rc = xioctl(fd, VIDIOC_G_CTRL, &control);
    printf("rc: %d - get value: %d\n", rc, control.value);
    control.value = hflip;
    rc = xioctl(fd, VIDIOC_S_CTRL, &control);
    printf("rc: %d - new value: %d\n", rc, control.value);
    return rc;
}

int v4l2_set_vflip(int fd, int vflip)
{
    struct v4l2_queryctrl queryctrl;
    struct v4l2_control control;
    int rc;

    printf("set Vflip: %d\n", vflip);
    rc = 0;
    memset(&control, 0, sizeof(control));
    control.id = V4L2_CID_VFLIP;
    rc = xioctl(fd, VIDIOC_G_CTRL, &control);
    printf("rc: %d - get value: %d\n", rc, control.value);
    control.value = vflip;
    rc = xioctl(fd, VIDIOC_S_CTRL, &control);
    printf("rc: %d - new value: %d\n", rc, control.value);
    return rc;
}
#endif

int v4l2_init_camera(int fd)
{
    uint32_t i;
    uint32_t index;
    struct v4l2_streamparm parms;
    struct v4l2_format fmt;
    struct v4l2_input input;
    struct v4l2_capability caps;

    CLEAR(fmt);
    CLEAR(input);
    CLEAR(caps);


    if (xioctl(fd, VIDIOC_QUERYCAP, &caps) == -1) {
        CAP_ERROR_RET("unable to query capabilities.");
    }

    if (!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        CAP_ERROR_RET("doesn't support video capturing.");
    }

    printf("Driver: \"%s\"\n", caps.driver);
    printf("Card: \"%s\"\n", caps.card);
    printf("Bus: \"%s\"\n", caps.bus_info);
    printf("Version: %d.%d\n", (caps.version >> 16) && 0xff, (caps.version >> 24) && 0xff);
    printf("Capabilities: %08x\n", caps.capabilities);

    input.index = 0;
    if (xioctl(fd, VIDIOC_ENUMINPUT, &input) == -1) {
        CAP_ERROR_RET("unable to enumerate input.");
    }

    printf("Input: %d\n", input.index);
    if (xioctl(fd, VIDIOC_S_INPUT, &input.index) == -1) {
        CAP_ERROR_RET("unable to set input.");
    }

    parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parms.parm.capture.capturemode = sensor_video_mode ? V4L2_MODE_VIDEO : V4L2_MODE_IMAGE;
    parms.parm.capture.timeperframe.numerator = 1;
    parms.parm.capture.timeperframe.denominator = sensor_video_mode ? 30 : 7; // set sampling Rate
    if (-1 == xioctl(fd, VIDIOC_S_PARM, &parms)) {
        CAP_ERROR_RET("unable to set stream parm.");
    }

    v4l2_display_pix_format(fd);
    //v4l2_display_sizes_pix_format(fd);
    printf("\n");

    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;        // V4L2_FIELD_ANY;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;

    if (xioctl(fd, VIDIOC_TRY_FMT, &fmt) == -1) {
        CAP_ERROR_RET("failed trying to set pixel format.");
    }

    if (fmt.fmt.pix.width != width || fmt.fmt.pix.height != height) {
        width = fmt.fmt.pix.width;
        height = fmt.fmt.pix.height;
        printf("Sensor size adjusted to: %dx%d pixels\n", width, height);
    } else {
        printf("Sensor size: %dx%d pixels\n", width, height);
    }

    if (xioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
        CAP_ERROR_RET("failed to set pixel format.");
    }

    switch (fmt.fmt.pix.pixelformat) {
    case V4L2_PIX_FMT_RGB24:
            printf("Pixel Format: V4L2_PIX_FMT_RGB24 [0x%08X]\n",fmt.fmt.pix.pixelformat);
            break;
        
    case V4L2_PIX_FMT_YUV420:
            printf("Pixel Format: V4L2_PIX_FMT_YUV420 [0x%08X]\n",fmt.fmt.pix.pixelformat);
            break;
        
    }
    return CAP_OK;
}

int v4l2_set_mmap(int fd, int *buffers_count)
{
    int i;
    int nbf;
    enum v4l2_buf_type type;
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;

    CLEAR(req);
    req.count = sensor_video_mode ? n_buffers : 1;
    req.memory = V4L2_MEMORY_MMAP;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
        CAP_ERROR_RET("failed requesting buffers.");
    }
    nbf = req.count;
    if (n_buffers != nbf) {
        CAP_ERROR_RET("insufficient buffer memory.");
    }

    buffers = (v4l2_buffer_t *) calloc(nbf, sizeof(v4l2_buffer_t));
    if (!buffers) {
        CAP_ERROR_RET("failed to allocated buffers memory.");
    }

    for (i = 0; i < nbf; i++) {
        CLEAR(buf);
        buf.index = i;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (xioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
            CAP_ERROR_RET("failed to query buffer.");
        }
        buffers[i].length = buf.length;
        buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        if (MAP_FAILED == buffers[i].start) {
            CAP_ERROR_RET("failed to mmap buffer.");
        }
    }

    for (i = 0; i < nbf; i++) {
        CLEAR(buf);
        buf.index = i;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (xioctl(fd, VIDIOC_QBUF, &buf) == -1) {
            CAP_ERROR_RET("failed to queue buffer.");
        }
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_STREAMON, &type) == -1) {
        CAP_ERROR_RET("failed to stream on.");
    }

    *buffers_count = nbf;

    return CAP_OK;
}

int v4l2_retrieve_frame(int fd, int buffers_count, FILE *f, int save_frame_yuv)
{
    int sz;
    fd_set fds;
    struct timeval tv;
    struct v4l2_buffer buf;
    int rc;
    char frame_name[128];

    CLEAR(tv);
    CLEAR(buf);

    rc = 1;
    while (rc > 0) {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        tv.tv_sec = 2;
        tv.tv_usec = 0;

        rc = select(fd + 1, &fds, NULL, NULL, &tv);
        if (-1 == rc) {
            if (EINTR == errno) {
                rc = 1;         // try again
                continue;
            }
            CAP_ERROR_RET("failed to select frame.");
        }
        /* we got something */
        break;
    }
    if (rc <= 0) {
        sprintf(frame_name, "errno: %d - check sensor, something wrong.", errno);
        CAP_ERROR_RET(frame_name);
    }

    buf.memory = V4L2_MEMORY_MMAP;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
        CAP_ERROR_RET("failed to retrieve frame.");
    }

    printf("Length: %d \tBytesused: %d \tAddress: %p\n", buf.length, buf.bytesused, &buffers[buf.index]);

    if (save_frame_yuv) {
        if (f != NULL) {
            sz = ALIGN_16B(width) * height * 3 / 2;
            fwrite(buffers[buf.index].start, sz, 1, f);
        }
    }

    if (xioctl(fd, VIDIOC_QBUF, &buf) == -1) {
        CAP_ERROR_RET("failed to queue buffer.");
    }

    return CAP_OK;
}

int v4l2_close_camera(int fd, int buffers_count)
{
    int i;
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_STREAMOFF, &type) == -1) {
        CAP_ERROR_RET("failed to stream off.");
    }

    for (i = 0; i < buffers_count; i++)
        munmap(buffers[i].start, buffers[i].length);

    close(fd);
}

int main(int argc, char *argv[])
{
    int i, n, save_frame;
    int fd;
    double after;
    double before;
    double avg, fps;
    int buffers_count;
    FILE *f;
    char frame_name[128];

    if (argc != 4) {
        CAP_ERROR_RET("./cap <width> <height> <frame count>")
    }
    n = 0;
    width = (int) atoi(argv[1]);
    height = (int) atoi(argv[2]);
    frame_count = (int) atoi(argv[3]);
    n_buffers = 4;
    sensor_video_mode = 1;
    sensor_exposure = -999;
    sensor_hflip = -1;
    sensor_vflip = -1;

    fd = open("/dev/video0", O_RDWR | O_NONBLOCK);
    if (fd == -1) {
        CAP_ERROR_RET("failed to open the camera.");
    }

    if (v4l2_init_camera(fd) == -1) {
        CAP_ERROR_RET("failed to init camera.");
    }

#ifdef _V4L2_KERNEL_
    if (sensor_exposure != -999) {
        v4l2_set_exposure(fd, sensor_exposure);
    }
    if (sensor_hflip != -1) {
        v4l2_set_hflip(fd, sensor_hflip);
    }

    if (sensor_vflip != -1) {
        v4l2_set_vflip(fd, sensor_vflip);
    }
#endif

    if (v4l2_set_mmap(fd, &buffers_count) == -1) {
        CAP_ERROR_RET("failed to mmap.");
    }

    sprintf(frame_name, "frame_%dx%d.yuv", width, height);
    printf("Frame #%d will be saved into %s!\n", frame_count, frame_name);
    f = fopen(frame_name, "wb");
    if (f != NULL) {
	    for (i = 0; i < frame_count; i++) {
	        before = get_wall_time();
	        if (v4l2_retrieve_frame(fd, buffers_count, f, frame_count) == -1) {
	            CAP_ERROR_RET("failed to retrieve frame.");
	        }
	        after = get_wall_time();
	        fps = 1.0 / (after - before);
	        if (i && (i + 1) != frame_count) {
	            avg += fps;
	            n++;
	        }
	        printf("FPS[%d]: %.2f\n", i, fps);
	    }
	    fclose(f);
    }

    v4l2_close_camera(fd, buffers_count);
    
    if (n) {
        printf("------- Avg FPS: %.2f --------\n\n", (double) (avg / (double) n));
    }
    return CAP_OK;
}
