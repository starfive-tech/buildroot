// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 StarFive Technology Co., Ltd.
 */
#include "common.h"
#include <stddef.h>
#include <jpeglib.h>
#include <libdrm/drm_fourcc.h>
#include <libavcodec/avcodec.h>

#include "stf_log.h"

/**
    Print error message and terminate programm with EXIT_FAILURE return code.
    \param s error message to print
*/
void errno_exit(const char* s)
{
    LOG(STF_LEVEL_ERR, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

void errno_print(const char *s)
{
    LOG(STF_LEVEL_ERR, "%s error %d, %s\n", s, errno, strerror(errno));
}

void dump_fourcc(uint32_t fourcc)
{
    LOG(STF_LEVEL_LOG, " %c%c%c%c \n",
        fourcc,
        fourcc >> 8,
        fourcc >> 16,
        fourcc >> 24);
}

// convert v4l2 format to fb format
int v4l2fmt_to_fbfmt(uint32_t format)
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

// convert v4l2 format to drm format
uint32_t v4l2fmt_to_drmfmt(uint32_t v4l2_fmt)
{
    uint32_t drm_fmt;
    // dump_fourcc(v4l2_fmt);

    switch (v4l2_fmt) {
    case V4L2_PIX_FMT_RGB565:
        drm_fmt = DRM_FORMAT_RGB565;
        break;
    case V4L2_PIX_FMT_RGB24:
        drm_fmt = DRM_FORMAT_ARGB8888;
        break;
    case V4L2_PIX_FMT_YUV420:
        drm_fmt = DRM_FORMAT_YUV420;
        break;
    case V4L2_PIX_FMT_YUYV:
        drm_fmt = DRM_FORMAT_YUYV;
        break;
    case V4L2_PIX_FMT_YVYU:
        drm_fmt = DRM_FORMAT_YVYU;
        break;
    case V4L2_PIX_FMT_NV21:
        drm_fmt = DRM_FORMAT_NV21;
        break;
    case V4L2_PIX_FMT_NV12:
        drm_fmt = DRM_FORMAT_NV12;
        break;
    default:
        drm_fmt = DRM_FORMAT_NV21;
        LOG(STF_LEVEL_WARN, "drm not support the V4L2_format\n");
        break;
    }
    return drm_fmt;
}

// convert ffmpeg format to drm format
uint32_t ffmpegfmt_to_drmfmt(uint32_t ffmpeg_fmt)
{
    uint32_t drm_fmt;
    // dump_fourcc(ffmpeg_fmt);

    switch (ffmpeg_fmt) {
    case AV_PIX_FMT_ARGB:
        drm_fmt = DRM_FORMAT_RGB565;
        break;
    case V4L2_PIX_FMT_RGB24:
        drm_fmt = DRM_FORMAT_ARGB8888;
        break;
    case AV_PIX_FMT_YUV420P:
        drm_fmt = DRM_FORMAT_YUV420;
        break;
    case AV_PIX_FMT_YUYV422:
        drm_fmt = DRM_FORMAT_YUYV;
        break;
    case AV_PIX_FMT_YVYU422:
        drm_fmt = DRM_FORMAT_YVYU;
        break;
    case AV_PIX_FMT_NV21:
        drm_fmt = DRM_FORMAT_NV21;
        break;
    case AV_PIX_FMT_NV12:
        drm_fmt = DRM_FORMAT_NV12;
        break;
    default:
        drm_fmt = DRM_FORMAT_NV21;
        LOG(STF_LEVEL_WARN, "drm not support the FFMPEG_format %d\n", ffmpeg_fmt);
        break;
    }
    return drm_fmt;
}

int write_file(char * filename, const uint8_t *image_buffer, int size)
{
    /*  More stuff */
    FILE * outfile;               /*  target file */
    if ((outfile = fopen(filename, "w+")) == NULL) {
        LOG(STF_LEVEL_ERR, "can't open %s\n", filename);
        return -1;
    }
    fwrite(image_buffer, size, 1, outfile);
    fclose(outfile);
    return 0 ;
}

int write_JPEG_file(char * filename, uint8_t *image_buffer,
        int image_width, int image_height, int quality)
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
        LOG(STF_LEVEL_ERR, "can't open %s\n", filename);
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
    return 0;
}

void jpegWrite(uint8_t* img, char* jpegFilename,
        uint32_t width, uint32_t height, int jpegQuality)
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


