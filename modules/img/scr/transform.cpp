#include "transform.h"

int8_t resize(frame_t* input, frame_t* output, int new_width, int new_height) {
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

int8_t resize(AVFrame* input, AVFrame* output, AVCodecContext* codecCtx, int new_width, int new_height) {
    if (!input || !codecCtx || !output) {
        std::cerr << "Ошибка: один из входных параметров `nullptr`!" << std::endl;
        return 1;
    }

    // Проверка, инициализирован ли AVFrame
    if (!output) {
        output = av_frame_alloc();
    }
    if (!output) {
        std::cerr << "Ошибка: не удалось выделить память для AVFrame!" << std::endl;
        return 1;
    }

    // Создаём SwsContext для масштабирования
    SwsContext* sws_ctx = sws_getContext(
        codecCtx->width, codecCtx->height, codecCtx->pix_fmt, // Исходные параметры
        new_width, new_height, AV_PIX_FMT_BGR24, // Целевой формат (BGR для OpenCV)
        SWS_BICUBIC, nullptr, nullptr, nullptr
    );

    if (!sws_ctx) {
        std::cerr << "Ошибка: не удалось создать SwsContext!" << std::endl;
        return 1;
    }

    // Настраиваем выходной кадр
    output->format = AV_PIX_FMT_BGR24;
    output->width = new_width;
    output->height = new_height;

    // Выделяем буфер для выходного кадра
    if (av_frame_get_buffer(output, 32) < 0) {
        std::cerr << "Ошибка: не удалось выделить буфер для AVFrame!" << std::endl;
        av_frame_free(&output);
        sws_freeContext(sws_ctx);
        return 1;
    }

    // Проверяем, что буферы данных не `nullptr`
    if (!input->data[0] || !output->data[0]) {
        std::cerr << "Ошибка: пустые буферы данных в AVFrame!" << std::endl;
        return 1;
    }

    // Выполняем масштабирование
    sws_scale(
        sws_ctx,
        input->data, input->linesize, 0, input->height,
        output->data, output->linesize
    );

    // Освобождаем контекст SwsContext
    sws_freeContext(sws_ctx);
    return 0;
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