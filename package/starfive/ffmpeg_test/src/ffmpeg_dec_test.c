// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 StarFive Technology Co., Ltd.
 */
#include <getopt.h>
#include <signal.h>
#include <inttypes.h>
#include <stdbool.h>
#include <libv4l2.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavcodec/avcodec.h>

#define INBUF_SIZE 4096
#define DRM_DEVICE_NAME      "/dev/dri/card0"

// static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
//         char *filename)
// {
//     FILE *f;
//     int i;
// printf("[%s,%d]: wrap=%d, xsize=%d, ysize=%d, filename=%s\n",__FUNCTION__,__LINE__, wrap, xsize, ysize, filename);
//     f = fopen(filename,"w");
//     for (i = 0; i < ysize; i++)
//         fwrite(buf + i * wrap, 1, xsize, f);
//     fclose(f);
// }

static void yuv_save(unsigned char **buf, int *wrap, int xsize, int ysize,
                     char *filename)
{
    FILE *f;
    // int i, j, plane_idx = 0;
    int i;
printf("[%s,%d]: wrap=%d, xsize=%d, ysize=%d, filename=%s\n",__FUNCTION__,__LINE__, *wrap, xsize, ysize, filename);
    f = fopen(filename,"w");
//    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);

    // while (buf[plane_idx] && wrap[plane_idx]) {
    //     for (i = 0; i < ysize; i++) {
    //         fwrite(buf[plane_idx] + i * wrap[plane_idx], 1, wrap[plane_idx], f);
    //     }
    //     plane_idx ++;
    // }

    unsigned char *y = buf[0];
    unsigned char *u = buf[1];
    unsigned char *v = buf[2];
    int y_linesize = wrap[0];
    int u_linesize = wrap[1];
    // int v_linesize = wrap[2];

    fwrite(y, 1, y_linesize*ysize, f);
    for (i = 0; i < ysize * u_linesize; i++) {
        fwrite(u+i, 1, 1, f);
        fwrite(v+i, 1, 1, f);
    }
    // for (i = 0; i < ysize; i++) {
    //     for (j = 0; j < u_linesize; j++) {
    //         fwrite(u+i*, 1, y_linesize*ysize, f);
    //     }
    // }

    fclose(f);
}


static void decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt,
                   const char *filename)
{
    char buf[1024];
    int ret;

    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }

        printf("saving frame %3d\n", dec_ctx->frame_number);
        fflush(stdout);

        /* the picture is allocated by the decoder. no need to
           free it */
        snprintf(buf, sizeof(buf), "%s-%d", filename, dec_ctx->frame_number);
        // pgm_save(frame->data[0], frame->linesize[0],
        //          frame->width, frame->height, buf);
        yuv_save(frame->data, frame->linesize,
                 frame->width, frame->height, buf);
printf("[%s,%d]: line=%d, %d, %d, %d. frame->data=%p, %p,%p,%p\n",
    __FUNCTION__,__LINE__, frame->linesize[0], frame->linesize[1], frame->linesize[2], frame->linesize[3],
    frame->data[0], frame->data[1], frame->data[2],frame->data[3]);

    }
}

int main(int argc, char **argv)
{
    const char *filename, *outfilename;
    const AVCodec *codec;
    AVCodecParserContext *parser;
    AVCodecContext *c= NULL;
    FILE *f;
    AVFrame *frame;
    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *data;
    size_t   data_size;
    int ret;
    AVPacket *pkt;

    av_log_set_level(AV_LOG_TRACE);
    if (argc <= 2) {
        fprintf(stderr, "Usage: %s <input file> <output file>\n", argv[0]);
        exit(0);
    }
    filename    = argv[1];
    outfilename = argv[2];

    pkt = av_packet_alloc();
    if (!pkt)
        exit(1);

    /* set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG streams) */
    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    /* find the MPEG-1 video decoder */
    // codec = avcodec_find_decoder(AV_CODEC_ID_H264); // AV_CODEC_ID_MPEG1VIDEO
    // if (!codec) {
    //     fprintf(stderr, "Codec not found\n");
    //     exit(1);
    // }

	/* find the video decoder: ie: h264_v4l2m2m */
	codec = avcodec_find_decoder_by_name("h264");  // h264_omx
	if (!codec) {
		fprintf(stderr,"Codec not found\n");
		exit(1);
	}

    parser = av_parser_init(codec->id);
    if (!parser) {
        fprintf(stderr, "parser not found\n");
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }
printf("[%s,%d]: \n",__FUNCTION__,__LINE__);

    /* For some codecs, such as msmpeg4 and mpeg4, width and height
       MUST be initialized there because this information is not
       available in the bitstream. */

    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
printf("[%s,%d]: \n",__FUNCTION__,__LINE__);
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }
printf("[%s,%d]: \n",__FUNCTION__,__LINE__);
    f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    while (!feof(f)) {
        /* read raw data from the input file */
        data_size = fread(inbuf, 1, INBUF_SIZE, f);
        if (!data_size)
            break;

        /* use the parser to split the data into frames */
        data = inbuf;
        while (data_size > 0) {
            ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size,
                                   data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (ret < 0) {
                fprintf(stderr, "Error while parsing\n");
                exit(1);
            }
            data      += ret;
            data_size -= ret;

            if (pkt->size)
                decode(c, frame, pkt, outfilename);
        }
    }

    /* flush the decoder */
    decode(c, frame, NULL, outfilename);

    fclose(f);

    av_parser_close(parser);
    avcodec_free_context(&c);
    av_frame_free(&frame);
    av_packet_free(&pkt);

    return 0;
}

