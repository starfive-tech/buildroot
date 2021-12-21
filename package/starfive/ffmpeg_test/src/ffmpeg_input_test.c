// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 StarFive Technology Co., Ltd.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <inttypes.h>
#include <stdbool.h>
#include <libv4l2.h>
#include <poll.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include "common.h"
#include "stf_drm.h"
#include "stf_log.h"

#define DRM_DEVICE_NAME      "/dev/dri/card0"
#define V4L2_DFT_DEVICE_NAME "/dev/video0"
#define FFMPEG_INPUT_NAME    "video4linux2"

typedef struct enum_value_t {
    int    value;
    const char *name;
} enum_value_t;

static const enum_value_t g_disp_values[] = {
    { STF_DISP_NONE, "NONE"},
    // { STF_DISP_FB,   "FB"},
    { STF_DISP_DRM,  "DRM"}
};

static const enum_value_t g_iomthd_values[] = {
    { IO_METHOD_MMAP,    "MMAP"},
    { IO_METHOD_USERPTR, "USERPTR"},
    { IO_METHOD_DMABUF,  "DMABUF"},
    { IO_METHOD_READ,    "READ"}
};

typedef struct {
    DRMParam_t drm_param;

    char *device_name;  // v4l2 device name
    char* out_file;    // save file
    FILE* out_fp;  // for save file

    uint32_t format;    // V4L2_PIX_FMT_RGB565
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    uint32_t image_size;

    enum STF_DISP_TYPE disp_type;
    enum IOMethod    io_mthd;
    int continuous;

    uint8_t jpegQuality;

    int dmabufs[BUFCOUNT];  // for dmabuf use, mmap not use it

    AVFormatContext* format_ctx;
    AVCodecContext* vcodec_ctx;
    AVPacket* packet;
} ConfigParam_t;
ConfigParam_t *gp_cfg_param = NULL;

static void sync_handler (int fd, unsigned int frame,
        unsigned int sec, unsigned int usec,
        void *data)
{
    int *waiting;
    waiting = data;
    *waiting = 0;
}

int sync_drm_frame(int buf_id)
{
    int ret;
    int waiting = 1;
    struct pollfd* fds = NULL;
    uint32_t nfds = 0;
    drmEventContext evctxt = {
        .version = DRM_EVENT_CONTEXT_VERSION,
        .page_flip_handler = sync_handler,
        .vblank_handler = NULL,
    };

    ret = drmModePageFlip (gp_cfg_param->drm_param.fd, gp_cfg_param->drm_param.dev_head->crtc_id,
            gp_cfg_param->drm_param.dev_head->bufs[buf_id].fb_id,
            DRM_MODE_PAGE_FLIP_EVENT, &waiting);
    if (ret) {
        goto pageflip_failed;
    }

    nfds = 1;
    fds = (struct pollfd*)malloc(sizeof(struct pollfd) * nfds);
    memset(fds, 0, sizeof(struct pollfd) * nfds);
    fds[0].fd = gp_cfg_param->drm_param.fd;
    fds[0].events = POLLIN;

    while (waiting) {
        do {
            ret = poll(fds, nfds, 3000);
        } while (ret == -1 && (errno == EAGAIN || errno == EINTR));
        ret = drmHandleEvent(gp_cfg_param->drm_param.fd, &evctxt);
        if (ret) {
            goto event_failed;
        }
    }
    return 0;

pageflip_failed:
    {
        LOG(STF_LEVEL_ERR, "drmModePageFlip failed: %s (%d)\n",
                strerror(errno), errno);
        return 1;
    }
event_failed:
    {
        LOG(STF_LEVEL_ERR, "drmHandleEvent failed: %s (%d)\n",
                strerror(errno), errno);
        return 1;
    }
}

void mainloop(void)
{
    AVFormatContext* fmtCtx = gp_cfg_param->format_ctx;
    AVPacket* packet = gp_cfg_param->packet;
    FILE* fp = gp_cfg_param->out_fp;
    uint32_t buf_id = 0;

    while (gp_cfg_param->continuous) {
        av_read_frame(fmtCtx, packet);
        if (STF_DISP_DRM == gp_cfg_param->disp_type) {
            LOG(STF_LEVEL_TRACE, "data length = %d, drm buf length = %d\n", packet->size, 
                    gp_cfg_param->drm_param.dev_head->bufs[buf_id].size);
            memcpy(gp_cfg_param->drm_param.dev_head->bufs[buf_id].buf, packet->data, packet->size);
            sync_drm_frame(buf_id++);
            if (buf_id >= BUFCOUNT) {
                buf_id = 0;
            }
        }
        if (fp) {
            LOG(STF_LEVEL_INFO, "save file: %s\n", gp_cfg_param->out_file);
            fwrite(packet->data, 1, packet->size, fp);
        }
        av_packet_unref(packet);
    }
}

static void alloc_default_config(ConfigParam_t **pp_data)
{
    ConfigParam_t *cfg_param = NULL;
    cfg_param = malloc(sizeof(*cfg_param));
    if (!cfg_param) {
        errno_exit("malloc");
    }
    memset(cfg_param, 0, sizeof(*cfg_param));

    cfg_param->device_name = V4L2_DFT_DEVICE_NAME;
    cfg_param->io_mthd = IO_METHOD_MMAP;
    cfg_param->format = V4L2_PIX_FMT_NV12;
    cfg_param->continuous = 1;
    *pp_data = cfg_param;
}

static void usage(FILE* fp, int argc, char** argv)
{
    fprintf(fp,
        "Usage: %s [options]\n\n"
        "Options:\n"
        "-d | --device name   Video device name [default /dev/video0]\n"
        "-h | --help          Print this message\n"
        "-o | --output        Set output filename\n"
        "-W | --width         Set v4l2 image width, default 1920\n"
        "-H | --height        Set v4l2 image height, default 1080\n"
        "-I | --interval      Set frame interval (fps) (-1 to skip)\n"
        "-c | --continuous    Do continous capture, stop with SIGINT.\n"
        "-v | --version       Print version\n"
        "-f | --format        image format, default 4\n"
        "                0: V4L2_PIX_FMT_RGB565\n"
        "                1: V4L2_PIX_FMT_RGB24\n"
        "                2: V4L2_PIX_FMT_YUV420\n"
        "                3: V4L2_PIX_FMT_YUYV\n"
        "                4: V4L2_PIX_FMT_NV21\n"
        "                5: V4L2_PIX_FMT_NV12\n"
        "                6: V4L2_PIX_FMT_YVYU\n"
        "                7: V4L2_PIX_FMT_SRGGB12\n"
        "                8: V4L2_PIX_FMT_SGRBG12\n"
        "                9: V4L2_PIX_FMT_SGBRG12\n"
        "                10: V4L2_PIX_FMT_SBGGR12\n"
        "                default: V4L2_PIX_FMT_RGB565\n"
        "-t | --distype       set display type, default 0\n"
        "                0: Not display\n"
        "                1: Use DRM Display\n"
        "\n"
        "Eg:\n"
        "\t sensor : ffmpeg_input_test -d /dev/video2 -f 5 -W 640 -H 480 -I 30 -t 1\n"
        "\t uvc cam: ffmpeg_input_test -d /dev/video5 -f 3 -W 640 -H 480 -I 30 -t 1\n"
        "\n"
        "Open debug log level: \n"
        "\t export V4L2_DEBUG=3\n"
        "\t default level 1, level range 0 ~ 7\n"
        "",
        argv[0]);
}

static const char short_options [] = "d:ho:W:H:I:vcf:t:X:Y:";

static const struct option long_options [] = {
    { "device",     required_argument,      NULL,           'd' },
    { "help",       no_argument,            NULL,           'h' },
    { "output",     required_argument,      NULL,           'o' },
    { "width",      required_argument,      NULL,           'W' },
    { "height",     required_argument,      NULL,           'H' },
    { "interval",   required_argument,      NULL,           'I' },
    { "version",    no_argument,            NULL,           'v' },
    { "continuous", no_argument,            NULL,           'c' },
    { "format",     required_argument,      NULL,           'f' },
    { "distype",    required_argument,      NULL,           't' },
    { 0, 0, 0, 0 }
};

void parse_options(int argc, char **argv, ConfigParam_t *cfg_param)
{
    int index, c = 0;
    int value = 0;

    while ((c = getopt_long(argc, argv, short_options, long_options, &index)) != -1) {
        switch (c) {
        case 0: /* getopt_long() flag */
            break;

        case 'd':
            cfg_param->device_name = strdup(optarg);
            break;

        case 'h':
            usage(stdout, argc, argv);
            exit(EXIT_SUCCESS);

        case 'o':
            // set filename
            cfg_param->out_file = strdup(optarg);
            LOG(STF_LEVEL_INFO, "save file: %s\n", cfg_param->out_file);
            break;

        case 'W':
            // set v4l2 width
            cfg_param->width = atoi(optarg);
            break;

        case 'H':
            // set v4l2 height
            cfg_param->height = atoi(optarg);
            break;

        case 'I':
            // set fps
            cfg_param->fps = atoi(optarg);
            break;

        case 'c':
            // set flag for continuous capture, interuptible by sigint
            cfg_param->continuous = 1;
            // InstallSIGINTHandler();
            break;

        case 'v':
            printf("Version: %s\n", TEST_VERSION);
            exit(EXIT_SUCCESS);
            break;

        case 'f':
            LOG(STF_LEVEL_INFO, "v4l2 format: %s\n", optarg);
            value = atoi(optarg);
            LOG(STF_LEVEL_INFO, "v4l2 format: %d\n", value);
            switch (value) {
            case  0:
                value = V4L2_PIX_FMT_RGB565;
                break;
            case  1:
                value = V4L2_PIX_FMT_RGB24;
                break;
            case  2:
                value = V4L2_PIX_FMT_YUV420;
                break;
            case  3:
                value = V4L2_PIX_FMT_YUYV;
                break;
            case  4:
                value = V4L2_PIX_FMT_NV21;
                break;
            case  5:
                value = V4L2_PIX_FMT_NV12;
                break;
            case  6:
                value = V4L2_PIX_FMT_YVYU;
                break;
            case  7:
                value = V4L2_PIX_FMT_SRGGB12;
                break;
            case  8:
                value = V4L2_PIX_FMT_SGRBG12;
                break;
            case  9:
                value = V4L2_PIX_FMT_SGBRG12;
                break;
            case  10:
                value = V4L2_PIX_FMT_SBGGR12;
                break;
            default:
                value = V4L2_PIX_FMT_RGB565;
                break;
            }
            cfg_param->format = value;
            break;

        case 't':
            value = atoi(optarg);
            if (value < STF_DISP_NONE || value > STF_DISP_DRM) {
                LOG(STF_LEVEL_ERR, "Display Type %d is out of range [%d, %d]\n", value,
                        STF_DISP_NONE, STF_DISP_DRM);
                exit(EXIT_FAILURE);
            }
            LOG(STF_LEVEL_INFO, "Display Type: %s\n", g_disp_values[value].name);
            cfg_param->disp_type = value;
            break;

        default:
            usage(stderr, argc, argv);
            exit(EXIT_FAILURE);
        }
    }
}

void StopContCapture(int sig_id)
{
    LOG(STF_LEVEL_INFO, "stoping continuous capture\n");
    gp_cfg_param->continuous = 0;
}
void signal_handler()
{
    struct sigaction sa;
    CLEAR(sa);

    sa.sa_handler = StopContCapture;
    if (sigaction(SIGINT, &sa, 0) != 0) {
        LOG(STF_LEVEL_ERR, "could not install SIGINT handler, continuous capture disabled\n");
        gp_cfg_param->continuous = 0;
    }
}

int main(int argc, char **argv)
{
    AVFormatContext* fmtCtx = NULL;
    AVInputFormat* inputFmt;
    AVCodecContext* pCodecCtx;
    AVCodec* pCodec;
    int videoindex, i;
    char str_data[32] = {0};
    AVDictionary *options = NULL;

    // init param and log
    signal_handler();
    init_log();
    alloc_default_config(&gp_cfg_param);
    parse_options(argc, argv, gp_cfg_param);
    // check_cfg_params(gp_cfg_param);

    // init ffmpeg env
    av_log_set_level(AV_LOG_TRACE);
    avcodec_register_all();
    avdevice_register_all();

    inputFmt = av_find_input_format(FFMPEG_INPUT_NAME);
    if (inputFmt == NULL) {
        LOG(STF_LEVEL_ERR, "can not find_input_format\n");
        exit(EXIT_FAILURE);
    }

    // open and set parameter to v4l2 device
    av_dict_set(&options, "f", "v4l2", 0);              // speed up probe process
    if (V4L2_PIX_FMT_NV12 == gp_cfg_param->format) {    // format
        av_dict_set(&options, "input_format", "nv12", 0);
    } else if (V4L2_PIX_FMT_YUYV == gp_cfg_param->format) {
        av_dict_set(&options, "input_format", "yuyv422", 0);
    } else {
        LOG(STF_LEVEL_ERR, "Current only support nv12 format\n");
        exit(EXIT_FAILURE);
    }
    if (gp_cfg_param->width && gp_cfg_param->height) {
        snprintf(str_data, 32, "%d*%d", gp_cfg_param->width, gp_cfg_param->height);
        av_dict_set(&options, "video_size", str_data, 0); // resolution eg: "640*480"
    }
    if (gp_cfg_param->fps) {
        snprintf(str_data, 32, "%d", gp_cfg_param->fps);
        av_dict_set(&options, "framerate", str_data, 0);  // framerate eg: "15"
    }
    if (avformat_open_input(&fmtCtx, gp_cfg_param->device_name, inputFmt, &options) < 0) {
        LOG(STF_LEVEL_ERR, "can not open input_file\n");
        exit(EXIT_FAILURE);
    }
    av_dict_free(&options);

    /* print device information*/
    av_dump_format(fmtCtx, 0, gp_cfg_param->device_name, 0);

    videoindex = -1;
    for (i = 0; i < fmtCtx->nb_streams; i++) {
        if (fmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
            break;
        }
    }
    if (videoindex == -1) {
        LOG(STF_LEVEL_ERR, "fail to find a video stream\n");
        exit(EXIT_FAILURE);
    }

    pCodecCtx = fmtCtx->streams[videoindex]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    LOG(STF_LEVEL_INFO, "picture width   =  %d \n", pCodecCtx->width);
    LOG(STF_LEVEL_INFO, "picture height  =  %d \n", pCodecCtx->height);
    LOG(STF_LEVEL_INFO, "Pixel   format  =  %d \n", pCodecCtx->pix_fmt);
    LOG(STF_LEVEL_INFO, "codec_id  =  %d \n", pCodecCtx->codec_id); // AV_CODEC_ID_RAWVIDEO

    if (gp_cfg_param->out_file && !gp_cfg_param->out_fp) {
        gp_cfg_param->out_fp = fopen(gp_cfg_param->out_file, "wb");
        if (!gp_cfg_param->out_fp) {
            LOG(STF_LEVEL_ERR, "fopen file failed %s\n", gp_cfg_param->out_file);
            exit(EXIT_FAILURE);
        }
    }

    gp_cfg_param->packet = (AVPacket*)av_malloc(sizeof(AVPacket));
    gp_cfg_param->format_ctx = fmtCtx;
    gp_cfg_param->vcodec_ctx = pCodecCtx;

    if (STF_DISP_DRM == gp_cfg_param->disp_type) {
        stf_drm_open(&gp_cfg_param->drm_param, DRM_DEVICE_NAME, gp_cfg_param->io_mthd);
        stf_drm_init(&gp_cfg_param->drm_param, gp_cfg_param->vcodec_ctx->width,
                gp_cfg_param->vcodec_ctx->height, gp_cfg_param->vcodec_ctx->pix_fmt,
                gp_cfg_param->io_mthd, gp_cfg_param->dmabufs,
                sizeof(gp_cfg_param->dmabufs) / sizeof(gp_cfg_param->dmabufs[0]));
    }

    // process frames
    mainloop();

    // release resource
    if (gp_cfg_param->out_fp) {
        fclose(gp_cfg_param->out_fp);
    }
    if (gp_cfg_param->packet) {
        av_free_packet(gp_cfg_param->packet);
    }
    if (gp_cfg_param->format_ctx) {
        avformat_close_input(&gp_cfg_param->format_ctx);
    }
    if (STF_DISP_DRM == gp_cfg_param->disp_type) {
        stf_drm_close(&gp_cfg_param->drm_param);
    }
    deinit_log();
    free(gp_cfg_param);
    return 0;
}
