#include "rkdrm.h"

int drmprime_fd_from_frame(const AVFrame *f)
{
    // TODO тут пересоздается AVDRMFrameDescriptor
    if (!f || f->format != AV_PIX_FMT_DRM_PRIME)
        return -1;
    const AVDRMFrameDescriptor *desc = (const AVDRMFrameDescriptor *)f->data[0];
    if (!desc || desc->nb_objects < 1)
        return -1;
    return desc->objects[0].fd; // обычно весь NV12 в одном объекте
}

int alloc_dma_buf(size_t size)
{
    int heap_fd = open("/dev/dma_heap/system", O_RDWR | O_CLOEXEC);

    if (heap_fd < 0)
        return -1;

    struct dma_heap_allocation_data alloc = {
        .len = size,
        .fd_flags = O_CLOEXEC | O_RDWR,
    };
    if (ioctl(heap_fd, DMA_HEAP_IOCTL_ALLOC, &alloc) < 0)
    {
        close(heap_fd);
        return -1;
    }

    close(heap_fd);
    return alloc.fd;
}