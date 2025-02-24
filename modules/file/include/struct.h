#ifndef ROCKCV_STRUCT_H
#define ROCKCV_STRUCT_H

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#include "codec.h"

struct context_rtsp_t
{
    AVFormatContext* input_format_ctx = nullptr;
    int video_stream_index;
    AVStream* input_stream = nullptr;
    AVCodecContext* input_codec_ctx = nullptr;
    const AVCodec* input_codec = nullptr;
    AVPacket packet;
    AVFrame* frame = nullptr;
    int max_stream_v;
};

struct context_video_t
{
    AVFormatContext* format_ctx = nullptr;
    int video_stream_index;
    AVStream* stream = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    const AVCodec* codec = nullptr;
    AVPacket packet;
    AVFrame* frame = nullptr;
    int max_stream_v;
};


#endif