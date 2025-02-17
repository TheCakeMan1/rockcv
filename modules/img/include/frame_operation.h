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
    uint8_t* buffer;
};

#define uniti_frame(name, avframes, AVCodecContexts) (name).avframe = av_frame_clone(avframes); \
    (name).avcodeccontext = avcodec_alloc_context3(NULL); \
    if ((name).avcodeccontext) { \
        AVCodecParameters *params = avcodec_parameters_alloc(); \
        avcodec_parameters_from_context(params, AVCodecContexts); \
        avcodec_parameters_to_context((name).avcodeccontext, params); \
        avcodec_parameters_free(&params); \
    }

#define convert_avframe_to_buffer(frame) \
    (frame)->buffer = (uint8_t*)av_malloc(av_image_get_buffer_size(AVPixelFormat((frame)->avframe->format), (frame)->avframe->width, (frame)->avframe->height, 1)); \
    av_image_copy_to_buffer((frame)->buffer, av_image_get_buffer_size(AVPixelFormat((frame)->avframe->format), (frame)->avframe->width, (frame)->avframe->height, 1), (frame)->avframe->data, (frame)->avframe->linesize, AVPixelFormat((frame)->avframe->format), (frame)->avframe->width, (frame)->avframe->height, 1);


// #define convert_buffer_to_avframe(buffer, avframe) \ 


#endif