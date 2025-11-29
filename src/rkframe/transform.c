#include "rkframe.h"

int imt(rkcv_shot_t *restrict ctx, rkcv_shot_t *restrict ctx_out)
{
    int fd_in = drmprime_fd_from_frame(ctx->frame);
    int fd_out = drmprime_fd_from_frame(ctx_out->frame);
    rga_info_t s = {0}, d = {0};

    s.fd = fd_in;
    d.fd = fd_out;
    s.mmuFlag = 1;
    d.mmuFlag = 1;

    int src_y_stride, src_hstride, dst_y_stride, dst_hstride;
    if (ctx->frame->format == AV_PIX_FMT_DRM_PRIME)
    {
        ctx->c->desc = (const AVDRMFrameDescriptor *)ctx->frame->data[0];
        ctx_out->c->desc = (const AVDRMFrameDescriptor *)ctx_out->frame->data[0];
        if (ctx->c->desc->layers[0].planes[1].pitch)
        {
            // YUV/NV12/NV21
            src_y_stride = ctx->c->desc->layers[0].planes[0].pitch;
            src_hstride = ctx->c->desc->layers[0].planes[1].offset / src_y_stride;
            dst_y_stride = ctx_out->c->desc->layers[0].planes[0].pitch;
            dst_hstride = ctx_out->c->desc->layers[0].planes[1].offset / dst_y_stride;
        }
        else
        {
            // RGB
            src_y_stride = ctx->c->desc->layers[0].planes[0].pitch;
            src_hstride = ctx->frame->height; // RGB одна плоскость
            dst_y_stride = ctx_out->c->desc->layers[0].planes[0].pitch;
            dst_hstride = ctx_out->frame->height;
        }
    }

    int fmt_local_in = convert_pix_fmt(ctx->c->frame_t->fmt, 1);

    rga_set_rect(&s.rect, 0, 0,
                 ctx->c->frame_t->width, ctx->c->frame_t->height,
                 src_y_stride, src_hstride, fmt_local_in);

    rga_set_rect(&d.rect, 0, 0,
                 ctx_out->c->frame_t->width, ctx_out->c->frame_t->height,
                 dst_y_stride, dst_hstride, fmt_local_in);

    int ret = RgaBlit(&s, &d, NULL);
    if (ret)
    {
        RKX_E_TAG("imt_f", "RGA blit failed: %d", ret);
        return -1;
    }

    return 0;
}
int ima(rkcv_shot_t *restrict ctx,
        rkcv_shot_t *restrict ctx_out)
{
    // /* --------------------------------------------------------------------
    //  * 1. FD вход / выход (DRM PRIME → dma-buf fd)
    //  * -------------------------------------------------------------------- */
    // int fd_in = drmprime_fd_from_frame(ctx->frame);
    // int fd_out = drmprime_fd_from_frame(ctx_out->frame);

    // /* если у тебя есть прототип run_gauss_proc() – ок,
    //    если нет, добавь extern перед использованием */
    // // run_gauss_proc();

    // /* --------------------------------------------------------------------
    //  * 2. Подготовка структур RGA
    //  * -------------------------------------------------------------------- */
    // rga_info_t s;
    // rga_info_t d;
    // memset(&s, 0, sizeof(s));
    // memset(&d, 0, sizeof(d));

    // s.fd = fd_in;
    // d.fd = fd_out;

    // s.mmuFlag = 1;
    // d.mmuFlag = 1;

    // int src_y_stride = 0;
    // int src_hstride = 0;
    // int dst_y_stride = 0;
    // int dst_hstride = 0;

    // /* --------------------------------------------------------------------
    //  * 3. DRM PRIME → stride / height_stride
    //  * -------------------------------------------------------------------- */
    // if (ctx->frame->format == AV_PIX_FMT_DRM_PRIME)
    // {
    //     ctx->c->desc = (const AVDRMFrameDescriptor *)ctx->frame->data[0];
    //     ctx_out->c->desc = (const AVDRMFrameDescriptor *)ctx_out->frame->data[0];

    //     const AVDRMFrameDescriptor *src_desc = ctx->c->desc;
    //     const AVDRMFrameDescriptor *dst_desc = ctx_out->c->desc;

    //     if (src_desc->layers[0].planes[1].pitch)
    //     {
    //         /* NV12 / NV21 */
    //         src_y_stride = src_desc->layers[0].planes[0].pitch;
    //         src_hstride = src_desc->layers[0].planes[1].offset / src_y_stride;

    //         dst_y_stride = dst_desc->layers[0].planes[0].pitch;
    //         dst_hstride = dst_desc->layers[0].planes[1].offset / dst_y_stride;
    //     }
    //     else
    //     {
    //         /* RGB */
    //         src_y_stride = src_desc->layers[0].planes[0].pitch;
    //         src_hstride = ctx->frame->height;

    //         dst_y_stride = dst_desc->layers[0].planes[0].pitch;
    //         dst_hstride = ctx_out->frame->height;
    //     }
    // }
    // else
    // {
    //     /* если сюда реально можешь попасть – надо аккуратно посчитать stride */
    //     src_y_stride = ctx->c->frame_t->width;
    //     src_hstride = ctx->c->frame_t->height;
    //     dst_y_stride = ctx_out->c->frame_t->width;
    //     dst_hstride = ctx_out->c->frame_t->height;
    // }

    // /* --------------------------------------------------------------------
    //  * 4. Формат основного кадра
    //  * -------------------------------------------------------------------- */
    // int fmt_main = convert_pix_fmt(ctx->c->frame_t->fmt, 1);

    // /* --------------------------------------------------------------------
    //  * 5. BLIT: копирование основного кадра IN → OUT
    //  * -------------------------------------------------------------------- */
    // rga_set_rect(&s.rect,
    //              0, 0,
    //              ctx->c->frame_t->width,
    //              ctx->c->frame_t->height,
    //              src_y_stride, src_hstride,
    //              fmt_main);

    // rga_set_rect(&d.rect,
    //              0, 0,
    //              ctx_out->c->frame_t->width,
    //              ctx_out->c->frame_t->height,
    //              dst_y_stride, dst_hstride,
    //              fmt_main);

    // int ret = RgaBlit(&s, &d, NULL);
    // if (ret)
    // {
    //     RKX_E_TAG("ima_f", "RGA copy failed: %d", ret);
    //     return -1;
    // }

    // /* --------------------------------------------------------------------
    //  * 6. Overlay (если нет overlay_fd → выходим)
    //  * ctx->overlay_fd / overlay_w / overlay_h / overlay_x / overlay_y
    //  * должны существовать в rkcv_shot_t.
    //  * -------------------------------------------------------------------- */
    // if (fd_in <= 0)
    //     return 0;

    // /* --------------------------------------------------------------------
    //  * 7. Overlay: RGBA8888 поверх выходного кадра
    //  * -------------------------------------------------------------------- */
    // rga_info_t so;
    // rga_info_t do_;
    // memset(&so, 0, sizeof(so));
    // memset(&do_, 0, sizeof(do_));

    // int fd_overlay = ctx->overlay_fd;
    // int ov_w = ctx->overlay_w;
    // int ov_h = ctx->overlay_h;
    // int pos_x = ctx->overlay_x;
    // int pos_y = ctx->overlay_y;

    // so.fd = fd_overlay;
    // so.mmuFlag = 1;
    // so.blend = 1; /* включаем смешивание (blend) */

    // /* формат overlay: RGBA8888 */
    // const int fmt_ov = RK_FORMAT_RGBA_8888;

    // /* stride для RGBA: ширина * 4 байта */
    // rga_set_rect(&so.rect,
    //              0, 0,
    //              ov_w, ov_h,
    //              ov_w * 4, ov_h,
    //              fmt_ov);

    // do_.fd = fd_out;
    // do_.mmuFlag = 1;

    // rga_set_rect(&do_.rect,
    //              pos_x, pos_y,
    //              ov_w, ov_h,
    //              dst_y_stride, dst_hstride,
    //              fmt_main);

    // /* osd_info НЕ трогаем – он у тебя не про alpha-блендинг */

    // int ret2 = RgaBlit(&so, &do_, NULL);
    // if (ret2)
    // {
    //     RKX_E_TAG("ima_f", "RGA overlay failed: %d", ret2);
    //     return -1;
    // }

    return 0;
}

// int ima(rkcv_shot_t *restrict ctx, rkcv_shot_t *restrict ctx_out, rkcv_shot_t *restrict ctx1)
// {
//     int fd_in = drmprime_fd_from_frame(ctx->frame);
//     int fd_in1 = drmprime_fd_from_frame(ctx1->frame);
//     int fd_out = drmprime_fd_from_frame(ctx_out->frame);
//     rga_info_t s = {0}, d = {0}, s1 = {0};

//     s.fd = fd_in;
//     s1.fd = fd_in1;
//     d.fd = fd_out;
//     s.mmuFlag = 1;
//     s1.mmuFlag = 1;
//     d.mmuFlag = 1;

//     int src_y_stride, src_hstride, dst_y_stride, dst_hstride, src_y_stride1, src_hstride1;
//     if (ctx->frame->format == AV_PIX_FMT_DRM_PRIME)
//     {
//         ctx->c->desc = (const AVDRMFrameDescriptor *)ctx->frame->data[0];
//         ctx1->c->desc = (const AVDRMFrameDescriptor *)ctx1->frame->data[0];
//         ctx_out->c->desc = (const AVDRMFrameDescriptor *)ctx_out->frame->data[0];
//         if (ctx->c->desc->layers[0].planes[1].pitch)
//         {
//             // YUV/NV12/NV21
//             src_y_stride = ctx->c->desc->layers[0].planes[0].pitch;
//             src_hstride = ctx->c->desc->layers[0].planes[1].offset / src_y_stride;
//             src_y_stride1 = ctx1->c->desc->layers[0].planes[0].pitch;
//             src_hstride1 = ctx1->c->desc->layers[0].planes[1].offset / src_y_stride1;
//             dst_y_stride = ctx_out->c->desc->layers[0].planes[0].pitch;
//             dst_hstride = ctx_out->c->desc->layers[0].planes[1].offset / dst_y_stride;
//         }
//         else
//         {
//             // RGB
//             src_y_stride = ctx->c->desc->layers[0].planes[0].pitch;
//             src_hstride = ctx->frame->height; // RGB одна плоскость
//             src_y_stride1 = ctx1->c->desc->layers[0].planes[0].pitch;
//             src_hstride1 = ctx1->frame->height;
//             dst_y_stride = ctx_out->c->desc->layers[0].planes[0].pitch;
//             dst_hstride = ctx_out->frame->height;
//         }
//     }

//     int fmt_local_in = convert_pix_fmt(ctx->c->frame_t->fmt, 1);

//     rga_set_rect(&s.rect, 0, 0,
//                  ctx->c->frame_t->width, ctx->c->frame_t->height,
//                  src_y_stride, src_hstride, fmt_local_in);

//     rga_set_rect(&s1.rect, 0, 0,
//                  ctx1->c->frame_t->width, ctx1->c->frame_t->height,
//                  src_y_stride1, src_hstride1, convert_pix_fmt(ctx1->c->frame_t->fmt, 1));

//     rga_set_rect(&d.rect, 0, 0,
//                  ctx_out->c->frame_t->width, ctx_out->c->frame_t->height,
//                  dst_y_stride, dst_hstride, fmt_local_in);

//     s.blend = 2; // src/dst alpha enable
//     // s1.alpha = 255;
//     s1.rop_code = 0x55;
//     // глобальная альфа
//     // s1.factor = 255;                        // 255 = полная непрозрачность

//     // Porter-Duff SRC_OVER (foreground поверх background)
//     // s1.rop_code = 0x55;

//     // важно: rop_mask_addr = 0 (если маска не используется)
//     // s1.rop_mask_addr = 0;

//     int ret = RgaBlit(&s, &d, &s1);
//     if (ret)
//     {
//         RKX_E_TAG("ima_f", "RGA blit failed: %d", ret);
//         return -1;
//     }

//     return 0;
// }

// int imt(rkcv_t *ctx, const s_convert_f c)
// {
//     AVFrame *src = ctx->frame;
//     if (!src)
//     {
//         log_warn("no input frame");
//         return -1;
//     }

//     if (src->format != AV_PIX_FMT_DRM_PRIME)
//     {
//         log_warn("expected DRM_PRIME input, got %d", src->format);
//         // fprintf(stderr, "imw: expected DRM_PRIME input, got %d\n", src->format);
//         return -1;
//     }

//     if (ctx->frame)
//     {
//         av_frame_free(&ctx->frame);
//     }

//     ctx->frame = av_frame_alloc();
//     if (!ctx->frame)
//     {
// #if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
//         log_error("error alloc frame");
// #endif
//         return -1;
//     }
//     ctx->frame->format = AV_PIX_FMT_DRM_PRIME;
//     ctx->frame->width = ctx->codec_ctx->width;
//     ctx->frame->height = ctx->codec_ctx->height;

//     if (av_hwframe_get_buffer(ctx->codec_ctx->hw_frames_ctx, ctx->frame, 0) < 0)
//     {
// #if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
//         log_warn("av_hwframe_get_buffer failed");
// #else
//         log_error("bad frame buffer");
// #endif
//         av_frame_free(&ctx->frame);
//         return -1;
//     }

//     int fd_in = drmprime_fd_from_frame(src);
//     int fd_out = drmprime_fd_from_frame(ctx->frame);
//     if (fd_in < 0 || fd_out < 0)
//     {
// #if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
//         log_error("bad drm fd");
// #else
//         log_error("bad frame buffer");
// #endif
//         // fprintf(stderr, "imw: bad drm fd\n");
//         av_frame_free(&ctx->frame);
//         return -1;
//     }

//     ctx->rect.s.fd = fd_in;
//     ctx->rect.s.mmuFlag = 1;
//     ctx->rect.d.fd = fd_out;
//     ctx->rect.d.mmuFlag = 1;
// }