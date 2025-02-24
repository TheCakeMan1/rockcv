#ifndef ROCKCV_IMG_H
#define ROCKCV_IMG_H

#include <cstdint>
#include <cstdio>

extern "C"
{
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
    AVPixelFormat format;
    int width;
    int height;
    uint8_t *buffer = nullptr;
    AVFrame *avframe = nullptr;
    AVCodecContext *avcodeccontext = nullptr;
    int dmabuf_fd;
};

#define uniti_frame(name, avframes, AVCodecContexts)                  \
    (name).avframe = av_frame_clone(avframes);                        \
    (name).avcodeccontext = avcodec_alloc_context3(NULL);             \
    if ((name).avcodeccontext)                                        \
    {                                                                 \
        AVCodecParameters *params = avcodec_parameters_alloc();       \
        avcodec_parameters_from_context(params, AVCodecContexts);     \
        avcodec_parameters_to_context((name).avcodeccontext, params); \
        avcodec_parameters_free(&params);                             \
    }

#define init_buffer(frame) (frame)->buffer = (uint8_t *)av_malloc(av_image_get_buffer_size(AVPixelFormat((frame).avframe->format), (frame).avframe->width, (frame).avframe->height, 1));

#define convert_avframe_to_buffer(frame) av_image_copy_to_buffer((frame).buffer, av_image_get_buffer_size(AVPixelFormat((frame).avframe->format), (frame).avframe->width, (frame).avframe->height, 1), (frame).avframe->data, (frame).avframe->linesize, AVPixelFormat((frame).avframe->format), (frame).avframe->width, (frame).avframe->height, 1);

void buffer_free(frame_t &frame);
// extern "C" void buffer_free(frame_t *frame);
void frame2mat(frame_t &input, cv::Mat &output);

// #define convert_avframe_to_buffer(frame) { \
//     init_buffer(frame); \
//     uint8_t* buffer = (uint8_t*)(frame)->buffer; \
//     int buffer_size = av_image_get_buffer_size(AVPixelFormat((frame)->avframe->format), (frame)->avframe->width, (frame)->avframe->height, 1); \
//     void* ptr = ::av_malloc(buffer_size); \
//     LOG_ALLOCATION(ptr, buffer_size); \
//     av_image_copy_to_buffer(buffer, buffer_size, (frame)->avframe->data, (frame)->avframe->linesize, AVPixelFormat((frame)->avframe->format), (frame)->avframe->width, (frame)->avframe->height, 1); \
//     (frame)->buffer = ptr; \
// }

// #define convert_buffer_to_avframe(frame) \
//     fast_copy_neon(frame->avframe->data[0], frame->buffer, \
//     frame->avframe->width, frame->avframe->height, \
//     frame->avframe->linesize[0], frame->avframe->linesize[0]);

#define convert_buffer_to_avframe(frame) av_image_copy(frame.avframe->data, frame.avframe->linesize, (const uint8_t **)&frame.buffer, frame.avframe->linesize, (AVPixelFormat)frame.avframe->format, frame.avframe->width, frame.avframe->height);

#endif