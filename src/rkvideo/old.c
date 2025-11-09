
// rkcv_t *video(rkcv_t *ctx_in,
//               const resolution_t *size,
//               const char *filename,
//               int codec,
//               int width,
//               int height,
//               int pix_fmt)
// {
//     rkcv_t *ctx = calloc(1, sizeof(rkcv_t));
//     if (unlikely(!ctx))
//     {
//         log_fatal("Ошибка: не удалось выделить память под rkcv_t");
//         return NULL;
//     }

//     if (avformat_alloc_output_context2(&ctx->format_ctx, NULL, NULL, filename) < 0)
//     {
//         log_fatal("Не удалось создать выходной формат");
//         free(ctx);
//         return NULL;
//     }

//     switch (codec)
//     {
//     case RKCodec_H264:
//         ctx->codec = avcodec_find_encoder_by_name("h264_rkmpp");
//         break;
//     case RKCodec_HEVC:
//         ctx->codec = avcodec_find_encoder_by_name("hevc_rkmpp");
//         break;
//     default:
//         ctx->codec = avcodec_find_encoder(codec);
//         break;
//     }

//     if (unlikely(!ctx->codec))
//     {
//         log_fatal("Не удалось найти кодек.");
//         avformat_free_context(ctx->format_ctx);
//         free(ctx);
//         return NULL;
//     }

//     ctx->stream = avformat_new_stream(ctx->format_ctx, NULL);
//     ctx->codec_ctx = avcodec_alloc_context3(ctx->codec);
//     if (unlikely(!ctx->stream || !ctx->codec_ctx))
//     {
//         log_fatal("Не удалось создать поток или контекст кодека.");
//         avformat_free_context(ctx->format_ctx);
//         free(ctx);
//         return NULL;
//     }

//     AVBufferRef *hw_device_ctx = NULL;
//     if (av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_RKMPP, NULL, NULL, 0) < 0)
//     {
//         log_fatal("Не удалось создать hwdevice для RKMPP");
//         avcodec_free_context(&ctx->codec_ctx);
//         avformat_free_context(ctx->format_ctx);
//         free(ctx);
//         return NULL;
//     }
//     ctx->codec_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

//     if (size)
//     {
//         ctx->codec_ctx->width = ctx->resolution.width = size->width;
//         ctx->codec_ctx->height = ctx->resolution.height = size->height;
//     }
//     else
//     {
//         ctx->codec_ctx->width = ctx->resolution.width = width ? width : ctx_in->codec_ctx->width;
//         ctx->codec_ctx->height = ctx->resolution.height = height ? height : ctx_in->codec_ctx->height;
//     }

//     ctx->codec_ctx->pix_fmt = pix_fmt ? pix_fmt : AV_PIX_FMT_NV12;
//     ctx->codec_ctx->time_base = ctx_in->stream->time_base;
//     ctx->codec_ctx->framerate = ctx_in->stream->r_frame_rate;

//     if (unlikely(avcodec_open2(ctx->codec_ctx, ctx->codec, NULL) < 0))
//     {
//         log_fatal("Не удалось открыть кодек.");
//         avcodec_free_context(&ctx->codec_ctx);
//         avformat_free_context(ctx->format_ctx);
//         free(ctx);
//         return NULL;
//     }

//     if (unlikely(avcodec_parameters_from_context(ctx->stream->codecpar, ctx->codec_ctx) < 0))
//     {
//         log_fatal("Не удалось сконфигурировать параметры потока.");
//         avcodec_free_context(&ctx->codec_ctx);
//         avformat_free_context(ctx->format_ctx);
//         free(ctx);
//         return NULL;
//     }

//     if (!(ctx->format_ctx->oformat->flags & AVFMT_NOFILE))
//     {
//         if (unlikely(avio_open(&ctx->format_ctx->pb, filename, AVIO_FLAG_WRITE) < 0))
//         {
//             log_fatal("Не удалось открыть выходной файл.");
//             avcodec_free_context(&ctx->codec_ctx);
//             avformat_free_context(ctx->format_ctx);
//             free(ctx);
//             return NULL;
//         }
//     }

//     ctx->stream->time_base = (AVRational){1, 25};
//     ctx->codec_ctx->time_base = ctx->stream->time_base;

//     if (unlikely(avformat_write_header(ctx->format_ctx, NULL) < 0))
//     {
//         log_fatal("Не удалось записать заголовки.");
//         avio_closep(&ctx->format_ctx->pb);
//         avcodec_free_context(&ctx->codec_ctx);
//         avformat_free_context(ctx->format_ctx);
//         free(ctx);
//         return NULL;
//     }

//     // ctx->frame = av_frame_alloc();
//     // ctx->frame->format = pix_fmt ? pix_fmt : AV_PIX_FMT_NV12;
//     // if (size)
//     // {
//     //     ctx->frame->width = ctx->resolution.width = size->width;
//     //     ctx->frame->height = ctx->resolution.height = size->height;
//     // }
//     // else
//     // {
//     //     ctx->frame->width = ctx->resolution.width = width ? width : ctx_in->codec_ctx->width;
//     //     ctx->frame->height = ctx->resolution.height = height ? height : ctx_in->codec_ctx->height;
//     // }

//     // if (av_frame_get_buffer(ctx->frame, 32) < 0)
//     // {
//     //     // std::cerr << "alloc_frame_nv12: av_frame_get_buffer failed\n";
//     //     // return -1;
//     // }
//     // if (av_frame_make_writable(ctx->frame) < 0)
//     // {
//     //     // std::cerr << "alloc_frame_nv12: frame not writable\n";
//     //     // return -1;
//     // }
//     AVBufferRef *hw_frames_ref = NULL;
//     AVHWFramesContext *frames_ctx = NULL;

//     hw_frames_ref = av_hwframe_ctx_alloc(ctx->codec_ctx->hw_device_ctx);
//     frames_ctx = (AVHWFramesContext *)(hw_frames_ref->data);
//     frames_ctx->format = AV_PIX_FMT_DRM_PRIME; // аппаратный формат
//     frames_ctx->sw_format = AV_PIX_FMT_NV12;   // формат в CPU памяти
//     frames_ctx->width = ctx->codec_ctx->width;
//     frames_ctx->height = ctx->codec_ctx->height;
//     frames_ctx->initial_pool_size = 4;

//     if (av_hwframe_ctx_init(hw_frames_ref) < 0)
//     {
//         log_fatal("Не удалось инициализировать hwframe context");
//     }

//     ctx->codec_ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);

//     // Теперь можно выделять фреймы в DRM-памяти:
//     ctx->frame = av_frame_alloc();
//     ctx->frame->format = AV_PIX_FMT_DRM_PRIME;
//     ctx->frame->width = ctx->codec_ctx->width;
//     ctx->frame->height = ctx->codec_ctx->height;

//     if (av_hwframe_get_buffer(hw_frames_ref, ctx->frame, 0) < 0)
//     {
//         log_fatal("Не удалось выделить hwframe");
//     }

//     ctx->frame_count = 0;

//     log_debug("Создан выходной поток для файла: %s", filename);
//     return ctx;
// }

static int alloc_frame_nv12(AVFrame *f, int w, int h)
{
    av_frame_unref(f);
    f->format = AV_PIX_FMT_NV12;
    f->width = w;
    f->height = h;
    // выравнивание 32 байта безопасно для RGA/NEON
    if (av_frame_get_buffer(f, 32) < 0)
    {
        // std::cerr << "alloc_frame_nv12: av_frame_get_buffer failed\n";
        return -1;
    }
    if (av_frame_make_writable(f) < 0)
    {
        // std::cerr << "alloc_frame_nv12: frame not writable\n";
        return -1;
    }
    return 0;
}
// int imw(rkcv_t *ctx, rkcv_t *ctx_in)
// {
//     // static bool out_ready = false;
//     // if (!out_ready)
//     // {
//     //     if (alloc_frame_nv12(ctx->frame, ctx->frame->width, ctx->frame->height) < 0)
//     //         // goto stop_recording;
//     //         printf("ERR\n");
//     //     out_ready = true;
//     // }

//     AVFrame *src_for_rga = ctx_in->frame;
//     AVFrame *sys_copy = NULL;

//     if (ctx->frame->format != AV_PIX_FMT_DRM_PRIME)
//     {
//         fprintf(stderr, "imw: expected DRM_PRIME input\n");
//         return -1;
//     }

//     if (ctx_in->frame->format == AV_PIX_FMT_DRM_PRIME)
//     {
//         // sys_copy = hw_to_sys_nv12(ctx_in->frame);
//         // if (!sys_copy)
//         // {
//         //     // std::cerr << "hw_to_sys_nv12 failed\n";
//         //     // goto stop_recording;
//         // }
//         // src_for_rga = sys_copy;
//     }

//     else if (ctx_in->frame->format != AV_PIX_FMT_NV12)
//     {
//         // На всякий случай: приведи к NV12 софтом (редко нужно с rkmpp)
//         AVFrame *tmp = av_frame_alloc();

//         // простой fallback через sws (создай sws_ctx_nv12 заранее один раз)
//         struct SwsContext *sws_ctx_nv12 = NULL;
//         if (!sws_ctx_nv12)
//         {
//             sws_ctx_nv12 = sws_getContext(ctx->frame->width, ctx->frame->height, (enum AVPixelFormat)ctx->frame->format,
//                                           ctx->frame->width, ctx->frame->height, AV_PIX_FMT_NV12,
//                                           SWS_BILINEAR, NULL, NULL, NULL);
//             if (!sws_ctx_nv12)
//             {
//                 av_frame_free(&tmp);
//                 // goto stop_recording;
//             }
//         }
//         uint8_t *dst_data[4] = {tmp->data[0], tmp->data[1], NULL, NULL};
//         int dst_lines[4] = {tmp->linesize[0], tmp->linesize[1], 0, 0};
//         sws_scale(sws_ctx_nv12, ctx->frame->data, ctx->frame->linesize, 0, ctx->frame->height, dst_data, dst_lines);

//         sys_copy = tmp;
//         src_for_rga = sys_copy;
//     }

//     // Масштабирование RGA: NV12(sysmem) -> NV12(frame_out 1280x720)
//     if (scale_with_rga_nv12_sysmem(src_for_rga, ctx->frame, ctx->frame->width, ctx->frame->height) < 0)
//     {
//         // std::cerr << "RGA scale failed, abort\n";
//         if (sys_copy)
//             av_frame_free(&sys_copy);
//         // goto stop_recording;
//     }
//     if (scale_with_rga_fd_blit(ctx->frame, int fd_out,
//                                int dst_w, int dst_h))

//         // PTS/таймстемпы
//         ctx->frame->pts = ctx->frame_count++;

//     // в кодер идёт frame_out (NV12 1280x720)
//     if (avcodec_send_frame(ctx->codec_ctx, ctx->frame) < 0)
//     {
//         // std::cerr << "send_frame failed\n";
//         if (sys_copy)
//             av_frame_free(&sys_copy);
//         // goto stop_recording;
//     }

//     AVPacket outpkt;
//     av_init_packet(&outpkt);
//     while (avcodec_receive_packet(ctx->codec_ctx, &outpkt) >= 0)
//     {
//         av_packet_rescale_ts(&outpkt, ctx->codec_ctx->time_base, ctx->stream->time_base);
//         outpkt.stream_index = ctx->stream->index;
//         av_write_frame(ctx->format_ctx, &outpkt);
//         av_packet_unref(&outpkt);
//     }
// }

static int scale_with_rga_nv12_sysmem(AVFrame *src, AVFrame *dst,
                                      int dst_w, int dst_h)
{
    rga_info_t src_info, dst_info;
    memset(&src_info, 0, sizeof(src_info));
    memset(&dst_info, 0, sizeof(dst_info));

    // Проверка
    if (!src || !dst || !src->data[0] || !src->data[1] || !dst->data[0] || !dst->data[1])
    {
        // std::cerr << "scale_with_rga_nv12_sysmem: invalid AVFrame pointers\n";
        return -1;
    }

    // Размеры исходного кадра
    int src_w = src->width;
    int src_h = src->height;
    int dst_w_aligned = (dst_w + 15) & ~15; // выравнивание 16 байт
    int dst_h_aligned = (dst_h + 7) & ~7;

    // Собираем временный NV12 буфер
    size_t y_size = src->linesize[0] * src_h;
    size_t uv_size = src->linesize[1] * (src_h / 2);
    size_t total_size = y_size + uv_size;

    uint8_t *src_nv12 = NULL;
    if (posix_memalign((void **)&src_nv12, 16, total_size) != 0)
    {
        // std::cerr << "posix_memalign failed\n";
        return -1;
    }
    memcpy(src_nv12, src->data[0], y_size);
    memcpy(src_nv12 + y_size, src->data[1], uv_size);

    // Аналогично для dst — создаём плоский NV12 буфер
    size_t dst_y_size = dst->linesize[0] * dst_h;
    size_t dst_uv_size = dst->linesize[1] * (dst_h / 2);
    size_t dst_total = dst_y_size + dst_uv_size;

    uint8_t *dst_nv12 = NULL;
    if (posix_memalign((void **)&dst_nv12, 16, dst_total) != 0)
    {
        free(src_nv12);
        // std::cerr << "posix_memalign dst failed\n";
        return -1;
    }

    // Настройка структур RGA
    // src_info.virAddr = src_nv12;
    src_info.virAddr = src_nv12;
    dst_info.virAddr = dst_nv12;
    src_info.mmuFlag = 1;
    dst_info.mmuFlag = 1;

    rga_set_rect(&src_info.rect,
                 0, 0,
                 src->width, src->height,
                 src->width, src->height,
                 RK_FORMAT_YCbCr_420_SP);

    rga_set_rect(&dst_info.rect,
                 0, 0,
                 dst_w, dst_h,
                 dst_w, dst_h,
                 RK_FORMAT_YCbCr_420_SP);

    int ret = c_RkRgaBlit(&src_info, &dst_info, NULL);
    if (ret)
    {
        // std::cerr << "RGA blit failed: " << ret << std::endl;
        free(src_nv12);
        free(dst_nv12);
        return -1;
    }

    // Разделяем плоский dst обратно на две плоскости
    memcpy(dst->data[0], dst_nv12, dst_y_size);
    memcpy(dst->data[1], dst_nv12 + dst_y_size, dst_uv_size);

    free(src_nv12);
    free(dst_nv12);
    return 0;
}

static int scale_with_rga_fd_blit(const AVFrame *src, int fd_out,
                                  int dst_w, int dst_h)
{
    if (!src || src->format != AV_PIX_FMT_DRM_PRIME)
    {
        fprintf(stderr, "scale_with_rga_fd_blit: source must be DRM_PRIME\n");
        return -1;
    }

    const AVDRMFrameDescriptor *desc = (const AVDRMFrameDescriptor *)src->data[0];
    if (!desc || desc->nb_objects < 1)
    {
        fprintf(stderr, "scale_with_rga_fd_blit: invalid AVDRMFrameDescriptor\n");
        return -1;
    }

    int fd_in = desc->objects[0].fd;
    int src_w = src->width;
    int src_h = src->height;

    // --- исходный буфер ---
    rga_info_t src_info;
    memset(&src_info, 0, sizeof(src_info));
    src_info.fd = fd_in;
    src_info.mmuFlag = 1;
    rga_set_rect(&src_info.rect,
                 0, 0, src_w, src_h, src_w, src_h,
                 RK_FORMAT_YCbCr_420_SP);

    // --- целевой буфер ---
    rga_info_t dst_info;
    memset(&dst_info, 0, sizeof(dst_info));
    dst_info.fd = fd_out;
    dst_info.mmuFlag = 1;
    rga_set_rect(&dst_info.rect,
                 0, 0, dst_w, dst_h, dst_w, dst_h,
                 RK_FORMAT_YCbCr_420_SP);

    // --- вызов RGA ---
    int ret = c_RkRgaBlit(&src_info, &dst_info, NULL);
    if (ret)
    {
        fprintf(stderr, "RGA blit failed: %d\n", ret);
        return -1;
    }

    return 0;
}

static AVFrame *hw_to_sys_nv12(AVFrame *hw)
{
    // ожидаем DRM_PRIME с NV12 внутри; сделаем системную копию
    AVFrame *sys = av_frame_alloc();
    if (!sys)
        return NULL;

    sys->format = AV_PIX_FMT_NV12;
    sys->width = hw->width;
    sys->height = hw->height;
    if (av_frame_get_buffer(sys, 32) < 0)
    {
        av_frame_free(&sys);
        // std::cerr << "hw_to_sys_nv12: av_frame_get_buffer failed\n";
        return NULL;
    }
    if (av_hwframe_transfer_data(sys, hw, 0) < 0)
    {
        av_frame_free(&sys);
        // std::cerr << "av_hwframe_transfer_data failed\n";
        return NULL;
    }
    return sys;
}

/*

typedef struct
{
    _Bool rga_init;
    int rga_fmt;
    rga_info_t s;
    rga_info_t d;
    const AVDRMFrameDescriptor *desc_in;
    const AVDRMFrameDescriptor *desc_out;
} rga_rect;

typedef struct
{
    int rot;
    int flip;
    resolution_t size;
    rga_info_t s;
    rga_info_t d;
    _Bool rga_init;
    int rga_fmt;
    const AVDRMFrameDescriptor *desc_in;
    const AVDRMFrameDescriptor *desc_out;
} s_convert_f;

typedef struct
{
    int rot;
    int flip;
    resolution_t size;
} s_convert_f;

typedef struct
{
    char *source;
    AVFormatContext *format_ctx;
    int video_stream_index;
    AVStream *stream;
    AVCodecContext *codec_ctx;
    const AVCodec *codec;
    AVPacket *packet;
    AVFrame *frame;
    int max_stream_v;
    AVDictionary *opts;
    _Bool check_rtsp;
    resolution_t resolution;
    int frame_count;
    int fps;
    const AVDRMFrameDescriptor *desc_in;
    const AVDRMFrameDescriptor *desc_out;
    rga_rect rect;
} rkcv_t;
*/
// typedef struct
// {
//     int rot;
//     int flip;
//     resolution_t size;
//     rga_info_t s;
//     rga_info_t d;
//     _Bool rga_init;
//     int rga_fmt;
//     const AVDRMFrameDescriptor *desc_in;
//     const AVDRMFrameDescriptor *desc_out;
// } s_convert_f;
