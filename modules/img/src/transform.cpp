#include "transform.h"

AVFrame* convertToRGB24(AVFrame* frame, AVCodecContext* codecCtx) {
    if (!frame || !codecCtx) return nullptr;

    // Проверяем, уже ли кадр в `RGB24`
    if (frame->format == AV_PIX_FMT_RGB24) {
        return frame;  // Уже правильный формат, возвращаем его же
    }

    // Создаём `SwsContext` для конвертации
    SwsContext* sws_ctx = sws_getContext(
        codecCtx->width, codecCtx->height, (AVPixelFormat)frame->format,
        codecCtx->width, codecCtx->height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    if (!sws_ctx) {
        std::cerr << "Ошибка: не удалось создать SwsContext для конвертации!" << std::endl;
        return nullptr;
    }

    // Создаём выходной `AVFrame`
    AVFrame* rgbFrame = av_frame_alloc();
    rgbFrame->format = AV_PIX_FMT_RGB24;
    rgbFrame->width = frame->width;
    rgbFrame->height = frame->height;
    av_frame_get_buffer(rgbFrame, 32);

    // Выполняем конвертацию
    sws_scale(
        sws_ctx, frame->data, frame->linesize, 0, frame->height,
        rgbFrame->data, rgbFrame->linesize
    );

    sws_freeContext(sws_ctx);
    return rgbFrame;
}

















int8_t ffresize(const frame_t* input, frame_t* output, int new_width, int new_height) {
    if (!input || !input->avframe || !output) {
        std::cerr << "Ошибка: входной или выходной AVFrame пустой!" << std::endl;
        return 1;
    }

    // Создаём SwsContext для масштабирования
    SwsContext* sws_ctx = sws_getContext(
        input->avcodeccontext->width,  input->avcodeccontext->height,  input->avcodeccontext->pix_fmt, // Исходные параметры
        new_width, new_height, AV_PIX_FMT_BGR24, // Целевой формат (BGR для OpenCV)
        SWS_BICUBIC, nullptr, nullptr, nullptr
    );

    if (!sws_ctx) {
        std::cerr << "Ошибка: не удалось создать SwsContext!" << std::endl;
        return 1;
    }

    // Создаём выходной кадр
    output->avframe = av_frame_alloc();
    if (!output->avframe) {
        std::cerr << "Ошибка: не удалось выделить память для AVFrame!" << std::endl;
        sws_freeContext(sws_ctx);
        return 1;
    }

    // Устанавливаем параметры выходного кадра
    output->avframe->format = AV_PIX_FMT_BGR24;
    output->avframe->width = new_width;
    output->avframe->height = new_height;

    // Выделяем буфер для выходного кадра
    if (av_frame_get_buffer(output->avframe, 32) < 0) {
        std::cerr << "Ошибка: не удалось выделить буфер для AVFrame!" << std::endl;
        av_frame_free(&output->avframe);
        sws_freeContext(sws_ctx);
        return 1;
    }

    // Выполняем масштабирование
    sws_scale(
        sws_ctx,
        input->avframe->data, input->avframe->linesize, 0, input->avframe->height,
        output->avframe->data, output->avframe->linesize
    );

    // Освобождаем контекст SwsContext
    sws_freeContext(sws_ctx);
    return 0;
}

int8_t ffresize(const AVFrame* input, AVFrame* output, AVCodecContext* codecCtx, int new_width, int new_height) {
    if (!input || !codecCtx || !output) {
        std::cerr << "Ошибка: один из входных параметров `nullptr`!" << std::endl;
        return 1;
    }
    if (!output) {
        output = av_frame_alloc();
    }
    if (!output) {
        std::cerr << "Ошибка: не удалось выделить память для AVFrame!" << std::endl;
        return 1;
    }
    SwsContext* sws_ctx = sws_getContext(
        codecCtx->width, codecCtx->height, codecCtx->pix_fmt, 
        new_width, new_height, AV_PIX_FMT_BGR24,
        SWS_BICUBIC, nullptr, nullptr, nullptr
    );
    if (!sws_ctx) {
        std::cerr << "Ошибка: не удалось создать SwsContext!" << std::endl;
        return 1;
    }
    output->format = AV_PIX_FMT_BGR24;
    output->width = new_width;
    output->height = new_height;
    if (av_frame_get_buffer(output, 32) < 0) {
        std::cerr << "Ошибка: не удалось выделить буфер для AVFrame!" << std::endl;
        av_frame_free(&output);
        sws_freeContext(sws_ctx);
        return 1;
    }
    if (!input->data[0] || !output->data[0]) {
        std::cerr << "Ошибка: пустые буферы данных в AVFrame!" << std::endl;
        return 1;
    }
    sws_scale(
        sws_ctx,
        input->data, input->linesize, 0, input->height,
        output->data, output->linesize
    );
    sws_freeContext(sws_ctx);
    return 0;
}












int8_t resize(const AVFrame* input, AVFrame* output, AVCodecContext* codecCtx, int new_width, int new_height) {
    if (!input || !codecCtx || !output) {
        std::cerr << "Ошибка: один из входных параметров `nullptr`!" << std::endl;
        return 1;
    }

    // Если output не выделен, создаём новый AVFrame
    if (!output->data[0]) {
        output->format = AV_PIX_FMT_RGB24;  // RGA работает с RGB
        output->width = new_width;
        output->height = new_height;
        
        if (av_frame_get_buffer(output, 32) < 0) {
            std::cerr << "Ошибка: не удалось выделить буфер для AVFrame!" << std::endl;
            return 1;
        }
    }

    // Инициализируем RGA
    rga_info_t src, dst;
    memset(&src, 0, sizeof(rga_info_t));
    memset(&dst, 0, sizeof(rga_info_t));

    // Настраиваем исходный буфер
    src.virAddr = input->data[0];  // Исходный буфер пикселей
    src.mmuFlag = 1;  // Включаем поддержку виртуальной памяти
    src.format = RK_FORMAT_RGB_888;
    rga_set_rect(&src.rect, 0, 0, input->width, input->height, input->width, input->height, src.format);

    // Настраиваем выходной буфер
    dst.virAddr = output->data[0];  // Буфер для результата
    dst.mmuFlag = 1;
    dst.format = RK_FORMAT_RGB_888;
    rga_set_rect(&dst.rect, 0, 0, new_width, new_height, new_width, new_height, dst.format);

    // Вызываем RGA для масштабирования
    if (c_RkRgaBlit(&src, &dst, NULL) != 0) {
        std::cerr << "Ошибка: RGA не смог выполнить масштабирование!" << std::endl;
        return 1;
    }

    return 0;  // Успешное масштабирование
}

// int8_t resize(frame_t* input, frame_t* output, int new_width, int new_height) {
//     if (!input || !input->avframe || !input->avcodeccontext || !output) {
//         std::cerr << "Ошибка: входной или выходной AVFrame пустой!" << std::endl;
//         return 1;
//     }

//     // Проверяем корректность формата
//     printf("%d\n", input->avframe->format);
//     if (input->avframe->format != AV_PIX_FMT_RGB24) {
//         std::cerr << "Ошибка: AVFrame имеет неподдерживаемый формат!" << std::endl;
//         return 1;
//     }

//     // Выделяем `output->avframe`, если он ещё не создан
//     if (!output->avframe) {
//         output->avframe = av_frame_alloc();
//         if (!output->avframe) {
//             std::cerr << "Ошибка: не удалось выделить память для AVFrame!" << std::endl;
//             return 1;
//         }
//         output->avframe->format = AV_PIX_FMT_RGB24;
//         output->avframe->width = new_width;
//         output->avframe->height = new_height;

//         if (av_frame_get_buffer(output->avframe, 32) < 0) {
//             std::cerr << "Ошибка: не удалось выделить буфер для AVFrame!" << std::endl;
//             av_frame_free(&output->avframe);
//             return 1;
//         }
//     }

//     // Проверка размеров
//     if (input->avframe->width <= 0 || input->avframe->height <= 0) {
//         std::cerr << "Ошибка: некорректные размеры входного AVFrame!" << std::endl;
//         return 1;
//     }
//     if (output->avframe->width <= 0 || output->avframe->height <= 0) {
//         std::cerr << "Ошибка: некорректные размеры выходного AVFrame!" << std::endl;
//         return 1;
//     }

//     // Инициализируем RGA
//     rga_info_t src, dst;
//     memset(&src, 0, sizeof(rga_info_t));
//     memset(&dst, 0, sizeof(rga_info_t));

//     // Проверяем, что буферы данных не `NULL`
//     if (!input->avframe->data[0] || !output->avframe->data[0]) {
//         std::cerr << "Ошибка: пустые буферы данных в AVFrame!" << std::endl;
//         return 1;
//     }

//     // Настраиваем исходный буфер
//     src.virAddr = input->avframe->data[0]; 
//     src.mmuFlag = 1;
//     src.format = RK_FORMAT_RGB_888;  // Убедись, что входной формат совпадает с реальными данными
//     rga_set_rect(&src.rect, 0, 0, input->avframe->width, input->avframe->height,
//                  input->avframe->width, input->avframe->height, src.format);

//     // Настраиваем выходной буфер
//     dst.virAddr = output->avframe->data[0];
//     dst.mmuFlag = 1;
//     dst.format = RK_FORMAT_RGB_888;
//     rga_set_rect(&dst.rect, 0, 0, new_width, new_height, new_width, new_height, dst.format);

//     // Проверяем указатели перед вызовом RGA
//     if (!src.virAddr || !dst.virAddr) {
//         std::cerr << "Ошибка: `virAddr` (указатели на данные) пусты!" << std::endl;
//         return 1;
//     }

//     // Вызываем RGA для масштабирования
//     int ret = c_RkRgaBlit(&src, &dst, NULL);
//     if (ret != 0) {
//         std::cerr << "Ошибка: RGA не смог выполнить масштабирование! Код ошибки: " << ret << std::endl;
//         return 1;
//     }

//     return 0;
// }

// int8_t resize(frame_t* input, frame_t* output, int new_width, int new_height) {
//     if (!input || !input->avframe || !input->avcodeccontext || !output) {
//         std::cerr << "Ошибка: входной или выходной AVFrame пустой!" << std::endl;
//         return 1;
//     }
    
//     if (!output->avframe) {
//         output->avframe = av_frame_alloc();
//         if (!output->avframe) {
//             std::cerr << "Ошибка: не удалось выделить память для AVFrame!" << std::endl;
//             return 1;
//         }
//         output->avframe->format = AV_PIX_FMT_RGB24;
//         output->avframe->width = new_width;
//         output->avframe->height = new_height;
        
//         if (av_frame_get_buffer(output->avframe, 32) < 0) {
//             std::cerr << "Ошибка: не удалось выделить буфер для AVFrame!" << std::endl;
//             av_frame_free(&output->avframe);
//             return 1;
//         }
//     }

//     if (!input->avframe->data[0] || !output->avframe->data[0]) {
//         std::cerr << "Ошибка: пустые буферы данных в AVFrame!" << std::endl;
//         return 1;
//     }

//     int src_width = input->avframe->width;
//     int src_height = input->avframe->height;
//     int src_format = input->avframe->format;
    
//     int dst_width = new_width;
//     int dst_height = new_height;
//     int dst_format = RK_FORMAT_RGBA_8888;
    
//     int src_buf_size = src_width * src_height * get_bpp_from_format(src_format);
//     int dst_buf_size = dst_width * dst_height * get_bpp_from_format(dst_format);
    
//     char *src_buf = (char *)malloc(src_buf_size);
//     char *dst_buf = (char *)malloc(dst_buf_size);
//     if (!src_buf || !dst_buf) {
//         std::cerr << "Ошибка: не удалось выделить память для буферов!" << std::endl;
//         free(src_buf);
//         free(dst_buf);
//         return 1;
//     }

//     memcpy(src_buf, input->avframe->data[0], src_buf_size);
//     memset(dst_buf, 0x80, dst_buf_size);

//     rga_buffer_handle_t src_handle = importbuffer_virtualaddr(src_buf, src_buf_size);
//     rga_buffer_handle_t dst_handle = importbuffer_virtualaddr(dst_buf, dst_buf_size);
//     if (src_handle == 0 || dst_handle == 0) {
//         std::cerr << "Ошибка: importbuffer не удался!" << std::endl;
//         free(src_buf);
//         free(dst_buf);
//         return 1;
//     }

//     rga_buffer_t src_img = wrapbuffer_handle(src_handle, src_width, src_height, src_format);
//     rga_buffer_t dst_img = wrapbuffer_handle(dst_handle, dst_width, dst_height, dst_format);
    
//     int ret = imcheck(src_img, dst_img, {}, {});
//     if (IM_STATUS_NOERROR != ret) {
//         std::cerr << "Ошибка: " << imStrError((IM_STATUS)ret) << std::endl;
//         free(src_buf);
//         free(dst_buf);
//         return 1;
//     }

//     ret = imresize(src_img, dst_img);
//     if (ret != IM_STATUS_NOERROR) {
//         std::cerr << "Ошибка при изменении размера изображения!" << std::endl;
//         free(src_buf);
//         free(dst_buf);
//         return 1;
//     }

//     memcpy(output->avframe->data[0], dst_buf, dst_buf_size);
    
//     free(src_buf);
//     free(dst_buf);
//     return 0;
// }

// int8_t resize(frame_t *input, frame_t *output, int new_width, int new_height, AVPixelFormat format) {
//     output->avframe = av_frame_alloc();

//     if (!input || !output || !input->avframe || !output->avframe) {
//         std::cerr << "Ошибка: неверные указатели на кадры!" << std::endl;
//         return 1;
//     }

//     rga_info_t src, dst;
//     memset(&src, 0, sizeof(rga_info_t));
//     memset(&dst, 0, sizeof(rga_info_t));

//     // for(auto i: input->avframe->data){
//     //     printf("НАШЕЛ НОВЫЙ ДАТА %d\n", sizeof(i));
//     // }
//     //         printf("_______________________-\n");

//     src.virAddr = input->avframe->data[0];
//     src.fd = -1;
//     src.testLog = 1;
//     src.mmuFlag = 1;
//     src.scale_mode = 1;
//     // src.format = input->avframe->format;
//     // src.format = AV_TO_RK_FORMAT(input->avframe->format);
//     src.format = AV_TO_RK_FORMAT(input->avframe->format);
//     rga_set_rect(&src.rect, 0, 0, input->avframe->width, input->avframe->height, 
//                 input->avframe->width, input->avframe->height, src.format);

//     av_image_alloc(output->avframe->data, output->avframe->linesize, new_width, new_height, format, 32);
        

//     output->avframe->width = new_width;
//     output->avframe->height = new_height;
//     output->avframe->format = format;
//     av_image_fill_linesizes(output->avframe->linesize, format, new_width);

//     dst.fd = -1;
//     // dst.virAddr = output->avframe->data[0];
//     dst.virAddr = output->avframe->data[0];
//     dst.testLog = 1;
//     dst.mmuFlag = 1;
//     src.scale_mode = 1; // Линейное масштабирование
//     dst.format = RK_FORMAT_BGR_888 ;
//     dst.sync_mode = RGA_BLIT_SYNC; 
//     // dst.format = RK_FORMAT_BGR_888;
//     rga_set_rect(&dst.rect, 0, 0, new_width, new_height, 
//                 new_width, new_height, dst.format);

//     // Масштабирование через RGA
//     if (c_RkRgaBlit(&src, &dst, NULL) != 0) {
//         std::cerr << "Ошибка: RGA масштабирование не удалось!" << std::endl;
//         return 1;
//     }

//     return 0;
// }

int8_t resize(frame_t *input, frame_t *output, int new_width, int new_height, AVPixelFormat format) {
    if (!input || !output || !input->avframe) {
        std::cerr << "Ошибка: неверные указатели на кадры!" << std::endl;
        return 1;
    }

    output->avframe = av_frame_alloc();
    output->avframe->width = new_width;
    output->avframe->height = new_height;
    output->avframe->format = format;
    
    int ret = av_image_alloc(output->avframe->data, output->avframe->linesize, 
                             new_width, new_height, format, 16);
    if (ret < 0) {
        std::cerr << "Ошибка выделения памяти для выходного кадра" << std::endl;
        return 1;
    }

    convert_avframe_to_buffer(input);

    uint8_t* output_buffer = (uint8_t*)av_malloc(av_image_get_buffer_size(
        AVPixelFormat(output->avframe->format), new_width, new_height, 1));

    if (!output_buffer) {  // Тут было ошибочное повторное условие if (!input_buffer)
        std::cerr << "Ошибка выделения памяти!" << std::endl;
        av_free(input->buffer);
        return 1;
    }

    // Подготовка RGA buffer handles
    rga_buffer_handle_t src_handle = importbuffer_virtualaddr(
        input->buffer, 
        av_image_get_buffer_size(AVPixelFormat(input->avframe->format), 
                                 input->avframe->width, 
                                 input->avframe->height, 1)
    );
    
    rga_buffer_handle_t dst_handle = importbuffer_virtualaddr(
        output_buffer,
        av_image_get_buffer_size(format, new_width, new_height, 1)
    );

    if (!src_handle || !dst_handle) {
        std::cerr << "Ошибка создания RGA buffer handles!" << std::endl;
        ret = 1;
        if (src_handle) releasebuffer_handle(src_handle);
        if (dst_handle) releasebuffer_handle(dst_handle);
        av_free(input->buffer);
        av_free(output_buffer);
        return ret;
    }

    // Создание RGA буферов
    rga_buffer_t src_img = wrapbuffer_handle(
        src_handle,
        input->avframe->width,
        input->avframe->height,
        AV_TO_RK_FORMAT(input->avframe->format)
    );
    
    rga_buffer_t dst_img = wrapbuffer_handle(
        dst_handle,
        new_width,
        new_height,
        AV_TO_RK_FORMAT(format)
    );

    // Проверка параметров
    ret = imcheck(src_img, dst_img, {}, {});
    if (ret != IM_STATUS_NOERROR) {
        std::cerr << "Ошибка проверки параметров: " << imStrError((IM_STATUS)ret) << std::endl;
        ret = 1;
    } else {
        // Выполнение масштабирования
        ret = imresize(src_img, dst_img);
        if (ret != IM_STATUS_SUCCESS) {
            std::cerr << "Ошибка масштабирования: " << imStrError((IM_STATUS)ret) << std::endl;
            ret = 1;
        } else {
            // Копируем данные обратно в AVFrame
            av_image_copy(output->avframe->data, output->avframe->linesize,
                          (const uint8_t**)&output_buffer,
                          output->avframe->linesize,
                          (AVPixelFormat)output->avframe->format,
                          output->avframe->width, output->avframe->height);
        }
    }

    // Освобождение ресурсов
    if (src_handle) releasebuffer_handle(src_handle);
    if (dst_handle) releasebuffer_handle(dst_handle);
    av_free(input->buffer);
    av_free(output_buffer);

    return ret;
}























int8_t resize(const cv::Mat &input, cv::Mat &output, int new_width, int new_height) {
    rga_info_t src, dst;
    memset(&src, 0, sizeof(rga_info_t));
    memset(&dst, 0, sizeof(rga_info_t));

    // Исходное изображение
    src.fd = -1;
    src.virAddr = input.data;
    src.mmuFlag = 1;
    src.format = RK_FORMAT_RGB_888; // Указываем формат RGB888
    rga_set_rect(&src.rect, 0, 0, input.cols, input.rows, input.cols, input.rows, src.format);

    // Выходное изображение
    output = cv::Mat(new_height, new_width, CV_8UC3); // RGB 8 бит на канал
    dst.fd = -1;
    dst.virAddr = output.data;
    dst.mmuFlag = 1;
    dst.format = RK_FORMAT_RGB_888;
    rga_set_rect(&dst.rect, 0, 0, new_width, new_height, new_width, new_height, dst.format);

    // Вызываем RGA для масштабирования
    if (c_RkRgaBlit(&src, &dst, NULL) != 0) {
        std::cerr << "Ошибка: RGA масштабирование не удалось!" << std::endl;
        return 1;
    }
    return 0;
}

int8_t rotate(const cv::Mat& input, cv::Mat& output, int angle) {
    if (input.empty()) {
        std::cerr << "Ошибка: входное изображение пустое!" << std::endl;
        return 1;
    }

    int new_width = (angle == 90 || angle == 270) ? input.rows : input.cols;
    int new_height = (angle == 90 || angle == 270) ? input.cols : input.rows;

    output = cv::Mat(new_height, new_width, input.type());

    rga_info_t src, dst;
    memset(&src, 0, sizeof(rga_info_t));
    memset(&dst, 0, sizeof(rga_info_t));

    src.virAddr = input.data;
    src.mmuFlag = 1;
    src.format = RK_FORMAT_RGB_888;
    rga_set_rect(&src.rect, 0, 0, input.cols, input.rows, input.cols, input.rows, src.format);

    dst.virAddr = output.data;
    dst.mmuFlag = 1; 
    dst.format = RK_FORMAT_RGB_888;
    rga_set_rect(&dst.rect, 0, 0, new_width, new_height, new_width, new_height, dst.format);

    if (angle == 90) src.rotation = HAL_TRANSFORM_ROT_90;
    else if (angle == 180) src.rotation = HAL_TRANSFORM_ROT_180;
    else if (angle == 270) src.rotation = HAL_TRANSFORM_ROT_270;
    else {
        std::cerr << "Ошибка: неподдерживаемый угол поворота!" << std::endl;
        return 1;
    }

    if (c_RkRgaBlit(&src, &dst, NULL) != 0) {
        std::cerr << "Ошибка: RGA не смог выполнить поворот!" << std::endl;
        return 1;
    }

    return 0;
}

int8_t mirror(const cv::Mat& input, cv::Mat& output, int type) {
    if (input.empty()) {
        std::cerr << "Ошибка: входное изображение пустое!" << std::endl;
        return 1;
    }

    int new_width = input.rows;
    int new_height = input.cols;

    output = cv::Mat(new_height, new_width, input.type());

    rga_info_t src, dst;
    memset(&src, 0, sizeof(rga_info_t));
    memset(&dst, 0, sizeof(rga_info_t));

    src.virAddr = input.data;
    src.mmuFlag = 1;
    src.format = RK_FORMAT_RGB_888;
    rga_set_rect(&src.rect, 0, 0, input.cols, input.rows, input.cols, input.rows, src.format);

    dst.virAddr = output.data;
    dst.mmuFlag = 1; 
    dst.format = RK_FORMAT_RGB_888;
    rga_set_rect(&dst.rect, 0, 0, new_width, new_height, new_width, new_height, dst.format);

    if (type == 1) src.rotation = HAL_TRANSFORM_FLIP_H;
    else if (type == 2) src.rotation = HAL_TRANSFORM_FLIP_V;
    else if (type == 3) src.rotation = HAL_TRANSFORM_FLIP_H_V;
    else {
        std::cerr << "Ошибка: неподдерживаемый угол поворота!" << std::endl;
        return 1;
    }

    if (c_RkRgaBlit(&src, &dst, NULL) != 0) {
        std::cerr << "Ошибка: RGA не смог выполнить поворот!" << std::endl;
        return 1;
    }

    return 0;
}