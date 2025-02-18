#ifndef ROCKCV_RTSP_H
#define ROCKCV_RTSP_H

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#include <iostream>

struct context_rtsp_t
{
    AVFormatContext* input_format_ctx = nullptr;
    int video_stream_index;
    AVStream* input_stream = nullptr;
    AVCodecContext* input_codec_ctx = nullptr;
    const AVCodec* input_codec;
    AVPacket packet;
    AVFrame* frame;
};

int8_t open_rtsp(const char* input, context_rtsp_t* context);
bool read_frame(context_rtsp_t* context);
bool read_packet(context_rtsp_t* context);



#endif