#include "rkvideo.h"
#include "font_data.h"

static FT_Library g_ftlib = NULL;
static FT_Face g_face = NULL;

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

    ctx->shot = calloc(1, sizeof(*ctx->shot));
    if (!ctx->shot)
    {
        log_fatal("Ошибка выделения памяти под ctx->shot");
    }

    ctx->shot->c = calloc(1, sizeof(*ctx->shot->c));
    if (!ctx->shot->c)
    {
        log_fatal("Ошибка выделения памяти под ctx->shot->c");
    }

    ctx->shot->c->frame_t = calloc(1, sizeof(info_frame_t));
    if (!ctx->shot->c->frame_t)
    {
        free(ctx->shot->c->frame_t);
        return NULL;
    }

    ctx->shot->frame = av_frame_alloc();
    if (!ctx->shot->frame)
    {
        log_fatal("Ошибка выделения av_frame");
    }

    ctx->packet = av_packet_alloc();
    if (!ctx->packet)
    {
        log_fatal("Ошибка выделения av_packet");
    }

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

#ifdef RKMPP_ENABLE
#else
    ctx->shot->c->frame_t->fmt = convert_pix_fmt_from_av(ctx->codec_ctx->pix_fmt);
#endif

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

    ctx->shot->c->time_base = ctx->stream->time_base;
    ctx->shot->c->frame_t->width = ctx->codec_ctx->width;
    ctx->shot->c->frame_t->height = ctx->codec_ctx->height;

    log_debug("Все успешно.");

    return ctx;
}

int readf(rkcv_t *ctx)
{
    while (avcodec_receive_frame(ctx->codec_ctx, ctx->shot->frame) >= 0)
    {
        if (!ctx->shot->c->frame_t->fmt)
        {
            AVDRMFrameDescriptor *desc = (AVDRMFrameDescriptor *)ctx->shot->frame->data[0];
            uint32_t drm_fmt = desc->layers[0].format;

            ctx->shot->c->frame_t->fmt = convert_pix_fmt_from_drm(drm_fmt);
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
            log_debug("DRM format=0x%x -> RK format=%d", drm_fmt, ctx->shot->c->frame_t->fmt);
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
                log_error("Ошибка отправки пакета в декодер.");
                break;
            }

            while (avcodec_receive_frame(ctx->codec_ctx, ctx->shot->frame) >= 0)
            {
                if (!ctx->shot->c->frame_t->fmt)
                {
                    AVDRMFrameDescriptor *desc = (AVDRMFrameDescriptor *)ctx->shot->frame->data[0];
                    uint32_t drm_fmt = desc->layers[0].format;

                    ctx->shot->c->frame_t->fmt = convert_pix_fmt_from_drm(drm_fmt);
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
                    log_debug("DRM format=0x%x -> RK format=%d", drm_fmt, ctx->shot->c->frame_t->fmt);
#endif
                }
                return 1;
            }
        }
    }

    return 1;
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
        log_fatal("calloc rkcv_t");
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
    // memset(ctx->shot->c, 0, sizeof(*ctx->shot->c));
    // memset(&ctx->shot->c->s, 0, sizeof(ctx->shot->c->s));
    // memset(&ctx->shot->c->d, 0, sizeof(ctx->shot->c->d));

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

    ctx->codec_ctx->pix_fmt = AV_PIX_FMT_DRM_PRIME;

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
    fc->sw_format = convert_pix_fmt(frame_t->fmt, 0);
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

int init_text_filter(AVFilterGraph **graph, AVFilterContext **buffersrc_ctx,
                     AVFilterContext **buffersink_ctx,
                     const AVCodecContext *codec_ctx,
                     const char *text, AVBufferRef *hw_device_ctx, s_text_f *text_t)
{
    int ret;
    char args[256];
    AVFilterGraph *filter_graph = avfilter_graph_alloc();
    if (!filter_graph)
        return AVERROR(ENOMEM);

    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    const AVFilter *hwdownload = avfilter_get_by_name("hwdownload");
    const AVFilter *format = avfilter_get_by_name("format");
    const AVFilter *drawtext = avfilter_get_by_name("drawtext");
    const AVFilter *hwupload = avfilter_get_by_name("hwupload");

    AVFilterContext *src_ctx = NULL, *sink_ctx = NULL;
    AVFilterContext *hwdownload_ctx = NULL, *format_ctx = NULL;
    AVFilterContext *drawtext_ctx = NULL, *hwupload_ctx = NULL;

    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             codec_ctx->width, codec_ctx->height, AV_PIX_FMT_DRM_PRIME,
             codec_ctx->time_base.num, codec_ctx->time_base.den,
             codec_ctx->sample_aspect_ratio.num, codec_ctx->sample_aspect_ratio.den);

    if ((ret = avfilter_graph_create_filter(&src_ctx, buffersrc, "in", args, NULL, filter_graph)) < 0)
        goto fail;

    // Связываем входной фильтр с hw_frames_ctx
    {
        AVBufferSrcParameters *par = av_buffersrc_parameters_alloc();
        memset(par, 0, sizeof(*par));
        par->format = AV_PIX_FMT_DRM_PRIME;
        par->hw_frames_ctx = av_buffer_ref(codec_ctx->hw_frames_ctx);
        ret = av_buffersrc_parameters_set(src_ctx, par);
        av_freep(&par);
        if (ret < 0)
            goto fail;
    }

    if ((ret = avfilter_graph_create_filter(&sink_ctx, buffersink, "out", NULL, NULL, filter_graph)) < 0)
        goto fail;

    // hwdownload
    if ((ret = avfilter_graph_create_filter(&hwdownload_ctx, hwdownload, "hwdownload", NULL, NULL, filter_graph)) < 0)
        goto fail;

    // выясняем sw_format
    const AVHWFramesContext *fc = (const AVHWFramesContext *)codec_ctx->hw_frames_ctx->data;
    enum AVPixelFormat swfmt = fc->sw_format;
    const char *fmtname = av_get_pix_fmt_name(swfmt);
    if (!fmtname)
        fmtname = "nv12"; // fallback

    char fmt_args[64];
    snprintf(fmt_args, sizeof(fmt_args), "%s", fmtname);
    if ((ret = avfilter_graph_create_filter(&format_ctx, format, "format", fmt_args, NULL, filter_graph)) < 0)
        goto fail;

    char drawtext_args[512];
    snprintf(drawtext_args, sizeof(drawtext_args),
             "text='%s':fontcolor=white:fontsize=%d:x=%d:y=%d:box=1:boxcolor=black@0.0",
             text_t->text, text_t->fontsize, text_t->x, text_t->y);
    if ((ret = avfilter_graph_create_filter(&drawtext_ctx, drawtext, "drawtext", drawtext_args, NULL, filter_graph)) < 0)
        goto fail;

    if ((ret = avfilter_graph_create_filter(&hwupload_ctx, hwupload, "hwupload", NULL, NULL, filter_graph)) < 0)
        goto fail;
    hwupload_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

    // связываем
    if ((ret = avfilter_link(src_ctx, 0, hwdownload_ctx, 0)) < 0)
        goto fail;
    if ((ret = avfilter_link(hwdownload_ctx, 0, format_ctx, 0)) < 0)
        goto fail;
    if ((ret = avfilter_link(format_ctx, 0, drawtext_ctx, 0)) < 0)
        goto fail;
    if ((ret = avfilter_link(drawtext_ctx, 0, hwupload_ctx, 0)) < 0)
        goto fail;
    if ((ret = avfilter_link(hwupload_ctx, 0, sink_ctx, 0)) < 0)
        goto fail;

    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
        goto fail;

    *graph = filter_graph;
    *buffersrc_ctx = src_ctx;
    *buffersink_ctx = sink_ctx;
    return 0;

fail:
    avfilter_graph_free(&filter_graph);
    return ret;
}

__attribute__((hot, flatten, warning("imw имеет тестовый вызов функции отображения текста на кадре"))) int imw(rkcv_t *restrict ctx, rkcv_shot_t *restrict ctx_in, s_text_f *text_t)
{
    if (!ctx_in->frame)
    {
        log_warn("no input frame");
        // fprintf(stderr, "imw: no input frame\n");
        return -1;
    }

    if (ctx_in->frame->format != AV_PIX_FMT_DRM_PRIME)
    {
        log_warn("expected DRM_PRIME input, got %d", ctx_in->frame->format);
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

    int fd_in = drmprime_fd_from_frame(ctx_in->frame);
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

    ctx_in->c->s.fd = fd_in;
    ctx_in->c->s.mmuFlag = 1;
    ctx->shot->c->s.fd = fd_out;
    ctx->shot->c->s.mmuFlag = 1;

    int src_y_stride, src_hstride, dst_y_stride, dst_hstride;
    if (ctx_in->frame->format == AV_PIX_FMT_DRM_PRIME)
    {
        ctx->shot->c->desc_in = (const AVDRMFrameDescriptor *)ctx_in->frame->data[0];
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
            src_hstride = ctx_in->frame->height; // RGB одна плоскость
            dst_y_stride = ctx->shot->c->desc_out->layers[0].planes[0].pitch;
            dst_hstride = ctx->shot->frame->height;
        }
    }

    if (!ctx->shot->c->rga_init)
    {
        if (ctx->shot->c->desc_in->layers[0].planes[1].pitch)
            ctx->shot->c->frame_t->fmt = RK_PIX_FMT_YCbCr_420_SP;
        else
            ctx->shot->c->frame_t->fmt = RK_PIX_FMT_RGBA_8888;
        int fmt_local_in = convert_pix_fmt(ctx_in->c->frame_t->fmt, 1);
        int fmt_local_out = convert_pix_fmt(ctx->shot->c->frame_t->fmt, 1);

        rga_set_rect(&ctx_in->c->s.rect, 0, 0,
                     ctx_in->c->frame_t->width, ctx_in->c->frame_t->height,
                     src_y_stride, src_hstride, fmt_local_in);
        log_debug("src frame w=%d h=%d fmt=%d stride=%d",
                  ctx_in->frame->width, ctx_in->frame->height,
                  ctx_in->frame->format, src_y_stride);

        rga_set_rect(&ctx->shot->c->s.rect, 0, 0,
                     ctx->codec_ctx->width, ctx->codec_ctx->height,
                     dst_y_stride, dst_hstride, fmt_local_out);
        log_debug("RGA rect: %d %d %d %d stride=%d fmt=%d",
                  ctx->shot->c->s.rect.xoffset,
                  ctx->shot->c->s.rect.yoffset,
                  ctx->shot->c->s.rect.width,
                  ctx->shot->c->s.rect.height,
                  ctx->shot->c->s.rect.wstride,
                  ctx->shot->c->s.rect.format);

        ctx->shot->c->rga_init = 1;
    }

    int ret = c_RkRgaBlit(&ctx_in->c->s, &ctx->shot->c->s, NULL);
    if (ret)
    {
#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
        log_error("RGA blit failed: %d", ret);
#else
        log_error("Error transform and write frame");
#endif
        av_frame_free(&ctx->shot->frame); // TODO обработка ошибки не срабатывает
        return -1;
    }

    if (text_t)
    { // после подготовки ctx->shot->frame
        static AVFilterGraph *graph = NULL;
        static AVFilterContext *src_ctx = NULL, *sink_ctx = NULL;

        if (!ctx->codec_ctx->hw_device_ctx)
        {
            if (av_hwdevice_ctx_create(&ctx->codec_ctx->hw_device_ctx,
                                       AV_HWDEVICE_TYPE_DRM, NULL, NULL, 0) < 0)
            {
                log_error("Failed to create hw_device_ctx");
                return -1;
            }
        }

        if (!ctx->codec_ctx->hw_frames_ctx)
        {
            AVBufferRef *frames_ref = av_hwframe_ctx_alloc(ctx->codec_ctx->hw_device_ctx);
            if (!frames_ref)
            {
                log_error("Failed to alloc hw_frames_ctx");
                return -1;
            }
            AVHWFramesContext *fc = (AVHWFramesContext *)frames_ref->data;
            fc->format = AV_PIX_FMT_DRM_PRIME;
            fc->sw_format = AV_PIX_FMT_NV12;
            fc->width = ctx->codec_ctx->width;
            fc->height = ctx->codec_ctx->height;
            if (av_hwframe_ctx_init(frames_ref) < 0)
            {
                log_error("Failed to init hw_frames_ctx");
                av_buffer_unref(&frames_ref);
                return -1;
            }
            ctx->codec_ctx->hw_frames_ctx = frames_ref;
        }

        if (!ctx->shot->frame->hw_frames_ctx)
            ctx->shot->frame->hw_frames_ctx = av_buffer_ref(ctx->codec_ctx->hw_frames_ctx);

        if (!graph)
        {
            AVBufferRef *hw_device_ref = av_buffer_ref(ctx->codec_ctx->hw_device_ctx);
            if (init_text_filter(&graph, &src_ctx, &sink_ctx,
                                 ctx->codec_ctx, "Demo Text", hw_device_ref, text_t) < 0)
            {
                log_error("init_text_filter failed");
                av_buffer_unref(&hw_device_ref);
                return -1;
            }
            av_buffer_unref(&hw_device_ref);
        }

        if (av_buffersrc_add_frame_flags(src_ctx, ctx->shot->frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
        {
            log_error("add frame to filter failed");
            return -1;
        }

        AVFrame *filt_frame = av_frame_alloc();
        int ret = av_buffersink_get_frame(sink_ctx, filt_frame);
        if (ret >= 0)
        {
            av_frame_unref(ctx->shot->frame);
            av_frame_move_ref(ctx->shot->frame, filt_frame);
        }
        else if (ret != AVERROR(EAGAIN))
        {
            log_error("failed to get filtered frame: %d", ret);
        }
        av_frame_free(&filt_frame);
    }

    if (ctx_in->frame->pts == AV_NOPTS_VALUE || ctx_in->frame->pts < 0)
    {
        log_warn("Что-то с временными метками, рассчитываю вручную. Могут быть сдвиги в временных метках.");
        ctx->shot->frame->pts = ctx->frame_count;
    }
    else
    {
        ctx->shot->frame->pts = av_rescale_q(ctx_in->frame->pts,
                                             ctx_in->c->time_base,
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