#ifndef ROCKCV_VIDEO_H
#define ROCKCV_VIDEO_H

#include "struct.h"
#include <iostream>
#include "codec.h"
// #include <chrono>
// #include <thread>

extern "C"{
#include <libavutil/pixdesc.h>
}

int8_t open_video(const char* input, context_video_t& context);
void print_stream_list(context_video_t& context);
bool read_f(context_video_t& context);
// void delay(context_video_t& context);

#endif