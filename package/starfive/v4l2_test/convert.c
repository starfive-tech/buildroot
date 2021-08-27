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
#include <asm/types.h>
#include <signal.h>
#include <stdint.h>
#include <inttypes.h>
#include <linux/fb.h>

extern struct fb_var_screeninfo vinfo;
extern struct fb_fix_screeninfo finfo;
extern int screensize;

int yuyv_resize(unsigned char *inBuf, unsigned char *outBuf, int imgWidth, int imgHeight)
{
    int rows;
    unsigned char *YUVindata, *YUVoutdata;    /* YUV and RGB pointer */
    int YUVinpos;    /* Y U V offset */
    int width, height;
    int x_offset, y_offset;
    unsigned char *tmp = malloc(screensize);
    unsigned int start_timems;
    unsigned int end_timems;
    struct timeval ts_start, ts_end;

    if (!tmp)
        return -1;

    gettimeofday(&ts_start, NULL);

    width = imgWidth > vinfo.xres ? vinfo.xres : imgWidth;
    height = imgHeight > vinfo.yres ? vinfo.yres : imgHeight;
    x_offset = (vinfo.xres - width) / 2;
    y_offset = (vinfo.yres - height) / 2;

    YUVindata = inBuf;
    YUVoutdata = tmp;

    if (imgWidth == vinfo.xres) {
        YUVinpos = (y_offset * vinfo.xres + x_offset) * 2;
        memcpy(&tmp[YUVinpos], inBuf, imgWidth * height * 2);
        memcpy(&outBuf[YUVinpos], &tmp[YUVinpos], imgWidth * height * 2);
        // memcpy(&outBuf[YUVinpos], inBuf, imgWidth * height * 2);
        gettimeofday(&ts_end, NULL);
        start_timems = ts_start.tv_sec * 1000 + ts_start.tv_usec/1000;
        end_timems = ts_end.tv_sec * 1000 + ts_end.tv_usec/1000;
        // printf("%s: copy use %dms, sizeof(int) = %d\n", __func__, end_timems - start_timems, sizeof(int));
        free(tmp);
        return 0;
    }

    /* two bytes for one pixels */
    for(rows = 0; rows < height; rows++)
    {
        // vinfo.xres, vinfo.yres vinfo.bits_per_pixel
        YUVoutdata = tmp + ((rows + y_offset) * vinfo.xres + x_offset) * 2;
        YUVinpos = rows * imgWidth * 2;

        memcpy(YUVoutdata, &YUVindata[YUVinpos], imgWidth * 2);
    }

    gettimeofday(&ts_end, NULL);
    start_timems = ts_start.tv_sec * 1000000 + ts_start.tv_usec;
    end_timems = ts_end.tv_sec * 1000000 + ts_end.tv_usec;
    // printf("%s: convert use %dus\n", __func__, end_timems - start_timems);

    gettimeofday(&ts_start, NULL);

    memcpy(outBuf, tmp, screensize);

    gettimeofday(&ts_end, NULL);
    start_timems = ts_start.tv_sec * 1000 + ts_start.tv_usec/1000;
    end_timems = ts_end.tv_sec * 1000 + ts_end.tv_usec/1000;
    // printf("%s: copy use %dms, sizeof(int) = %d\n", __func__, end_timems - start_timems, sizeof(int));

    free(tmp);
    return 0;

}

int convert_yuyv_to_nv12(unsigned char *inBuf, unsigned char *outBuf, int imgWidth, int imgHeight, int is_yuyv)
{
    int rows, cols;
    unsigned char *nv12data, *YUVdata;
    int Ypos;
    int fb_Ypos, fb_Upos, fb_Vpos;
    int width, height;
    int x_offset, y_offset;
    unsigned char *tmp = malloc(screensize);
    unsigned int start_timems;
    unsigned int end_timems;
    struct timeval ts_start, ts_end;

    if (!tmp)
        return -1;

    gettimeofday(&ts_start, NULL);

    width = imgWidth > vinfo.xres ? vinfo.xres : imgWidth;
    height = imgHeight > vinfo.yres ? vinfo.yres : imgHeight;
    x_offset = (vinfo.xres - width) / 2;
    y_offset = (vinfo.yres - height) / 2;

    YUVdata = inBuf;
    nv12data = tmp;

    /* two bytes for every pixels */
    for(rows = 0; rows < height; rows++)
    {
        // vinfo.xres, vinfo.yres vinfo.bits_per_pixel
        fb_Ypos = ((rows + y_offset) * vinfo.xres + x_offset);
        fb_Upos = ((rows + y_offset) / 2 * vinfo.xres / 2 + x_offset / 2) * 2;
        fb_Upos = vinfo.xres * vinfo.yres + fb_Upos;
        fb_Vpos = fb_Upos + 1;

        Ypos = rows * imgWidth * 2;

        for (cols = 0; cols < width; cols += 2) {
            nv12data[fb_Ypos+cols] = YUVdata[Ypos+cols*2];
            nv12data[fb_Ypos+cols+1] = YUVdata[Ypos+cols*2+2];
            nv12data[fb_Upos+cols] = YUVdata[Ypos+cols*2+1];
            nv12data[fb_Vpos+cols] = YUVdata[Ypos+cols*2+3];
        }
    }

    gettimeofday(&ts_end, NULL);
    start_timems = ts_start.tv_sec * 1000000 + ts_start.tv_usec;
    end_timems = ts_end.tv_sec * 1000000 + ts_end.tv_usec;
    // printf("%s: convert use %dus\n", __func__, end_timems - start_timems);

    gettimeofday(&ts_start, NULL);

    memcpy(outBuf, tmp, screensize);

    gettimeofday(&ts_end, NULL);
    start_timems = ts_start.tv_sec * 1000 + ts_start.tv_usec/1000;
    end_timems = ts_end.tv_sec * 1000 + ts_end.tv_usec/1000;
    // printf("%s: copy use %dms, sizeof(int) = %d\n", __func__, end_timems - start_timems, sizeof(int));

    free(tmp);
    return 0;
}

int convert_nv21_to_nv12(unsigned char *inBuf, unsigned char *outBuf, int imgWidth, int imgHeight, int is_nv21)
{
    int rows, cols;
    unsigned char *nv12data, *nv21data;
    int Ypos, Upos, Vpos;
    int fb_Ypos, fb_Upos, fb_Vpos;
    int width, height;
    int x_offset, y_offset;
    unsigned char *tmp = malloc(screensize);
    unsigned int start_timems;
    unsigned int end_timems;
    struct timeval ts_start, ts_end;

    if (!tmp)
        return -1;

    gettimeofday(&ts_start, NULL);

    width = imgWidth > vinfo.xres ? vinfo.xres : imgWidth;
    height = imgHeight > vinfo.yres ? vinfo.yres : imgHeight;
    x_offset = (vinfo.xres - width) / 2;
    y_offset = (vinfo.yres - height) / 2;

    nv21data = inBuf;
    nv12data = tmp;

    if (imgWidth == vinfo.xres) {
        fb_Ypos = y_offset * vinfo.xres + x_offset;
        fb_Upos = (y_offset / 2 * vinfo.xres / 2 + x_offset / 2) * 2;
        fb_Upos = vinfo.xres * vinfo.yres + fb_Upos;
        Upos = imgWidth * imgHeight;
        memcpy(&tmp[fb_Ypos], inBuf, imgWidth * height);
        memcpy(&tmp[fb_Upos], &inBuf[Upos], imgWidth * height / 2);
        memcpy(&outBuf[fb_Ypos], &tmp[fb_Ypos], imgWidth * height * 2);
        memcpy(&outBuf[fb_Upos], &tmp[fb_Upos], imgWidth * height / 2);
        // memcpy(&outBuf[fb_Ypos], inBuf, imgWidth * height);
        // memcpy(&outBuf[fb_Upos], inBuf, imgWidth * height / 2);
        free(tmp);
        return 0;
    }

    /* two bytes for every pixels */
    for(rows = 0; rows < height; rows+=2)
    {
        // vinfo.xres, vinfo.yres vinfo.bits_per_pixel
        fb_Ypos = ((rows + y_offset) * vinfo.xres + x_offset);
        fb_Upos = ((rows + y_offset) / 2 * vinfo.xres / 2 + x_offset / 2) * 2;
        fb_Upos = vinfo.xres * vinfo.yres + fb_Upos;
        fb_Vpos = fb_Upos + 1;

        Ypos = rows * imgWidth;
        Upos = imgWidth * imgHeight + Ypos / 2;
        Vpos = Upos + 1;
        memcpy(&nv12data[fb_Ypos], &nv21data[Ypos], width);
        memcpy(&nv12data[fb_Ypos+vinfo.xres], &nv21data[Ypos+imgWidth], width);

        if (is_nv21) {
            for (cols = 0; cols < width; cols += 2) {
                nv12data[fb_Upos+cols] = nv21data[Vpos+cols];
                nv12data[fb_Vpos+cols] = nv21data[Upos+cols];
            }
        } else
            memcpy(&nv12data[fb_Upos], &nv21data[Upos], width);
    }

    gettimeofday(&ts_end, NULL);
    start_timems = ts_start.tv_sec * 1000000 + ts_start.tv_usec;
    end_timems = ts_end.tv_sec * 1000000 + ts_end.tv_usec;
    // printf("%s: convert use %dus\n", __func__, end_timems - start_timems);

    gettimeofday(&ts_start, NULL);

    memcpy(outBuf, tmp, screensize);

    gettimeofday(&ts_end, NULL);
    start_timems = ts_start.tv_sec * 1000 + ts_start.tv_usec/1000;
    end_timems = ts_end.tv_sec * 1000 + ts_end.tv_usec/1000;
    // printf("%s: copy use %dms, sizeof(int) = %d\n", __func__, end_timems - start_timems, sizeof(int));

    free(tmp);
    return 0;
}

int convert_nv21_to_rgb(unsigned char *inBuf, unsigned char *outBuf, int imgWidth, int imgHeight, int is_nv21)
{
    int rows ,cols;
    int y, u, v, r, g, b;
    unsigned char *YUVdata, *RGBdata;
    int Ypos, Upos, Vpos;
    unsigned int i = 0;
    int width, height;
    int x_offset, y_offset;
    unsigned char *tmp = malloc(screensize);
    unsigned int start_timems;
    unsigned int end_timems;
    struct timeval ts_start, ts_end;

    if (!tmp)
        return -1;

    gettimeofday(&ts_start, NULL);

    width = imgWidth > vinfo.xres ? vinfo.xres : imgWidth;
    height = imgHeight > vinfo.yres ? vinfo.yres : imgHeight;
    x_offset = (vinfo.xres - width) / 2;
    y_offset = (vinfo.yres - height) / 2;

    YUVdata = inBuf;
    RGBdata = tmp;

    /* two bytes for every pixels */
    for(rows = 0; rows < height; rows++)
    {
        // vinfo.xres, vinfo.yres vinfo.bits_per_pixel
        RGBdata = tmp + ((rows + y_offset) * vinfo.xres + x_offset) * vinfo.bits_per_pixel / 8;

        Ypos = rows * imgWidth;
        Vpos = Upos = imgWidth * imgHeight + Ypos / 2;
        if (is_nv21)
            Vpos = Upos + 1;
        else
            Upos = Vpos + 1;
        i = 0;

        for (cols = 0; cols < width; cols++)
        {
            y = YUVdata[Ypos];
            u = YUVdata[Upos] - 128;
            v = YUVdata[Vpos] - 128;

            r = y + v + ((v * 103) >> 8);
            g = y - ((u * 88) >> 8) - ((v * 183) >> 8);
            b = y + u + ((u * 198) >> 8);

            r = r > 255 ? 255 : (r < 0 ? 0 : r);
            g = g > 255 ? 255 : (g < 0 ? 0 : g);
            b = b > 255 ? 255 : (b < 0 ? 0 : b);

            /* low -> high r g b */
            if (vinfo.bits_per_pixel == 16) {   // RGB565
                *(RGBdata ++) = (((g & 0x1c) << 3) | (b >> 3));   /* g low 5bit，b high 5bit */
                *(RGBdata ++) = ((r & 0xf8) | (g >> 5));    /* r high 5bit，g high 3bit */
            } else if (vinfo.bits_per_pixel == 24) {   // RGB888
                *(RGBdata ++) = b;
                *(RGBdata ++) = g;
                *(RGBdata ++) = r;
            } else { // RGB8888
                *(RGBdata ++) = b;
                *(RGBdata ++) = g;
                *(RGBdata ++) = r;
                *(RGBdata ++) = 0xFF;
            }
            Ypos++;
            i++;
            /* every 4 time y to update 1 time uv */
            if(!(i & 0x03))
            {
                Vpos = Upos = imgWidth * imgHeight + Ypos/2;
                if (is_nv21)
                    Vpos = Upos + 1;
                else
                    Upos = Vpos + 1;
            }
        }
    }

    gettimeofday(&ts_end, NULL);
    start_timems = ts_start.tv_sec * 1000000 + ts_start.tv_usec;
    end_timems = ts_end.tv_sec * 1000000 + ts_end.tv_usec;
    // printf("%s: convert use %dus\n", __func__, end_timems - start_timems);

    gettimeofday(&ts_start, NULL);

#if 1
    memcpy(outBuf, tmp, screensize);
#else
    int *p_outBuf, *p_tmp;
    int size = screensize/4;
    p_outBuf = outBuf;
    p_tmp = tmp;

    for (i = 0; i < size; i++)
        p_outBuf[i] = p_tmp[i];
#endif
    gettimeofday(&ts_end, NULL);
    start_timems = ts_start.tv_sec * 1000 + ts_start.tv_usec/1000;
    end_timems = ts_end.tv_sec * 1000 + ts_end.tv_usec/1000;
    // printf("%s: copy use %dms, sizeof(int) = %d\n", __func__, end_timems - start_timems, sizeof(int));

    free(tmp);
    return 0;
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
                            unsigned char UV0[])
{
    int x;  //Column index

    //Process 4 source pixels per iteration (2 pixels of row I0 and 2 pixels of row I1).
    for (x = 0; x < image_width; x += 2)
    {
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

        // //Store destination U element.
        UV0[x + 0]    = u0;

        // //Store destination V element (next to stored U element).
        UV0[x + 1]    = v0;
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
              unsigned char J[])
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

    width = image_width > vinfo.xres ? vinfo.xres : image_width;
    height = image_height > vinfo.yres ? vinfo.yres : image_height;
    x_offset = (vinfo.xres - width) / 2;
    y_offset = (vinfo.yres - height) / 2;

    //In each iteration: process two rows of Y plane, and one row of interleaved UV plane.
    for (y = 0; y < height; y += 2)
    {
        I0 = &I[y*image_width*step];        //Input row width is image_width*3 bytes (each pixel is R,G,B).
        I1 = &I[(y+1)*image_width*step];

        Y0 = &J[(y+y_offset)*vinfo.xres+x_offset];            //Output Y row width is image_width bytes (one Y element per pixel).
        Y1 = &J[(y+1+y_offset)*vinfo.xres+x_offset];

        UV0 = &UV[vinfo.xres*vinfo.yres+((y+y_offset)/2*vinfo.xres/2+x_offset/2)*2];    //Output UV row - width is same as Y row width.

        //Process two source rows into: Two Y destination row, and one destination interleaved U,V row.
        Rgb2NV12TwoRows(I0,
                        I1,
                        step,
                        width,
                        Y0,
                        Y1,
                        UV0);
    }
}

int convert_rgb565_to_nv12(unsigned char *inBuf, unsigned char *outBuf, int imgWidth, int imgHeight, int is_nv21)
{
    unsigned char *tmp = malloc(screensize);
    unsigned int start_timems;
    unsigned int end_timems;
    struct timespec ts_start, ts_end;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    Rgb2NV12(inBuf, 2, imgWidth, imgHeight, tmp);

    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    start_timems = ts_start.tv_sec * 1000 + ts_start.tv_nsec/1000000;
    end_timems = ts_end.tv_sec * 1000 + ts_end.tv_nsec/1000000;
    // printf("%s: convert use %dms\n", __func__, end_timems - start_timems);

    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    memcpy(outBuf, tmp, screensize);

    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    start_timems = ts_start.tv_sec * 1000 + ts_start.tv_nsec/1000000;
    end_timems = ts_end.tv_sec * 1000 + ts_end.tv_nsec/1000000;
    // printf("%s: use %dms\n", __func__, end_timems - start_timems);

    free(tmp);
    return 0;
}

int convert_yuyv_to_rgb(unsigned char *inBuf, unsigned char *outBuf, int imgWidth, int imgHeight, int cvtMethod)
{
    int rows ,cols;
    int y, u, v, r, g, b;
    unsigned char *YUVdata, *RGBdata;
    int Ypos, Upos, Vpos;
    unsigned int i = 0;
    int width, height;
    int x_offset, y_offset;
    unsigned char *tmp = malloc(screensize);
    unsigned int start_timems;
    unsigned int end_timems;
    struct timespec ts_start, ts_end;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    width = imgWidth > vinfo.xres ? vinfo.xres : imgWidth;
    height = imgHeight > vinfo.yres ? vinfo.yres : imgHeight;
    x_offset = (vinfo.xres - width) / 2;
    y_offset = (vinfo.yres - height) / 2;

    YUVdata = inBuf;
    RGBdata = tmp;

    /* two bytes for every pixels */
    for(rows = 0; rows < height; rows++)
    {
        // vinfo.xres, vinfo.yres vinfo.bits_per_pixel
        RGBdata = tmp + ((rows + y_offset) * vinfo.xres + x_offset) * vinfo.bits_per_pixel / 8;

        Ypos = rows * imgWidth * 2;
        Upos = Ypos + 1;
        Vpos = Upos + 2;
        i = 0;

        for(cols = 0; cols < width; cols++)
        {
            y = YUVdata[Ypos];
            u = YUVdata[Upos] - 128;
            v = YUVdata[Vpos] - 128;

            r = y + v + ((v * 103) >> 8);
            g = y - ((u * 88) >> 8) - ((v * 183) >> 8);
            b = y + u + ((u * 198) >> 8);

            r = r > 255 ? 255 : (r < 0 ? 0 : r);
            g = g > 255 ? 255 : (g < 0 ? 0 : g);
            b = b > 255 ? 255 : (b < 0 ? 0 : b);

            /* low -> high r g b */
            if (vinfo.bits_per_pixel == 16) {   // RGB565
                *(RGBdata ++) = (((g & 0x1c) << 3) | (b >> 3));    /* g low 5bits，b high 5bits */
                *(RGBdata ++) = ((r & 0xf8) | (g >> 5));    /* r high 5bits, g high 3bits */
            } else if (vinfo.bits_per_pixel == 24) {   // RGB888
                *(RGBdata ++) = b;
                *(RGBdata ++) = g;
                *(RGBdata ++) = r;
            } else { // RGB8888
                *(RGBdata ++) = b;
                *(RGBdata ++) = g;
                *(RGBdata ++) = r;
                *(RGBdata ++) = 0xFF;
            }
            /* two bytes contain 1 y */
            Ypos += 2;
            //Ypos++;
            i++;
            /* every 2 y to update 1 uv */
            if(!(i & 0x01))
            {
                Upos = Ypos + 1;
                Vpos = Upos + 2;
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    start_timems = ts_start.tv_sec * 1000 + ts_start.tv_nsec/1000000;
    end_timems = ts_end.tv_sec * 1000 + ts_end.tv_nsec/1000000;
    // printf("%s: convert use %dms\n", __func__, end_timems - start_timems);

    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    memcpy(outBuf, tmp, screensize);

    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    start_timems = ts_start.tv_sec * 1000 + ts_start.tv_nsec/1000000;
    end_timems = ts_end.tv_sec * 1000 + ts_end.tv_nsec/1000000;
    // printf("%s: use %dms\n", __func__, end_timems - start_timems);

    free(tmp);
    return 0;
}

int convert_yuv444_to_rgb(unsigned char *inBuf, unsigned char *outBuf, int imgWidth, int imgHeight, int cvtMethod)
{
    int rows ,cols;
    int y, u, v, r, g, b;
    unsigned char *YUVdata, *RGBdata;
    int Ypos;
    unsigned char *tmp = malloc(screensize);

    YUVdata = inBuf;
    RGBdata = tmp;
    /*  YUV */
    Ypos = 0;

    for(rows = 0; rows < imgHeight; rows++)
    {
        for(cols = 0; cols < imgWidth; cols++)
        {
            y = YUVdata[Ypos];
            u = YUVdata[Ypos + 1] - 128;
            v = YUVdata[Ypos + 2] - 128;

            r = y + v + ((v * 103) >> 8);
            g = y - ((u * 88) >> 8) - ((v * 183) >> 8);
            b = y + u + ((u * 198) >> 8);

            r = r > 255 ? 255 : (r < 0 ? 0 : r);
            g = g > 255 ? 255 : (g < 0 ? 0 : g);
            b = b > 255 ? 255 : (b < 0 ? 0 : b);

            /* low -> high r g b */
            if (vinfo.bits_per_pixel == 16) {   // RGB565
                *(RGBdata ++) = (((g & 0x1c) << 3) | (b >> 3));    /* g low 5bits，b high 5bits */
                *(RGBdata ++) = ((r & 0xf8) | (g >> 5));    /* r high 5bits，g high 3bits */
            } else if (vinfo.bits_per_pixel == 24) {   // RGB888
                *(RGBdata ++) = b;
                *(RGBdata ++) = g;
                *(RGBdata ++) = r;
            } else { // RGB8888
                *(RGBdata ++) = b;
                *(RGBdata ++) = g;
                *(RGBdata ++) = r;
                *(RGBdata ++) = 0xFF;
            }
            Ypos += 3;
        }
    }

    memcpy(outBuf, tmp, screensize);
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
    unsigned char *tmp = malloc(screensize);

    width = imgWidth > vinfo.xres ? vinfo.xres : imgWidth;
    height = imgHeight > vinfo.yres ? vinfo.yres : imgHeight;
    x_offset = (vinfo.xres - width) / 2;
    y_offset = (vinfo.yres - height) / 2;

    RGB565data = inBuf;
    RGBdata = tmp;

    if (imgWidth == vinfo.xres) {
        RGBpos = (y_offset * vinfo.xres + x_offset) * 2;
        memcpy(&tmp[RGBpos], inBuf, imgWidth * height * 2);
        memcpy(&outBuf[RGBpos], &tmp[RGBpos], imgWidth * height * 2);
        // memcpy(&outBuf[RGBpos], inBuf, imgWidth * height * 2);
        free(tmp);
        return 0;
    }

    RGBpos = 0;
    for(rows = 0; rows < imgHeight; rows++)
    {
        RGBdata = tmp + ((rows + y_offset) * vinfo.xres + x_offset) * vinfo.bits_per_pixel / 8;
        RGBpos = rows * imgWidth * 2;
        if (vinfo.bits_per_pixel == 16) {   // RGB565
            memcpy(RGBdata, &RGB565data[RGBpos], imgWidth * 2);
        } else {
            for(cols = 0; cols < imgWidth; cols++)
            {
                *(RGBdata ++) = RGB565data[RGBpos] & 0x1F;
                *(RGBdata ++) = (RGB565data[RGBpos + 1] & 0x7) << 3 | RGB565data[RGBpos] >> 5;
                *(RGBdata ++) = RGB565data[RGBpos + 1] >> 3;
                if (vinfo.bits_per_pixel == 32) {   // RGB888
                    *(RGBdata ++) = 0xFF;
                }
                RGBpos += 2;
            }
        }
    }

    memcpy(outBuf, tmp, screensize);
    free(tmp);
    return 0;
}

int convert_rgb888_to_rgb(unsigned char *inBuf, unsigned char *outBuf, int imgWidth, int imgHeight, int cvtMethod)
{
    int rows ,cols;
    unsigned char *RGB888data, *RGBdata;
    int RGBpos;
    int width, height;
    int x_offset, y_offset;
    unsigned char *tmp = malloc(screensize);
    unsigned char r, g, b;

    width = imgWidth > vinfo.xres ? vinfo.xres : imgWidth;
    height = imgHeight > vinfo.yres ? vinfo.yres : imgHeight;
    x_offset = (vinfo.xres - width) / 2;
    y_offset = (vinfo.yres - height) / 2;

    RGB888data = inBuf;
    RGBdata = tmp;

    RGBpos = 0;
    for(rows = 0; rows < imgHeight; rows++)
    {
        RGBdata = tmp + ((rows + y_offset) * vinfo.xres + x_offset) * vinfo.bits_per_pixel / 8;
        RGBpos = rows * imgWidth * 3;
        if (vinfo.bits_per_pixel == 24) {   // RGB888
            memcpy(RGBdata, &RGB888data[RGBpos], imgWidth * 3);
        } else {
            for(cols = 0; cols < imgWidth; cols++)
            {
                if (vinfo.bits_per_pixel == 16) {   // RGB565
                    b = RGB888data[RGBpos];
                    g = RGB888data[RGBpos + 1];
                    r = RGB888data[RGBpos + 2];
                    *(RGBdata ++) = (((g & 0x1c) << 3) | (b >> 3));    /* g low 5bits，b high 5bits */
                    *(RGBdata ++) = ((r & 0xf8) | (g >> 5));    /* r high 5bits，g high 3bits */
                } else {   // RGB8888
                    *(RGBdata ++) = RGB888data[RGBpos];
                    *(RGBdata ++) = RGB888data[RGBpos + 1];
                    *(RGBdata ++) = RGB888data[RGBpos + 2];
                    *(RGBdata ++) = 0xFF;
                }
                RGBpos += 3;
            }
        }
    }

    memcpy(outBuf, tmp, screensize);
    free(tmp);
    return 0;
}
