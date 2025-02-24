#ifndef ROCKCV_TYPE_DRM_H
#define ROCKCV_TYPE_DRM_H

#include <iostream>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/dma-buf.h>
#include <rga/RgaUtils.h>
#include <rga/RgaApi.h>
#include <rga/im2d.h>
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_drm.h>
}
#include "frame_operation.h"
#include "dma_alloc.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/dma-heap.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

struct drm_buffer_t {
    int drm_fd;
    int dma_fd;
    uint32_t handle;
    uint32_t pitch;
    uint32_t size;
    uint8_t* map;
};

void free_dma_buffer(void *va, size_t size, int fd);
void read_from_dma_buffer(void *va, size_t size);
void write_to_dma_buffer(void *va, size_t size, uint8_t value);
int create_dma_buffer(const char *heap_path, size_t size, int *fd, void **va);


// struct drm_mode_create_dumb{
//     uint32_t width;
//     uint32_t height;
//     uint32_t bpp;
//     uint32_t handle;
//     uint32_t pitch;
//     uint32_t size;
// };

// int ion_alloc_fd(int size, int *fd);

// int create_drm_buffer(drm_buffer_t& drm_buf, int new_width, int new_height);
// // Освобождение DRM-буфера
// void release_drm_buffer(drm_buffer_t& drm_buf);


#endif