/***************************************************************************
 *   v4l2grab Version 0.3                                                  *
 *   Copyright (C) 2012 by Tobias Müller                                   *
 *   Tobias_Mueller@twam.info                                              *
 *                                                                         *
 *   based on V4L2 Specification, Appendix B: Video Capture Example        *
 *   (http://v4l2spec.bytesex.org/spec/capture-example.html)               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

 /**************************************************************************
 *   Modification History                                                  *
 *                                                                         *
 *   Matthew Witherwax      21AUG2013                                      *
 *      Added ability to change frame interval (ie. frame rate/fps)        *
 * Martin Savc              7JUL2015
 *      Added support for continuous capture using SIGINT to stop.
 ***************************************************************************/

// compile with all three access methods
#if !defined(IO_READ) && !defined(IO_MMAP) && !defined(IO_USERPTR)
#define IO_READ
#define IO_MMAP
#define IO_USERPTR
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <jpeglib.h>
#include <libv4l2.h>
#include <signal.h>
#include <stdint.h>
#include <inttypes.h>
#include <linux/fb.h>
#include <stdbool.h>

#include "config.h"
#include "yuv.h"
#include "convert.h"

#define CLEAR(x) memset (&(x), 0, sizeof (x))

#ifndef VERSION
#define VERSION "v0.0.1"
#endif

#define FILENAME_MAX_LEN     30
struct stfisp_fw_info {
    char filename[FILENAME_MAX_LEN];
};

struct v4l2_subdev_frame_size_enum {
    __u32 index;
    __u32 pad;
    __u32 code;
    __u32 min_width;
    __u32 max_width;
    __u32 min_height;
    __u32 max_height;
    __u32 which;
    __u32 reserved[8];
};


#define VIDIOC_STFISP_LOAD_FW \
        _IOW('V', BASE_VIDIOC_PRIVATE + 1, struct stfisp_fw_info)

#define VIDIOC_SUBDEV_ENUM_FRAME_SIZE \
        _IOWR('V', 74, struct v4l2_subdev_frame_size_enum)
#define MEDIA_BUS_FMT_SRGGB10_1X10  0x300f

#define FBIOPAN_GET_PP_MODE        0x4609
#define FBIOPAN_SET_PP_MODE        0x460a

enum COLOR_FORMAT{
    COLOR_YUV422_UYVY = 0,  // 00={Y1,V0,Y0,U0}
    COLOR_YUV422_VYUY = 1,  // 01={Y1,U0,Y0,V0}
    COLOR_YUV422_YUYV = 2,  // 10={V0,Y1,U0,Y0}
    COLOR_YUV422_YVYU = 3,  // 11={U0,Y1,V0,Y0}

    COLOR_YUV420P,          // 4
    COLOR_YUV420_NV21,      // 5
    COLOR_YUV420_NV12,      // 6

    COLOR_RGB888_ARGB,      // 7
    COLOR_RGB888_ABGR,      // 8
    COLOR_RGB888_RGBA,      // 9
    COLOR_RGB888_BGRA,      // 10
    COLOR_RGB565,           // 11
};

struct pp_video_mode {
    enum COLOR_FORMAT format;
    unsigned int height;
    unsigned int width;
    unsigned int addr;
};

struct pp_mode {
    char pp_id;
    bool bus_out;        /*out to ddr*/
    bool fifo_out;        /*out to lcdc*/
    bool inited;
    struct pp_video_mode src;
    struct pp_video_mode dst;
};

#if defined(IO_MMAP) || defined(IO_USERPTR)
// minimum number of buffer to request in VIDIOC_REQBUFS call
#define VIDIOC_REQBUFS_COUNT 5
#endif

typedef enum {
#ifdef IO_READ
        IO_METHOD_READ,
#endif
#ifdef IO_MMAP
        IO_METHOD_MMAP,
#endif
#ifdef IO_USERPTR
        IO_METHOD_USERPTR,
#endif
} io_method;

struct buffer {
    void *                  start;
    size_t                  length;
};

static io_method        g_io_mthd   = IO_METHOD_MMAP;
static int              g_fd        = -1;
static int              g_fb_fd     = -1;
static int              g_stfbc_fd  = -1;
struct buffer *         g_buffers     = NULL;
static unsigned int     n_buffers   = 0;
struct fb_var_screeninfo g_vinfo;
struct fb_fix_screeninfo g_finfo;
static unsigned char *g_fb_buf = NULL;
long g_screensize = 1920 * 1080 * 2;
long g_imagesize  = 1920 * 1080 * 2;
// global settings
static unsigned int format= V4L2_PIX_FMT_RGB565;
static int pixformat = COLOR_RGB565;
static unsigned int width = 1920;
static unsigned int height = 1080;
static unsigned int left = 0;
static unsigned int up = 0;
static unsigned int right = 0;
static unsigned int down = 0;
static unsigned int crop_flag = 0;
//static int istride = 1280;
static unsigned int fps = 30;
static unsigned int test_fps = 0;
static int continuous = 0;
static unsigned char jpegQuality = 70;
static char* jpegFilename = NULL;
static char* deviceName = "/dev/video0";
static unsigned int fps_count = 0;
static struct v4l2_subdev_frame_size_enum frame_size;
//static const char* const continuousFilenameFmt = "%s_%010"PRIu32"_%"PRId64".jpg";

inline int clip(int value, int min, int max) {
    return (value > max ? max : value < min ? min : value);
}

static int v4l2fmt_to_fbfmt(unsigned int format)
{
    int pixformat = COLOR_RGB565;

    switch (format) {
    case V4L2_PIX_FMT_RGB565:
        pixformat = COLOR_RGB565;
        break;
    case V4L2_PIX_FMT_RGB24:
        pixformat = COLOR_RGB888_ARGB;
        break;
    case V4L2_PIX_FMT_YUV420:
        pixformat = COLOR_YUV420P;
        break;
    case V4L2_PIX_FMT_YUYV:
        pixformat = COLOR_YUV422_YUYV;
        break;
    case V4L2_PIX_FMT_NV21:
        pixformat = COLOR_YUV420_NV21;
        break;
    case V4L2_PIX_FMT_NV12:
        pixformat = COLOR_YUV420_NV12;
        break;
    case V4L2_PIX_FMT_YVYU:
        pixformat = COLOR_YUV422_YVYU;
        break;
    default:
        pixformat = COLOR_RGB565;
        break;
    }

    return pixformat;
}

void test_float_1()
{
    int count = 1920 * 1080 * 2;
    float r, g, b;
    float y = 10, u = 20, v = 30;
    int int_r, int_g, int_b;
    int int_y = 10, int_u = 20, int_v = 30;
    struct timeval tv1, tv2, tv3;
    long long elapse = 0;
    int i;

    gettimeofday(&tv1, NULL);
    while (count--) {
        b = 1.164 * (y - 16) + 2.018 * (u - 128);
        g = 1.164 * (y - 16) - 0.813 * (v - 128) - 0.391 * (u - 128);
        r = 1.164 * (y - 16) + 1.596 * (v - 128);

        y ++;
        u ++;
        v ++;
    }
    gettimeofday(&tv2, NULL);
    elapse = (tv2.tv_sec - tv1.tv_sec) * 1000 + (tv2.tv_usec - tv1.tv_usec) / 1000;
    printf("elapse: %lldms, out: r=%f, g=%f, b=%f\n",
            elapse ,r, g, b);

    count = 1920 * 1080 * 2;
    gettimeofday(&tv1, NULL);
    while (count--) {
        int_b = 1164 * (y - 16) + 2018 * (u - 128);
        int_g = 1164 * (y - 16) - 813 * (v - 128) - 391 * (u - 128);
        int_r = 1164 * (y - 16) + 1596 * (v - 128);

        int_y ++;
        int_u ++;
        int_v ++;
    }
    gettimeofday(&tv2, NULL);
    elapse = (tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec);
    printf("elapse: %lldus, out: r=%d, g=%d, b=%d\n",
            elapse , int_r, int_g, int_b);

    count = 1920 * 1080 * 2;
    unsigned char* arraybuf = NULL;
    arraybuf = (unsigned char*)malloc(count);
    if (!arraybuf) {
        printf("arraybuf malloc error\n");
        return;
    }

    unsigned char* arraybuf2 = NULL;
    arraybuf2 = (unsigned char*)malloc(count);
    if (!arraybuf2) {
        printf("arraybuf2 malloc error\n");
        return;
    }

    gettimeofday(&tv1, NULL);

    for (i = 0; i < count; i++) {
        arraybuf[i] = i + 1;
    }

    gettimeofday(&tv2, NULL);

    memcpy(arraybuf2, arraybuf, count);

    gettimeofday(&tv3, NULL);

    elapse = (tv2.tv_sec - tv1.tv_sec) * 1000 + (tv2.tv_usec - tv1.tv_usec) / 1000;
    printf("for() elapse: %lldms\n", elapse);

    elapse = (tv3.tv_sec - tv2.tv_sec) * 1000000 + (tv3.tv_usec - tv2.tv_usec);
    printf("memcpy() run elapse: %lldus\n", elapse);

    convert_nv21_to_rgb(arraybuf, arraybuf2, 1920, 1080, 1);

    free(arraybuf);
    free(arraybuf2);
}

/**
SIGINT interput handler
*/
void StopContCapture(int sig_id) {
    printf("stoping continuous capture\n");
    continuous = 0;
}

void InstallSIGINTHandler() {
    struct sigaction sa;
    CLEAR(sa);

    sa.sa_handler = StopContCapture;
    if(sigaction(SIGINT, &sa, 0) != 0)
    {
        fprintf(stderr,"could not install SIGINT handler, continuous capture disabled");
        continuous = 0;
    }
}

/**
    Print error message and terminate programm with EXIT_FAILURE return code.

    \param s error message to print
*/
static void errno_exit(const char* s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

/**
    Do ioctl and retry if error was EINTR ("A signal was caught during the ioctl() operation."). Parameters are the same as on ioctl.

    \param fd file descriptor
    \param request request
    \param argp argument
    \returns result from ioctl
*/
static int xioctl(int fd, int request, void* argp)
{
    int r;

    do r = v4l2_ioctl(fd, request, argp);
    while (-1 == r && EINTR == errno);

    return r;
}

/**
    Write image to jpeg file.

    \param img image to write
*/
int write_file (char * filename,unsigned char *image_buffer, int size)
{
    /*  More stuff */

    FILE * outfile;               /*  target file */

    if ((outfile = fopen(filename, "w+")) == NULL) {

        fprintf(stderr, "can't open %s\n", filename);

        return -1;

    }
    fwrite(image_buffer, size, 1, outfile);

    fclose(outfile);
    return 0 ;
}

int write_JPEG_file (char * filename,unsigned char *image_buffer, int image_width, int image_height, int quality )

{

    struct jpeg_compress_struct cinfo;

    struct jpeg_error_mgr jerr;

    /*  More stuff */

    FILE * outfile;               /*  target file */

    JSAMPROW row_pointer[1];      /*  pointer to JSAMPLE row[s] */

    int row_stride;               /*  physical row width in image buffer */



    /*  Step 1: allocate and initialize JPEG compression object */

    cinfo.err = jpeg_std_error(&jerr);

    /*  Now we can initialize the JPEG compression object. */

    jpeg_create_compress(&cinfo);



    /*  Step 2: specify data destination (eg, a file) */

    /*  Note: steps 2 and 3 can be done in either order. */



    if ((outfile = fopen(filename, "w+")) == NULL) {

        fprintf(stderr, "can't open %s\n", filename);

        return -1;

    }

    jpeg_stdio_dest(&cinfo, outfile);



    /*  Step 3: set parameters for compression */

    cinfo.image_width = image_width;      /*  image width and height, in pixels */

    cinfo.image_height = image_height;

    cinfo.input_components = 3;           /*  # of color components per pixel */

    cinfo.in_color_space = JCS_RGB;       /*  colorspace of input image */



    jpeg_set_defaults(&cinfo);

    jpeg_set_quality(&cinfo, quality, TRUE /*  limit to baseline-JPEG values */);



    /*  Step 4: Start compressor */



    jpeg_start_compress(&cinfo, TRUE);



    /*  Step 5: while (scan lines remain to be written) */

    row_stride = image_width * 3; /*  JSAMPLEs per row in image_buffer */



    while (cinfo.next_scanline < cinfo.image_height) {

        row_pointer[0] = & image_buffer[cinfo.next_scanline * row_stride];

        (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);

    }



    /*  Step 6: Finish compression */



    jpeg_finish_compress(&cinfo);

    /*  After finish_compress, we can close the output file. */

    fclose(outfile);



    /*  Step 7: release JPEG compression object */



    /*  This is an important step since it will release a good deal of memory. */

    jpeg_destroy_compress(&cinfo);



    /*  And we're done! */

    return 0 ;

}

static void jpegWrite(unsigned char* img, char* jpegFilename)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    JSAMPROW row_pointer[1];
    FILE *outfile = fopen( jpegFilename, "wb" );

    // try to open file for saving
    if (!outfile) {
        errno_exit("jpeg");
    }

    // create jpeg data
    cinfo.err = jpeg_std_error( &jerr );
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);

    // set image parameters
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_YCbCr;

    // set jpeg compression parameters to default
    jpeg_set_defaults(&cinfo);
    // and then adjust quality setting
    jpeg_set_quality(&cinfo, jpegQuality, TRUE);

    // start compress
    jpeg_start_compress(&cinfo, TRUE);

    // feed data
    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &img[cinfo.next_scanline * cinfo.image_width *  cinfo.input_components];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    // finish compression
    jpeg_finish_compress(&cinfo);

    // destroy jpeg data
    jpeg_destroy_compress(&cinfo);

    // close output file
    fclose(outfile);
}

/**
    process image read
*/
static void imageProcess(const void* p, struct timeval timestamp)
{
    //timestamp.tv_sec
    //timestamp.tv_usec
    unsigned char* src = (unsigned char*)p;
    unsigned char* dst = malloc(width*height*3*sizeof(char));
    static int count = 0;

    if (test_fps)
        return;

    // write jpeg
    char filename[512];
    switch (format) {
        case V4L2_PIX_FMT_YUV420:
            if (jpegFilename) {
                // sprintf(filename, "%d-yuv420-%s", count, jpegFilename);
                // YUV420toYUV444(width, height, src, dst);
                // jpegWrite(dst, filename);
                sprintf(filename, "raw-%d-yuv420-%s", count, jpegFilename);
                write_file(filename, src, g_imagesize);
                count++;
            }
            break;
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_YVYU:
            if (jpegFilename) {
                // sprintf(filename, "%d-yuv422-%s", count, jpegFilename);
                // YUV422toYUV444(width, height, src, dst);
                // jpegWrite(dst, filename);
                sprintf(filename, "raw-%d-yuv422-%s", count, jpegFilename);
                write_file(filename, src, g_imagesize);
                count++;
            }
            if (pixformat == v4l2fmt_to_fbfmt(format)) {
                yuyv_resize(src, g_fb_buf, width, height);
            } else if (g_vinfo.grayscale) {
                convert_yuyv_to_nv12(src, g_fb_buf, width, height, 1);
            } else {
                convert_yuyv_to_rgb(src, g_fb_buf, width, height, 0);
            }
            break;
        case V4L2_PIX_FMT_NV21:
            if (jpegFilename) {
                // sprintf(filename, "%d-nv21-%s", count, jpegFilename);
                // YUV420NV21toYUV444(width, height, src, dst);
                // jpegWrite(dst, filename);
                sprintf(filename, "raw-%d-nv21-%s", count, jpegFilename);
                write_file(filename, src, g_imagesize);
                count++;
            }
            if (pixformat == v4l2fmt_to_fbfmt(format))
                convert_nv21_to_nv12(src, g_fb_buf, width, height, 0);
            else if (g_vinfo.grayscale)
                convert_nv21_to_nv12(src, g_fb_buf, width, height, 1);
            else
                convert_nv21_to_rgb(src, g_fb_buf, width, height, 1);
            break;
        case V4L2_PIX_FMT_NV12:
            if (jpegFilename) {
                // sprintf(filename, "%d-nv12-%s", count, jpegFilename);
                // YUV420NV12toYUV444(width, height, src, dst);
                // jpegWrite(dst, filename);
                sprintf(filename, "raw-%d-nv12-%s", count, jpegFilename);
                write_file(filename, src, g_imagesize);
                count++;
            }
            if (pixformat == v4l2fmt_to_fbfmt(format))
                convert_nv21_to_nv12(src, g_fb_buf, width, height, 0);
            else if (g_vinfo.grayscale)
                convert_nv21_to_nv12(src, g_fb_buf, width, height, 0);
            else
                convert_nv21_to_rgb(src, g_fb_buf, width, height, 0);
            break;
        case V4L2_PIX_FMT_RGB24:
            if (jpegFilename) {
                // sprintf(filename, "%d-rgb-%s", count, jpegFilename);
                // RGB565toRGB888(width, height, src, dst);
                // write_JPEG_file(filename, src, width, height, jpegQuality);
                sprintf(filename, "raw-%d-rgb-%s", count, jpegFilename);
                write_file(filename, src, g_imagesize);
                count++;
            }
            convert_rgb888_to_rgb(src, g_fb_buf, width, height, 0);
            break;
        case V4L2_PIX_FMT_RGB565:
            if (jpegFilename) {
                // sprintf(filename, "%d-rgb565-%s", count, jpegFilename);
                // RGB565toRGB888(width, height, src, dst);
                // write_JPEG_file(filename, dst, width, height, jpegQuality);
                sprintf(filename, "raw-%d-rgb565-%s", count, jpegFilename);
                write_file(filename, src, g_imagesize);
                count++;
            }
            if (pixformat == v4l2fmt_to_fbfmt(format))
                convert_rgb565_to_rgb(src, g_fb_buf, width, height, 0);
            else if (g_vinfo.grayscale)
                convert_rgb565_to_nv12(src, g_fb_buf, width, height, 0);
            else
                convert_rgb565_to_rgb(src, g_fb_buf, width, height, 0);
            break;
        case V4L2_PIX_FMT_SRGGB12:
            if (jpegFilename)
                sprintf(filename, "raw-%d-RGGB12-%s", count, jpegFilename);
            else
                sprintf(filename, "raw-%d-RGGB12.raw", count);
            write_file(filename, src, g_imagesize);
            RAW12toRAW16(width, height, src, dst);
            if (jpegFilename)
                sprintf(filename, "raw-%d-RGGB16-%s", count, jpegFilename);
            else
                sprintf(filename, "raw-%d-RGGB16.raw", count);
            write_file(filename, dst, width * height * 2);
            count++;
            break;
        case V4L2_PIX_FMT_SGRBG12:
            if (jpegFilename)
                sprintf(filename, "raw-%d-GRBG12-%s", count, jpegFilename);
            else
                sprintf(filename, "raw-%d-GRBG12.raw", count);
            write_file(filename, src, g_imagesize);
            RAW12toRAW16(width, height, src, dst);
            if (jpegFilename)
                sprintf(filename, "raw-%d-GRBG16-%s", count, jpegFilename);
            else
                sprintf(filename, "raw-%d-GRBG16.raw", count);
            write_file(filename, dst, width * height * 2);
            count++;
            break;
        case V4L2_PIX_FMT_SGBRG12:
            if (jpegFilename)
                sprintf(filename, "raw-%d-GBRG12-%s", count, jpegFilename);
            else
                sprintf(filename, "raw-%d-GBRG12.raw", count);
            write_file(filename, src, g_imagesize);
            RAW12toRAW16(width, height, src, dst);
            if (jpegFilename)
                sprintf(filename, "raw-%d-GBRG16-%s", count, jpegFilename);
            else
                sprintf(filename, "raw-%d-GBRG16.raw", count);
            write_file(filename, dst, width * height * 2);
            count++;
            break;
        case V4L2_PIX_FMT_SBGGR12:
            if (jpegFilename)
                sprintf(filename, "raw-%d-BGGR12-%s", count, jpegFilename);
            else
                sprintf(filename, "raw-%d-BGGR12.raw", count);
            write_file(filename, src, g_imagesize);
            RAW12toRAW16(width, height, src, dst);
            if (jpegFilename)
                sprintf(filename, "raw-%d-BGGR16-%s", count, jpegFilename);
            else
                sprintf(filename, "raw-%d-BGGR16.raw", count);
            write_file(filename, dst, width * height * 2);
            count++;
            break;
        default:
            printf("unknow format\n");
            break;
    }

    // free temporary image
    free(dst);
}

/**
    read single frame
*/
static int frameRead(void)
{
    struct v4l2_buffer buf;
#ifdef IO_USERPTR
    unsigned int i;
#endif

    switch (g_io_mthd) {
#ifdef IO_READ
        case IO_METHOD_READ:
            if (-1 == v4l2_read(g_fd, g_buffers[0].start, g_buffers[0].length)) {
                switch (errno) {
                    case EAGAIN:
                        return 0;

                    case EIO:
                        // Could ignore EIO, see spec.
                        // fall through

                    default:
                        errno_exit("read");
                }
            }

            struct timespec ts;
            struct timeval timestamp;
            clock_gettime(CLOCK_MONOTONIC,&ts);
            timestamp.tv_sec = ts.tv_sec;
            timestamp.tv_usec = ts.tv_nsec/1000;

            imageProcess(g_buffers[0].start,timestamp);
            break;
#endif

#ifdef IO_MMAP
        case IO_METHOD_MMAP:
            CLEAR(buf);

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;

            if (-1 == xioctl(g_fd, VIDIOC_DQBUF, &buf)) {
                switch (errno) {
                    case EAGAIN:
                        return 0;

                    case EIO:
                        // Could ignore EIO, see spec
                        // fall through

                    default:
                        errno_exit("VIDIOC_DQBUF");
                }
            }

            assert(buf.index < n_buffers);

            imageProcess(g_buffers[buf.index].start, buf.timestamp);
            if (-1 == xioctl(g_fd, VIDIOC_QBUF, &buf))
                errno_exit("VIDIOC_QBUF");

            break;
#endif

#ifdef IO_USERPTR
            case IO_METHOD_USERPTR:
                CLEAR (buf);

                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_USERPTR;

                if (-1 == xioctl(g_fd, VIDIOC_DQBUF, &buf)) {
                    switch (errno) {
                    case EAGAIN:
                        return 0;

                    case EIO:
                        // Could ignore EIO, see spec.
                        // fall through

                    default:
                        errno_exit("VIDIOC_DQBUF");
                    }
                }

                for (i = 0; i < n_buffers; ++i) {
                    if (buf.m.userptr == (unsigned long)g_buffers[i].start 
                            && buf.length == g_buffers[i].length) {
                        break;
                    }
                }
                assert (i < n_buffers);

                imageProcess((void *)buf.m.userptr, buf.timestamp);

                if (-1 == xioctl(g_fd, VIDIOC_QBUF, &buf))
                    errno_exit("VIDIOC_QBUF");
                break;
#endif
    }

    static unsigned int start_timems;
    unsigned int time_ms, tmp_ms;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    tmp_ms = ts.tv_sec * 1000 + ts.tv_nsec/1000000;
    time_ms = tmp_ms - start_timems;
    start_timems = tmp_ms;

    if (!(++fps_count%5)) {
        fps_count = 0;
        printf("%s format = 0x%x\n", __func__, format);
        printf("fps: %d\n", 1000/time_ms);
    }

    return 1;
}

/**
    mainloop: read frames and process them
*/
static void mainLoop(void)
{
    int count, i;
    unsigned int numberOfTimeouts;

    numberOfTimeouts = 0;
    count = 3;

    while (count-- > 0) {
        for (i = 0; i < 1; i++) {
            fd_set fds;
            struct timeval tv;
            int r;

            FD_ZERO(&fds);
            FD_SET(g_fd, &fds);

            /* Timeout. */
            tv.tv_sec = 1;
            tv.tv_usec = 0;

            r = select(g_fd + 1, &fds, NULL, NULL, &tv);

            if (-1 == r) {
                if (EINTR == errno)
                    continue;

                errno_exit("select");
            }

            if (0 == r) {
                if (numberOfTimeouts <= 0) {
                    // count++;
                } else {
                    fprintf(stderr, "select timeout\n");
                    exit(EXIT_FAILURE);
                }
            }
            if(continuous == 1) {
                count = 3;
            }

            if (frameRead())
                break;

            /* EAGAIN - continue select loop. */
        }
    }
}

/**
    stop capturing
*/
static void captureStop(void)
{
    enum v4l2_buf_type type;

    switch (g_io_mthd) {
#ifdef IO_READ
        case IO_METHOD_READ:
            /* Nothing to do. */
            break;
#endif

#ifdef IO_MMAP
        case IO_METHOD_MMAP:
#endif
#ifdef IO_USERPTR
        case IO_METHOD_USERPTR:
#endif
#if defined(IO_MMAP) || defined(IO_USERPTR)
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            if (-1 == xioctl(g_fd, VIDIOC_STREAMOFF, &type))
            errno_exit("VIDIOC_STREAMOFF");

            break;
#endif
    }
}

/**
  start capturing
*/
static void captureStart(void)
{
    unsigned int i;
    enum v4l2_buf_type type;

    switch (g_io_mthd) {
#ifdef IO_READ
        case IO_METHOD_READ:
            /* Nothing to do. */
            break;
#endif

#ifdef IO_MMAP
        case IO_METHOD_MMAP:
            for (i = 0; i < n_buffers; ++i) {
                struct v4l2_buffer buf;

                CLEAR(buf);

                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;

                if (-1 == xioctl(g_fd, VIDIOC_QBUF, &buf))
                    errno_exit("VIDIOC_QBUF");
                }

            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            if (-1 == xioctl(g_fd, VIDIOC_STREAMON, &type))
                errno_exit("VIDIOC_STREAMON");

            break;
#endif

#ifdef IO_USERPTR
        case IO_METHOD_USERPTR:
            for (i = 0; i < n_buffers; ++i) {
                struct v4l2_buffer buf;

            CLEAR (buf);

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_USERPTR;
            buf.index = i;
            buf.m.userptr = (unsigned long) g_buffers[i].start;
            buf.length = g_buffers[i].length;

            if (-1 == xioctl(g_fd, VIDIOC_QBUF, &buf))
                errno_exit("VIDIOC_QBUF");
            }

            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            if (-1 == xioctl(g_fd, VIDIOC_STREAMON, &type))
                errno_exit("VIDIOC_STREAMON");

            break;
#endif
    }
}

static void deviceUninit(void)
{
    unsigned int i;

    switch (g_io_mthd) {
#ifdef IO_READ
        case IO_METHOD_READ:
            free(g_buffers[0].start);
            break;
#endif

#ifdef IO_MMAP
        case IO_METHOD_MMAP:
            for (i = 0; i < n_buffers; ++i)
                if (-1 == v4l2_munmap(g_buffers[i].start, g_buffers[i].length))
                    errno_exit("munmap");
            break;
#endif

#ifdef IO_USERPTR
        case IO_METHOD_USERPTR:
            for (i = 0; i < n_buffers; ++i)
                free(g_buffers[i].start);
            break;
#endif
    }

    free(g_buffers);

    if (!test_fps) {
        if (-1 == munmap((void *)g_fb_buf, g_screensize)) {
            printf(" Error: framebuffer device munmap() failed.\n");
            exit (EXIT_FAILURE) ;
        }
    }
}

#ifdef IO_READ
static void readInit(unsigned int buffer_size)
{
    g_buffers = calloc(1, sizeof(*g_buffers));

    if (!g_buffers) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    g_buffers[0].length = buffer_size;
    g_buffers[0].start = malloc(buffer_size);

    if (!g_buffers[0].start) {
        fprintf (stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }
}
#endif

#ifdef IO_MMAP
static void mmapInit(void)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count = VIDIOC_REQBUFS_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(g_fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s does not support memory mapping\n", deviceName);
            exit(EXIT_FAILURE);
        } else {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory on %s\n", deviceName);
        exit(EXIT_FAILURE);
    }

    g_buffers = calloc(req.count, sizeof(*g_buffers));

    if (!g_buffers) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        if (-1 == xioctl(g_fd, VIDIOC_QUERYBUF, &buf))
            errno_exit("VIDIOC_QUERYBUF");

        g_buffers[n_buffers].length = buf.length;
        g_buffers[n_buffers].start = v4l2_mmap(NULL, /* start anywhere */
                buf.length, PROT_READ | PROT_WRITE, /* required */
                MAP_SHARED, /* recommended */
                g_fd, buf.m.offset);

        if (MAP_FAILED == g_buffers[n_buffers].start)
            errno_exit("mmap");
        g_imagesize = buf.length;
    }
}
#endif

#ifdef IO_USERPTR
static void userptrInit(unsigned int buffer_size)
{
    struct v4l2_requestbuffers req;
    unsigned int page_size;

    page_size = getpagesize();
    buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

    CLEAR(req);

    req.count = VIDIOC_REQBUFS_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    if (-1 == xioctl(g_fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s does not support user pointer i/o\n", deviceName);
            exit(EXIT_FAILURE);
        } else {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    g_buffers = calloc(4, sizeof(*g_buffers));

    if (!g_buffers) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
        g_buffers[n_buffers].length = buffer_size;
        g_buffers[n_buffers].start = memalign(/* boundary */ page_size, buffer_size);

        if (!g_buffers[n_buffers].start) {
            fprintf(stderr, "Out of memory\n");
            exit(EXIT_FAILURE);
        }
    }
}
#endif

/**
    initialize device
*/
static void deviceInit(void)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    struct v4l2_streamparm frameint;
    //struct v4l2_streamparm frameget;
    unsigned int min;

    if (-1 == xioctl(g_fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s is no V4L2 device\n",deviceName);
            exit(EXIT_FAILURE);
        } else {
            errno_exit("VIDIOC_QUERYCAP");
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "%s is no video capture device\n",deviceName);
        exit(EXIT_FAILURE);
    }

    switch (g_io_mthd) {
#ifdef IO_READ
        case IO_METHOD_READ:
            if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
                fprintf(stderr, "%s does not support read i/o\n",deviceName);
                exit(EXIT_FAILURE);
            }
            break;
#endif

#ifdef IO_MMAP
        case IO_METHOD_MMAP:
#endif
#ifdef IO_USERPTR
        case IO_METHOD_USERPTR:
#endif
#if defined(IO_MMAP) || defined(IO_USERPTR)
                  if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
                fprintf(stderr, "%s does not support streaming i/o\n",deviceName);
                exit(EXIT_FAILURE);
            }
            break;
#endif
    }

    /* Select video input, video standard and tune here. */
    CLEAR(cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(g_fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == xioctl(g_fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
                case EINVAL:
                    /* Cropping not supported. */
                    break;
                default:
                    /* Errors ignored. */
                    break;
            }
        }
    } else {
        /* Errors ignored. */
    }

    CLEAR(fmt);

    // v4l2_format
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    fmt.fmt.pix.pixelformat = format;

    /* If the user has set the fps to -1, don't try to set the frame interval */
    if (fps != -1)
    {
        CLEAR(frameint);

        /* Attempt to set the frame interval. */
        frameint.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        frameint.parm.capture.timeperframe.numerator = 1;
        frameint.parm.capture.timeperframe.denominator = fps;
        if (-1 == xioctl(g_fd, VIDIOC_S_PARM, &frameint))
            fprintf(stderr,"Unable to set frame interval.\n");
        fprintf(stderr,"set frame interval = %d.\n", frameint.parm.capture.timeperframe.denominator);
    }

    if (-1 == xioctl(g_fd, VIDIOC_S_FMT, &fmt))
        errno_exit("VIDIOC_S_FMT");

    if (crop_flag) {
        struct v4l2_selection sel_crop = {
            V4L2_BUF_TYPE_VIDEO_CAPTURE,
            V4L2_SEL_TGT_CROP,
            0,
            { left, up, right, down }
        };
        struct v4l2_selection get_crop = {
            V4L2_BUF_TYPE_VIDEO_CAPTURE,
            V4L2_SEL_TGT_CROP,
        };

        fprintf(stderr, "sel_crop.left = %d, %d, %d, %d\n",
            sel_crop.r.left, sel_crop.r.top, sel_crop.r.width, sel_crop.r.height);
        if (-1 == xioctl(g_fd, VIDIOC_S_SELECTION, &sel_crop)) {
            fprintf(stderr,"S_SELECTION Failed.\n");
        }

        fprintf(stderr, "sel_crop.left = %d, %d, %d, %d\n",
            sel_crop.r.left, sel_crop.r.top, sel_crop.r.width, sel_crop.r.height);
        if (-1 == xioctl(g_fd, VIDIOC_G_SELECTION, &get_crop)) {
            fprintf(stderr,"G_SELECTION Failed.\n");
        }

        fprintf(stderr, "get_crop.left = %d, %d, %d, %d\n",
            get_crop.r.left, get_crop.r.top, get_crop.r.width, get_crop.r.height);
        if (memcmp(&sel_crop, &get_crop, sizeof(sel_crop))) {
            fprintf(stderr,"set/get selection diff.\n");
        }

        memset(&fmt, 0, sizeof(struct v4l2_format));
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.field = V4L2_FIELD_ANY;

        if (-1 == ioctl(g_fd, VIDIOC_G_FMT, &fmt))
            errno_exit("VIDIOC_G_FMT");
    }

    //if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUV420) {
    //if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB565) {
    if (fmt.fmt.pix.pixelformat != format) {
        fprintf(stderr,"Libv4l didn't accept format. Can't proceed.\n");
        exit(EXIT_FAILURE);
    }

    /* Note VIDIOC_S_FMT may change width and height. */
    if (width != fmt.fmt.pix.width) {
        width = fmt.fmt.pix.width;
        fprintf(stderr,"Image width set to %i by device %s.\n", width, deviceName);
    }

    if (height != fmt.fmt.pix.height) {
        height = fmt.fmt.pix.height;
        fprintf(stderr,"Image height set to %i by device %s.\n", height, deviceName);
    }

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    switch (g_io_mthd) {
#ifdef IO_READ
        case IO_METHOD_READ:
            readInit(fmt.fmt.pix.sizeimage);
            break;
#endif

#ifdef IO_MMAP
        case IO_METHOD_MMAP:
            mmapInit();
            break;
#endif

#ifdef IO_USERPTR
        case IO_METHOD_USERPTR:
            userptrInit(fmt.fmt.pix.sizeimage);
            break;
#endif
    }

    if (!test_fps) {
        struct pp_mode pp_info[3];

        pixformat = v4l2fmt_to_fbfmt(format);
        if (-1 == ioctl(g_stfbc_fd, FBIOPAN_GET_PP_MODE, &pp_info[0])) {
            printf("Error reading variable information.\n");
            exit (EXIT_FAILURE);
        }
        printf("get pp format :%d, %d\n", pp_info[1].src.format, __LINE__);

        pp_info[1].src.format = pixformat;

        if (-1 == ioctl(g_stfbc_fd, FBIOPAN_SET_PP_MODE, &pp_info[0])) {
            printf("Error reading variable information.\n");
            exit (EXIT_FAILURE);
        }

        if (-1 == ioctl(g_stfbc_fd, FBIOPAN_GET_PP_MODE, &pp_info[0])) {
            printf("Error reading variable information.\n");
            exit (EXIT_FAILURE);
        }
        printf("get pp format :%d, %d\n", pp_info[1].src.format, __LINE__);
        pixformat = pp_info[1].src.format;

        // Get fixed screen information
        if (-1 == xioctl(g_fb_fd, FBIOGET_FSCREENINFO, &g_finfo)) {
            printf("Error reading fixed information.\n");
            exit (EXIT_FAILURE);
        }

        // Get variable screen information
        if (-1 == xioctl(g_fb_fd, FBIOGET_VSCREENINFO, &g_vinfo)) {
            printf("Error reading variable information.\n");
            exit (EXIT_FAILURE);
        }

        printf("g_vinfo.xres = %d, g_vinfo.yres = %d, grayscale = %d\n", g_vinfo.xres, g_vinfo.yres, g_vinfo.grayscale);
        printf("g_vinfo.xoffset = %d, g_vinfo.yoffset = %d\n", g_vinfo.xoffset, g_vinfo.yoffset);
        printf("g_vinfo.bits_per_pixel = %d, g_finfo.line_length = %d\n", g_vinfo.bits_per_pixel, g_finfo.line_length);

        // if (ioctl(g_fb_fd, FBIOPUT_VSCREENINFO, &g_vinfo) < 0) {
        //     printf("FBIOPUT_VSCREENINFO.\n");
        //     exit (EXIT_FAILURE);
        // }

        // printf("g_vinfo.bits_per_pixel = %d, g_finfo.line_length = %d\n", g_vinfo.bits_per_pixel, g_finfo.line_length);
        g_screensize = g_vinfo.xres * g_vinfo.yres * g_vinfo.bits_per_pixel / 8;
        // g_screensize = g_vinfo.xres * g_vinfo.yres * 32 / 8;
        //mmap framebuffer
        g_fb_buf = (unsigned char *)mmap(NULL, g_screensize, PROT_READ | PROT_WRITE, MAP_SHARED, g_fb_fd, 0);
        if (g_fb_buf == (void *)(-1)) {
            printf("Error: failed to map framebuffer device to memory.\n");
            exit (EXIT_FAILURE) ;
        }
        memset(g_fb_buf, 0x00, g_screensize);
    }
}

/**
    close device
*/
static void deviceClose(void)
{
    if (-1 == v4l2_close(g_fd))
        errno_exit("close");

    g_fd = -1;
    if (!test_fps) {
        close(g_fb_fd);
        close(g_stfbc_fd);
    }
}

/**
    open device
*/
static void deviceOpen(void)
{
    struct stat st;
    //struct v4l2_capability cap;

    // stat file
    if (-1 == stat(deviceName, &st)) {
        fprintf(stderr, "Cannot identify '%s': %d, %s\n", deviceName, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // check if its device
    if (!S_ISCHR(st.st_mode)) {
        fprintf(stderr, "%s is no device\n", deviceName);
        exit(EXIT_FAILURE);
    }

    // open device
    g_fd = v4l2_open(deviceName, O_RDWR /* required */ | O_NONBLOCK, 0);
    // g_fd = v4l2_open(deviceName, O_RDWR, 0);

    // check if opening was successfull
    if (-1 == g_fd) {
        fprintf(stderr, "Cannot open '%s': %d, %s\n", deviceName, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    //open framebuffer
    if (!test_fps) {
        g_fb_fd = open("/dev/fb0", O_RDWR);
        if (g_fb_fd == -1) {
            printf("Error: cannot open framebuffer device.\n");
            exit (EXIT_FAILURE);
        }
        g_stfbc_fd = open("/dev/stfbcdev", O_RDWR);
        if (g_stfbc_fd == -1) {
            printf("Error: cannot open stfbcdev device.\n");
            exit (EXIT_FAILURE);
        }
    }
}

void loadfw_start(char *filename)
{
    struct stfisp_fw_info fw_info = {0};

    // open device
    g_fd = open(deviceName, O_RDWR /* required */ | O_NONBLOCK, 0);

    // check if opening was successfull
    if (-1 == g_fd) {
        fprintf(stderr, "Cannot open '%s': %d, %s\n", deviceName, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (filename && (strlen(filename) < FILENAME_MAX_LEN))
            memcpy(fw_info.filename, filename, strlen(filename) + 1);

    fprintf(stderr, "VIDIOC_STFISP_LOAD_FW = 0x%lx, filename = %s, size = %lu\n",
                    VIDIOC_STFISP_LOAD_FW, fw_info.filename, sizeof(struct stfisp_fw_info));
    if (-1 == ioctl(g_fd, VIDIOC_STFISP_LOAD_FW, &fw_info)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s is no V4L2 device\n",deviceName);
            exit(EXIT_FAILURE);
        } else {
            errno_exit("VIDIOC_STFISP_LOAD_FW");
        }
    }
}

void sensor_image_size_info(void)
{
    unsigned int i = 0;
    fprintf(stderr, "go in sensor_image_size_info....\n");
    // open device
    g_fd = open(deviceName, O_RDWR /* required */ | O_NONBLOCK, 0);

    // check if opening was successfull
    if (-1 == g_fd) {
        fprintf(stderr, "Cannot open '%s': %d, %s\n", deviceName, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < 4; i++)
    {
        frame_size.index = i;
        frame_size.code = MEDIA_BUS_FMT_SRGGB10_1X10;
        if (-1 == ioctl(g_fd, VIDIOC_SUBDEV_ENUM_FRAME_SIZE, &frame_size)) {
            errno_exit("VIDIOC_SIZE_INFO");
        }

        printf("image_size: width[%d] = %d, height[%d] = %d \n",
                    i, frame_size.min_width, i, frame_size.min_height);
    }
}

/**
    print usage information
*/
static void usage(FILE* fp, int argc, char** argv)
{
    fprintf(fp,
        "Usage: %s [options]\n\n"
        "Options:\n"
        "-d | --device name   Video device name [/dev/video0]\n"
        "-h | --help          Print this message\n"
        "-o | --output        Set JPEG output filename\n"
        "-q | --quality       Set JPEG quality (0-100)\n"
        "-m | --mmap          Use memory mapped buffer\n"
        "-r | --read          Use read() calls\n"
        "-u | --userptr       Use application allocated buffer\n"
        "-W | --width         Set image width\n"
        "-H | --height        Set image height\n"
        "-X | --left          Set image x start\n"
        "-Y | --up            Set image y start\n"
        "-R | --right         Set image x width\n"
        "-D | --down          Set image y height\n"
        "-I | --interval      Set frame interval (fps) (-1 to skip)\n"
        "-c | --continuous    Do continous capture, stop with SIGINT.\n"
        "-v | --version       Print version\n"
        "-f | --format        image format\n"
        "                0:V4L2_PIX_FMT_RGB565\n"
        "                1:V4L2_PIX_FMT_RGB24\n"
        "                2:V4L2_PIX_FMT_YUV420\n"
        "                3:V4L2_PIX_FMT_YUYV\n"
        "                4:V4L2_PIX_FMT_NV21\n"
        "                5:V4L2_PIX_FMT_NV12\n"
        "                6:V4L2_PIX_FMT_YVYU\n"
        "                7:V4L2_PIX_FMT_SRGGB12\n"
        "                8:V4L2_PIX_FMT_SGRBG12\n"
        "                9:V4L2_PIX_FMT_SGBRG12\n"
        "                10:V4L2_PIX_FMT_SBGGR12\n"
        "                default:V4L2_PIX_FMT_RGB565\n"
        "-t | --testfps       test fps\n"
        "-l | --loadfw        load stfisp fw image\n"
        "-s | --g_imagesize     print image size\n"
        "",
        argv[0]);
    }

static const char short_options [] = "d:ho:q:mruW:H:I:vcf:tX:Y:R:D:l:s";

static const struct option
long_options [] = {
    { "device",     required_argument,      NULL,           'd' },
    { "help",       no_argument,            NULL,           'h' },
    { "output",     required_argument,      NULL,           'o' },
    { "quality",    required_argument,      NULL,           'q' },
    { "mmap",       no_argument,            NULL,           'm' },
    { "read",       no_argument,            NULL,           'r' },
    { "userptr",    no_argument,            NULL,           'u' },
    { "width",      required_argument,      NULL,           'W' },
    { "height",     required_argument,      NULL,           'H' },
    { "left",      required_argument,      NULL,           'X' },
    { "up",       required_argument,      NULL,            'Y' },
    { "right",       required_argument,      NULL,         'R' },
    { "down",       required_argument,      NULL,          'D' },
    { "interval",   required_argument,      NULL,           'I' },
    { "version",    no_argument,            NULL,        'v' },
    { "continuous", no_argument,            NULL,        'c' },
    { "format",     required_argument,      NULL,        'f' },
    { "testfps",    no_argument,            NULL,        't' },
    { "loadfw",     required_argument,      NULL,        'l' },
    { "g_imagesize",  no_argument,            NULL,        's' },
    { 0, 0, 0, 0 }
};

int main(int argc, char **argv)
{
    for (;;) {
        int index, c = 0;

        c = getopt_long(argc, argv, short_options, long_options, &index);

        if (-1 == c)
            break;

        switch (c) {
            case 0: /* getopt_long() flag */
                break;

            case 'd':
                deviceName = optarg;
                break;

            case 'h':
                // print help
                usage(stdout, argc, argv);
                exit(EXIT_SUCCESS);

            case 'o':
                // set jpeg filename
                jpegFilename = optarg;
                break;

            case 'q':
                // set jpeg quality
                jpegQuality = atoi(optarg);
                break;

            case 'm':
#ifdef IO_MMAP
                g_io_mthd = IO_METHOD_MMAP;
#else
                fprintf(stderr, "You didn't compile for mmap support.\n");
                exit(EXIT_FAILURE);
#endif
                break;

            case 'r':
#ifdef IO_READ
                g_io_mthd = IO_METHOD_READ;
#else
                fprintf(stderr, "You didn't compile for read support.\n");
                exit(EXIT_FAILURE);
#endif
                break;

            case 'u':
#ifdef IO_USERPTR
                g_io_mthd = IO_METHOD_USERPTR;
#else
                fprintf(stderr, "You didn't compile for userptr support.\n");
                exit(EXIT_FAILURE);
#endif
                break;

            case 'W':
                // set width
                width = atoi(optarg);
                break;

            case 'H':
                // set height
                height = atoi(optarg);
                break;

            case 'X':
                // set x start
                left = atoi(optarg);
                crop_flag = 1;
                break;

            case 'Y':
                // set y start
                up = atoi(optarg);
                crop_flag = 1;
                break;

            case 'R':
                // set x width
                right = atoi(optarg);
                crop_flag = 1;
                break;

            case 'D':
                // set y height
                down = atoi(optarg);
                crop_flag = 1;
                break;

            case 'I':
                // set fps
                fps = atoi(optarg);
                break;

            case 'c':
                // set flag for continuous capture, interuptible by sigint
                continuous = 1;
                InstallSIGINTHandler();
                break;


            case 'v':
                printf("Version: %s\n", VERSION);
                exit(EXIT_SUCCESS);
                break;

            case 'f':
                printf("format: %s\n", optarg);
                format = atoi(optarg);
                switch (format) {
                    case  0:
                        format = V4L2_PIX_FMT_RGB565;
                        break;
                    case  1:
                        format = V4L2_PIX_FMT_RGB24;
                        break;
                    case  2:
                        format = V4L2_PIX_FMT_YUV420;
                        break;
                    case  3:
                        format = V4L2_PIX_FMT_YUYV;
                        break;
                    case  4:
                        format = V4L2_PIX_FMT_NV21;
                        break;
                    case  5:
                        format = V4L2_PIX_FMT_NV12;
                        break;
                    case  6:
                        format = V4L2_PIX_FMT_YVYU;
                        break;
                    case  7:
                        format = V4L2_PIX_FMT_SRGGB12;
                        break;
                    case  8:
                        format = V4L2_PIX_FMT_SGRBG12;
                        break;
                    case  9:
                        format = V4L2_PIX_FMT_SGBRG12;
                        break;
                    case  10:
                        format = V4L2_PIX_FMT_SBGGR12;
                        break;
                    default:
                        format = V4L2_PIX_FMT_RGB565;
                        break;
                }
                break;

            case 't':
                test_fps = 1;
                break;
            case 'l':
                loadfw_start(optarg);
                exit(EXIT_SUCCESS);
                break;
            case 's':
                sensor_image_size_info();
                exit(EXIT_SUCCESS);
                break;
            default:
                usage(stderr, argc, argv);
                exit(EXIT_FAILURE);
        }
    }

    // open and initialize device
    deviceOpen();
    deviceInit();

    test_float_1();
    // start capturing
    captureStart();

    // process frames
    mainLoop();

    // stop capturing
    captureStop();

    // close device
    deviceUninit();
    deviceClose();

    exit(EXIT_SUCCESS);

    return 0;
}
