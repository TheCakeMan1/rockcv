#ifndef _RKVIDEO_STRUCT_H
#define _RKVIDEO_STRUCT_H

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

typedef struct
{
    char *source;
    AVFormatContext *format_ctx; // Инициализация через NULL при создании
    int video_stream_index;
    AVStream *stream;
    AVCodecContext *codec_ctx;
    const AVCodec *codec;
    AVPacket *packet;
    AVFrame *frame;
    int max_stream_v;
    AVDictionary *opts;
    _Bool check_rtsp;
} rkcv_t;

#endif