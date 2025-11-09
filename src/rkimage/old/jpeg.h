#ifndef _RKIMAGE_JPEG_H
#define _RKIMAGE_JPEG_H

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include "image_codec.h"
#include "rkvideo.h"

int save_frame_as_image(rkcv_t *ctx, const char *filename);

#endif