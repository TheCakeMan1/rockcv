#ifndef ROCKCV_TYPE_DRM_H
#define ROCKCV_TYPE_DRM_H

#include <iostream>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <rga/rga.h>
#include <rga/RgaUtils.h>
#include <libavutil/frame.h>
#include "frame_operation.h"
#include <drm/drm.h>

struct drm_buffer_t {
    int drm_fd;
    int dma_fd;
    uint32_t handle;
    uint32_t pitch;
    uint32_t size;
    uint8_t* map;
};

// struct drm_mode_create_dumb{
//     uint32_t width;
//     uint32_t height;
//     uint32_t bpp;
//     uint32_t handle;
//     uint32_t pitch;
//     uint32_t size;
// };

int create_drm_buffer(drm_buffer_t& drm_buf, int new_width, int new_height);
// Освобождение DRM-буфера
void release_drm_buffer(drm_buffer_t& drm_buf);


#endif