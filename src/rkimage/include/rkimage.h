#ifndef _RKIMAGE_H
#define _RKIMAGE_H

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include "rkvideo.h"

static inline const char *get_codec_image(const char *filename)
{
    const char *ext = strrchr(filename, '.'); // ищем последнее '.'
    if (!ext || *(ext + 1) == '\0')
        return "mjpeg"; // дефолт

    ext++; // пропускаем '.'

    if (!strcasecmp(ext, "jpg") || !strcasecmp(ext, "jpeg"))
    {
#ifdef RKMPP_ENABLE
        return "mjpeg_rkmpp";
#pragma message "кодек изображения mjpeg заменен на mjpeg_rkmpp"
#else
        return "mjpeg";
#endif
    }
    else if (!strcasecmp(ext, "png"))
        return "png";
    else if (!strcasecmp(ext, "bmp"))
        return "bmp";
    else if (!strcasecmp(ext, "webp"))
        return "libwebp";
    else if (!strcasecmp(ext, "tiff") || !strcasecmp(ext, "tif"))
        return "tiff";
    else if (!strcasecmp(ext, "ppm"))
        return "ppm";
    else
        return "mjpeg"; // fallback
}

static inline enum AVPixelFormat get_codec_frame(const char *codec)
{
    if (!strcasecmp(codec, "ppm"))
        return AV_PIX_FMT_RGB24;
    else if (!strcasecmp(codec, "mjpeg"))
        // return AV_PIX_FMT_YUVJ420P;
        // return AV_PIX_FMT_YUVJ422P;
        return AV_PIX_FMT_YUVJ444P;
    else if (!strcasecmp(codec, "mjpeg_rkmpp"))
        // return AV_PIX_FMT_YUVJ420P;
        // return AV_PIX_FMT_YUVJ422P;
        return AV_PIX_FMT_RGB0;
    else if (!strcasecmp(codec, "png"))
        return AV_PIX_FMT_RGB24;
}

int ims(rkcv_t *ctx, const char *filename);

#endif