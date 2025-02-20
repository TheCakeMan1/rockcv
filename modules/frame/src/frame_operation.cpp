#include "frame_operation.h"

// void buffer_free(frame_t* frame){
//     if (frame->buffer != nullptr) {
//         uint8_t *new_buffer = (uint8_t*)av_realloc(frame->buffer, av_image_get_buffer_size(
//             (AVPixelFormat)(frame->avframe->format), 
//             frame->avframe->width, 
//             frame->avframe->height, 
//             1
//         ));
//         frame->buffer = new_buffer;
//     } else {
//         frame->buffer = (uint8_t*)av_malloc(av_image_get_buffer_size(
//             (AVPixelFormat)(frame->avframe->format), 
//             frame->avframe->width, 
//             frame->avframe->height, 
//             1
//         ));
//         if (!frame->buffer) {
//             std::cerr << "Ошибка выделения памяти для output->buffer!" << std::endl;
//         }
//     }
// }


void buffer_free(frame_t* frame) {
    if (!frame || !frame->avframe) return;

    int new_size = av_image_get_buffer_size(
        (AVPixelFormat)(frame->avframe->format),
        frame->avframe->width,
        frame->avframe->height,
        1
    );

    if (new_size <= 0) {
        std::cerr << "Ошибка вычисления размера буфера!" << std::endl;
        return;
    }

    if (frame->buffer) {
        // Проверяем, изменилась ли необходимая память
        if (av_image_get_buffer_size((AVPixelFormat)(frame->avframe->format),
                                     frame->avframe->width, 
                                     frame->avframe->height, 
                                     1) > new_size) {
            // Освобождаем, если новый размер больше
            av_freep(&frame->buffer);
            frame->buffer = (uint8_t*)av_malloc(new_size);
        } else {
            // Перевыделяем, если размер тот же или меньше
            uint8_t *new_buffer = (uint8_t*)av_realloc(frame->buffer, new_size);
            if (!new_buffer) {
                std::cerr << "Ошибка перевыделения памяти для frame->buffer!" << std::endl;
                return;
            }
            frame->buffer = new_buffer;
        }
    } else {
        // Выделяем память, если буфер ещё не создан
        frame->buffer = (uint8_t*)av_malloc(new_size);
        if (!frame->buffer) {
            std::cerr << "Ошибка выделения памяти для frame->buffer!" << std::endl;
        }
    }
}
