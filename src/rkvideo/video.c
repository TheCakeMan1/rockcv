#include "rkvideo.h"

#define USE_CPU_DRAW_TEXT // TODO удалить когда появится другая реализация

static inline _Bool is_rtsp_source(const char *source)
{
    if (unlikely(!source))
        return 0;

    return (strncmp(source, "rtsp://", 7) == 0) ||
           (strncmp(source, "rtsps://", 8) == 0);
}

static inline _Bool is_image_source(const char *source)
{
    if (!source)
        return 0;

    const char *ext = strrchr(source, '.');
    if (!ext)
        return 0;

    ext++; // пропускаем точку

    return
        // растровые форматы
        strcasecmp(ext, "png") == 0 ||
        strcasecmp(ext, "apng") == 0 ||
        strcasecmp(ext, "jpg") == 0 ||
        strcasecmp(ext, "jpeg") == 0 ||
        strcasecmp(ext, "jpe") == 0 ||
        strcasecmp(ext, "jfif") == 0 ||
        strcasecmp(ext, "bmp") == 0 ||
        strcasecmp(ext, "dib") == 0 ||
        strcasecmp(ext, "gif") == 0 ||
        strcasecmp(ext, "tif") == 0 ||
        strcasecmp(ext, "tiff") == 0 ||
        strcasecmp(ext, "webp") == 0 ||
        strcasecmp(ext, "ico") == 0 ||
        strcasecmp(ext, "cur") == 0 ||
        strcasecmp(ext, "pbm") == 0 ||
        strcasecmp(ext, "pgm") == 0 ||
        strcasecmp(ext, "ppm") == 0 ||
        strcasecmp(ext, "pnm") == 0 ||
        strcasecmp(ext, "pfm") == 0 ||
        strcasecmp(ext, "pcx") == 0 ||
        strcasecmp(ext, "tga") == 0 ||
        strcasecmp(ext, "icns") == 0 ||

        // форматы сжатия/современные
        strcasecmp(ext, "heic") == 0 ||
        strcasecmp(ext, "heif") == 0 ||
        strcasecmp(ext, "avif") == 0 ||

        // RAW форматы (популярные)
        strcasecmp(ext, "dng") == 0 ||
        strcasecmp(ext, "cr2") == 0 ||
        strcasecmp(ext, "cr3") == 0 ||
        strcasecmp(ext, "nef") == 0 ||
        strcasecmp(ext, "nrw") == 0 ||
        strcasecmp(ext, "arw") == 0 ||
        strcasecmp(ext, "rw2") == 0 ||
        strcasecmp(ext, "orf") == 0 ||
        strcasecmp(ext, "srw") == 0 ||
        strcasecmp(ext, "raf") == 0 ||
        strcasecmp(ext, "eps") == 0 || // иногда используется как контейнер картинки
        strcasecmp(ext, "psd") == 0 || // Photoshop
        strcasecmp(ext, "xcf") == 0;   // GIMP
}

__attribute__((hot, flatten)) int readf(rkcv_t *ctx)
{
    if (unlikely(ctx->type_source != RK_TYPE_SOURCE_RTSP))
    {
        RKX_E_TAG("readf_f", "Невозможно прочитать кадр из картинки");
        return -1;
    }
    while (avcodec_receive_frame(ctx->codec_ctx, ctx->shot->frame) >= 0)
    {
        if (!ctx->shot->c->frame_t->fmt)
        {
#if USE_DRM_BUFFER
            AVDRMFrameDescriptor *desc = (AVDRMFrameDescriptor *)ctx->shot->frame->data[0];
            uint32_t drm_fmt = desc->layers[0].format;

            ctx->shot->c->frame_t->fmt = convert_pix_fmt_from_drm(drm_fmt);
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
            RKX_D("DRM format=0x%x -> RK format=%d", drm_fmt, ctx->shot->c->frame_t->fmt);
#endif
#else
            ctx->shot->c->frame_t->fmt = convert_pix_fmt(ctx->shot->frame->format, 0);
#endif
        }
        return 1;
    }

    while (av_read_frame(ctx->format_ctx, ctx->packet) >= 0)
    {
        if (ctx->packet->stream_index == ctx->video_stream_index)
        {
            if (avcodec_send_packet(ctx->codec_ctx, ctx->packet) < 0)
            {
                RKX_E("Ошибка отправки пакета в декодер");
                break;
            }

            while (avcodec_receive_frame(ctx->codec_ctx, ctx->shot->frame) >= 0)
            {
                if (!ctx->shot->c->frame_t->fmt)
                {
#if USE_DRM_BUFFER
                    AVDRMFrameDescriptor *desc = (AVDRMFrameDescriptor *)ctx->shot->frame->data[0];
                    uint32_t drm_fmt = desc->layers[0].format;

                    ctx->shot->c->frame_t->fmt = convert_pix_fmt_from_drm(drm_fmt);
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
                    RKX_D("DRM format=0x%x -> RK format=%d", drm_fmt, ctx->shot->c->frame_t->fmt);
#endif
#else
                    ctx->shot->c->frame_t->fmt = convert_pix_fmt(ctx->shot->frame->format, 0);
#endif
                }
                return 1;
            }
        }
    }

    return 0;
}

__attribute__((cold, warn_unused_result))
rkcv_t *
openv(char *source)
{
    size_t total_size =
        sizeof(rkcv_t) +
        sizeof(rkcv_shot_t) +
        sizeof(s_convert_f) +
        sizeof(info_frame_t) +
        3 * __alignof__(max_align_t);

    rkcv_t *ctx = calloc(1, total_size);
    if (unlikely(!ctx))
    {
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
        RKX_F_TAG("openv_f", "Couldn't allocate memory");
#endif
        return NULL;
    }

    uint8_t *ptr = (uint8_t *)(ctx + 1); // сразу за rkcv_t

    ctx->shot = (rkcv_shot_t *)ptr;
    ptr += sizeof(rkcv_shot_t);

    ctx->shot->c = (s_convert_f *)ptr;
    ptr += sizeof(s_convert_f);

    ctx->shot->c->frame_t = (info_frame_t *)ptr;
    ptr += sizeof(info_frame_t);

    if ((uintptr_t)ptr > (uintptr_t)ctx + total_size)
    {
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
        RKX_F_TAG("openv_f", "ptr overflow total_size");
#endif
        free(ctx);
        return NULL;
    }

#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
    RKX_D_TAG("openv_f", "выделено %zu байт под rkcv_t, shot, convert, frame_t", total_size);
    // log_debug("openv(): выделено %zu байт под rkcv_t, shot, convert, frame_t", total_size);
#endif

    ctx->shot->frame = av_frame_alloc();
    if (unlikely(!ctx->shot->frame))
    {
        RKX_F_TAG("openv_f", "Ошибка выделения av_frame");
        // log_fatal("Ошибка выделения av_frame");
    }

    ctx->packet = av_packet_alloc();
    if (unlikely(!ctx->packet))
    {
        RKX_F_TAG("openv_f", "Ошибка выделения av_packet");
        // log_fatal("Ошибка выделения av_packet");
    }

    if (is_rtsp_source(source))
    {
        ctx->type_source = RK_TYPE_SOURCE_RTSP;
    }
    else if (is_image_source(source))
    {
        ctx->type_source = RK_TYPE_SOURCE_IMAGE;
    }
    else
    {
        // TODO дописать все виды источников, а также аудио
        RKX_F("Неизвестный источник");
        return NULL;
    }

    if (ctx->type_source == RK_TYPE_SOURCE_RTSP)
    {
        avformat_network_init();

        ctx->opts = NULL;
        av_dict_set(&ctx->opts, "rtsp_transport", "tcp", 0);
        av_dict_set(&ctx->opts, "stimeout", "5000000", 0); // 5 сек таймаут

        if (avformat_open_input(&ctx->format_ctx, source, NULL, &ctx->opts) < 0)
        {
            RKX_F_TAG("openv_f", "Не удалось открыть входной файл: %s", source);
            free(ctx);
            return NULL;
        }
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
        else
        {
            RKX_D_TAG("openv_f", "Входной файл открыт");
            av_dict_free(&ctx->opts);
        }
#endif
    }
    else
    {
        if (unlikely(avformat_open_input(&ctx->format_ctx, source, NULL, NULL) < 0))
        {
            RKX_F_TAG("openv_f", "Не удалось открыть входной файл: %s", source);
            free(ctx);
            return NULL;
        }
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
        else
        {
            RKX_I_TAG("openv_f", "Входной файл открыт");
        }
#endif
    }

    ctx->format_ctx->flags |= AVFMT_FLAG_GENPTS;

    if (unlikely(avformat_find_stream_info(ctx->format_ctx, NULL) < 0))
    {
        RKX_F_TAG("openv_f", "Не удалось найти информацию о потоках");
        avformat_close_input(&ctx->format_ctx);
        free(ctx);
        return NULL;
    }
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
    else
    {
        RKX_D_TAG("openv_f", "Найдена информация о потоках");
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
        RKX_F_TAG("openv_f", "Входной файл не содержит видеопоток");
        // log_fatal("Входной файл не содержит видеопоток.");
        // fprintf(stderr, "Входной файл не содержит видеопоток.\n");
        avformat_close_input(&ctx->format_ctx);
        // free(ctx->source);
        free(ctx);
        return NULL;
    }
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
    else
    {
        RKX_D_TAG("openv_f", "Найдена информация о видеопоток");
        // log_debug("Найдена информация о видеопоток");
    }
#endif

    ctx->stream = ctx->format_ctx->streams[ctx->video_stream_index];

#ifdef RKMPP_ENABLE
    if (ctx->stream->codecpar->codec_id == AV_CODEC_ID_H264)
    {
#if HAS_H264_D_RKMPP
        ctx->codec = avcodec_find_decoder_by_name("h264_rkmpp");
#if defined(BUILD_DEV)
#pragma message "видео декодек h264 заменен на h264_rkmpp"
#endif
#else
        ctx->codec = avcodec_find_decoder(AV_CODEC_ID_H264);
#pragma error "decoder h264_rkmpp not found"
#endif
    }
    else if (ctx->stream->codecpar->codec_id == AV_CODEC_ID_HEVC)
    {
#if HAS_HEVC_D_RKMPP
        ctx->codec = avcodec_find_decoder_by_name("hevc_rkmpp");
#if defined(BUILD_DEV)
#pragma message "видео декодек hevc заменен на hevc_rkmpp"
#endif
#else
        ctx->codec = avcodec_find_decoder(AV_CODEC_ID_HEVC);
#pragma error "decoder hevc_rkmpp not found"
#endif
    }
    else if (ctx->stream->codecpar->codec_id == AV_CODEC_ID_H263)
    {
#if HAS_H263_D_RKMPP
        ctx->codec = avcodec_find_decoder_by_name("h263_rkmpp");
#if defined(BUILD_DEV)
#pragma message "видео декодек hevc заменен на h263_rkmpp"
#endif
#else
        ctx->codec = avcodec_find_decoder(AV_CODEC_ID_H263);
#pragma error "decoder h263_rkmpp not found"
#endif
    }
    else if (ctx->stream->codecpar->codec_id == AV_CODEC_ID_AV1)
    {
#if HAS_H263_D_RKMPP
        ctx->codec = avcodec_find_decoder_by_name("av1_rkmpp");
#if defined(BUILD_DEV)
#pragma message "видео декодек av1 заменен на av1_rkmpp"
#endif
#else
        ctx->codec = avcodec_find_decoder(AV_CODEC_ID_AV1);
#pragma error "decoder av1_rkmpp not found"
#endif
    }
    else if (ctx->stream->codecpar->codec_id == AV_CODEC_ID_MJPEG)
    {
#if HAS_H263_D_RKMPP
        ctx->codec = avcodec_find_decoder_by_name("mjpeg_rkmpp");
#if defined(BUILD_DEV)
#pragma message "декодек изображений mjpeg заменен на mjpeg_rkmpp"
#endif
#else
        ctx->codec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
#pragma error "decoder mjpeg_rkmpp not found"
#endif
    }
    else
    {
        ctx->codec = avcodec_find_decoder(ctx->stream->codecpar->codec_id);
    }
#else
    ctx->codec = avcodec_find_decoder(ctx->stream->codecpar->codec_id);
#endif
    if (unlikely(!ctx->codec))
    {
        RKX_F_TAG("openv_f", "Не удалось найти декодер");
        // log_fatal("Не удалось найти декодер.");
        // fprintf(stderr, "Не удалось найти декодер.\n");
        avformat_close_input(&ctx->format_ctx);
        // free(ctx->source);
        free(ctx);
        return NULL;
    }
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
    else
    {
        RKX_D_TAG("openv_f", "Удалось найти декодер");
    }
#endif

    ctx->codec_ctx = avcodec_alloc_context3(ctx->codec);
    if (unlikely(!ctx->codec_ctx))
    {
        RKX_F_TAG("openv_f", "Ошибка выделения AVCodecContext");
        avformat_close_input(&ctx->format_ctx);
        free(ctx);
        return NULL;
    }
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
    else
    {
        RKX_D_TAG("openv_f", "Выделился AVCodecContext");
    }
#endif

#ifdef RKMPP_ENABLE
#else
    ctx->shot->c->frame_t->fmt = convert_pix_fmt_from_av(ctx->codec_ctx->pix_fmt);
#endif

    if (unlikely(avcodec_parameters_to_context(ctx->codec_ctx, ctx->stream->codecpar) < 0))
    {
        RKX_F_TAG("openv_f", "Не удалось сконфигурировать контекст декодера");
        avcodec_free_context(&ctx->codec_ctx);
        avformat_close_input(&ctx->format_ctx);
        free(ctx);
        return NULL;
    }
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
    else
    {
        RKX_D_TAG("openv_f", "Удалось сконфигурировать контекст декодера");
    }
#endif

#if USE_DRM_BUFFER
    AVBufferRef *hw_device_ctx = NULL;
    if (av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_RKMPP, NULL, NULL, 0) < 0)
    {
        RKX_F_TAG("openv_f", "Не удалось создать RKMpp hwdevice");
        return NULL;
    }
    ctx->codec_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
#endif

    if (unlikely(avcodec_open2(ctx->codec_ctx, ctx->codec, NULL) < 0))
    {
        RKX_F_TAG("openv_f", "Не удалось открыть декодер");
        avcodec_free_context(&ctx->codec_ctx);
        avformat_close_input(&ctx->format_ctx);
        free(ctx);
        return NULL;
    }
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
    else
    {
        RKX_D_TAG("openv_f", "Удалось открыть декодер");
    }
#endif

    if (ctx->type_source == RK_TYPE_SOURCE_IMAGE)
    {
        ctx->type_source = RK_TYPE_SOURCE_RTSP;
        if (!readf(ctx))
        {
            RKX_E_TAG("openv_f", "Нет кадра в картинке");
        }
        ctx->type_source = RK_TYPE_SOURCE_IMAGE;
        ctx->shot->c->frame_t->width = ctx->codec_ctx->width;
        ctx->shot->c->frame_t->height = ctx->codec_ctx->height;
    }
    else
    {
        ctx->shot->c->time_base = ctx->stream->time_base;
        ctx->shot->c->frame_t->width = ctx->codec_ctx->width;
        ctx->shot->c->frame_t->height = ctx->codec_ctx->height;
    }

    RKX_D_TAG("openv_f", "Все успешно");
    RKX_T("______________________________");

    return ctx;
}

rkcv_t *video(rkcv_t *ctx_in,
              const char *filename,
              int codec,
              info_frame_t *frame_t)
{
#ifdef BUILD_DEV
    START_VIDEO = 1;
#endif

    rkcv_t *ctx = aligned_alloc(64, sizeof(*ctx));
    if (unlikely(!ctx))
    {
        // RKX_F_TAG("calloc rkcv_t");

        // log_fatal("calloc rkcv_t");
        return NULL;
    }
    ctx->shot = calloc(1, sizeof(*ctx->shot));

    ctx->shot->c = calloc(1, sizeof(*ctx->shot->c));
    if (!ctx->shot->c)
    {
        perror("calloc");
    }

    ctx->shot->c->frame_t = calloc(1, sizeof(info_frame_t));
    if (!ctx->shot->c->frame_t)
    {
        free(ctx->shot->c->frame_t);
        return NULL;
    }

    if (unlikely(avformat_alloc_output_context2(&ctx->format_ctx, NULL, NULL, filename) < 0))
    {
        // log_fatal("avformat_alloc_output_context2");
        free(ctx);
        return NULL;
    }

#if RKMPP_ENABLE
    switch (codec)
    {
    case RKCodec_H264:
#if HAS_H264_E_RKMPP
        ctx->codec = avcodec_find_encoder_by_name("h264_rkmpp");
#if defined(BUILD_DEV)
#pragma message "видео кодек h264 заменен на h264_rkmpp"
#endif
#else
        ctx->codec = avcodec_find_encoder(AV_CODEC_ID_H264);
#pragma error "encoder h264_rkmpp not found"
#endif
        break;
    case RKCodec_HEVC:
#if HAS_HEVC_E_RKMPP
        ctx->codec = avcodec_find_encoder_by_name("hevc_rkmpp");
#if defined(BUILD_DEV)
#pragma message "видео кодек hevc заменен на hevc_rkmpp"
#endif
#else
        ctx->codec = avcodec_find_encoder(AV_CODEC_ID_HEVC);
#pragma error "encoder hevc_rkmpp not found"
#endif
        break;
    case RKCodec_H263:
#if HAS_H263_E_RKMPP
        ctx->codec = avcodec_find_encoder_by_name("h263_rkmpp");
#if defined(BUILD_DEV)
#pragma message "видео кодек h263 заменен на h263_rkmpp"
#endif
#else
        ctx->codec = avcodec_find_encoder(AV_CODEC_ID_H263);
#pragma error "encoder h263_rkmpp not found"
#endif
        break;
    default:
        ctx->codec = avcodec_find_encoder(codec);
        break;
    }
#else
    ctx->codec = avcodec_find_encoder(codec);
#endif
    if (unlikely(!ctx->codec))
    {
        // log_fatal("encoder not found");
        goto fail_fmt;
    }

    ctx->stream = avformat_new_stream(ctx->format_ctx, NULL);
    ctx->codec_ctx = avcodec_alloc_context3(ctx->codec);
    if (unlikely(!ctx->stream || !ctx->codec_ctx))
    {
        // log_fatal("new_stream/alloc_context");
        goto fail_fmt;
    }

    ctx->codec_ctx->width = ctx->shot->c->frame_t->width = frame_t->width ? frame_t->width : ctx_in->codec_ctx->width;
    ctx->codec_ctx->height = ctx->shot->c->frame_t->height = frame_t->height ? frame_t->height : ctx_in->codec_ctx->height;

    if (ctx->codec_ctx->width % 16)
        ctx->codec_ctx->width = (ctx->codec_ctx->width + 15) & ~15;
    if (ctx->codec_ctx->height % 2)
        ctx->codec_ctx->height &= ~1;

    ctx->shot->c->frame_t->width = ctx->codec_ctx->width;
    ctx->shot->c->frame_t->height = ctx->codec_ctx->height;

    AVRational fps_r = av_guess_frame_rate(ctx_in->format_ctx, ctx_in->stream, NULL);
    if (fps_r.num == 0 || fps_r.den == 0)
    {
        fps_r = ctx_in->stream->avg_frame_rate.num ? ctx_in->stream->avg_frame_rate
                                                   : (AVRational){25, 1};
    }

    ctx->codec_ctx->framerate = fps_r;
    ctx->codec_ctx->time_base = av_inv_q(fps_r);
    ctx->codec_ctx->pkt_timebase = ctx->codec_ctx->time_base;
    ctx->stream->time_base = ctx->codec_ctx->time_base;
    ctx->stream->avg_frame_rate = fps_r;
    ctx->fps = av_q2d(fps_r);

    // TODO DRM буфер не у всех форматов есть
#if USE_DRM_BUFFER
    ctx->codec_ctx->pix_fmt = AV_PIX_FMT_DRM_PRIME;
#else
    ctx->codec_ctx->pix_fmt = convert_pix_fmt(frame_t->fmt, 0);
#endif

#if USE_DRM_BUFFER
    AVBufferRef *hw_dev = NULL;
    if (unlikely(av_hwdevice_ctx_create(&hw_dev, AV_HWDEVICE_TYPE_RKMPP, NULL, NULL, 0) < 0))
    {
        // log_fatal("av_hwdevice_ctx_create(RKMPP)");
        goto fail_ctxs;
    }
    ctx->codec_ctx->hw_device_ctx = av_buffer_ref(hw_dev);

    AVBufferRef *hw_frames = av_hwframe_ctx_alloc(ctx->codec_ctx->hw_device_ctx);
    if (unlikely(!hw_frames))
    {
        // log_fatal("av_hwframe_ctx_alloc");
        goto fail_ctxs;
    }

    AVHWFramesContext *fc = (AVHWFramesContext *)hw_frames->data;
    fc->format = AV_PIX_FMT_DRM_PRIME;
    fc->sw_format = convert_pix_fmt(frame_t->fmt, 0);
    fc->width = ctx->codec_ctx->width;
    fc->height = ctx->codec_ctx->height;
    fc->initial_pool_size = 8;

    if (unlikely(av_hwframe_ctx_init(hw_frames) < 0))
    {
        // log_fatal("av_hwframe_ctx_init");
        goto fail_frames;
    }
    ctx->codec_ctx->hw_frames_ctx = av_buffer_ref(hw_frames);

    if (unlikely(avcodec_open2(ctx->codec_ctx, ctx->codec, NULL) < 0))
    {
        // log_fatal("avcodec_open2");
        goto fail_frames;
    }
#endif

    /* ---------------------- */
    /* Оптимизация задержки и копирования */
    ctx->format_ctx->flags |= AVFMT_FLAG_NOBUFFER;
    ctx->codec_ctx->flags |= AV_CODEC_FLAG_LOW_DELAY;
    /* ---------------------- */

    if (unlikely(avcodec_parameters_from_context(ctx->stream->codecpar, ctx->codec_ctx) < 0))
    {
        // log_fatal("avcodec_parameters_from_context");
        goto fail_opened;
    }

    // Открываем файл
    if (unlikely(!(ctx->format_ctx->oformat->flags & AVFMT_NOFILE)))
    {
        if (avio_open(&ctx->format_ctx->pb, filename, AVIO_FLAG_WRITE) < 0)
        {
            // log_fatal("avio_open");
            goto fail_opened;
        }
    }

    if (unlikely(avformat_write_header(ctx->format_ctx, NULL) < 0))
    {
        // log_fatal("avformat_write_header");
        if (ctx->format_ctx->pb)
            avio_closep(&ctx->format_ctx->pb);
        goto fail_opened;
    }

    ctx->frame_count = 0; // TODO надо рассмотреть необходимость этой переменной

// log_debug("Создан выходной поток для файла: %s", filename);
#if USE_DRM_BUFFER
    av_buffer_unref(&hw_frames);
    av_buffer_unref(&hw_dev);
#endif

    ctx->shot->frame = av_frame_alloc();
    if (!ctx->shot->frame)
    {
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
        // log_error("error alloc frame");
#endif
        return NULL;
    }
    ctx->shot->frame->format = AV_PIX_FMT_DRM_PRIME; // TODO он тут не всегда DRM, надо поправить
    ctx->shot->frame->width = ctx->codec_ctx->width;
    ctx->shot->frame->height = ctx->codec_ctx->height;
    return ctx;

fail_opened:
    avcodec_close(ctx->codec_ctx);
#if USE_DRM_BUFFER
fail_frames:
    if (hw_frames)
        av_buffer_unref(&hw_frames);
fail_ctxs:
    if (hw_dev)
        av_buffer_unref(&hw_dev);
#endif
fail_fmt:
    if (ctx->codec_ctx)
        avcodec_free_context(&ctx->codec_ctx);
    if (ctx->format_ctx)
        avformat_free_context(ctx->format_ctx);
    free(ctx);
    return NULL;
}

__attribute__((deprecated("imw: тестовая неотлаженная функция при передаче параметра наложения текста, использовать с осторожностью")))
__attribute__((hot, flatten)) int
imw(rkcv_t *restrict ctx, rkcv_shot_t *restrict ctx_in, s_text_f *text_t)
{
    if (!ctx_in->frame)
    {
        RKX_E_TAG("imw_f", "The input frame is empty.");
        return -1;
    }
#if USE_DRM_BUFFER
    if (ctx_in->frame->format != AV_PIX_FMT_DRM_PRIME)
    {
        RKX_E_TAG("imw_f", "The pixel format is not supported yet %d.", ctx_in->frame->format);
        return -1;
    }
#endif

    if (!ctx->shot->frame)
    {
        ctx->shot->frame = av_frame_alloc();
        if (!ctx->shot->frame)
        {
            RKX_E_TAG("imw_f", "No memory allocated for the shot frame.");
            return -1;
        }
        // TODO тоже DRM не всегда есть не у всех форматов
#if USE_DRM_BUFFER
        ctx->shot->frame->format = AV_PIX_FMT_DRM_PRIME;
#else
        ctx->shot->frame->format = ctx->codec_ctx->pix_fmt;
#endif
        ctx->shot->frame->width = ctx->codec_ctx->width;
        ctx->shot->frame->height = ctx->codec_ctx->height;
    }
    else
    {
        av_frame_unref(ctx->shot->frame); // ← ОБЯЗАТЕЛЬНО

        ctx->shot->frame->format =
#if USE_DRM_BUFFER
            AV_PIX_FMT_DRM_PRIME;
#else
            ctx->codec_ctx->pix_fmt;
#endif
        ctx->shot->frame->width = ctx->codec_ctx->width;
        ctx->shot->frame->height = ctx->codec_ctx->height;

        ctx->shot->frame->pts = AV_NOPTS_VALUE;
        ctx->shot->frame->pkt_dts = AV_NOPTS_VALUE;
        ctx->shot->frame->best_effort_timestamp = AV_NOPTS_VALUE;
        ctx->shot->frame->duration = 0;
        ctx->shot->frame->opaque = NULL;
    }

#if USE_DRM_BUFFER
    if (av_hwframe_get_buffer(ctx->codec_ctx->hw_frames_ctx, ctx->shot->frame, 0) < 0)
    {
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
        RKX_E_TAG("imw_f", "av_hwframe_get_buffer failed.");
#else
        RKX_E_TAG("imw_f", "Bad frame buffer.");
#endif
        av_frame_free(&ctx->shot->frame);
        return -1;
    }
#else
    if (av_frame_get_buffer(ctx->shot->frame, 32) < 0)
    {
        RKX_E_TAG("imw_f", "av_frame_get_buffer failed");
        return -1;
    }
    if (av_frame_make_writable(ctx->shot->frame) < 0)
    {
        RKX_E_TAG("imw_f", "writable failed");
        return -1;
    }
#endif

#if USE_DRM_BUFFER
    int fd_in = drmprime_fd_from_frame(ctx_in->frame);
    int fd_out = drmprime_fd_from_frame(ctx->shot->frame);
    if (fd_in < 0 || fd_out < 0)
    {
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
        RKX_E_TAG("imw_f", "bad drm fd.");
#else
        RKX_E_TAG("imw_f", "bad frame buffer.");
#endif
        av_frame_free(&ctx->shot->frame);
        return -1;
    }
    ctx_in->c->s.fd = fd_in;
    ctx->shot->c->s.fd = fd_out;
#else
    ctx_in->c->s.fd = -1;
    ctx->shot->c->s.fd = -1;
    ctx_in->c->s.virAddr = ctx_in->frame->data[0];
    ctx->shot->c->s.virAddr = ctx->shot->frame->data[0];
#endif

    ctx_in->c->s.mmuFlag = 1;
    ctx->shot->c->s.mmuFlag = 1;

    // int src_y_stride, src_hstride, dst_y_stride, dst_hstride;
    // if (ctx_in->frame->format == AV_PIX_FMT_DRM_PRIME)
    // {
    //     ctx_in->c->desc = (const AVDRMFrameDescriptor *)ctx_in->frame->data[0];
    //     ctx->shot->c->desc = (const AVDRMFrameDescriptor *)ctx->shot->frame->data[0];

    //     if (ctx_in->c->desc->layers[0].planes[1].pitch)
    //     {
    //         // YUV/NV12/NV21
    //         src_y_stride = ctx_in->c->desc->layers[0].planes[0].pitch;
    //         src_hstride = ctx_in->c->desc->layers[0].planes[1].offset / src_y_stride;
    //         dst_y_stride = ctx->shot->c->desc->layers[0].planes[0].pitch;
    //         dst_hstride = ctx->shot->c->desc->layers[0].planes[1].offset / dst_y_stride;
    //     }
    //     else
    //     {
    //         // RGB
    //         src_y_stride = ctx_in->c->desc->layers[0].planes[0].pitch;
    //         src_hstride = ctx_in->frame->height; // RGB одна плоскость
    //         dst_y_stride = ctx->shot->c->desc->layers[0].planes[0].pitch;
    //         dst_hstride = ctx->shot->frame->height;
    //     }
    // }
    // else
    // {
    // }

    if (!ctx->shot->c->rga_init)
    {
#if USE_DRM_BUFFER
        ctx_in->c->desc = (const AVDRMFrameDescriptor *)ctx_in->frame->data[0];
        ctx->shot->c->desc = (const AVDRMFrameDescriptor *)ctx->shot->frame->data[0];
        int src_y_stride = ctx_in->c->desc->layers[0].planes[0].pitch;
        int dst_y_stride = ctx->shot->c->desc->layers[0].planes[0].pitch;
        int src_hstride =
            ctx_in->c->desc->layers[0].planes[1].pitch ? ctx_in->c->desc->layers[0].planes[1].offset / src_y_stride : ctx_in->frame->height;

        int dst_hstride =
            ctx->shot->c->desc->layers[0].planes[1].pitch ? ctx->shot->c->desc->layers[0].planes[1].offset / dst_y_stride : ctx->shot->frame->height;
        if (ctx->shot->c->desc->layers[0].planes[1].pitch)
            ctx->shot->c->frame_t->fmt = RK_PIX_FMT_YCbCr_420_SP;
        else
            ctx->shot->c->frame_t->fmt = RK_PIX_FMT_RGBA_8888;
#else
        ctx_in->c->frame_t->fmt = convert_pix_fmt_from_av(ctx_in->frame->format);

        ctx->shot->c->frame_t->fmt = convert_pix_fmt_from_av(ctx->codec_ctx->pix_fmt);

        int src_y_stride = ctx_in->frame->linesize[0];
        int dst_y_stride = ctx->shot->frame->linesize[0];

        int src_hstride = ctx_in->frame->height;
        int dst_hstride = ctx->shot->frame->height;
#endif

        int fmt_local_in = convert_pix_fmt(ctx_in->c->frame_t->fmt, 1);
        int fmt_local_out = convert_pix_fmt(ctx->shot->c->frame_t->fmt, 1);

        rga_set_rect(&ctx_in->c->s.rect, 0, 0,
                     ctx_in->c->frame_t->width, ctx_in->c->frame_t->height,
                     src_y_stride, src_hstride, fmt_local_in);
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
        RKX_D_TAG("imw_f", "src frame w=%d h=%d fmt=%d stride=%d",
                  ctx_in->frame->width, ctx_in->frame->height,
                  ctx_in->frame->format, src_y_stride);
#endif

        rga_set_rect(&ctx->shot->c->s.rect, 0, 0,
                     ctx->codec_ctx->width, ctx->codec_ctx->height,
                     dst_y_stride, dst_hstride, fmt_local_out);
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
        RKX_D_TAG("imw_f", "RGA rect: %d %d %d %d stride=%d fmt=%d",
                  ctx->shot->c->s.rect.xoffset,
                  ctx->shot->c->s.rect.yoffset,
                  ctx->shot->c->s.rect.width,
                  ctx->shot->c->s.rect.height,
                  ctx->shot->c->s.rect.wstride,
                  ctx->shot->c->s.rect.format);
#endif
        fprintf(stderr, "RGA in: fmt=%d, out: fmt=%d, codec pix_fmt=%d, frame->format=%d\n",
                fmt_local_in, fmt_local_out,
                ctx->codec_ctx->pix_fmt,
                ctx->shot->frame->format);
        ctx->shot->c->rga_init = 1;
    }

    int ret = RgaBlit(&ctx_in->c->s, &ctx->shot->c->s, NULL);
    if (ret)
    {
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
        RKX_E_TAG("imw_f", "RGA blit failed: %d", ret);
        // log_error("RGA blit failed: %d", ret);
#else
        // log_error("Error transform and write frame");
#endif
        av_frame_free(&ctx->shot->frame); // TODO обработка ошибки не срабатывает
        return -1;
    }

    if (0)
    {
#ifdef USE_CPU_DRAW_TEXT
        const int W = ctx->codec_ctx->width;
        const int H = ctx->codec_ctx->height;

        // 1) DRM_PRIME -> SW (NV12), получаем кадр с произвольными linesize
        AVFrame *sw = av_frame_alloc();
        if (!sw)
            return -1;
        sw->format = convert_pix_fmt(ctx->shot->c->frame_t->fmt, 0);
        sw->width = W;
        sw->height = H;
#if USE_DRM_BUFFER
        if (av_hwframe_transfer_data(sw, ctx->shot->frame, 0) < 0)
        {
            av_frame_free(&sw);
            return -1;
        }
#endif
        if (av_frame_make_writable(sw) < 0)
        {
            av_frame_free(&sw);
            return -1;
        }

        image_buffer_t img = {0};
        img.width = W;
        img.height = H;
        img.format = ctx->shot->c->frame_t->fmt;
        const char *txt = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюя";
        const unsigned int clr = RKCOLOR_WHITE;
        const int fs = 30;
        uint8_t *tight;

        if (ctx->shot->c->frame_t->fmt == RK_PIX_FMT_YCbCr_420_SP || ctx->shot->c->frame_t->fmt == RK_PIX_FMT_YCrCb_420_SP)
        {
            const int y_size = W * H;
            const int uv_size = W * (H / 2);
            const int tight_size = y_size + uv_size;

            tight = av_malloc(tight_size);
            if (!tight)
            {
                av_frame_free(&sw);
                return -1;
            }
            uint8_t *tightY = tight;
            uint8_t *tightUV = tight + y_size;

            // копия Y
            for (int y = 0; y < H; y++)
            {
                memcpy(tightY + y * W, sw->data[0] + y * sw->linesize[0], W);
            }
            // копия UV (по строкам, ширина = W байт)
            for (int y = 0; y < H / 2; y++)
            {
                memcpy(tightUV + y * W, sw->data[1] + y * sw->linesize[1], W);
            }

            img.virt_addr = tight;
            img.size = tight_size;

            draw_text(&img, txt, text_t->x, text_t->y, text_t->color, fs);

            // 4) Копия обратно из плотного буфера в sw с учётом linesize
            for (int y = 0; y < H; y++)
            {
                memcpy(sw->data[0] + y * sw->linesize[0], tightY + y * W, W);
            }
            for (int y = 0; y < H / 2; y++)
            {
                memcpy(sw->data[1] + y * sw->linesize[1], tightUV + y * W, W);
            }
        }
        else
        {
            RKX_F_TAG("imw_f", "Такой формат не поддерживается"); // TODO сделать нормальный механизм защиты
            abort();
        }

        AVFrame *hw_new = av_frame_alloc();
        if (!hw_new)
        {
            av_free(tight);
            av_frame_free(&sw);
            return -1;
        }
        hw_new->format = AV_PIX_FMT_DRM_PRIME;
        hw_new->width = W;
        hw_new->height = H;

        if (av_hwframe_get_buffer(ctx->codec_ctx->hw_frames_ctx, hw_new, 0) < 0)
        {
            av_frame_free(&hw_new);
            av_free(tight);
            av_frame_free(&sw);
            return -1;
        }
        if (av_hwframe_transfer_data(hw_new, sw, 0) < 0)
        {
            av_frame_free(&hw_new);
            av_free(tight);
            av_frame_free(&sw);
            return -1;
        }

        av_frame_unref(ctx->shot->frame);
        av_frame_move_ref(ctx->shot->frame, hw_new);

        // 6) Очистка
        av_frame_free(&hw_new); // после move_ref() он пуст
        av_free(tight);
        av_frame_free(&sw);
#else
        RKX_F_TAG("imw_t", "There is no text writing module.");
#endif
    }

    if (ctx_in->frame->pts == AV_NOPTS_VALUE || ctx_in->frame->pts < 0)
    {
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
        RKX_W_TAG("imw_t", "Timestamps are broken pts: %d, calc: %d.", ctx_in->frame->pts, ctx->frame_count);
#endif
        ctx->shot->frame->pts = ctx->frame_count;
    }
    else
    {
        ctx->shot->frame->pts = av_rescale_q(ctx_in->frame->pts,
                                             ctx_in->c->time_base,
                                             ctx->codec_ctx->time_base);
    }
    ctx->frame_count++;
    if (!ctx->shot->frame)
    {
        fprintf(stderr, "shot->frame is NULL\n");
        return -1;
    }

    fprintf(stderr, "send_frame: codec pix_fmt=%d, frame format=%d, w=%d/%d, h=%d/%d\n",
            ctx->codec_ctx->pix_fmt,
            ctx->shot->frame->format,
            ctx->codec_ctx->width, ctx->shot->frame->width,
            ctx->codec_ctx->height, ctx->shot->frame->height);

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
            // log_error("Не удалось записать кадр в видео файл.");
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