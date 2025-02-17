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

void convertAVFrameColor(AVFrame *srcFrame, AVFrame *dstFrame, AVPixelFormat dstFormat);

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

// STRIDE для AVPixelFormat (FFmpeg)
#define STRIDE_FOR_FORMAT(av_fmt, width) \
    ((av_fmt) == AV_PIX_FMT_RGB24     ? ALIGN_UP((width) * 3, 16)  : /* RGB888 */ \
    (av_fmt) == AV_PIX_FMT_BGR24      ? ALIGN_UP((width) * 3, 16)  : /* BGR888 */ \
    (av_fmt) == AV_PIX_FMT_RGBA       ? ALIGN_UP((width) * 4, 16)  : /* RGBA8888 */ \
    (av_fmt) == AV_PIX_FMT_BGRA       ? ALIGN_UP((width) * 4, 16)  : /* BGRA8888 */ \
    (av_fmt) == AV_PIX_FMT_GRAY8      ? ALIGN_UP((width) * 1, 16)  : /* Одноканальный */ \
    (av_fmt) == AV_PIX_FMT_NV12       ? ALIGN_UP((width), 16)      : /* NV12 - UV вместе */ \
    (av_fmt) == AV_PIX_FMT_NV21       ? ALIGN_UP((width), 16)      : /* NV21 - UV вместе */ \
    (av_fmt) == AV_PIX_FMT_YUV420P    ? ALIGN_UP((width), 16)      : /* Y */ \
    (av_fmt) == AV_PIX_FMT_YUYV422    ? ALIGN_UP((width) * 2, 16)  : /* YUYV422 - 2 байта на пиксель */ \
    (av_fmt) == AV_PIX_FMT_UYVY422    ? ALIGN_UP((width) * 2, 16)  : /* UYVY422 */ \
    (av_fmt) == AV_PIX_FMT_YUV422P    ? ALIGN_UP((width), 16)      : /* Y */ \
    (av_fmt) == AV_PIX_FMT_YUV444P    ? ALIGN_UP((width) * 3, 16)  : /* YUV444 - Y, U, V */ \
                                        ALIGN_UP((width) * 3, 16))  /* По умолчанию считаем 3 байта */

// STRIDE для RGA (rga_format_t)
#define RK_STRIDE_FOR_FORMAT(rk_fmt, width) \
    ((rk_fmt) == RK_FORMAT_RGB_888     ? ALIGN_UP((width) * 3, 16)  : \
    (rk_fmt) == RK_FORMAT_BGR_888      ? ALIGN_UP((width) * 3, 16)  : \
    (rk_fmt) == RK_FORMAT_RGBA_8888    ? ALIGN_UP((width) * 4, 16)  : \
    (rk_fmt) == RK_FORMAT_BGRA_8888    ? ALIGN_UP((width) * 4, 16)  : \
    (rk_fmt) == RK_FORMAT_YCrCb_420_SP ? ALIGN_UP((width), 16)      : /* NV12/NV21 */ \
    (rk_fmt) == RK_FORMAT_YCbCr_420_SP ? ALIGN_UP((width), 16)      : \
    (rk_fmt) == RK_FORMAT_YUYV_422     ? ALIGN_UP((width) * 2, 16)  : \
    (rk_fmt) == RK_FORMAT_UYVY_422     ? ALIGN_UP((width) * 2, 16)  : \
    (rk_fmt) == RK_FORMAT_YCbCr_422_SP ? ALIGN_UP((width), 16)      : \
                                         ALIGN_UP((width) * 3, 16))  /* По умолчанию считаем 3 байта */

#endif