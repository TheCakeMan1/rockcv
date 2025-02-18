#include "transform.h"

int8_t resize(frame_t *input, frame_t *output, int new_width, int new_height, AVPixelFormat format) {    
    rga_info_t src, dst;
    memset(&src, 0, sizeof(rga_info_t));
    memset(&dst, 0, sizeof(rga_info_t));

    if (!input || !input->avframe) {
        std::cerr << "Ошибка: входной кадр NULL!" << std::endl;
        return 1;
    }
    
    //* Чистим avframe
    if (output->avframe != nullptr){
        av_freep(&output->avframe->data[0]);
        av_frame_free(&output->avframe);
        avcodec_free_context(&output->avcodeccontext);
        output->avframe = nullptr;
        output->avcodeccontext = nullptr;
    }

    output->avframe = av_frame_alloc();
    output->avframe->width = new_width;
    output->avframe->height = new_height;
    output->avframe->format = format;
    if (!output->avframe) {
        std::cerr << "Ошибка выделения памяти для avframe!" << std::endl;
        return 1;
    }

    int ret = av_image_alloc(output->avframe->data, output->avframe->linesize, 
                             new_width, new_height, format, 16);
    
    if (ret < 0) {
        av_frame_free(&output->avframe);
        std::cerr << "Ошибка выделения памяти для выходного кадра! Код ошибки: " << ret << std::endl;
        return 1;
    }
        
    //* Чистим буферы
    buffer_free(input);
    buffer_free(output);
    
    //* Заполняем память
    convert_avframe_to_buffer(input);

    src.fd = -1;
    src.virAddr = input->buffer;
    src.mmuFlag = 1;
    src.format = AV_TO_RK_FORMAT(AVPixelFormat(input->avframe->format));
    rga_set_rect(&src.rect, 0, 0, input->avframe->width, input->avframe->height, 
                ALIGN_UP(input->avframe->width, 16), ALIGN_UP(input->avframe->height, 16), src.format);

    dst.fd = -1;
    dst.virAddr = output->buffer;
    dst.mmuFlag = 1;
    dst.format = AV_TO_RK_FORMAT(format);
    rga_set_rect(&dst.rect, 0, 0, new_width, new_height, 
                new_width, new_height, dst.format);

    int rga_status = c_RkRgaBlit(&src, &dst, NULL);

    if (rga_status != 0) {
        std::cerr << "Ошибка: RGA масштабирование не удалось! Код ошибки: " << rga_status << std::endl;
        av_free(input->buffer);
        av_free(output->buffer);
        av_frame_free(&output->avframe);
        return 1;
    }

    convert_buffer_to_avframe(output);

    return 0;
}

int8_t rotate(frame_t *input, frame_t *output, int angle, AVPixelFormat format) {    
    rga_info_t src, dst;
    memset(&src, 0, sizeof(rga_info_t));
    memset(&dst, 0, sizeof(rga_info_t));

    if (!input || !input->avframe) {
        std::cerr << "Ошибка: входной кадр NULL!" << std::endl;
        return 1;
    }
    
    //* Чистим avframe
    if (output->avframe != nullptr){
        av_freep(&output->avframe->data[0]);
        av_frame_free(&output->avframe);
        avcodec_free_context(&output->avcodeccontext);
        output->avframe = nullptr;
        output->avcodeccontext = nullptr;
    }

    output->avframe = av_frame_alloc();
    output->avframe->width = input->avframe->width;
    output->avframe->height = input->avframe->height;
    output->avframe->format = format;
    if (!output->avframe) {
        std::cerr << "Ошибка выделения памяти для avframe!" << std::endl;
        return 1;
    }

    int ret = av_image_alloc(output->avframe->data, output->avframe->linesize, 
                             output->avframe->width,  output->avframe->height, format, 16);
    
    if (ret < 0) {
        av_frame_free(&output->avframe);
        std::cerr << "Ошибка выделения памяти для выходного кадра! Код ошибки: " << ret << std::endl;
        return 1;
    }
    
    //* Чистим буферы
    buffer_free(output);
    buffer_free(input);
    
    //* Заполняем память
    convert_avframe_to_buffer(input);

    src.fd = -1;
    src.virAddr = input->buffer;
    src.mmuFlag = 1;
    if (angle == 90) src.rotation = HAL_TRANSFORM_ROT_90;
    else if (angle == 180) src.rotation = HAL_TRANSFORM_ROT_180;
    else if (angle == 270) src.rotation = HAL_TRANSFORM_ROT_270;
    else {
        std::cerr << "Ошибка: неподдерживаемый угол поворота!" << std::endl;
        return 1;
    }
    src.format = AV_TO_RK_FORMAT(AVPixelFormat(input->avframe->format));
    rga_set_rect(&src.rect, 0, 0, input->avframe->width, input->avframe->height, 
                ALIGN_UP(input->avframe->width, 16), ALIGN_UP(input->avframe->height, 16), src.format);

    dst.fd = -1;
    dst.virAddr = output->buffer;
    dst.mmuFlag = 1;
    dst.format = AV_TO_RK_FORMAT(format);
    rga_set_rect(&dst.rect, 0, 0,  output->avframe->width,  output->avframe->height, 
                 output->avframe->width,  output->avframe->height, dst.format);

    int rga_status = c_RkRgaBlit(&src, &dst, NULL);

    if (rga_status != 0) {
        std::cerr << "Ошибка: RGA масштабирование не удалось! Код ошибки: " << rga_status << std::endl;
        av_free(input->buffer);
        av_free(output->buffer);
        av_frame_free(&output->avframe);
        return 1;
    }

    av_image_copy(output->avframe->data, output->avframe->linesize, 
                  (const uint8_t**)&output->buffer, output->avframe->linesize, 
                  AVPixelFormat(output->avframe->format), 
                  output->avframe->width, output->avframe->height);
    
    return 0;
}

int8_t flip(frame_t *input, frame_t *output, int type, AVPixelFormat format) {    
    rga_info_t src, dst;
    memset(&src, 0, sizeof(rga_info_t));
    memset(&dst, 0, sizeof(rga_info_t));

    if (!input || !input->avframe) {
        std::cerr << "Ошибка: входной кадр NULL!" << std::endl;
        return 1;
    }
    
    //* Чистим avframe
    if (output->avframe != nullptr){
        av_freep(&output->avframe->data[0]);
        av_frame_free(&output->avframe);
        avcodec_free_context(&output->avcodeccontext);
        output->avframe = nullptr;
        output->avcodeccontext = nullptr;
    }

    output->avframe = av_frame_alloc();
    output->avframe->width = input->avframe->width;
    output->avframe->height = input->avframe->height;
    output->avframe->format = format;
    if (!output->avframe) {
        std::cerr << "Ошибка выделения памяти для avframe!" << std::endl;
        return 1;
    }

    int ret = av_image_alloc(output->avframe->data, output->avframe->linesize, 
                             output->avframe->width,  output->avframe->height, format, 16);
    
    if (ret < 0) {
        av_frame_free(&output->avframe);
        std::cerr << "Ошибка выделения памяти для выходного кадра! Код ошибки: " << ret << std::endl;
        return 1;
    }
    
    //* Чистим буферы
    buffer_free(input);
    buffer_free(output);
    
    //* Заполняем память
    convert_avframe_to_buffer(input);

    src.fd = -1;
    src.virAddr = input->buffer;
    src.mmuFlag = 1;
    if (type == 1) src.rotation = HAL_TRANSFORM_FLIP_H;
    else if (type == 2) src.rotation = HAL_TRANSFORM_FLIP_V;
    else if (type == 3) src.rotation = HAL_TRANSFORM_FLIP_H_V;
    else {
        std::cerr << "Ошибка: неподдерживаемый угол поворота!" << std::endl;
        return 1;
    }
    src.format = AV_TO_RK_FORMAT(AVPixelFormat(input->avframe->format));
    rga_set_rect(&src.rect, 0, 0, input->avframe->width, input->avframe->height, 
                ALIGN_UP(input->avframe->width, 16), ALIGN_UP(input->avframe->height, 16), src.format);

    dst.fd = -1;
    dst.virAddr = output->buffer;
    dst.mmuFlag = 1;
    dst.format = AV_TO_RK_FORMAT(format);
    rga_set_rect(&dst.rect, 0, 0,  output->avframe->width,  output->avframe->height, 
                 output->avframe->width,  output->avframe->height, dst.format);

    int rga_status = c_RkRgaBlit(&src, &dst, NULL);

    if (rga_status != 0) {
        std::cerr << "Ошибка: RGA масштабирование не удалось! Код ошибки: " << rga_status << std::endl;
        av_free(input->buffer);
        av_free(output->buffer);
        av_frame_free(&output->avframe);
        return 1;
    }

    convert_buffer_to_avframe(output);
    
    return 0;
}

int8_t crop(frame_t *input, frame_t *output, int x_start, int y_start, int x_stop, int y_stop, AVPixelFormat format) {    
    rga_info_t src, dst;
    memset(&src, 0, sizeof(rga_info_t));
    memset(&dst, 0, sizeof(rga_info_t));

    if (!input || !input->avframe) {std::cerr << "Ошибка: входной кадр NULL!" << std::endl;return 1;}
    
    //* Чистим avframe
    if (output->avframe != nullptr){
        av_freep(&output->avframe->data[0]);
        av_frame_free(&output->avframe);
        avcodec_free_context(&output->avcodeccontext);
        output->avframe = nullptr;
        output->avcodeccontext = nullptr;
    }

    output->avframe = av_frame_alloc();
    output->avframe->width = input->avframe->width;
    output->avframe->height = input->avframe->height;
    output->avframe->format = format;
    if (!output->avframe) {
        std::cerr << "Ошибка выделения памяти для avframe!" << std::endl;
        return 1;
    }

    int ret = av_image_alloc(output->avframe->data, output->avframe->linesize, 
                             output->avframe->width,  output->avframe->height, format, 16);
    
    if (ret < 0) {
        av_frame_free(&output->avframe);
        std::cerr << "Ошибка выделения памяти для выходного кадра! Код ошибки: " << ret << std::endl;
        return 1;
    }
    
    //* Чистим буферы
    buffer_free(input);
    buffer_free(output);
    
    //* Заполняем память
    convert_avframe_to_buffer(input);

    src.fd = -1;
    src.virAddr = input->buffer;
    src.mmuFlag = 1;
    src.format = AV_TO_RK_FORMAT(AVPixelFormat(input->avframe->format));
    rga_set_rect(&src.rect, x_start, y_start, x_stop, y_stop, 
                ALIGN_UP(input->avframe->width, 16), ALIGN_UP(input->avframe->height, 16), src.format);

    dst.fd = -1;
    dst.virAddr = output->buffer;
    dst.mmuFlag = 1;
    dst.format = AV_TO_RK_FORMAT(format);
    rga_set_rect(&dst.rect, 0, 0,  output->avframe->width,  output->avframe->height, 
                 output->avframe->width,  output->avframe->height, dst.format);

    int rga_status = c_RkRgaBlit(&src, &dst, NULL);

    if (rga_status != 0) {
        std::cerr << "Ошибка: RGA масштабирование не удалось! Код ошибки: " << rga_status << std::endl;
        av_free(input->buffer);
        av_free(output->buffer);
        av_frame_free(&output->avframe);
        return 1;
    }

    convert_buffer_to_avframe(output);

    // av_image_copy(output->avframe->data, output->avframe->linesize, 
    //               (const uint8_t**)&output->buffer, output->avframe->linesize, 
    //               AVPixelFormat(output->avframe->format), 
    //               output->avframe->width, output->avframe->height);
    
    return 0;
}
