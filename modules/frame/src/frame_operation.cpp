#include "frame_operation.h"

void buffer_free(frame_t* frame){
    if (frame->buffer != nullptr) {
        uint8_t *new_buffer = (uint8_t*)av_realloc(frame->buffer, av_image_get_buffer_size(
            (AVPixelFormat)(frame->avframe->format), 
            frame->avframe->width, 
            frame->avframe->height, 
            1
        ));
        frame->buffer = new_buffer;
    } else {
        frame->buffer = (uint8_t*)av_malloc(av_image_get_buffer_size(
            (AVPixelFormat)(frame->avframe->format), 
            frame->avframe->width, 
            frame->avframe->height, 
            1
        ));
        if (!frame->buffer) {
            std::cerr << "Ошибка выделения памяти для output->buffer!" << std::endl;
        }
    }
}
