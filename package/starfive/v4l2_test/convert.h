
// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 StarFive Technology Co., Ltd.
 */
#ifndef _CONVERT_H_
#define _CONVERT_H_

extern int yuyv_resize(unsigned char *inBuf, unsigned char *outBuf, int imgWidth, int imgHeight);

extern int convert_yuyv_to_nv12(unsigned char *inBuf, unsigned char *outBuf, int imgWidth, int imgHeight, int is_yuyv);
extern int convert_nv21_to_nv12(unsigned char *inBuf, unsigned char *outBuf, int imgWidth, int imgHeight, int is_nv21);
extern int convert_nv21_to_rgb(unsigned char *inBuf, unsigned char *outBuf, int imgWidth, int imgHeight, int is_nv21);
extern int convert_rgb565_to_nv12(unsigned char *inBuf, unsigned char *outBuf, int imgWidth, int imgHeight, int is_nv21);
extern int convert_yuyv_to_rgb(unsigned char *inBuf, unsigned char *outBuf, int imgWidth, int imgHeight, int cvtMethod);
extern int convert_yuv444_to_rgb(unsigned char *inBuf, unsigned char *outBuf, int imgWidth, int imgHeight, int cvtMethod);
extern int convert_rgb565_to_rgb(unsigned char *inBuf, unsigned char *outBuf, int imgWidth, int imgHeight, int cvtMethod);
extern int convert_rgb888_to_rgb(unsigned char *inBuf, unsigned char *outBuf, int imgWidth, int imgHeight, int cvtMethod);

#endif // _CONVERT_H_
