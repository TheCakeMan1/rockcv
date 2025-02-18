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
// #include "drm.h"
#include <linux/dma-heap.h>

// #include <opencv2/opencv.hpp> 

// int8_t resize(const cv::Mat &input, cv::Mat &output, int new_width, int new_height);
// int8_t rotate(const cv::Mat& input, cv::Mat& output, int angle);
// int8_t mirror(const cv::Mat& input, cv::Mat& output, int type);
int8_t resize(const AVFrame* input, AVFrame* output, int new_width, int new_height);

int8_t ffresize(const AVFrame* input, AVFrame* output, AVCodecContext* codecCtx, int new_width, int new_height);
int8_t ffresize(const frame_t* input, frame_t* output, int new_width, int new_height);


/**
 * @brief Отзеркаливает изображение относительно параметра который вы передадите в type.
 *
 * @param frame_t *input Входное изображение
 * @param frame_t *output Выходное изображение
 * @param angle угол поворота. может быть кратен 90 но не больше 270 включительно
 * @param AVPixelFormat format формат на выходе
 *
 */
int8_t rotate(frame_t *input, frame_t *output, int angle, AVPixelFormat format = AV_PIX_FMT_RGB24);

// #define LEANEAE IM_HAL_TRANSFORM_NEAREST
/**
 * @brief Уменьшает размер кадра с сохранением сожержмимого.
 *
 * @param frame_t *input Входное изображение
 * @param frame_t *output Выходное изображение
 * @param new_width новая ширина
 * @param new_height новая длина
 * @param AVPixelFormat format формат на выходе
 * @param mode С сохранением пропорций(1) или нет(2)
 */
int8_t resize(frame_t *input, frame_t *output, int new_width, int new_height, AVPixelFormat format, bool mode = 0);

/**
 * @brief Отзеркаливает изображение относительно параметра который вы передадите в type.
 *
 * @param frame_t *input Входное изображение
 * @param frame_t *output Выходное изображение
 * @param int type Выходное изображение
 * 
 *    * 1 - по горизонтали
 * 
 *    * 2 - по вертикали
 * 
 *    * 3 - по горизонтали и по вертикали
 * @param AVPixelFormat format формат на выходе
 */
int8_t flip(frame_t *input, frame_t *output, int type, AVPixelFormat format = AV_PIX_FMT_RGB24);

/**
 * @brief Вырезает область кадра
 *
 * @param frame_t *input Входное изображение
 * @param frame_t *output Выходное изображение
 * @param x_start левый верхний угол
 * @param y_start левый верхний угол
 * @param x_stop правый нижний угол
 * @param y_stot правый нижний угол
 * @param AVPixelFormat format формат на выходе
 */
int8_t crop(frame_t *input, frame_t *output, int x_start, int y_start, int x_stop, int y_stop, AVPixelFormat format = AV_PIX_FMT_RGB24);


#endif