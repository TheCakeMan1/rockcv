#ifndef _RKDRM_H
#define _RKDRM_H
#include <libavutil/hwcontext_drm.h>
#include <linux/dma-heap.h>
#include <fcntl.h>
#include <libavformat/avformat.h>
#include <unistd.h>
#include <sys/ioctl.h>

int drmprime_fd_from_frame(const AVFrame *f);
int alloc_dma_buf(size_t size);

#endif