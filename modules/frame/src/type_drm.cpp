#include "type_drm.h"

int create_dma_buffer(const char *heap_path, size_t size, int *fd, void **va) {
    struct dma_heap_allocation_data alloc_data = {0};
    alloc_data.len = size;
    alloc_data.fd_flags = O_RDWR;
    alloc_data.heap_flags = 0;

    // Открываем dma_heap
    int heap_fd = open(heap_path, O_RDWR);
    if (heap_fd < 0) {
        perror("Ошибка открытия /dev/dma_heap/");
        return -1;
    }

    // Запрашиваем буфер
    if (ioctl(heap_fd, DMA_HEAP_IOCTL_ALLOC, &alloc_data) < 0) {
        perror("Ошибка выделения DMA буфера");
        close(heap_fd);
        return -1;
    }
    close(heap_fd);  // Закрываем heap_fd, больше не нужен

    *fd = alloc_data.fd;

    // Маппим буфер в user-space
    *va = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);
    if (*va == MAP_FAILED) {
        perror("Ошибка mmap");
        close(*fd);
        return -1;
    }

    return 0;
}

// Функция записи данных в DMA-буфер
void write_to_dma_buffer(void *va, size_t size, uint8_t value) {
    memset(va, value, size);
}

// Функция чтения данных из DMA-буфера
void read_from_dma_buffer(void *va, size_t size) {
    uint8_t *data = (uint8_t *)va;
    printf("Первые 16 байт DMA-буфера: ");
    for (size_t i = 0; i < 16 && i < size; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

// Функция освобождения DMA-буфера
void free_dma_buffer(void *va, size_t size, int fd) {
    if (va && va != MAP_FAILED) {
        munmap(va, size);
    }
    if (fd >= 0) {
        close(fd);
    }
}

// int8_t resize_dma(frame_t *input, frame_t *output, int new_width, int new_height, AVPixelFormat format, bool mode) {    
//     rga_info_t src, dst;
//     memset(&src, 0, sizeof(rga_info_t));
//     memset(&dst, 0, sizeof(rga_info_t));

//     if (!input || !input->avframe) {
//         std::cerr << "Ошибка: входной кадр NULL!" << std::endl;
//         return 1;
//     }
    
//     //* Освобождаем предыдущие ресурсы
//     if (output->avframe != nullptr) {
//         av_freep(&output->avframe->data[0]);
//         av_frame_free(&output->avframe);
//         avcodec_free_context(&output->avcodeccontext);
//         output->avframe = nullptr;
//         output->avcodeccontext = nullptr;
//     }

//     new_width = ALIGN_UP(new_width, 16);
//     new_height = ALIGN_UP(new_height, 16);

//     output->avframe = av_frame_alloc();
//     output->avframe->width = new_width;
//     output->avframe->height = new_height;
//     output->avframe->format = format;

//     if (!output->avframe) {
//         std::cerr << "Ошибка выделения памяти для avframe!" << std::endl;
//         return 1;
//     }

//     int dma_fd_input = input->dmabuf_fd;
//     int dma_fd_output = output->dmabuf_fd;

//     if (dma_fd_input < 0 || dma_fd_output < 0) {
//         std::cerr << "Ошибка: невалидный DMA FD!" << std::endl;
//         return 1;
//     }

//     src.fd = dma_fd_input;
//     src.mmuFlag = 1;
//     src.format = AV_TO_RK_FORMAT(AVPixelFormat(input->avframe->format));

//     dst.fd = dma_fd_output;
//     dst.mmuFlag = 1;
//     dst.format = AV_TO_RK_FORMAT(format);

//     if(mode) {
//         int orig_w = input->avframe->width;
//         int orig_h = input->avframe->height;

//         float scale_w = (float)new_width / orig_w;
//         float scale_h = (float)new_height / orig_h;
//         float scale = std::min(scale_w, scale_h); 

//         int resized_w = orig_w * scale;
//         int resized_h = orig_h * scale;

//         int pad_x = (new_width - resized_w) / 2;
//         int pad_y = (new_height - resized_h) / 2;
//         rga_set_rect(&src.rect, 0, 0, orig_w, orig_h, 
//             ALIGN_UP(orig_w, 16), ALIGN_UP(orig_h, 16), src.format);
//         rga_set_rect(&dst.rect, pad_x, pad_y, resized_w, resized_h, 
//             new_width, new_height, dst.format);
//     } else {
//         rga_set_rect(&src.rect, 0, 0, input->avframe->width, input->avframe->height, 
//             ALIGN_UP(input->avframe->width, 16), ALIGN_UP(input->avframe->height, 16), src.format);
//         rga_set_rect(&dst.rect, 0, 0, new_width, new_height, 
//             new_width, new_height, dst.format);
//     }

//     int rga_status = c_RkRgaBlit(&src, &dst, NULL);
//     if (rga_status != 0) {
//         std::cerr << "Ошибка: RGA масштабирование через DMA не удалось! Код ошибки: " << rga_status << std::endl;
//         return 1;
//     }

//     return 0;
// }

// int ion_alloc_fd(int size, int *fd) {
//     int ion_fd = open("/dev/ion", O_RDONLY);
//     if (ion_fd < 0) {
//         perror("Не удалось открыть /dev/ion");
//         return -1;
//     }

//     struct ion_allocation_data alloc_data = {
//         .len = size,
//         .heap_id_mask = ION_HEAP_TYPE_DMA_MASK,
//         .flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC
//     };

//     if (ioctl(ion_fd, ION_IOC_ALLOC, &alloc_data) < 0) {
//         perror("ION_IOC_ALLOC не удался");
//         close(ion_fd);
//         return -1;
//     }

//     struct ion_fd_data fd_data = { .handle = alloc_data.handle };
//     if (ioctl(ion_fd, ION_IOC_SHARE, &fd_data) < 0) {
//         perror("ION_IOC_SHARE не удался");
//         close(ion_fd);
//         return -1;
//     }

//     *fd = fd_data.fd;
//     close(ion_fd);
//     return 0;
// }

// int create_drm_buffer(drm_buffer_t& drm_buf, int width, int height) {
//     drm_buf.drm_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
//     if (drm_buf.drm_fd < 0) {
//         std::cerr << "Ошибка: не удалось открыть DRM!" << std::endl;
//         return -1;
//     }

//     struct drm_mode_create_dumb create = {};
//     create.width = width;
//     create.height = height;
//     create.bpp = 24;  // 3 байта на пиксель (BGR888)

//     if (ioctl(drm_buf.drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) < 0) {
//         std::cerr << "Ошибка: не удалось создать DRM буфер!" << std::endl;
//         close(drm_buf.drm_fd);
//         return -1;
//     }

//     drm_buf.handle = create.handle;
//     drm_buf.pitch = create.pitch;
//     drm_buf.size = create.size;

//     struct drm_prime_handle prime = {};
//     prime.handle = create.handle;
//     if (ioctl(drm_buf.drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime) < 0) {
//         std::cerr << "Ошибка: не удалось получить DMA FD!" << std::endl;
//         close(drm_buf.drm_fd);
//         return -1;
//     }

//     drm_buf.dma_fd = prime.fd;

//     struct drm_mode_map_dumb map = {};
//     map.handle = create.handle;

//     if (ioctl(drm_buf.drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &map) < 0) {
//         std::cerr << "Ошибка: не удалось отобразить DRM буфер!" << std::endl;
//         close(drm_buf.drm_fd);
//         return -1;
//     }

//     drm_buf.map = static_cast<uint8_t*>(mmap(0, create.size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_buf.drm_fd, map.offset));
//     if (drm_buf.map == MAP_FAILED) {
//         std::cerr << "Ошибка: не удалось отобразить память!" << std::endl;
//         close(drm_buf.drm_fd);
//         return -1;
//     }

//     return 0;
// }

// void release_drm_buffer(drm_buffer_t& drm_buf) {
//     if (drm_buf.map) {
//         munmap(drm_buf.map, drm_buf.size);
//     }
//     close(drm_buf.dma_fd);
//     close(drm_buf.drm_fd);
// }