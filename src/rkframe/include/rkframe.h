
#ifndef _RKFRAME_H
#define _RKFRAME_H
#include <stdio.h>
#include <libavutil/hwcontext_drm.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <rga/RgaApi.h>
#include <rga/RgaUtils.h>
#include "drm/drm_fourcc.h"
#include "rkdrm.h"

typedef enum
{
    RK_PIX_FMT_RGBA_8888,
    RK_PIX_FMT_RGBX_8888,
    RK_PIX_FMT_RGB_888,
    RK_PIX_FMT_RGB_BGRA_8888,
    RK_PIX_FMT_BGR_888,
    RK_PIX_FMT_RGB_565,
    RK_PIX_FMT_BGR_565,
    RK_PIX_FMT_ARGB_8888,
    RK_PIX_FMT_ABGR_8888,
    RK_PIX_FMT_XRGB_8888,
    RK_PIX_FMT_XBGR_8888,

    RK_PIX_FMT_YCbCr_420_P,
    RK_PIX_FMT_YCbCr_420_SP,
    RK_PIX_FMT_YCrCb_420_SP,
    RK_PIX_FMT_YCbCr_422_P,
    RK_PIX_FMT_YCbCr_422_SP,
    // RK_PIX_FMT_YCrCb_422_SP, // TODO не нашел реализацию
    RK_PIX_FMT_YCbCr_400,
    RK_PIX_FMT_YUYV_422,
    RK_PIX_FMT_UYVY_422,
    RK_PIX_FMT_YVYU_422,

    RK_PIX_FMT_YCbCr_420_SP_10B,
    RK_PIX_FMT_YCrCb_420_SP_10B,
    RK_PIX_FMT_YCbCr_422_SP_10B,
    RK_PIX_FMT_YCrCb_422_SP_10B,
} rk_pix_fmt_t;

static inline int convert_pix_fmt(rk_pix_fmt_t fmt, _Bool to_rga)
{
    switch (fmt)
    {
    case RK_PIX_FMT_RGBA_8888:
        return to_rga ? RK_FORMAT_RGBA_8888 : AV_PIX_FMT_RGBA;
    case RK_PIX_FMT_RGBX_8888:
        return to_rga ? RK_FORMAT_RGBX_8888 : AV_PIX_FMT_RGB0;
    case RK_PIX_FMT_RGB_888:
        return to_rga ? RK_FORMAT_RGB_888 : AV_PIX_FMT_RGB24;
    case RK_PIX_FMT_RGB_BGRA_8888:
        return to_rga ? RK_FORMAT_BGRA_8888 : AV_PIX_FMT_BGRA;
    case RK_PIX_FMT_BGR_888:
        return to_rga ? RK_FORMAT_BGR_888 : AV_PIX_FMT_BGR24;
    case RK_PIX_FMT_RGB_565:
        return to_rga ? RK_FORMAT_RGB_565 : AV_PIX_FMT_RGB565LE;
    case RK_PIX_FMT_BGR_565:
        return to_rga ? RK_FORMAT_BGR_565 : AV_PIX_FMT_BGR565LE;
    case RK_PIX_FMT_ARGB_8888:
        return to_rga ? RK_FORMAT_ARGB_8888 : AV_PIX_FMT_ARGB;
    case RK_PIX_FMT_ABGR_8888:
        return to_rga ? RK_FORMAT_ABGR_8888 : AV_PIX_FMT_ABGR;
    case RK_PIX_FMT_XRGB_8888:
        return to_rga ? RK_FORMAT_XRGB_8888 : AV_PIX_FMT_0RGB;
    case RK_PIX_FMT_XBGR_8888:
        return to_rga ? RK_FORMAT_XBGR_8888 : AV_PIX_FMT_0BGR;

    case RK_PIX_FMT_YCbCr_420_P:
        return to_rga ? RK_FORMAT_YCbCr_420_P : AV_PIX_FMT_YUV420P;
    case RK_PIX_FMT_YCbCr_420_SP:
        return to_rga ? RK_FORMAT_YCbCr_420_SP : AV_PIX_FMT_NV12;
    case RK_PIX_FMT_YCrCb_420_SP:
        return to_rga ? RK_FORMAT_YCrCb_420_SP : AV_PIX_FMT_NV21;
    case RK_PIX_FMT_YCbCr_422_P:
        return to_rga ? RK_FORMAT_YCbCr_422_P : AV_PIX_FMT_YUV422P;
    case RK_PIX_FMT_YCbCr_422_SP:
        return to_rga ? RK_FORMAT_YCbCr_422_SP : AV_PIX_FMT_NV16;
    // case RK_PIX_FMT_YCrCb_422_SP:
    //     return to_rga ? RK_FORMAT_YCrCb_422_SP : AV_PIX_FMT_NV61;
    case RK_PIX_FMT_YCbCr_400:
        return to_rga ? RK_FORMAT_YCbCr_400 : AV_PIX_FMT_GRAY8;
    case RK_PIX_FMT_YUYV_422:
        return to_rga ? RK_FORMAT_YUYV_422 : AV_PIX_FMT_YUYV422;
    case RK_PIX_FMT_UYVY_422:
        return to_rga ? RK_FORMAT_UYVY_422 : AV_PIX_FMT_UYVY422;
    case RK_PIX_FMT_YVYU_422:
        return to_rga ? RK_FORMAT_YVYU_422 : AV_PIX_FMT_YVYU422;

    case RK_PIX_FMT_YCbCr_420_SP_10B:
        return to_rga ? RK_FORMAT_YCbCr_420_SP_10B : AV_PIX_FMT_P010LE;
    case RK_PIX_FMT_YCrCb_420_SP_10B:
        return to_rga ? RK_FORMAT_YCrCb_420_SP_10B : AV_PIX_FMT_P010LE;
    case RK_PIX_FMT_YCbCr_422_SP_10B:
        return to_rga ? RK_FORMAT_YCbCr_422_SP_10B : AV_PIX_FMT_P210LE;
    case RK_PIX_FMT_YCrCb_422_SP_10B:
        return to_rga ? RK_FORMAT_YCrCb_422_SP_10B : AV_PIX_FMT_P210LE;

    default:
        return -1;
    }
}

static inline int8_t convert_pix_fmt_from_av(int fmt)
{
    switch (fmt)
    {
    case AV_PIX_FMT_RGBA:
        return RK_PIX_FMT_RGBA_8888;
    case AV_PIX_FMT_RGB0:
        return RK_PIX_FMT_RGBX_8888;
    case AV_PIX_FMT_RGB24:
        return RK_PIX_FMT_RGB_888;
    case AV_PIX_FMT_BGRA:
        return RK_PIX_FMT_RGB_BGRA_8888;
    case AV_PIX_FMT_BGR24:
        return RK_PIX_FMT_BGR_888;
    case AV_PIX_FMT_RGB565LE:
        return RK_PIX_FMT_RGB_565;
    case AV_PIX_FMT_BGR565LE:
        return RK_PIX_FMT_BGR_565;
    case AV_PIX_FMT_ARGB:
        return RK_PIX_FMT_ARGB_8888;
    case AV_PIX_FMT_ABGR:
        return RK_PIX_FMT_ABGR_8888;
    case AV_PIX_FMT_0RGB:
        return RK_PIX_FMT_XRGB_8888;
    case AV_PIX_FMT_0BGR:
        return RK_PIX_FMT_XBGR_8888;

    case AV_PIX_FMT_YUV420P:
        return RK_PIX_FMT_YCbCr_420_P;
    case AV_PIX_FMT_NV12:
        return RK_PIX_FMT_YCbCr_420_SP;
    case AV_PIX_FMT_NV21:
        return RK_PIX_FMT_YCrCb_420_SP;
    case AV_PIX_FMT_YUV422P:
        return RK_PIX_FMT_YCbCr_422_P;
    case AV_PIX_FMT_NV16:
        return RK_PIX_FMT_YCbCr_422_SP;
    // case AV_PIX_FMT_NV61:
    //     return RK_PIX_FMT_YCrCb_422_SP;
    case AV_PIX_FMT_GRAY8:
        return RK_PIX_FMT_YCbCr_400;
    case AV_PIX_FMT_YUYV422:
        return RK_PIX_FMT_YUYV_422;
    case AV_PIX_FMT_UYVY422:
        return RK_PIX_FMT_UYVY_422;
    case AV_PIX_FMT_YVYU422:
        return RK_PIX_FMT_YVYU_422;

    case AV_PIX_FMT_P010LE:
        return RK_PIX_FMT_YCbCr_420_SP_10B;
    case AV_PIX_FMT_P210LE:
        return RK_PIX_FMT_YCbCr_422_SP_10B;

    default:
        return -1; // неизвестный формат
    }
}

static inline int8_t convert_pix_fmt_from_drm(uint32_t drm_fmt)
{
    switch (drm_fmt)
    {
    /* ---- YUV 4:2:0 ---- */
    case DRM_FORMAT_NV12:
        return RK_PIX_FMT_YCbCr_420_SP; // Y plane + interleaved UV
    case DRM_FORMAT_NV21:
        return RK_PIX_FMT_YCrCb_420_SP; // Y plane + interleaved VU
    case DRM_FORMAT_YUV420:
        return RK_PIX_FMT_YCbCr_420_P; // planar YUV
    // case DRM_FORMAT_YVU420:
    //     return RK_PIX_FMT_YCrCb_420_P;

    /* ---- YUV 4:2:2 ---- */
    case DRM_FORMAT_NV16:
        return RK_PIX_FMT_YCbCr_422_SP;
    // case DRM_FORMAT_NV61:
    //     return RK_PIX_FMT_YCrCb_422_SP;
    case DRM_FORMAT_YUYV:
        return RK_PIX_FMT_YUYV_422;
    case DRM_FORMAT_UYVY:
        return RK_PIX_FMT_UYVY_422;
    case DRM_FORMAT_YVYU:
        return RK_PIX_FMT_YVYU_422;

    /* ---- 10-бит ---- */
    case DRM_FORMAT_P010:
        return RK_PIX_FMT_YCbCr_420_SP_10B;
    case DRM_FORMAT_P210:
        return RK_PIX_FMT_YCbCr_422_SP_10B;

    /* ---- RGB ---- */
    case DRM_FORMAT_RGB888:
        return RK_PIX_FMT_RGB_888;
    case DRM_FORMAT_BGR888:
        return RK_PIX_FMT_BGR_888;
    case DRM_FORMAT_RGB565:
        return RK_PIX_FMT_RGB_565;
    case DRM_FORMAT_BGR565:
        return RK_PIX_FMT_BGR_565;

    /* ---- ARGB / ABGR / RGBA / BGRA ---- */
    case DRM_FORMAT_ARGB8888:
        return RK_PIX_FMT_ARGB_8888;
    case DRM_FORMAT_ABGR8888:
        return RK_PIX_FMT_ABGR_8888;
    case DRM_FORMAT_XRGB8888:
        return RK_PIX_FMT_XRGB_8888;
    case DRM_FORMAT_XBGR8888:
        return RK_PIX_FMT_XBGR_8888;
    case DRM_FORMAT_RGBA8888:
        return RK_PIX_FMT_RGBA_8888;
    case DRM_FORMAT_BGRA8888:
        return RK_PIX_FMT_RGB_BGRA_8888;

    /* ---- Одноканальные / серые ---- */
    case DRM_FORMAT_R8:
    // case DRM_FORMAT_Y8:
    //     return RK_PIX_FMT_YCbCr_400;

    /* ---- Неизвестный формат ---- */
    default:
        return -1;
    }
}

enum RK_FILTERS
{
    RK_GAUSS
};

typedef struct
{
    int width;
    int height;
    rk_pix_fmt_t fmt;
} info_frame_t;

typedef struct
{
    char *text;
    int x;
    int y;
    int fontsize;
} s_text_f;

typedef struct __attribute__((aligned(64)))
{
    info_frame_t *frame_t;
    int rot;
    int flip;
    rga_info_t s;
    rga_info_t d;
    _Bool rga_init;
    AVRational time_base;
    const AVDRMFrameDescriptor *desc_in;
    const AVDRMFrameDescriptor *desc_out;
} s_convert_f;

typedef struct
{
    enum RK_FILTERS filter_t;
    s_convert_f *c;
    AVFrame *frame;
    AVBufferRef *drm_hwdev;
} rkcv_shot_t;

rkcv_shot_t *nshot(info_frame_t *frame_t);

// rkcv_shot_t *nshot(int width, int height, rk_pix_fmt_t sw_fmt);
void free_shot(rkcv_shot_t *shot);

#endif