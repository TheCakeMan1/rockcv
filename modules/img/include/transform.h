#ifndef ROCKCV_TRANSFORM_H
#define ROCKCV_TRANSFORM_H

#include <rga/RgaUtils.h>
#include <rga/RgaApi.h>
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#include "img_op.h"

#include <opencv2/opencv.hpp> 
int8_t resize(const cv::Mat &input, cv::Mat &output, int new_width, int new_height);
int8_t resize(AVFrame* input, AVFrame* output, AVCodecContext* codecCtx, int new_width, int new_height);
int8_t resize(frame_t* input, frame_t* output, int new_width, int new_height);

#endif