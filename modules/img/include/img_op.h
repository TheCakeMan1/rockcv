#ifndef ROCKCV_IMG_H
#define ROCKCV_IMG_H

#include <cstdint>
#include <cstdio>

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
}

#include <opencv2/opencv.hpp>

#include <ostream>
// #define WITH_OPENCV
#include <iostream>

struct frame_t
{
    AVFrame* avframe;
    AVCodecContext* avcodeccontext;
};

#define frame_t(name) \ 
    frame_t name;\
    memset(&name, 0, sizeof(frame_t));

// frame imread(const char* filename);
// #ifdef WITH_OPENCV
// void frame2mat(frame* frame, cv::Mat mat);
// #endif

#endif