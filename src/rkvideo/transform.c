#include "rkvideo.h"

int imt(rkcv_t *ctx, const s_convert_f c)
{
    AVFrame *src = ctx->frame;
    if (!src)
    {
        log_warn("no input frame");
        return -1;
    }

    if (src->format != AV_PIX_FMT_DRM_PRIME)
    {
        log_warn("expected DRM_PRIME input, got %d", src->format);
        // fprintf(stderr, "imw: expected DRM_PRIME input, got %d\n", src->format);
        return -1;
    }

    if (ctx->frame)
    {
        av_frame_free(&ctx->frame);
    }

    ctx->frame = av_frame_alloc();
    if (!ctx->frame)
    {
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
        log_error("error alloc frame");
#endif
        return -1;
    }
    ctx->frame->format = AV_PIX_FMT_DRM_PRIME;
    ctx->frame->width = ctx->codec_ctx->width;
    ctx->frame->height = ctx->codec_ctx->height;

    if (av_hwframe_get_buffer(ctx->codec_ctx->hw_frames_ctx, ctx->frame, 0) < 0)
    {
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
        log_warn("av_hwframe_get_buffer failed");
#else
        log_error("bad frame buffer");
#endif
        av_frame_free(&ctx->frame);
        return -1;
    }

    int fd_in = drmprime_fd_from_frame(src);
    int fd_out = drmprime_fd_from_frame(ctx->frame);
    if (fd_in < 0 || fd_out < 0)
    {
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
        log_error("bad drm fd");
#else
        log_error("bad frame buffer");
#endif
        // fprintf(stderr, "imw: bad drm fd\n");
        av_frame_free(&ctx->frame);
        return -1;
    }

    ctx->rect.s.fd = fd_in;
    ctx->rect.s.mmuFlag = 1;
    ctx->rect.d.fd = fd_out;
    ctx->rect.d.mmuFlag = 1;
}