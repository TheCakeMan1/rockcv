#include "rkimage.h"

int ims(rkcv_t *ctx, const char *filename)
{
    // TODO разбраться с выбором качества
    // TODO дописать логи и проверить логику

#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
    log_debug("Создание изображения.");
#endif
    const char *codec_name = get_codec_image(filename);
    const AVCodec *codec = avcodec_find_encoder_by_name(codec_name);

    if (!codec)
    {
        log_fatal("Кодек %s не найден", codec_name);
        return -1;
        // fprintf(stderr, "⚠️ Кодек %s не найден, пробуем mjpeg (CPU)\n", codec_name);
        // codec = avcodec_find_encoder_by_name("mjpeg");
        // if (!codec)
        // {
        //     fprintf(stderr, "❌ Ни mjpeg_rkmpp, ни mjpeg не найдены!\n");
        //     return -1;
        // }
        // codec_name = "mjpeg";
    }

#if defined(RKLOG_ENABLE) && defined(BUILD_DEV)
    log_debug("Используем кодек: %s.", codec_name);
#endif

    AVCodecContext *c = avcodec_alloc_context3(codec);
    if (!c)
        return -1;

    c->width = ctx->shot->frame->width;
    c->height = ctx->shot->frame->height;
    c->time_base = (AVRational){1, 25};
    c->framerate = (AVRational){25, 1};
    c->bit_rate = 1000000;
    // av_dict_set_int(c->priv_data, "qp", 20, 0);     // не во всех сборках
    // av_dict_set(c->priv_data, "quality", "100", 0); // не гарантируется

    // c->color_range = AVCOL_RANGE_JPEG;
    // c->color_primaries = AVCOL_PRI_BT709;
    // c->color_trc = AVCOL_TRC_BT709;
    // c->colorspace = AVCOL_SPC_BT709;

    // c->pix_fmt = AV_PIX_FMT_RGB0;
    c->pix_fmt = get_codec_frame(codec_name);

    if (avcodec_open2(c, codec, NULL) < 0)
    {
        fprintf(stderr, "❌ Не удалось открыть кодек %s\n", codec->name);
        avcodec_free_context(&c);
        return -1;
    }

    // Конвертация входного кадра в формат кодека
    struct SwsContext *sws_ctx = sws_getContext(
        ctx->shot->frame->width, ctx->shot->frame->height, (enum AVPixelFormat)ctx->shot->frame->format,
        ctx->shot->frame->width, ctx->shot->frame->height, c->pix_fmt,
        SWS_BILINEAR, NULL, NULL, NULL);

    if (!sws_ctx)
    {
        fprintf(stderr, "❌ Ошибка sws_getContext()\n");
        avcodec_free_context(&c);
        return -1;
    }

    AVFrame *out_image = av_frame_alloc();
    out_image->format = c->pix_fmt;
    out_image->width = c->width;
    out_image->height = c->height;

    if (av_frame_get_buffer(out_image, 128) < 0)
    {
        fprintf(stderr, "❌ Ошибка av_frame_get_buffer()\n");
        sws_freeContext(sws_ctx);
        av_frame_free(&out_image);
        avcodec_free_context(&c);
        return -1;
    }

    if (av_frame_make_writable(out_image) < 0)
    {
        fprintf(stderr, "❌ Ошибка av_frame_make_writable()\n");
        goto cleanup;
    }

    // Конвертация цветов
    int scaled = sws_scale(
        sws_ctx,
        (const uint8_t *const *)ctx->shot->frame->data, ctx->shot->frame->linesize,
        0, ctx->shot->frame->height,
        out_image->data, out_image->linesize);

    if (scaled <= 0)
    {
        fprintf(stderr, "❌ Ошибка sws_scale()\n");
        goto cleanup;
    }

    // Кодирование
    AVPacket *pkt = av_packet_alloc();
    if (!pkt)
        goto cleanup;

    int ret = avcodec_send_frame(c, out_image);
    if (ret < 0)
    {
        fprintf(stderr, "Ошибка отправки кадра: %s\n", av_err2str(ret));
        goto cleanup;
    }

    // 🔧 Flush для RKMpp
    avcodec_send_frame(c, NULL);

    int tries = 100;
    while (tries--)
    {
        ret = avcodec_receive_packet(c, pkt);
        if (ret == AVERROR(EAGAIN))
        {
            usleep(2000);
            continue;
        }
        else if (ret == AVERROR_EOF)
            break;
        else if (ret < 0)
        {
            fprintf(stderr, "Ошибка при получении пакета: %s\n", av_err2str(ret));
            break;
        }

        FILE *f = fopen(filename, "wb");
        if (f)
        {
            fwrite(pkt->data, 1, pkt->size, f);
            fclose(f);
            printf("✅ Сохранено изображение: %s (%d байт)\n", filename, pkt->size);
        }
        else
        {
            perror("fopen");
        }

        av_packet_unref(pkt);
        break;
    }

cleanup:
    av_packet_free(&pkt);
    av_frame_free(&out_image);
    sws_freeContext(sws_ctx);
    avcodec_free_context(&c);
    return 0;
}