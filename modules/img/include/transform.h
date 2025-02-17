#ifndef ROCKCV_TRANSFORM_H
#define ROCKCV_TRANSFORM_H

#include <rga/RgaUtils.h>
#include <rga/RgaApi.h>
#include <rga/im2d.h>
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#include "frame_operation.h"
#include "codec.h"
#include "type_drm.h"

#include <opencv2/opencv.hpp> 

int8_t resize(const cv::Mat &input, cv::Mat &output, int new_width, int new_height);
int8_t rotate(const cv::Mat& input, cv::Mat& output, int angle);
int8_t mirror(const cv::Mat& input, cv::Mat& output, int type);

int8_t ffresize(const AVFrame* input, AVFrame* output, AVCodecContext* codecCtx, int new_width, int new_height);
int8_t ffresize(const frame_t* input, frame_t* output, int new_width, int new_height);
int8_t resize(frame_t *input, frame_t *output, int new_width, int new_height, AVPixelFormat format);
int8_t resize(const AVFrame* input, AVFrame* output, int new_width, int new_height);
AVFrame* convertToRGB24(AVFrame* frame, AVCodecContext* codecCtx);


#endif