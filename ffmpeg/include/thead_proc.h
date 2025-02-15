#ifndef _THEAD_PROC_H_
#define _THEAD_PROC_H_

#include "rknn_api.h"
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <string.h>
#include "postprocess.h"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "opencv2/opencv.hpp"
#include "opencv2/highgui.hpp"
#include "rga.h"
#include <rga/RgaUtils.h>
#include <rga/rga.h>
#include <rga/im2d.h>
#include "rknpu.h"
#include "profile.h"
#include "rknpu_file.h"

// #define CREATE_QUEUE(index) \
//     extern std::queue<cv::Mat> input_queue_proc_##index;\
//     extern std::queue<std::pair<cv::Mat, rknn_output *>> output_queue_proc_##index;\
//     extern std::mutex input_mutex_proc_##index, output_mutex_proc_##index;\
//     extern std::condition_variable input_cv_proc_##index, output_cv_proc_##index;\

// extern std::queue<cv::Mat> input_queue_proc1;
// extern std::queue<std::pair<cv::Mat, rknn_output *>> output_queue_proc1;
// extern std::mutex input_mutex_proc1, output_mutex_proc1;
// extern std::condition_variable input_cv_proc1, output_cv_proc1;

// extern std::queue<cv::Mat> input_queue_proc2;
// extern std::queue<std::pair<cv::Mat, rknn_output *>> output_queue_proc2;
// extern std::mutex input_mutex_proc2, output_mutex_proc2;
// extern std::condition_variable input_cv_proc2, output_cv_proc2;

// static unsigned char *load_model(const char *filename, int &fileSize);
static void dump_tensor_attr(rknn_tensor_attr *attr);
void resizeWithPadding(const cv::Mat &img, cv::Mat &resized_img, int target_width, int target_height);
// int proc_1(char *model_name);
// int proc_2(char *model_name);
// int proc_3(char *model_name);
#endif