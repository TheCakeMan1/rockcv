#include "type_drm.h"

int create_drm_buffer(drm_buffer_t& drm_buf, int width, int height) {
    drm_buf.drm_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (drm_buf.drm_fd < 0) {
        std::cerr << "Ошибка: не удалось открыть DRM!" << std::endl;
        return -1;
    }

    struct drm_mode_create_dumb create = {};
    create.width = width;
    create.height = height;
    create.bpp = 24;  // 3 байта на пиксель (BGR888)

    if (ioctl(drm_buf.drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) < 0) {
        std::cerr << "Ошибка: не удалось создать DRM буфер!" << std::endl;
        close(drm_buf.drm_fd);
        return -1;
    }

    drm_buf.handle = create.handle;
    drm_buf.pitch = create.pitch;
    drm_buf.size = create.size;

    struct drm_prime_handle prime = {};
    prime.handle = create.handle;
    if (ioctl(drm_buf.drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime) < 0) {
        std::cerr << "Ошибка: не удалось получить DMA FD!" << std::endl;
        close(drm_buf.drm_fd);
        return -1;
    }

    drm_buf.dma_fd = prime.fd;

    struct drm_mode_map_dumb map = {};
    map.handle = create.handle;

    if (ioctl(drm_buf.drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &map) < 0) {
        std::cerr << "Ошибка: не удалось отобразить DRM буфер!" << std::endl;
        close(drm_buf.drm_fd);
        return -1;
    }

    drm_buf.map = static_cast<uint8_t*>(mmap(0, create.size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_buf.drm_fd, map.offset));
    if (drm_buf.map == MAP_FAILED) {
        std::cerr << "Ошибка: не удалось отобразить память!" << std::endl;
        close(drm_buf.drm_fd);
        return -1;
    }

    return 0;
}

void release_drm_buffer(drm_buffer_t& drm_buf) {
    if (drm_buf.map) {
        munmap(drm_buf.map, drm_buf.size);
    }
    close(drm_buf.dma_fd);
    close(drm_buf.drm_fd);
}