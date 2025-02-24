#ifndef ROCKCV_RTSP_H
#define ROCKCV_RTSP_H

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
}
#include <iostream>
#include "struct.h"
#include "codec.h"
#include "frame_operation.h"

int8_t open_rtsp(const char *input, context_rtsp_t *context);
bool read_frame(context_rtsp_t *context);
bool read_packet(context_rtsp_t *context);
bool read_f(context_rtsp_t &context, frame_t &frame);

void print_stream_list(context_rtsp_t &context);

#endif