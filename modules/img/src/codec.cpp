#include "codec.h"

void convertAVFrameColor(AVFrame *srcFrame, AVFrame *dstFrame, AVPixelFormat dstFormat) {
    if (!srcFrame || !dstFrame) {
        std::cerr << "Ошибка: неверный AVFrame!" << std::endl;
        return;
    }

    SwsContext *swsCtx = sws_getContext(
        srcFrame->width, srcFrame->height, (AVPixelFormat)srcFrame->format,
        dstFrame->width, dstFrame->height, dstFormat,
        SWS_BILINEAR, NULL, NULL, NULL);

    if (!swsCtx) {
        std::cerr << "Ошибка: невозможно создать SwsContext!" << std::endl;
        return;
    }

    sws_scale(swsCtx, srcFrame->data, srcFrame->linesize, 0, srcFrame->height, dstFrame->data, dstFrame->linesize);

    sws_freeContext(swsCtx);
}