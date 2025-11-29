#include "rkframe.h"

__attribute__((deprecated("nshot: тестовая неотлаженная функция, использовать с осторожностью"))) rkcv_shot_t *nshot(const info_frame_t *in_frame)
{
    if (!in_frame)
        return NULL;

    info_frame_t frame = *in_frame;

    frame.width = (frame.width + 15) & ~15;
    frame.height = frame.height & ~1;

    rkcv_shot_t *temp = calloc(1, sizeof(*temp));
    if (unlikely(!temp))
        return NULL;

    temp->c = calloc(1, sizeof(*temp->c) + sizeof(info_frame_t));
    if (unlikely(!temp->c))
        goto cleanup;

    // Указатель на info_frame_t идёт сразу после структуры
    temp->c->frame_t = (info_frame_t *)(temp->c + 1);
    *temp->c->frame_t = frame;

    temp->frame = av_frame_alloc();
    if (unlikely(!temp->frame))
        goto cleanup;

    AVBufferRef *hw_dev = NULL;
    if (unlikely(av_hwdevice_ctx_create(&hw_dev, AV_HWDEVICE_TYPE_RKMPP, NULL, NULL, 0) < 0))
        goto cleanup;

    AVBufferRef *hw_frames = av_hwframe_ctx_alloc(hw_dev);
    if (unlikely(!hw_frames))
        goto cleanup;

    AVHWFramesContext *fc = (AVHWFramesContext *)hw_frames->data;
    fc->format = AV_PIX_FMT_DRM_PRIME;
    fc->sw_format = convert_pix_fmt(frame.fmt, 0);
    fc->width = frame.width;
    fc->height = frame.height;
    fc->initial_pool_size = 2;

    if (unlikely(av_hwframe_ctx_init(hw_frames) < 0))
        goto cleanup;

    if (unlikely(av_hwframe_get_buffer(hw_frames, temp->frame, 0) < 0))
        goto cleanup;

    temp->c->desc = (const AVDRMFrameDescriptor *)temp->frame->data[0];

#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
    // log_debug("Создался shot %dx%d fmt=%d", frame.width, frame.height, frame.fmt);
#endif

    av_buffer_unref(&hw_frames);
    av_buffer_unref(&hw_dev);
    return temp;

cleanup:
    if (temp)
    {
        av_frame_free(&temp->frame);
        if (temp->c)
            free(temp->c);
        free(temp);
    }
    if (hw_dev)
        av_buffer_unref(&hw_dev);
    return NULL;
}

void free_shot(rkcv_shot_t *shot)
{
    if (!shot)
        return;

    if (shot->frame)
        av_frame_free(&shot->frame);

    free(shot->c);
    // shot->opcl_ctx и shot->drm_hwdev освобождаются отдельно, если были созданы
    free(shot);
}
