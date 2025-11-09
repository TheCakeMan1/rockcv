#include "rkframe.h"

// rkcv_shot_t *nshot(int width, int height, rk_pix_fmt_t sw_fmt)
rkcv_shot_t *nshot(info_frame_t *frame_t)
{
    rkcv_shot_t *temp = calloc(1, sizeof(*temp));
    if (!temp)
        return NULL;

    temp->c = calloc(1, sizeof(s_convert_f));
    if (!temp->c)
    {
        free(temp);
        return NULL;
    }

    temp->c->frame_t = calloc(1, sizeof(info_frame_t));
    if (!temp->c->frame_t)
    {
        free(temp);
        return NULL;
    }

    temp->frame = av_frame_alloc();
    if (!temp->frame)
    {
        free(temp->c);
        free(temp);
        return NULL;
    }

    // --- создаём HW device и frame context ---
    AVBufferRef *hw_dev = NULL;
    if (av_hwdevice_ctx_create(&hw_dev, AV_HWDEVICE_TYPE_RKMPP, NULL, NULL, 0) < 0)
    {
        fprintf(stderr, "av_hwdevice_ctx_create(RKMPP) failed\n");
        goto fail;
    }

    AVBufferRef *hw_frames = av_hwframe_ctx_alloc(hw_dev);
    if (!hw_frames)
    {
        fprintf(stderr, "av_hwframe_ctx_alloc failed\n");
        goto fail_dev;
    }

    if (frame_t->width % 16)
        frame_t->width = (frame_t->width + 15) & ~15;
    if (frame_t->height % 2)
        frame_t->height &= ~1;

    AVHWFramesContext *fc = (AVHWFramesContext *)hw_frames->data;
    fc->format = AV_PIX_FMT_DRM_PRIME;                // аппаратный формат
    fc->sw_format = convert_pix_fmt(frame_t->fmt, 0); // программный (например, NV12 или RGB)
    fc->width = frame_t->width;
    fc->height = frame_t->height;
    fc->initial_pool_size = 2;

    if (av_hwframe_ctx_init(hw_frames) < 0)
    {
        fprintf(stderr, "av_hwframe_ctx_init failed\n");
        goto fail_frames;
    }

    // --- выделяем сам буфер ---
    if (av_hwframe_get_buffer(hw_frames, temp->frame, 0) < 0)
    {
        fprintf(stderr, "av_hwframe_get_buffer failed\n");
        goto fail_frames;
    }

    // теперь temp->frame уже содержит DRM fd
    // int fd = drmprime_fd_from_frame(temp->frame);
    // temp->c->s.fd = fd;
    // temp->c->s.mmuFlag = 1;
    temp->c->frame_t->fmt = frame_t->fmt; // пример для NV12
    temp->c->frame_t->width = frame_t->width;
    temp->c->frame_t->height = frame_t->height;

    temp->c->desc_in = (const AVDRMFrameDescriptor *)temp->frame->data[0];

    av_buffer_unref(&hw_frames);
    av_buffer_unref(&hw_dev);
    return temp;

fail_frames:
    av_buffer_unref(&hw_frames);
fail_dev:
    av_buffer_unref(&hw_dev);
fail:
    av_frame_free(&temp->frame);
    free(temp->c);
    free(temp);
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
