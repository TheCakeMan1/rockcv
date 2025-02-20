#ifndef ROCKCV_CODEC_H
#define ROCKCV_CODEC_H

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#include <rga/RgaUtils.h>
#include <rga/RgaApi.h>
#include <rga/im2d.h>
#include <iostream>

// void convertAVFrameColor(AVFrame *srcFrame, AVFrame *dstFrame, AVPixelFormat dstFormat);

#define AV_TO_RK_FORMAT(av_fmt) \
    ((av_fmt) == AV_PIX_FMT_YUV420P  ? RK_FORMAT_YCbCr_420_SP  : \
    (av_fmt) == AV_PIX_FMT_NV12      ? RK_FORMAT_YCrCb_420_SP  : \
    (av_fmt) == AV_PIX_FMT_NV21      ? RK_FORMAT_YCbCr_420_SP  : \
    (av_fmt) == AV_PIX_FMT_YUYV422   ? RK_FORMAT_YUYV_422      : \
    (av_fmt) == AV_PIX_FMT_UYVY422   ? RK_FORMAT_UYVY_422      : \
    (av_fmt) == AV_PIX_FMT_RGB24     ? RK_FORMAT_RGB_888       : \
    (av_fmt) == AV_PIX_FMT_BGR24     ? RK_FORMAT_BGR_888       : \
    (av_fmt) == AV_PIX_FMT_RGBA      ? RK_FORMAT_RGBA_8888     : \
    (av_fmt) == AV_PIX_FMT_BGRA      ? RK_FORMAT_BGRA_8888     : \
    (av_fmt) == AV_PIX_FMT_GRAY8     ? RK_FORMAT_Y4            : \
    (av_fmt) == AV_PIX_FMT_YUV422P   ? RK_FORMAT_YCbCr_422_SP  : \
                                       RK_FORMAT_UNKNOWN)

// Выравнивание stride на 16 байт (для работы с RGA)
#define ALIGN_UP(value, align) (((value) + (align) - 1) & ~((align) - 1))

const AVCodec* get_codec_by_id_rkmpp(AVCodecID id);



#endif