#include "transform.h"
#include "func.h"

namespace rockf
{
    // int8_t copy(frame_t *input, frame_t *output)
    // {
    //     rga_info_t src, dst;
    //     memset(&src, 0, sizeof(rga_info_t));
    //     memset(&dst, 0, sizeof(rga_info_t));

    //     if (!input || !input->avframe)
    //     {
    //         std::cerr << "Ошибка: входной кадр NULL!" << std::endl;
    //         return 1;
    //     }

    //     //* Чистим avframe
    //     if (output->avframe != nullptr)
    //     {
    //         av_freep(&output->avframe->data[0]);
    //         av_frame_free(&output->avframe);
    //         avcodec_free_context(&output->avcodeccontext);
    //         output->avframe = nullptr;
    //         output->avcodeccontext = nullptr;
    //     }

    //     output->avframe = av_frame_alloc();
    //     output->avframe->width = input->avframe->width;
    //     output->avframe->height = input->avframe->height;
    //     output->avframe->format = input->avframe->format;

    //     if (!output->avframe)
    //     {
    //         std::cerr << "Ошибка выделения памяти для avframe!" << std::endl;
    //         return 1;
    //     }

    //     int ret = av_image_alloc(output->avframe->data, output->avframe->linesize,
    //                              input->avframe->width, input->avframe->height, (AVPixelFormat)input->avframe->format, 16);

    //     if (ret < 0)
    //     {
    //         av_frame_free(&output->avframe);
    //         std::cerr << "Ошибка выделения памяти для выходного кадра! Код ошибки: " << ret << std::endl;
    //         return 1;
    //     }

    //     //* Чистим буферы
    //     buffer_free(input);
    //     buffer_free(output);

    //     //* Заполняем память
    //     convert_avframe_to_buffer(input);

    //     src.fd = -1;
    //     src.virAddr = input->buffer;
    //     src.mmuFlag = 1;
    //     src.format = AV_TO_RK_FORMAT(AVPixelFormat(input->avframe->format));

    //     dst.fd = -1;
    //     dst.virAddr = output->buffer;
    //     dst.mmuFlag = 1;
    //     dst.format = AV_TO_RK_FORMAT(input->avframe->format);

    //     rga_set_rect(&src.rect, 0, 0, input->avframe->width, input->avframe->height,
    //                  ALIGN_UP(input->avframe->width, 16), ALIGN_UP(input->avframe->height, 16), src.format);
    //     rga_set_rect(&dst.rect, 0, 0, input->avframe->width, input->avframe->height,
    //                  ALIGN_UP(input->avframe->width, 16), ALIGN_UP(input->avframe->height, 16), dst.format);

    //     int rga_status = c_RkRgaBlit(&src, &dst, NULL);
    //     if (rga_status != 0)
    //     {
    //         std::cerr << "Ошибка: RGA масштабирование не удалось! Код ошибки: " << rga_status << std::endl;
    //         av_free(input->buffer);
    //         av_free(output->buffer);
    //         av_frame_free(&output->avframe);
    //         return 1;
    //     }

    //     convert_buffer_to_avframe(output);

    //     return 0;
    // }

    // int8_t convert(frame_t *input, frame_t *output, AVPixelFormat format)
    // {
    //     rga_info_t src, dst;
    //     memset(&src, 0, sizeof(rga_info_t));
    //     memset(&dst, 0, sizeof(rga_info_t));

    //     if (!input || !input->avframe)
    //     {
    //         std::cerr << "Ошибка: входной кадр NULL!" << std::endl;
    //         return 1;
    //     }

    //     //* Чистим avframe
    //     if (output->avframe != nullptr)
    //     {
    //         av_freep(&output->avframe->data[0]);
    //         av_frame_free(&output->avframe);
    //         avcodec_free_context(&output->avcodeccontext);
    //         output->avframe = nullptr;
    //         output->avcodeccontext = nullptr;
    //     }

    //     output->avframe = av_frame_alloc();
    //     output->avframe->width = input->avframe->width;
    //     output->avframe->height = input->avframe->height;
    //     output->avframe->format = format;

    //     if (!output->avframe)
    //     {
    //         std::cerr << "Ошибка выделения памяти для avframe!" << std::endl;
    //         return 1;
    //     }

    //     int ret = av_image_alloc(output->avframe->data, output->avframe->linesize,
    //                              input->avframe->width, input->avframe->height, format, 16);

    //     if (ret < 0)
    //     {
    //         av_frame_free(&output->avframe);
    //         std::cerr << "Ошибка выделения памяти для выходного кадра! Код ошибки: " << ret << std::endl;
    //         return 1;
    //     }

    //     //* Чистим буферы
    //     buffer_free(input);
    //     buffer_free(output);

    //     //* Заполняем память
    //     convert_avframe_to_buffer(input);

    //     src.fd = -1;
    //     src.virAddr = input->buffer;
    //     src.mmuFlag = 1;
    //     src.format = AV_TO_RK_FORMAT(AVPixelFormat(input->avframe->format));

    //     dst.fd = -1;
    //     dst.virAddr = output->buffer;
    //     dst.mmuFlag = 1;
    //     dst.format = AV_TO_RK_FORMAT(format);

    //     rga_set_rect(&src.rect, 0, 0, input->avframe->width, input->avframe->height,
    //                  ALIGN_UP(input->avframe->width, 16), ALIGN_UP(input->avframe->height, 16), src.format);
    //     rga_set_rect(&dst.rect, 0, 0, input->avframe->width, input->avframe->height,
    //                  ALIGN_UP(input->avframe->width, 16), ALIGN_UP(input->avframe->height, 16), dst.format);

    //     int rga_status = c_RkRgaBlit(&src, &dst, NULL);
    //     if (rga_status != 0)
    //     {
    //         std::cerr << "Ошибка: RGA масштабирование не удалось! Код ошибки: " << rga_status << std::endl;
    //         av_free(input->buffer);
    //         av_free(output->buffer);
    //         av_frame_free(&output->avframe);
    //         return 1;
    //     }

    //     convert_buffer_to_avframe(output);

    //     return 0;
    // }

    int8_t resize(frame_t &input, frame_t &output, int new_width, int new_height, AVPixelFormat format, bool mode)
    {
        rga_info_t src, dst;
        memset(&src, 0, sizeof(rga_info_t));
        memset(&dst, 0, sizeof(rga_info_t));

        output.height = align_up(new_height, 16);
        output.width = align_up(new_width, 16);
        output.format = format;

        if (!&input || !&input.avframe)
        {
            std::cerr << "Ошибка: входной кадр NULL!" << std::endl;
            return 1;
        }

        //* Чистим avframe
        if (output.avframe != nullptr)
        {
            av_freep(&output.avframe->data[0]);
            av_frame_free(&output.avframe);
            avcodec_free_context(&output.avcodeccontext);
            output.avframe = nullptr;
            output.avcodeccontext = nullptr;
        }

        buffer_free(output);

        src.fd = -1;
        src.virAddr = input.buffer;
        src.mmuFlag = 1;
        src.format = av_to_rk_format(AVPixelFormat(input.avframe->format));

        dst.fd = -1;
        dst.virAddr = output.buffer;
        dst.mmuFlag = 1;
        dst.format = av_to_rk_format(format);

        if (mode)
        {
            int orig_w = input.avframe->width;
            int orig_h = input.avframe->height;

            int resized_w, resized_h, pad_x, pad_y;

            calc_resize_neon(orig_w, orig_h, output.width, output.height, &resized_w, &resized_h, &pad_x, &pad_y);

            rga_set_rect(&src.rect, 0, 0, orig_w, orig_h,
                         align_up(orig_w, 16), align_up(orig_h, 16), src.format);
            rga_set_rect(&dst.rect, pad_x, pad_y, resized_w, resized_h,
                         output.width, output.height, dst.format);
        }
        else
        {
            rga_set_rect(&src.rect, 0, 0, input.avframe->width, input.avframe->height,
                         align_up(input.avframe->width, 16), align_up(input.avframe->height, 16), src.format);
            rga_set_rect(&dst.rect, 0, 0, output.width, output.height,
                         output.width, output.height, dst.format);
        }

        int rga_status = c_RkRgaBlit(&src, &dst, NULL);
        if (rga_status != 0)
        {
            std::cerr << "Ошибка: RGA масштабирование не удалось! Код ошибки: " << rga_status << std::endl;
            // av_frame_free(&output.avframe);
            return 1;
        }

        return 0;
    }

    // int8_t resize(frame_t *input, frame_t *output, int new_width, int new_height, AVPixelFormat format, bool mode) {
    //     rga_info_t src, dst;
    //     memset(&src, 0, sizeof(rga_info_t));
    //     memset(&dst, 0, sizeof(rga_info_t));

    //     if (!input || !input->avframe) {
    //         std::cerr << "Ошибка: входной кадр NULL!" << std::endl;
    //         return 1;
    //     }

    //     //* Чистим avframe
    //     if (output->avframe != nullptr){
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
    //     -1;
    //     if (!output->avframe) {
    //         std::cerr << "Ошибка выделения памяти для avframe!" << std::endl;
    //         return 1;
    //     }

    //     int ret = av_image_alloc(output->avframe->data, output->avframe->linesize,
    //                              new_width, new_height, format, 16);

    //     if (ret < 0) {
    //         av_frame_free(&output->avframe);
    //         std::cerr << "Ошибка выделения памяти для выходного кадра! Код ошибки: " << ret << std::endl;
    //         return 1;
    //     }

    //     //* Чистим буферы
    //     buffer_free(input);
    //     buffer_free(output);

    //     //* Заполняем память
    //     convert_avframe_to_buffer(input);

    //     src.fd = -1;
    //     src.virAddr = input->buffer;
    //     src.mmuFlag = 1;
    //     src.format = AV_TO_RK_FORMAT(AVPixelFormat(input->avframe->format));

    //     dst.fd = -1;
    //     dst.virAddr = output->buffer;
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
    //         std::cerr << "Ошибка: RGA масштабирование не удалось! Код ошибки: " << rga_status << std::endl;
    //         av_free(input->buffer);
    //         av_free(output->buffer);
    //         av_frame_free(&output->avframe);
    //         return 1;
    //     }

    //     convert_buffer_to_avframe(output);
    //     return 0;
    // }

    // int8_t rotate(frame_t *input, frame_t *output, int angle, AVPixelFormat format)
    // {
    //     rga_info_t src, dst;
    //     memset(&src, 0, sizeof(rga_info_t));
    //     memset(&dst, 0, sizeof(rga_info_t));

    //     if (!input || !input->avframe)
    //     {
    //         std::cerr << "Ошибка: входной кадр NULL!" << std::endl;
    //         return 1;
    //     }

    //     //* Чистим avframe
    //     if (output->avframe != nullptr)
    //     {
    //         av_freep(&output->avframe->data[0]);
    //         av_frame_free(&output->avframe);
    //         avcodec_free_context(&output->avcodeccontext);
    //         output->avframe = nullptr;
    //         output->avcodeccontext = nullptr;
    //     }

    //     output->avframe = av_frame_alloc();
    //     output->avframe->width = input->avframe->width;
    //     output->avframe->height = input->avframe->height;
    //     output->avframe->format = format;
    //     if (!output->avframe)
    //     {
    //         std::cerr << "Ошибка выделения памяти для avframe!" << std::endl;
    //         return 1;
    //     }

    //     int ret = av_image_alloc(output->avframe->data, output->avframe->linesize,
    //                              output->avframe->width, output->avframe->height, format, 16);

    //     if (ret < 0)
    //     {
    //         av_frame_free(&output->avframe);
    //         std::cerr << "Ошибка выделения памяти для выходного кадра! Код ошибки: " << ret << std::endl;
    //         return 1;
    //     }

    //     //* Чистим буферы
    //     buffer_free(output);
    //     buffer_free(input);

    //     //* Заполняем память
    //     convert_avframe_to_buffer(input);

    //     src.fd = -1;
    //     src.virAddr = input->buffer;
    //     src.mmuFlag = 1;
    //     if (angle == 90)
    //         src.rotation = HAL_TRANSFORM_ROT_90;
    //     else if (angle == 180)
    //         src.rotation = HAL_TRANSFORM_ROT_180;
    //     else if (angle == 270)
    //         src.rotation = HAL_TRANSFORM_ROT_270;
    //     else
    //     {
    //         std::cerr << "Ошибка: неподдерживаемый угол поворота!" << std::endl;
    //         return 1;
    //     }
    //     src.format = AV_TO_RK_FORMAT(AVPixelFormat(input->avframe->format));
    //     rga_set_rect(&src.rect, 0, 0, input->avframe->width, input->avframe->height,
    //                  ALIGN_UP(input->avframe->width, 16), ALIGN_UP(input->avframe->height, 16), src.format);

    //     dst.fd = -1;
    //     dst.virAddr = output->buffer;
    //     dst.mmuFlag = 1;
    //     dst.format = AV_TO_RK_FORMAT(format);
    //     rga_set_rect(&dst.rect, 0, 0, output->avframe->width, output->avframe->height,
    //                  ALIGN_UP(output->avframe->width, 16), ALIGN_UP(output->avframe->height, 16), dst.format);

    //     int rga_status = c_RkRgaBlit(&src, &dst, NULL);

    //     if (rga_status != 0)
    //     {
    //         std::cerr << "Ошибка: RGA масштабирование не удалось! Код ошибки: " << rga_status << std::endl;
    //         av_free(input->buffer);
    //         av_free(output->buffer);
    //         av_frame_free(&output->avframe);
    //         return 1;
    //     }

    //     convert_buffer_to_avframe(output);

    //     return 0;
    // }

    // int8_t flip(frame_t *input, frame_t *output, int type, AVPixelFormat format)
    // {
    //     rga_info_t src, dst;
    //     memset(&src, 0, sizeof(rga_info_t));
    //     memset(&dst, 0, sizeof(rga_info_t));

    //     if (!input || !input->avframe)
    //     {
    //         std::cerr << "Ошибка: входной кадр NULL!" << std::endl;
    //         return 1;
    //     }

    //     //* Чистим avframe
    //     if (output->avframe != nullptr)
    //     {
    //         av_freep(&output->avframe->data[0]);
    //         av_frame_free(&output->avframe);
    //         avcodec_free_context(&output->avcodeccontext);
    //         output->avframe = nullptr;
    //         output->avcodeccontext = nullptr;
    //     }

    //     output->avframe = av_frame_alloc();
    //     output->avframe->width = input->avframe->width;
    //     output->avframe->height = input->avframe->height;
    //     output->avframe->format = format;
    //     if (!output->avframe)
    //     {
    //         std::cerr << "Ошибка выделения памяти для avframe!" << std::endl;
    //         return 1;
    //     }

    //     int ret = av_image_alloc(output->avframe->data, output->avframe->linesize,
    //                              output->avframe->width, output->avframe->height, format, 16);

    //     if (ret < 0)
    //     {
    //         av_frame_free(&output->avframe);
    //         std::cerr << "Ошибка выделения памяти для выходного кадра! Код ошибки: " << ret << std::endl;
    //         return 1;
    //     }

    //     //* Чистим буферы
    //     buffer_free(input);
    //     buffer_free(output);

    //     //* Заполняем память
    //     convert_avframe_to_buffer(input);

    //     src.fd = -1;
    //     src.virAddr = input->buffer;
    //     src.mmuFlag = 1;
    //     if (type == 1)
    //         src.rotation = HAL_TRANSFORM_FLIP_H;
    //     else if (type == 2)
    //         src.rotation = HAL_TRANSFORM_FLIP_V;
    //     else if (type == 3)
    //         src.rotation = HAL_TRANSFORM_FLIP_H_V;
    //     else
    //     {
    //         std::cerr << "Ошибка: неподдерживаемый угол поворота!" << std::endl;
    //         return 1;
    //     }
    //     src.format = AV_TO_RK_FORMAT(AVPixelFormat(input->avframe->format));
    //     rga_set_rect(&src.rect, 0, 0, input->avframe->width, input->avframe->height,
    //                  ALIGN_UP(input->avframe->width, 16), ALIGN_UP(input->avframe->height, 16), src.format);

    //     dst.fd = -1;
    //     dst.virAddr = output->buffer;
    //     dst.mmuFlag = 1;
    //     dst.format = AV_TO_RK_FORMAT(format);
    //     rga_set_rect(&dst.rect, 0, 0, output->avframe->width, output->avframe->height,
    //                  ALIGN_UP(output->avframe->width, 16), ALIGN_UP(output->avframe->height, 16), dst.format);

    //     int rga_status = c_RkRgaBlit(&src, &dst, NULL);

    //     if (rga_status != 0)
    //     {
    //         std::cerr << "Ошибка: RGA масштабирование не удалось! Код ошибки: " << rga_status << std::endl;
    //         av_free(input->buffer);
    //         av_free(output->buffer);
    //         av_frame_free(&output->avframe);
    //         return 1;
    //     }

    //     convert_buffer_to_avframe(output);

    //     return 0;
    // }

    // int8_t crop(frame_t *input, frame_t *output, int x_start, int y_start, int x_stop, int y_stop, AVPixelFormat format)
    // {
    //     rga_info_t src, dst;
    //     memset(&src, 0, sizeof(rga_info_t));
    //     memset(&dst, 0, sizeof(rga_info_t));

    //     if (!input || !input->avframe)
    //     {
    //         std::cerr << "Ошибка: входной кадр NULL!" << std::endl;
    //         return 1;
    //     }

    //     //* Чистим avframe
    //     if (output->avframe != nullptr)
    //     {
    //         av_freep(&output->avframe->data[0]);
    //         av_frame_free(&output->avframe);
    //         avcodec_free_context(&output->avcodeccontext);
    //         output->avframe = nullptr;
    //         output->avcodeccontext = nullptr;
    //     }

    //     output->avframe = av_frame_alloc();
    //     output->avframe->width = input->avframe->width;
    //     output->avframe->height = input->avframe->height;
    //     output->avframe->format = format;
    //     if (!output->avframe)
    //     {
    //         std::cerr << "Ошибка выделения памяти для avframe!" << std::endl;
    //         return 1;
    //     }

    //     int ret = av_image_alloc(output->avframe->data, output->avframe->linesize,
    //                              output->avframe->width, output->avframe->height, format, 16);

    //     if (ret < 0)
    //     {
    //         av_frame_free(&output->avframe);
    //         std::cerr << "Ошибка выделения памяти для выходного кадра! Код ошибки: " << ret << std::endl;
    //         return 1;
    //     }

    //     //* Чистим буферы
    //     buffer_free(input);
    //     buffer_free(output);

    //     //* Заполняем память
    //     convert_avframe_to_buffer(input);

    //     src.fd = -1;
    //     src.virAddr = input->buffer;
    //     src.mmuFlag = 1;
    //     src.format = AV_TO_RK_FORMAT(AVPixelFormat(input->avframe->format));
    //     rga_set_rect(&src.rect, x_start, y_start, x_stop, y_stop,
    //                  ALIGN_UP(input->avframe->width, 16), ALIGN_UP(input->avframe->height, 16), src.format);

    //     dst.fd = -1;
    //     dst.virAddr = output->buffer;
    //     dst.mmuFlag = 1;
    //     dst.format = AV_TO_RK_FORMAT(format);
    //     rga_set_rect(&dst.rect, 0, 0, output->avframe->width, output->avframe->height,
    //                  ALIGN_UP(output->avframe->width, 16), ALIGN_UP(output->avframe->height, 16), dst.format);

    //     int rga_status = c_RkRgaBlit(&src, &dst, NULL);

    //     if (rga_status != 0)
    //     {
    //         std::cerr << "Ошибка: RGA масштабирование не удалось! Код ошибки: " << rga_status << std::endl;
    //         av_free(input->buffer);
    //         av_free(output->buffer);
    //         av_frame_free(&output->avframe);
    //         return 1;
    //     }

    //     convert_buffer_to_avframe(output);

    //     return 0;
    // }

}