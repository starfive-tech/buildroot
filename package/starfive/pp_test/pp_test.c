// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 StarFive Technology Co., Ltd.
 */
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
#include <stdbool.h>
#include <asm/types.h>
#include <linux/videodev2.h>
// #include "jpeglib.h"
// #include <libv4l2.h>
#include <signal.h>
#include <stdint.h>
#include <inttypes.h>
#include <linux/fb.h>

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
    bool bus_out;    /*out to ddr*/
    bool fifo_out;    /*out to lcdc*/
    bool inited;
    struct pp_video_mode src;
    struct pp_video_mode dst;
};

#define FBIOPAN_GET_PP_MODE    0x4609
#define FBIOPAN_SET_PP_MODE    0x460a

static unsigned char *g_fb_buf = NULL;
static int g_fb_fd = -1;

long g_screensize = 1920 * 1080 * 2;
//static unsigned char jpegQuality = 70;
struct fb_var_screeninfo g_vinfo;
struct fb_fix_screeninfo g_finfo;

static const char short_options[] = "f:";

static const struct option long_options[] = {
    { "format",     required_argument,      NULL,        'f' },
    { 0, 0, 0, 0 }
};

/**
    print usage information
*/
static void usage(FILE* fp, int argc, char** argv)
{
    fprintf(fp,
        "Usage: %s [options]\n\n"
        "Options:\n"
        "-f | --format        image format RGB/Yuv\n"
        "",
        argv[0]);
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

//Y' = 0.257*R' + 0.504*G' + 0.098*B' + 16
static int Rgb2Y(int r0, int g0, int b0)
{
    // float y0 = 0.257f*r0 + 0.504f*g0 + 0.098f*b0 + 16.0f;
    // int y0 = (257*r0 + 504*g0 + 98*b0)/1000 + 16;
    // Y = (77*R + 150*G + 29*B)>>8;
    int y0 = (77*r0+150*g0+29*b0) >> 8;
    return y0;
}

//U equals Cb'
//Cb' = -0.148*R' - 0.291*G' + 0.439*B' + 128
static int Rgb2U(int r0, int g0, int b0)
{
    // float u0 = -0.148f*r0 - 0.291f*g0 + 0.439f*b0 + 128.0f;
    // int u0 = (-148*r0 - 291*g0 + 439*b0)/1000 + 128;
    // U = ((-44*R  - 87*G  + 131*B)>>8) + 128;
    int u0 = ((-44*r0  - 87*g0  + 131*b0)>>8) + 128;
    return u0;
}

//V equals Cr'
//Cr' = 0.439*R' - 0.368*G' - 0.071*B' + 128
static int Rgb2V(int r0, int g0, int b0)
{
    // float v0 = 0.439f*r0 - 0.368f*g0 - 0.071f*b0 + 128.0f;
    // int v0 = (439*r0 - 368*g0 - 71*b0)/1000 + 128;
    // V = ((131*R - 110*G - 21*B)>>8) + 128 ;
    int v0 = ((131*r0 - 110*g0 - 21*b0)>>8) + 128;
    return v0;
}

//Convert two rows from RGB to two Y rows, and one row of interleaved U,V.
//I0 and I1 points two sequential source rows.
//I0 -> rgbrgbrgbrgbrgbrgb...
//I1 -> rgbrgbrgbrgbrgbrgb...
//Y0 and Y1 points two sequential destination rows of Y plane.
//Y0 -> yyyyyy
//Y1 -> yyyyyy
//UV0 points destination rows of interleaved UV plane.
//UV0 -> uvuvuv
static void Rgb2NV12TwoRows(const unsigned char I0[],
                            const unsigned char I1[],
                            int step,
                            const int image_width,
                            unsigned char Y0[],
                            unsigned char Y1[],
                            unsigned char UV0[],
                            int is_nv21)
{
    int x;  //Column index

    //Process 4 source pixels per iteration (2 pixels of row I0 and 2 pixels of row I1).
    for (x = 0; x < image_width; x += 2) {
        //Load R,G,B elements from first row (and convert to int).
        unsigned char b00 = (I0[x*step + 0] & 0x1F) << 3;
        unsigned char g00 = ((I0[x*step + 1] & 0x7) << 3 | I0[x*step + 0] >> 5) << 2;
        unsigned char r00 = I0[x*step + 1] & (~0x7);

        //Load next R,G,B elements from first row (and convert to int).
        unsigned char b01 = (I0[x*step + step+0] & 0x1F) << 3;
        unsigned char g01 = ((I0[x*step + step+1] & 0x7) << 3 | I0[x*step + step+0] >> 5) << 2;
        unsigned char r01 = I0[x*step + step+1] & (~0x7);

        //Load R,G,B elements from second row (and convert to int).
        unsigned char b10 = (I1[x*step + 0] & 0x1F) << 3;
        unsigned char g10 = ((I1[x*step + 1] & 0x7) << 3 | I1[x*step + 0] >> 5) << 2;
        unsigned char r10 = I1[x*step + 1] & (~0x7);

        //Load next R,G,B elements from second row (and convert to int).
        unsigned char b11 = (I1[x*step + step+0] & 0x1F) << 3;
        unsigned char g11 = ((I1[x*step + step+1] & 0x7) << 3 | I1[x*step + step+0] >> 5) << 2;
        unsigned char r11 = I1[x*step + step+1] & (~0x7);

        //Calculate 4 Y elements.
        unsigned char y00 = Rgb2Y(r00, g00, b00);
        unsigned char y01 = Rgb2Y(r01, g01, b01);
        unsigned char y10 = Rgb2Y(r10, g10, b10);
        unsigned char y11 = Rgb2Y(r11, g11, b11);

        //Calculate 4 U elements.
        unsigned char u00 = Rgb2U(r00, g00, b00);
        unsigned char u01 = Rgb2U(r01, g01, b01);
        unsigned char u10 = Rgb2U(r10, g10, b10);
        unsigned char u11 = Rgb2U(r11, g11, b11);

        //Calculate 4 V elements.
        unsigned char v00 = Rgb2V(r00, g00, b00);
        unsigned char v01 = Rgb2V(r01, g01, b01);
        unsigned char v10 = Rgb2V(r10, g10, b10);
        unsigned char v11 = Rgb2V(r11, g11, b11);

        //Calculate destination U element: average of 2x2 "original" U elements.
        unsigned char u0 = (u00 + u01 + u10 + u11)/4;

        //Calculate destination V element: average of 2x2 "original" V elements.
        unsigned char v0 = (v00 + v01 + v10 + v11)/4;

        //Store 4 Y elements (two in first row and two in second row).
        Y0[x + 0]    = y00;
        Y0[x + 1]    = y01;
        Y1[x + 0]    = y10;
        Y1[x + 1]    = y11;

        if (is_nv21) {
            // //Store destination U element.
            UV0[x + 0]    = v0;

            // //Store destination V element (next to stored U element).
            UV0[x + 1]    = u0;
        } else {
            // //Store destination U element.
            UV0[x + 0]    = u0;

            // //Store destination V element (next to stored U element).
            UV0[x + 1]    = v0;
        }
    }
}

//Convert image I from pixel ordered RGB to NV12 format.
//I - Input image in pixel ordered RGB format
//image_width - Number of columns of I
//image_height - Number of rows of I
//J - Destination "image" in NV12 format.

//I is pixel ordered RGB color format (size in bytes is image_width*image_height*3):
//RGBRGBRGBRGBRGBRGB
//RGBRGBRGBRGBRGBRGB
//RGBRGBRGBRGBRGBRGB
//RGBRGBRGBRGBRGBRGB
//
//J is in NV12 format (size in bytes is image_width*image_height*3/2):
//YYYYYY
//YYYYYY
//UVUVUV
//Each element of destination U is average of 2x2 "original" U elements
//Each element of destination V is average of 2x2 "original" V elements
//
//Limitations:
//1. image_width must be a multiple of 2.
//2. image_height must be a multiple of 2.
//3. I and J must be two separate arrays (in place computation is not supported).
void Rgb2NV12(const unsigned char I[], int step,
              const int image_width,
              const int image_height,
              unsigned char J[],
              int is_nv21)
{
    //In NV12 format, UV plane starts below Y plane.
    // unsigned char *UV = &J[image_width*image_height];
    unsigned char *UV = J;

    //I0 and I1 points two sequential source rows.
    const unsigned char *I0;  //I0 -> rgbrgbrgbrgbrgbrgb...
    const unsigned char *I1;  //I1 -> rgbrgbrgbrgbrgbrgb...

    //Y0 and Y1 points two sequential destination rows of Y plane.
    unsigned char *Y0;    //Y0 -> yyyyyy
    unsigned char *Y1;    //Y1 -> yyyyyy

    //UV0 points destination rows of interleaved UV plane.
    unsigned char *UV0; //UV0 -> uvuvuv

    int y;  //Row index
    int width, height;
    int x_offset, y_offset;

    width = image_width > g_vinfo.xres ? g_vinfo.xres : image_width;
    height = image_height > g_vinfo.yres ? g_vinfo.yres : image_height;
    x_offset = (g_vinfo.xres - width) / 2;
    y_offset = (g_vinfo.yres - height) / 2;

    //In each iteration: process two rows of Y plane, and one row of interleaved UV plane.
    for (y = 0; y < height; y += 2)
    {
        I0 = &I[y*image_width*step];        //Input row width is image_width*3 bytes (each pixel is R,G,B).
        I1 = &I[(y+1)*image_width*step];

        Y0 = &J[(y+y_offset)*g_vinfo.xres+x_offset];            //Output Y row width is image_width bytes (one Y element per pixel).
        Y1 = &J[(y+1+y_offset)*g_vinfo.xres+x_offset];

        UV0 = &UV[g_vinfo.xres*g_vinfo.yres+((y+y_offset)/2*g_vinfo.xres/2+x_offset/2)*2];    //Output UV row - width is same as Y row width.

        //Process two source rows into: Two Y destination row, and one destination interleaved U,V row.
        Rgb2NV12TwoRows(I0,
                        I1,
                        step,
                        width,
                        Y0,
                        Y1,
                        UV0,
                        is_nv21);
    }
}

int convert_rgb565_to_nv12(unsigned char *inBuf, unsigned char *outBuf, int imgWidth, int imgHeight, int is_nv21)
{
    unsigned char *tmp = malloc(g_screensize);
    unsigned int start_timems;
    unsigned int end_timems;
    struct timespec ts_start, ts_end;

    printf("convert rgb565 to %s\n", is_nv21 ? "nv21" : "nv12");
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    Rgb2NV12(inBuf, 2, imgWidth, imgHeight, tmp, is_nv21);

    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    start_timems = ts_start.tv_sec * 1000 + ts_start.tv_nsec/1000000;
    end_timems = ts_end.tv_sec * 1000 + ts_end.tv_nsec/1000000;
    printf("%s: convert use %dms\n", __func__, end_timems - start_timems);

    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    memcpy(outBuf, tmp, g_screensize);

    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    start_timems = ts_start.tv_sec * 1000 + ts_start.tv_nsec/1000000;
    end_timems = ts_end.tv_sec * 1000 + ts_end.tv_nsec/1000000;
    printf("%s: use %dms\n", __func__, end_timems - start_timems);

    free(tmp);
    return 0;
}

int convert_rgb565_to_rgb(unsigned char *inBuf, unsigned char *outBuf, int imgWidth, int imgHeight, int cvtMethod)
{
    int rows ,cols;
    unsigned char *RGB565data, *RGBdata;
    int RGBpos;
    int width, height;
    int x_offset, y_offset;
    unsigned char *tmp = malloc(g_screensize);

    printf("%s, %d\n", __func__, __LINE__);

    width = imgWidth > g_vinfo.xres ? g_vinfo.xres : imgWidth;
    height = imgHeight > g_vinfo.yres ? g_vinfo.yres : imgHeight;
    x_offset = (g_vinfo.xres - width) / 2;
    y_offset = (g_vinfo.yres - height) / 2;

    RGB565data = inBuf;
    RGBdata = tmp;

    if (imgWidth == g_vinfo.xres) {
        RGBpos = (y_offset * g_vinfo.xres + x_offset) * 2;
        memcpy(&tmp[RGBpos], inBuf, imgWidth * height * 2);
        memcpy(&outBuf[RGBpos], &tmp[RGBpos], imgWidth * height * 2);
        // memcpy(&outBuf[RGBpos], inBuf, imgWidth * height * 2);
        free(tmp);
        return 0;
    }

    RGBpos = 0;
    for(rows = 0; rows < imgHeight; rows++)
    {
        RGBdata = tmp + ((rows + y_offset) * g_vinfo.xres + x_offset) * g_vinfo.bits_per_pixel / 8;
        RGBpos = rows * imgWidth * 2;
        if (g_vinfo.bits_per_pixel == 16) {   // RGB565
            memcpy(RGBdata, &RGB565data[RGBpos], imgWidth * 2);
        } else {
            for(cols = 0; cols < imgWidth; cols++)
            {
                *(RGBdata ++) = RGB565data[RGBpos] & 0x1F;
                *(RGBdata ++) = (RGB565data[RGBpos + 1] & 0x7) << 3 | RGB565data[RGBpos] >> 5;
                *(RGBdata ++) = RGB565data[RGBpos + 1] >> 3;
                if (g_vinfo.bits_per_pixel == 32) {   // RGB888
                    *(RGBdata ++) = 0xFF;
                }
                RGBpos += 2;
            }
        }
    }

    memcpy(outBuf, tmp, g_screensize);
    free(tmp);
    return 0;
}

static void deviceInit(void)
{
    g_fb_fd = open("/dev/fb0", O_RDWR);
    if (g_fb_fd == -1) {
        printf("Error: cannot open framebuffer device.\n");
        exit (EXIT_FAILURE);
    }

    // Get fixed screen information
    if (-1 == ioctl(g_fb_fd, FBIOGET_FSCREENINFO, &g_finfo)) {
        printf("Error reading fixed information.\n");
        exit (EXIT_FAILURE);
    }

    // Get variable screen information
    if (-1 == ioctl(g_fb_fd, FBIOGET_VSCREENINFO, &g_vinfo)) {
        printf("Error reading variable information.\n");
        exit (EXIT_FAILURE);
    }

    printf("g_vinfo.xres = %d, g_vinfo.yres = %d, grayscale = %d\n", g_vinfo.xres, g_vinfo.yres, g_vinfo.grayscale);
    printf("g_vinfo.xoffset = %d, g_vinfo.yoffset = %d\n", g_vinfo.xoffset, g_vinfo.yoffset);
    printf("g_vinfo.bits_per_pixel = %d, g_finfo.line_length = %d\n", g_vinfo.bits_per_pixel, g_finfo.line_length);

    g_screensize = g_vinfo.xres * g_vinfo.yres * g_vinfo.bits_per_pixel / 8;
    //mmap framebuffer
    g_fb_buf = (unsigned char *)mmap(NULL, g_screensize, PROT_READ | PROT_WRITE, MAP_SHARED, g_fb_fd, 0);
    if (g_fb_buf == (void *)(-1)) {
        printf("Error: failed to map framebuffer device to memory.\n");
        exit (EXIT_FAILURE) ;
    }
    memset(g_fb_buf, 0x00, g_screensize);
}

static void deviceUninit(void)
{
    if (-1 == munmap((void *)g_fb_buf, g_screensize)) {
        printf(" Error: framebuffer device munmap() failed.\n");
        exit (EXIT_FAILURE) ;
    }
    close(g_fb_fd);
}

int main(int argc, char **argv)
{
    struct pp_mode pp_info[3];
    int stfbc_fd = -1;
    int format = 0;
    int i, j;
    unsigned char *rgb565demo_buffer = NULL;
    int pixformat = COLOR_YUV420_NV21;  // or COLOR_RGB565

    for (;;) {
        int index, c = 0;
        c = getopt_long(argc, argv, short_options, long_options, &index);
        if (-1 == c) {
            break;
        }

        switch (c) {
        case 0: /* getopt_long() flag */
            break;

        case 'f':
            printf("format: %s\n", optarg);
            format = atoi(optarg);
            switch (format) {
            case  0:
                pixformat = COLOR_RGB565;
                break;
            case  1:
                pixformat = COLOR_RGB888_ARGB;
                break;
            case  2:
                pixformat = COLOR_YUV420P;
                break;
            case  3:
                pixformat = COLOR_YUV422_YUYV;
                break;
            case  4:
                pixformat = COLOR_YUV420_NV21;
                break;
            case  5:
                pixformat = COLOR_YUV420_NV12;
                break;
            case  6:
                pixformat = COLOR_YUV422_YVYU;
                break;
            default:
                pixformat = COLOR_RGB565;
                break;
            }
            break;

        default:
            usage(stderr, argc, argv);
            exit(EXIT_FAILURE);
        }
    }

    stfbc_fd = open("/dev/stfbcdev", O_RDWR);
    if (stfbc_fd == -1) {
        printf("Error: cannot open framebuffer device./n");
        exit (EXIT_FAILURE);
    }

    if (-1 == ioctl(stfbc_fd, FBIOPAN_GET_PP_MODE, &pp_info[0])) {
        printf("Error reading variable information./n");
        exit (EXIT_FAILURE);
    }
    printf(" get pp format :%d,%d\n",pp_info[1].src.format,__LINE__);

    pp_info[1].src.format = pixformat;

    if (-1 == ioctl(stfbc_fd, FBIOPAN_SET_PP_MODE, &pp_info[0])) {
        printf("Error reading variable information./n");
        exit (EXIT_FAILURE);
    }

    if (-1 == ioctl(stfbc_fd, FBIOPAN_GET_PP_MODE, &pp_info[0])) {
        printf("Error reading variable information./n");
        exit (EXIT_FAILURE);
    }
    printf(" get pp format :%d,%d\n",pp_info[1].src.format,__LINE__);
    pixformat = pp_info[1].src.format;

    deviceInit();

    rgb565demo_buffer = malloc(g_screensize);
    if (!rgb565demo_buffer) {
        exit (EXIT_FAILURE);
    }

    unsigned char *dst = NULL;
    dst = malloc(g_screensize);
    if (!dst) {
        exit (EXIT_FAILURE);
    }

    // init rgb raw data
    for (j = 0; j < g_vinfo.yres/4; j++)
        for (i = 0; i < g_vinfo.xres; i++) {
            rgb565demo_buffer[2*(j*g_vinfo.xres+i)] = 0b00000000;
            rgb565demo_buffer[2*(j*g_vinfo.xres+i)+1] = 0b11111000;
        }
    for (j = g_vinfo.yres/4; j < g_vinfo.yres/2; j++)
        for (i = 0; i < g_vinfo.xres; i++) {
            rgb565demo_buffer[2*(j*g_vinfo.xres+i)] = 0b11100000;
            rgb565demo_buffer[2*(j*g_vinfo.xres+i)+1] = 0b00000111;
    }

    for (j = g_vinfo.yres/2; j < g_vinfo.yres * 3/4; j++)
        for (i = 0; i < g_vinfo.xres; i++) {
            rgb565demo_buffer[2*(j*g_vinfo.xres+i)] = 0b00011111;
            rgb565demo_buffer[2*(j*g_vinfo.xres+i)+1] = 0b00000000;
        }
    for (j = g_vinfo.yres * 3/4; j < g_vinfo.yres; j++)
        for (i = 0; i < g_vinfo.xres; i++) {
            rgb565demo_buffer[2*(j*g_vinfo.xres+i)] = 0b11111111;
            rgb565demo_buffer[2*(j*g_vinfo.xres+i)+1] = 0b11111111;
        }
    switch (pixformat) {
    case COLOR_RGB565:
        convert_rgb565_to_rgb(rgb565demo_buffer, g_fb_buf, g_vinfo.xres, g_vinfo.yres, 1);
        convert_rgb565_to_rgb(rgb565demo_buffer, dst, g_vinfo.xres, g_vinfo.yres, 1);
        write_file("test_format-rgb565.raw", dst, g_screensize);
        break;
    case COLOR_RGB888_ARGB:
        break;
    case COLOR_YUV420P:
        break;
    case COLOR_YUV422_YUYV:
        break;
    case COLOR_YUV420_NV21:
        convert_rgb565_to_nv12(rgb565demo_buffer, g_fb_buf, g_vinfo.xres, g_vinfo.yres, 1);
        convert_rgb565_to_nv12(rgb565demo_buffer, dst, g_vinfo.xres, g_vinfo.yres, 1);
        write_file("test_format-nv21.raw", dst, g_screensize);
        break;
    case COLOR_YUV420_NV12:
        convert_rgb565_to_nv12(rgb565demo_buffer, g_fb_buf, g_vinfo.xres, g_vinfo.yres, 0);
        convert_rgb565_to_nv12(rgb565demo_buffer, dst, g_vinfo.xres, g_vinfo.yres, 0);
        write_file("test_format-nv12.raw", dst, g_screensize);
        break;
    case COLOR_YUV422_YVYU:
        break;
    default:
        break;
    }

    write_file("test_rgb565.raw", rgb565demo_buffer, g_screensize);
    free(rgb565demo_buffer);
    free(dst);
    sleep(10);
    deviceUninit();
    return 0;
}
