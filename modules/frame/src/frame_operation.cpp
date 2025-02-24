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

void frame2mat(frame_t &input, cv::Mat &output)
{
    output = cv::Mat(input.height,
                     input.width,
                     CV_8UC3,
                     input.buffer,
                     0);
}

void buffer_free(frame_t &frame)
{
    int new_size = av_image_get_buffer_size(
        (AVPixelFormat)(frame.format),
        frame.width,
        frame.height,
        1);

    if (new_size <= 0)
    {
        std::cerr << "Ошибка вычисления размера буфера!" << std::endl;
        return;
    }

    if (frame.buffer)
    {
        uint8_t *new_buffer = (uint8_t *)av_realloc(frame.buffer, new_size);
        if (!new_buffer)
        {
            std::cerr << "Ошибка перевыделения памяти для frame.buffer!" << std::endl;
            return;
        }
        frame.buffer = new_buffer;
    }
    else
    {
        frame.buffer = (uint8_t *)av_malloc(new_size);
        if (!frame.buffer)
        {
            std::cerr << "Ошибка выделения памяти для frame->buffer!" << std::endl;
        }
    }
}
