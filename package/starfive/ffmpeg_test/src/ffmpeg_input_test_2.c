/*=============================================================================
 * #     FileName: read_device.c
 * #         Desc: use ffmpeg read a frame data from v4l2, and convert
 * #			   the output data format
 * #       Author: licaibiao
 * #   LastChange: 2017-03-28
 * =============================================================================*/
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* input_name = "video4linux2";
char* file_name = "/dev/video2";
char* out_file = "yuv420.yuv";

void captureOneFrame(void)
{
    AVFormatContext* fmtCtx = NULL;
    AVInputFormat* inputFmt;
    AVPacket* packet;
    AVCodecContext* pCodecCtx;
    AVCodec* pCodec;
    struct SwsContext* sws_ctx;
    FILE* fp;
    int i;
    int ret;
    int videoindex;

    enum AVPixelFormat dst_pix_fmt = AV_PIX_FMT_NV12; // AV_PIX_FMT_YUV420P;
    const char* dst_size = NULL;
    const char* src_size = NULL;
    uint8_t* src_data[4];
    uint8_t* dst_data[4];
    int src_linesize[4];
    int dst_linesize[4];
    int src_bufsize;
    int dst_bufsize;
    int src_w;
    int src_h;
    int dst_w = 1920;
    int dst_h = 1080;

    av_log_set_level(AV_LOG_TRACE);

    fp = fopen(out_file, "wb");
    if (fp < 0) {
        printf("open frame data file failed\n");
        return;
    }

    inputFmt = av_find_input_format(input_name);
    if (inputFmt == NULL) {
        printf("can not find_input_format\n");
        return;
    }

    AVDictionary *options = NULL;
    av_dict_set(&options, "f", "v4l2", 0);  // speed probe
    av_dict_set(&options, "input_format", "nv12", 0); // format
    av_dict_set(&options, "video_size", "1920*1080", 0); // resolution
    av_dict_set(&options, "framerate", "15", 0); // framerate

    if (avformat_open_input(&fmtCtx, file_name, inputFmt, &options) < 0) {
        printf("can not open_input_file\n");
        return;
    }
    av_dict_free(&options);

    av_dump_format(fmtCtx, 0, file_name, 0);
    printf("fmtCtx->nb_streams   =  %d \n", fmtCtx->nb_streams);
    videoindex = -1;
    for (i = 0; i < fmtCtx->nb_streams; i++) {
        if (fmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
            break;
        }
    }
    if (videoindex == -1) {
        printf("Didn't find a video stream.\n");
        return -1;
    }

    pCodecCtx = fmtCtx->streams[videoindex]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

    printf("picture width   =  %d \n", pCodecCtx->width);
    printf("picture height  =  %d \n", pCodecCtx->height);
    printf("Pixel   Format  =  %d \n", pCodecCtx->pix_fmt);

    sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, dst_w, dst_h, dst_pix_fmt,
        SWS_BILINEAR, NULL, NULL, NULL);

    src_bufsize = av_image_alloc(src_data, src_linesize, pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, 16);
    dst_bufsize = av_image_alloc(dst_data, dst_linesize, dst_w, dst_h, dst_pix_fmt, 1);

    packet = (AVPacket*)av_malloc(sizeof(AVPacket));

    int loop = 1000;
    //	while(loop--){
    av_read_frame(fmtCtx, packet);
    memcpy(src_data[0], packet->data, packet->size);
    sws_scale(sws_ctx, src_data, src_linesize, 0, pCodecCtx->height, dst_data, dst_linesize);
    fwrite(dst_data[0], 1, dst_bufsize, fp);
    //	}

    fclose(fp);
    av_free_packet(packet);
    av_freep(&dst_data[0]);
    sws_freeContext(sws_ctx);
    avformat_close_input(&fmtCtx);
}

int main(void)
{
    avcodec_register_all();
    avdevice_register_all();
    captureOneFrame();
    return 0;
}
