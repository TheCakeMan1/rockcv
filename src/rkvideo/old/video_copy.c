#include "rkvideo.h"

static inline bool is_rtsp_source(const char *source)
{
    if (unlikely(!source))
        return false;

    return (strncmp(source, "rtsp://", 7) == 0) ||
           (strncmp(source, "rtsps://", 8) == 0);
}

rkcv_t *openv(char *source)
{
    rkcv_t *ctx = calloc(1, sizeof(rkcv_t));
    if (unlikely(!ctx))
    {
        log_fatal("Ошибка: не удалось выделить память под rkcv_t");
        return NULL;
    }
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
    else
    {
        log_debug("Выделена память под rkcv_t");
    }
#endif
    ctx->check_rtsp = is_rtsp_source(source);
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
    if (likely(ctx->check_rtsp))
    {
        log_debug("Обнаружен как rtsp поток");
    }
    else
    {
        log_debug("Обнаружен как video поток");
    }
#endif

    ctx->source = strdup(source);
    if (unlikely(!ctx->source))
    {
        log_fatal("Ошибка: strdup(source)");
        free(ctx);
        return NULL;
    }
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
    else
    {
        log_debug("strdup удачно");
    }
#endif

    if (ctx->check_rtsp)
    {
        avformat_network_init();

        ctx->opts = NULL;
        av_dict_set(&ctx->opts, "rtsp_transport", "tcp", 0);
        av_dict_set(&ctx->opts, "stimeout", "5000000", 0); // 5 сек таймаут

        if (avformat_open_input(&ctx->format_ctx, source, NULL, &ctx->opts) < 0)
        {
            log_fatal("Не удалось открыть входной файл: %s", source);
            free(ctx->source);
            free(ctx);
            return NULL;
        }
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
        else
        {
            log_debug("Входной файл открыт");
            av_dict_free(&ctx->opts);
        }
#endif
    }
    else
    {
        if (unlikely(avformat_open_input(&ctx->format_ctx, source, NULL, NULL) < 0))
        {
            log_fatal("Не удалось открыть входной файл: %s", source);
            free(ctx->source);
            free(ctx);
            return NULL;
        }
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
        else
        {
            log_debug("Входной файл открыт");
        }
#endif
    }

    ctx->format_ctx->flags |= AVFMT_FLAG_GENPTS;

    if (unlikely(avformat_find_stream_info(ctx->format_ctx, NULL) < 0))
    {
        log_fatal("Не удалось найти информацию о потоках.");
        // fprintf(stderr, "Не удалось найти информацию о потоках.\n");
        avformat_close_input(&ctx->format_ctx);
        free(ctx->source);
        free(ctx);
        return NULL;
    }
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
    else
    {
        log_debug("Найдена информация о потоках");
    }
#endif

    // Поиск видеопотока
    ctx->video_stream_index = -1;
    for (unsigned i = 0; i < ctx->format_ctx->nb_streams; i++)
    {
        if (ctx->format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            ctx->video_stream_index = (uint8_t)i;
            break;
        }
    }

    if (unlikely(ctx->video_stream_index == -1))
    {
        log_fatal("Входной файл не содержит видеопоток.");
        // fprintf(stderr, "Входной файл не содержит видеопоток.\n");
        avformat_close_input(&ctx->format_ctx);
        free(ctx->source);
        free(ctx);
        return NULL;
    }
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
    else
    {
        log_debug("Найдена информация о видеопоток");
    }
#endif

    ctx->stream = ctx->format_ctx->streams[ctx->video_stream_index];

#ifdef RKMPP_ENABLE
    if (ctx->stream->codecpar->codec_id == AV_CODEC_ID_H264)
    {
        ctx->codec = avcodec_find_decoder_by_name("h264_rkmpp");
#pragma message "видео кодек h264 кодек h264 заменен на h264_rkmpp"
    }
    else if (ctx->stream->codecpar->codec_id == AV_CODEC_ID_HEVC)
    {
        ctx->codec = avcodec_find_decoder_by_name("hevc_rkmpp");
#pragma message "видео кодек hevc заменен на hevc_rkmpp"
    }
    else if (ctx->stream->codecpar->codec_id == AV_CODEC_ID_H263)
    {
        ctx->codec = avcodec_find_decoder_by_name("h263_rkmpp");
#pragma message "видео кодек hevc заменен на h263_rkmpp"
    }
    else if (ctx->stream->codecpar->codec_id == AV_CODEC_ID_AV1)
    {
        ctx->codec = avcodec_find_decoder_by_name("av1_rkmpp");
#pragma message "видео кодек hevc заменен на h263_rkmpp"
    }
    else
    {
        ctx->codec = avcodec_find_decoder(ctx->stream->codecpar->codec_id);
    }
#endif
    if (unlikely(!ctx->codec))
    {
        log_fatal("Не удалось найти декодер.");
        // fprintf(stderr, "Не удалось найти декодер.\n");
        avformat_close_input(&ctx->format_ctx);
        free(ctx->source);
        free(ctx);
        return NULL;
    }
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
    else
    {
        log_debug("Удалось найти декодер");
    }
#endif

    ctx->codec_ctx = avcodec_alloc_context3(ctx->codec);
    if (unlikely(!ctx->codec_ctx))
    {
        log_fatal("Ошибка выделения AVCodecContext.");
        // fprintf(stderr, "Ошибка выделения AVCodecContext.\n");
        avformat_close_input(&ctx->format_ctx);
        free(ctx->source);
        free(ctx);
        return NULL;
    }
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
    else
    {
        log_debug("Выделился AVCodecContext.");
    }
#endif

    ctx->codec_ctx->pix_fmt = AV_PIX_FMT_DRM_PRIME;

    if (unlikely(avcodec_parameters_to_context(ctx->codec_ctx, ctx->stream->codecpar) < 0))
    {
        log_fatal("Не удалось сконфигурировать контекст декодера.");
        // fprintf(stderr, "Не удалось сконфигурировать контекст декодера.\n");
        avcodec_free_context(&ctx->codec_ctx);
        avformat_close_input(&ctx->format_ctx);
        free(ctx->source);
        free(ctx);
        return NULL;
    }
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
    else
    {
        log_debug("Удалось сконфигурировать контекст декодера.");
    }
#endif

    AVBufferRef *hw_device_ctx = NULL;
    if (av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_RKMPP, NULL, NULL, 0) < 0)
    {
        fprintf(stderr, "Не удалось создать RKMpp hwdevice\n");
        return NULL;
    }
    ctx->codec_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

    if (unlikely(avcodec_open2(ctx->codec_ctx, ctx->codec, NULL) < 0))
    {
        log_fatal("Не удалось открыть декодер.");
        // fprintf(stderr, "Не удалось открыть декодер.\n");
        avcodec_free_context(&ctx->codec_ctx);
        avformat_close_input(&ctx->format_ctx);
        free(ctx->source);
        free(ctx);
        return NULL;
    }
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
    else
    {
        log_debug("Удалось открыть декодер.");
    }
#endif
    ctx->shot = calloc(1, sizeof(*ctx->shot));

    ctx->shot->frame = av_frame_alloc();
    ctx->packet = av_packet_alloc();
    log_debug("Все успешно.");

    return ctx;
}

int readf(rkcv_t *ctx)
{
    while (avcodec_receive_frame(ctx->codec_ctx, ctx->shot->frame) >= 0)
    {
        return 1;
    }

    while (av_read_frame(ctx->format_ctx, ctx->packet) >= 0)
    {
        if (ctx->packet->stream_index == ctx->video_stream_index)
        {
            if (avcodec_send_packet(ctx->codec_ctx, ctx->packet) < 0)
            {
                log_error("Ошибка отправки пакета в декодер.");
                break;
            }

            while (avcodec_receive_frame(ctx->codec_ctx, ctx->shot->frame) >= 0)
            {
                return 1;
            }
        }
    }

    return 1;
}

rkcv_t *video(rkcv_t *ctx_in,
              const resolution_t *size,
              const char *filename,
              int codec,
              int width,
              int height,
              int pix_fmt)
{
#ifdef BUILD_DEV
    START_VIDEO = 1;
#endif

    rkcv_t *ctx = aligned_alloc(64, sizeof(*ctx));
    if (unlikely(!ctx))
    {
        log_fatal("calloc rkcv_t");
        return NULL;
    }
    ctx->shot = calloc(1, sizeof(*ctx->shot));

    ctx->shot->c = malloc(sizeof(*ctx->shot->c));
    if (!ctx->shot->c)
    {
        perror("malloc");
    }
    memset(ctx->shot->c, 0, sizeof(*ctx->shot->c));
    memset(&ctx->shot->c->s, 0, sizeof(ctx->shot->c->s));
    memset(&ctx->shot->c->d, 0, sizeof(ctx->shot->c->d));

    if (unlikely(avformat_alloc_output_context2(&ctx->format_ctx, NULL, NULL, filename) < 0))
    {
        log_fatal("avformat_alloc_output_context2");
        free(ctx);
        return NULL;
    }

    switch (codec)
    {
    case RKCodec_H264:
        ctx->codec = avcodec_find_encoder_by_name("h264_rkmpp");
        break;
    case RKCodec_HEVC:
        ctx->codec = avcodec_find_encoder_by_name("hevc_rkmpp");
        break;
    default:
        ctx->codec = avcodec_find_encoder(codec);
        break;
    }
    if (unlikely(!ctx->codec))
    {
        log_fatal("encoder not found");
        goto fail_fmt;
    }

    ctx->stream = avformat_new_stream(ctx->format_ctx, NULL);
    ctx->codec_ctx = avcodec_alloc_context3(ctx->codec);
    if (unlikely(!ctx->stream || !ctx->codec_ctx))
    {
        log_fatal("new_stream/alloc_context");
        goto fail_fmt;
    }

    // --- размеры кадра ---
    if (size)
    {
        ctx->codec_ctx->width = ctx->shot->c->width = size->width;
        ctx->codec_ctx->height = ctx->shot->c->height = size->height;
    }
    else
    {
        ctx->codec_ctx->width = ctx->shot->c->width = width ? width : ctx_in->codec_ctx->width;
        ctx->codec_ctx->height = ctx->shot->c->height = height ? height : ctx_in->codec_ctx->height;
    }

    // Выравнивание под железо (полезно для rkmpp/rga)
    if (ctx->codec_ctx->width % 16)
        ctx->codec_ctx->width = (ctx->codec_ctx->width + 15) & ~15;
    if (ctx->codec_ctx->height % 2)
        ctx->codec_ctx->height &= ~1;

    // Тайминги
    // ctx->codec_ctx->time_base = (AVRational){1, 25};
    // ctx->stream->time_base = ctx->codec_ctx->time_base;
    // ctx->codec_ctx->framerate = ctx_in->stream->r_frame_rate.num ? ctx_in->stream->r_frame_rate : (AVRational){25, 1};
    // --- Определяем FPS ---

    AVRational fps_r = av_guess_frame_rate(ctx_in->format_ctx, ctx_in->stream, NULL);
    if (fps_r.num == 0 || fps_r.den == 0)
    {
        fps_r = ctx_in->stream->avg_frame_rate.num ? ctx_in->stream->avg_frame_rate
                                                   : (AVRational){25, 1};
    }

    // --- Устанавливаем time_base и fps ---
    // ctx->codec_ctx->framerate = av_d2q(fps, 1000);
    // ctx->codec_ctx->time_base = av_inv_q(ctx->codec_ctx->framerate);
    // ctx->stream->time_base = ctx->codec_ctx->time_base;
    // ctx->fps = fps; // сохранить в структуру для расчёта PTS
    ctx->codec_ctx->framerate = fps_r;
    ctx->codec_ctx->time_base = av_inv_q(fps_r);
    ctx->codec_ctx->pkt_timebase = ctx->codec_ctx->time_base; // ВАЖНО!
    ctx->stream->time_base = ctx->codec_ctx->time_base;
    ctx->stream->avg_frame_rate = fps_r; // полезно для MP4/MKV
    ctx->fps = av_q2d(fps_r);

    // --- ВАЖНО: энкодер ждёт DRM_PRIME ---
    ctx->codec_ctx->pix_fmt = AV_PIX_FMT_DRM_PRIME;

    // --- устройство и пул аппаратных кадров (hw_frames_ctx) ---
    AVBufferRef *hw_dev = NULL;
    if (unlikely(av_hwdevice_ctx_create(&hw_dev, AV_HWDEVICE_TYPE_RKMPP, NULL, NULL, 0) < 0))
    {
        log_fatal("av_hwdevice_ctx_create(RKMPP)");
        goto fail_ctxs;
    }
    ctx->codec_ctx->hw_device_ctx = av_buffer_ref(hw_dev);

    AVBufferRef *hw_frames = av_hwframe_ctx_alloc(ctx->codec_ctx->hw_device_ctx);
    if (unlikely(!hw_frames))
    {
        log_fatal("av_hwframe_ctx_alloc");
        goto fail_ctxs;
    }

    AVHWFramesContext *fc = (AVHWFramesContext *)hw_frames->data;
    fc->format = AV_PIX_FMT_DRM_PRIME;
    // fc->sw_format = AV_PIX_FMT_NV12;
    fc->sw_format = pix_fmt;
    fc->width = ctx->codec_ctx->width;
    fc->height = ctx->codec_ctx->height;
    fc->initial_pool_size = 8;

    if (unlikely(av_hwframe_ctx_init(hw_frames) < 0))
    {
        log_fatal("av_hwframe_ctx_init");
        goto fail_frames;
    }
    ctx->codec_ctx->hw_frames_ctx = av_buffer_ref(hw_frames);

    if (unlikely(avcodec_open2(ctx->codec_ctx, ctx->codec, NULL) < 0))
    {
        log_fatal("avcodec_open2");
        goto fail_frames;
    }

    /* ---------------------- */
    /* Оптимизация задержки и копирования */
    ctx->format_ctx->flags |= AVFMT_FLAG_NOBUFFER;
    ctx->codec_ctx->flags |= AV_CODEC_FLAG_LOW_DELAY;
    /* ---------------------- */

    if (unlikely(avcodec_parameters_from_context(ctx->stream->codecpar, ctx->codec_ctx) < 0))
    {
        log_fatal("avcodec_parameters_from_context");
        goto fail_opened;
    }

    // Открываем файл
    if (unlikely(!(ctx->format_ctx->oformat->flags & AVFMT_NOFILE)))
    {
        if (avio_open(&ctx->format_ctx->pb, filename, AVIO_FLAG_WRITE) < 0)
        {
            log_fatal("avio_open");
            goto fail_opened;
        }
    }

    if (unlikely(avformat_write_header(ctx->format_ctx, NULL) < 0))
    {
        log_fatal("avformat_write_header");
        if (ctx->format_ctx->pb)
            avio_closep(&ctx->format_ctx->pb);
        goto fail_opened;
    }

    ctx->frame_count = 0; // TODO надо рассмотреть необходимость этой переменной

    log_debug("Создан выходной поток для файла: %s", filename);
    av_buffer_unref(&hw_frames);
    av_buffer_unref(&hw_dev);

    ctx->shot->frame = av_frame_alloc();
    if (!ctx->shot->frame)
    {
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
        log_error("error alloc frame");
#endif
        return NULL;
    }
    ctx->shot->frame->format = AV_PIX_FMT_DRM_PRIME; // TODO он тут не всегда DRM, надо поправить
    ctx->shot->frame->width = ctx->codec_ctx->width;
    ctx->shot->frame->height = ctx->codec_ctx->height;
    return ctx;

fail_opened:
    avcodec_close(ctx->codec_ctx);
fail_frames:
    if (hw_frames)
        av_buffer_unref(&hw_frames);
fail_ctxs:
    if (hw_dev)
        av_buffer_unref(&hw_dev);
fail_fmt:
    if (ctx->codec_ctx)
        avcodec_free_context(&ctx->codec_ctx);
    if (ctx->format_ctx)
        avformat_free_context(ctx->format_ctx);
    free(ctx);
    return NULL;
}

__attribute__((hot, flatten)) int imw(rkcv_t *restrict ctx, rkcv_t *restrict ctx_in)
{
    // AVFrame *src = ctx_in->shot->frame;
    if (!ctx_in->shot->frame)
    {
        log_warn("no input frame");
        // fprintf(stderr, "imw: no input frame\n");
        return -1;
    }

    if (ctx_in->shot->frame->format != AV_PIX_FMT_DRM_PRIME)
    {
        log_warn("expected DRM_PRIME input, got %d", ctx_in->shot->frame->format);
        // fprintf(stderr, "imw: expected DRM_PRIME input, got %d\n", src->format);
        return -1;
    }

    if (!ctx->shot->frame)
    {
        ctx->shot->frame = av_frame_alloc();
        if (!ctx->shot->frame)
        {
            log_error("alloc frame failed");
            return -1;
        }
        ctx->shot->frame->format = AV_PIX_FMT_DRM_PRIME;
        ctx->shot->frame->width = ctx->codec_ctx->width;
        ctx->shot->frame->height = ctx->codec_ctx->height;
    }
    else
    {
        // av_frame_unref(ctx->shot->frame);
        ctx->shot->frame->pts = AV_NOPTS_VALUE;
        ctx->shot->frame->pkt_dts = AV_NOPTS_VALUE;
        ctx->shot->frame->best_effort_timestamp = AV_NOPTS_VALUE;
        ctx->shot->frame->duration = 0;
        ctx->shot->frame->opaque = NULL;
    }

    if (av_hwframe_get_buffer(ctx->codec_ctx->hw_frames_ctx, ctx->shot->frame, 0) < 0)
    {
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
        log_warn("av_hwframe_get_buffer failed");
#else
        log_error("bad frame buffer");
#endif
        av_frame_free(&ctx->shot->frame);
        return -1;
    }

    int fd_in = drmprime_fd_from_frame(ctx_in->shot->frame);
    int fd_out = drmprime_fd_from_frame(ctx->shot->frame);
    if (fd_in < 0 || fd_out < 0)
    {
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
        log_error("bad drm fd");
#else
        log_error("bad frame buffer");
#endif
        av_frame_free(&ctx->shot->frame);
        return -1;
    }

    ctx->shot->c->s.fd = fd_in;
    ctx->shot->c->s.mmuFlag = 1;
    ctx->shot->c->d.fd = fd_out;
    ctx->shot->c->d.mmuFlag = 1;

    int src_y_stride, src_hstride, dst_y_stride, dst_hstride;
    if (ctx_in->shot->frame->format == AV_PIX_FMT_DRM_PRIME)
    {
        ctx->shot->c->desc_in = (const AVDRMFrameDescriptor *)ctx_in->shot->frame->data[0];
        ctx->shot->c->desc_out = (const AVDRMFrameDescriptor *)ctx->shot->frame->data[0];

        if (ctx->shot->c->desc_in->layers[0].planes[1].pitch)
        {
            // YUV/NV12/NV21
            src_y_stride = ctx->shot->c->desc_in->layers[0].planes[0].pitch;
            src_hstride = ctx->shot->c->desc_in->layers[0].planes[1].offset / src_y_stride;
            dst_y_stride = ctx->shot->c->desc_out->layers[0].planes[0].pitch;
            dst_hstride = ctx->shot->c->desc_out->layers[0].planes[1].offset / dst_y_stride;
        }
        else
        {
            // RGB
            src_y_stride = ctx->shot->c->desc_in->layers[0].planes[0].pitch;
            src_hstride = ctx_in->shot->frame->height; // RGB одна плоскость
            dst_y_stride = ctx->shot->c->desc_out->layers[0].planes[0].pitch;
            dst_hstride = ctx->shot->frame->height;
        }
    }

    if (!ctx->shot->c->rga_init)
    {
        if (ctx->shot->c->desc_in->layers[0].planes[1].pitch)
            ctx->shot->c->rga_fmt = RK_FORMAT_YCbCr_420_SP;
        else
            ctx->shot->c->rga_fmt = RK_FORMAT_RGBA_8888;

        rga_set_rect(&ctx->shot->c->s.rect, 0, 0,
                     ctx_in->codec_ctx->width, ctx_in->codec_ctx->height,
                     src_y_stride, src_hstride, ctx->shot->c->rga_fmt);
        rga_set_rect(&ctx->shot->c->d.rect, 0, 0,
                     ctx->codec_ctx->width, ctx->codec_ctx->height,
                     dst_y_stride, dst_hstride, ctx->shot->c->rga_fmt);
        ctx->shot->c->rga_init = 1;
    }

    int ret = c_RkRgaBlit(&ctx->shot->c->s, &ctx->shot->c->d, NULL);
    if (ret)
    {
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
        log_error("RGA blit failed: %d", ret);
#else
        log_error("Error transform frame");
#endif
        av_frame_free(&ctx->shot->frame);
        return -1;
    }

    if (ctx_in->shot->frame->pts == AV_NOPTS_VALUE || ctx_in->shot->frame->pts < 0)
    {
        log_warn("Что-то с временными метками, рассчитываю вручную. Могут быть сдвиги в временных метках.");
        ctx->shot->frame->pts = ctx->frame_count;
    }
    else
    {
        ctx->shot->frame->pts = av_rescale_q(ctx_in->shot->frame->pts,
                                             ctx_in->stream->time_base,
                                             ctx->codec_ctx->time_base);
    }
    ctx->frame_count++;

    ret = avcodec_send_frame(ctx->codec_ctx, ctx->shot->frame);
    if (ret < 0)
    {
        fprintf(stderr, "avcodec_send_frame failed: %d\n", ret);
        av_frame_free(&ctx->shot->frame);
        return -1;
    }

    if (ctx->packet)
    {
        av_packet_unref(ctx->packet);
        // av_packet_free(&ctx->packet);
    }
    else
    {
        ctx->packet = av_packet_alloc();
    }

    while (avcodec_receive_packet(ctx->codec_ctx, ctx->packet) >= 0)
    {
        av_packet_rescale_ts(ctx->packet, ctx->codec_ctx->time_base, ctx->stream->time_base);
        ctx->packet->stream_index = ctx->stream->index;
        if (av_interleaved_write_frame(ctx->format_ctx, ctx->packet) < 0)
        {
            av_packet_unref(ctx->packet);
            av_frame_unref(ctx->shot->frame);
            ctx->shot->frame->pts = AV_NOPTS_VALUE;
            ctx->shot->frame->pkt_dts = AV_NOPTS_VALUE;
            ctx->shot->frame->best_effort_timestamp = AV_NOPTS_VALUE;
            ctx->shot->frame->duration = 0;
            ctx->shot->frame->opaque = NULL;
            log_error("Не удалось записать кадр в видео файл.");
            return -1;
        }
        av_packet_unref(ctx->packet);
    }

    av_frame_unref(ctx->shot->frame);

    return 0;
}

int realese(rkcv_t *ctx)
{
#ifdef BUILD_DEV
    END_VIDEO = 1;
#endif
    if (ctx->codec_ctx)
    {
        avcodec_send_frame(ctx->codec_ctx, NULL); // сигнал "всё, кадров больше нет"
        AVPacket *pkt = av_packet_alloc();
        while (avcodec_receive_packet(ctx->codec_ctx, pkt) == 0)
        {
            av_packet_rescale_ts(pkt, ctx->codec_ctx->time_base, ctx->stream->time_base);
            pkt->stream_index = ctx->stream->index;
            av_interleaved_write_frame(ctx->format_ctx, pkt);
            av_packet_unref(pkt);
        }
        av_packet_free(&pkt);
    }

    av_write_trailer(ctx->format_ctx);
    if (ctx->format_ctx->pb)
        avio_closep(&ctx->format_ctx->pb);
}