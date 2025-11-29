#ifndef _RKVIDEO_H
#define _RKVIDEO_H

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/opt.h>
#include <stddef.h>

#include "font32.h"

#include "rkutils.h"
#include "rkdrm.h"
#include "rkframe.h"
#include "rkdraw.h"

#ifdef RKRGALIB_ENABLE
// #include <rga/RgaApi.h>
// #include <rga/RgaUtils.h>
#else
// #include "NormalRga.h"
#endif
#include "RgaApi.h"

// TODO сделать в коде вывод списка стримов нормально
// TODO написать выбор стрима в видео файле, если их несколько

// #define RK_TRANSFORM_ROT_0 0x00000000
// #define RK_TRANSFORM_ROT_90 HAL_TRANSFORM_ROT_90
// #define RK_TRANSFORM_ROT_180 HAL_TRANSFORM_ROT_180
// #define RK_TRANSFORM_ROT_270 HAL_TRANSFORM_ROT_270

// #define RK_TRANSFORM_FLIP_MASK 0x00000003
// #define RK_TRANSFORM_FLIP_H HAL_TRANSFORM_FLIP_H
// #define RK_TRANSFORM_FLIP_V HAL_TRANSFORM_FLIP_V

enum RKCodec
{
    /* video codecs */
    RKCodec_MPEG1VIDEO = 1,
    RKCodec_MPEG2VIDEO,
    RKCodec_H261,
    RKCodec_H263,
    RKCodec_RV10,
    RKCodec_RV20,
    RKCodec_MJPEG,
    RKCodec_MJPEGB,
    RKCodec_LJPEG,
    RKCodec_SP5X,
    RKCodec_JPEGLS,
    RKCodec_MPEG4,
    RKCodec_RAWVIDEO,
    RKCodec_MSMPEG4V1,
    RKCodec_MSMPEG4V2,
    RKCodec_MSMPEG4V3,
    RKCodec_WMV1,
    RKCodec_WMV2,
    RKCodec_H263P,
    RKCodec_H263I,
    RKCodec_FLV1,
    RKCodec_SVQ1,
    RKCodec_SVQ3,
    RKCodec_DVVIDEO,
    RKCodec_HUFFYUV,
    RKCodec_CYUV,
    RKCodec_H264,
    RKCodec_INDEO3,
    RKCodec_VP3,
    RKCodec_THEORA,
    RKCodec_ASV1,
    RKCodec_ASV2,
    RKCodec_FFV1,
    RKCodec_4XM,
    RKCodec_VCR1,
    RKCodec_CLJR,
    RKCodec_MDEC,
    RKCodec_ROQ,
    RKCodec_INTERPLAY_VIDEO,
    RKCodec_XAN_WC3,
    RKCodec_XAN_WC4,
    RKCodec_RPZA,
    RKCodec_CINEPAK,
    RKCodec_WS_VQA,
    RKCodec_MSRLE,
    RKCodec_MSVIDEO1,
    RKCodec_IDCIN,
    RKCodec_8BPS,
    RKCodec_SMC,
    RKCodec_FLIC,
    RKCodec_TRUEMOTION1,
    RKCodec_VMDVIDEO,
    RKCodec_MSZH,
    RKCodec_ZLIB,
    RKCodec_QTRLE,
    RKCodec_TSCC,
    RKCodec_ULTI,
    RKCodec_QDRAW,
    RKCodec_VIXL,
    RKCodec_QPEG,
    RKCodec_PNG,
    RKCodec_PPM,
    RKCodec_PBM,
    RKCodec_PGM,
    RKCodec_PGMYUV,
    RKCodec_PAM,
    RKCodec_FFVHUFF,
    RKCodec_RV30,
    RKCodec_RV40,
    RKCodec_VC1,
    RKCodec_WMV3,
    RKCodec_LOCO,
    RKCodec_WNV1,
    RKCodec_AASC,
    RKCodec_INDEO2,
    RKCodec_FRAPS,
    RKCodec_TRUEMOTION2,
    RKCodec_BMP,
    RKCodec_CSCD,
    RKCodec_MMVIDEO,
    RKCodec_ZMBV,
    RKCodec_AVS,
    RKCodec_SMACKVIDEO,
    RKCodec_NUV,
    RKCodec_KMVC,
    RKCodec_FLASHSV,
    RKCodec_CAVS,
    RKCodec_JPEG2000,
    RKCodec_VMNC,
    RKCodec_VP5,
    RKCodec_VP6,
    RKCodec_VP6F,
    RKCodec_TARGA,
    RKCodec_DSICINVIDEO,
    RKCodec_TIERTEXSEQVIDEO,
    RKCodec_TIFF,
    RKCodec_GIF,
    RKCodec_DXA,
    RKCodec_DNXHD,
    RKCodec_THP,
    RKCodec_SGI,
    RKCodec_C93,
    RKCodec_BETHSOFTVID,
    RKCodec_PTX,
    RKCodec_TXD,
    RKCodec_VP6A,
    RKCodec_AMV,
    RKCodec_VB,
    RKCodec_PCX,
    RKCodec_SUNRAST,
    RKCodec_INDEO4,
    RKCodec_INDEO5,
    RKCodec_MIMIC,
    RKCodec_RL2,
    RKCodec_ESCAPE124,
    RKCodec_DIRAC,
    RKCodec_BFI,
    RKCodec_CMV,
    RKCodec_MOTIONPIXELS,
    RKCodec_TGV,
    RKCodec_TGQ,
    RKCodec_TQI,
    RKCodec_AURA,
    RKCodec_AURA2,
    RKCodec_V210X,
    RKCodec_TMV,
    RKCodec_V210,
    RKCodec_DPX,
    RKCodec_MAD,
    RKCodec_FRWU,
    RKCodec_FLASHSV2,
    RKCodec_CDGRAPHICS,
    RKCodec_R210,
    RKCodec_ANM,
    RKCodec_BINKVIDEO,
    RKCodec_IFF_ILBM,
#define RKCodec_IFF_BYTERUN1 RKCodec_IFF_ILBM
    RKCodec_KGV1,
    RKCodec_YOP,
    RKCodec_VP8,
    RKCodec_PICTOR,
    RKCodec_ANSI,
    RKCodec_A64_MULTI,
    RKCodec_A64_MULTI5,
    RKCodec_R10K,
    RKCodec_MXPEG,
    RKCodec_LAGARITH,
    RKCodec_PRORES,
    RKCodec_JV,
    RKCodec_DFA,
    RKCodec_WMV3IMAGE,
    RKCodec_VC1IMAGE,
    RKCodec_UTVIDEO,
    RKCodec_BMV_VIDEO,
    RKCodec_VBLE,
    RKCodec_DXTORY,
    RKCodec_V410,
    RKCodec_XWD,
    RKCodec_CDXL,
    RKCodec_XBM,
    RKCodec_ZEROCODEC,
    RKCodec_MSS1,
    RKCodec_MSA1,
    RKCodec_TSCC2,
    RKCodec_MTS2,
    RKCodec_CLLC,
    RKCodec_MSS2,
    RKCodec_VP9,
    RKCodec_AIC,
    RKCodec_ESCAPE130,
    RKCodec_G2M,
    RKCodec_WEBP,
    RKCodec_HNM4_VIDEO,
    RKCodec_HEVC,
#define RKCodec_H265 RKCodec_HEVC
    RKCodec_FIC,
    RKCodec_ALIAS_PIX,
    RKCodec_BRENDER_PIX,
    RKCodec_PAF_VIDEO,
    RKCodec_EXR,
    RKCodec_VP7,
    RKCodec_SANM,
    RKCodec_SGIRLE,
    RKCodec_MVC1,
    RKCodec_MVC2,
    RKCodec_HQX,
    RKCodec_TDSC,
    RKCodec_HQ_HQA,
    RKCodec_HAP,
    RKCodec_DDS,
    RKCodec_DXV,
    RKCodec_SCREENPRESSO,
    RKCodec_RSCC,
    RKCodec_AVS2,
    RKCodec_PGX,
    RKCodec_AVS3,
    RKCodec_MSP2,
    RKCodec_VVC,
#define RKCodec_H266 RKCodec_VVC
    RKCodec_Y41P,
    RKCodec_AVRP,
    RKCodec_012V,
    RKCodec_AVUI,
#if FF_API_AYUV_CODECID
    RKCodec_AYUV,
#endif
    RKCodec_TARGA_Y216,
    RKCodec_V308,
    RKCodec_V408,
    RKCodec_YUV4,
    RKCodec_AVRN,
    RKCodec_CPIA,
    RKCodec_XFACE,
    RKCodec_SNOW,
    RKCodec_SMVJPEG,
    RKCodec_APNG,
    RKCodec_DAALA,
    RKCodec_CFHD,
    RKCodec_TRUEMOTION2RT,
    RKCodec_M101,
    RKCodec_MAGICYUV,
    RKCodec_SHEERVIDEO,
    RKCodec_YLC,
    RKCodec_PSD,
    RKCodec_PIXLET,
    RKCodec_SPEEDHQ,
    RKCodec_FMVC,
    RKCodec_SCPR,
    RKCodec_CLEARVIDEO,
    RKCodec_XPM,
    RKCodec_AV1,
    RKCodec_BITPACKED,
    RKCodec_MSCC,
    RKCodec_SRGC,
    RKCodec_SVG,
    RKCodec_GDV,
    RKCodec_FITS,
    RKCodec_IMM4,
    RKCodec_PROSUMER,
    RKCodec_MWSC,
    RKCodec_WCMV,
    RKCodec_RASC,
    RKCodec_HYMT,
    RKCodec_ARBC,
    RKCodec_AGM,
    RKCodec_LSCR,
    RKCodec_VP4,
    RKCodec_IMM5,
    RKCodec_MVDV,
    RKCodec_MVHA,
    RKCodec_CDTOONS,
    RKCodec_MV30,
    RKCodec_NOTCHLC,
    RKCodec_PFM,
    RKCodec_MOBICLIP,
    RKCodec_PHOTOCD,
    RKCodec_IPU,
    RKCodec_ARGO,
    RKCodec_CRI,
    RKCodec_SIMBIOSIS_IMX,
    RKCodec_SGA_VIDEO,
    RKCodec_GEM,
    RKCodec_VBN,
    RKCodec_JPEGXL,
    RKCodec_QOI,
    RKCodec_PHM,
    RKCodec_RADIANCE_HDR,
    RKCodec_WBMP,
    RKCodec_MEDIA100,
    RKCodec_VQC,
    RKCodec_PDV,
    RKCodec_EVC,
    RKCodec_RTV1,
    RKCodec_VMIX,
};

typedef struct
{
    _Bool rga_init;
    int rga_fmt;
    rga_info_t s;
#if USE_DRM_BUFFER
    const AVDRMFrameDescriptor *desc_in;
    const AVDRMFrameDescriptor *desc_out;
#endif
} rga_rect;

typedef struct __attribute__((aligned(64)))
{
    int8_t type_source;
    AVFormatContext *format_ctx;
    uint8_t video_stream_index;
    AVStream *stream;
    AVCodecContext *codec_ctx;
    const AVCodec *codec;
    AVPacket *packet;
    AVDictionary *opts;
    int frame_count;
    int fps;
    rkcv_shot_t *shot;
} rkcv_t;

rkcv_t *openv(char *source);
int readf(rkcv_t *ctx);
rkcv_t *video(rkcv_t *ctx_in, const char *filename, int codec, info_frame_t *frame_t);
__attribute__((deprecated("imw: тестовая неотлаженная функция при передаче параметра наложения текста, использовать с осторожностью"))) int imw(rkcv_t *restrict ctx, rkcv_shot_t *restrict ctx_in, s_text_f *text_t);
int realese(rkcv_t *ctx);

__attribute__((__always_inline__, cold)) inline static void printf_streams(rkcv_t *ctx)
{
    // TODO нестабильный вывод fps, надо проверить
    if (!ctx || !ctx->format_ctx)
    {
#if RKLOG_ENABLE
        fprintf(stderr, "Неверный контекст FFmpeg\n");
#endif
        exit(1);
    }

    printf("=== Список потоков ===\n");
    for (unsigned i = 0; i < ctx->format_ctx->nb_streams; i++)
    {
        AVStream *st = ctx->format_ctx->streams[i];
        AVCodecParameters *par = st->codecpar;

        switch (par->codec_type)
        {
        case AVMEDIA_TYPE_VIDEO:
        {
            const AVCodec *codec = avcodec_find_decoder(par->codec_id);
            printf("  #%u: VIDEO  | %s | %dx%d | %.2f fps\n",
                   i,
                   codec ? codec->name : "unknown",
                   par->width,
                   par->height,
                   st->avg_frame_rate.den && st->avg_frame_rate.num
                       ? av_q2d(st->codecpar->framerate)
                       : 0.0);
            break;
        }
        case AVMEDIA_TYPE_AUDIO:
        {
            const AVCodec *codec = avcodec_find_decoder(par->codec_id);
            printf("  #%u: AUDIO  | %s | %d Hz \n",
                   i,
                   codec ? codec->name : "unknown",
                   par->sample_rate);
            break;
        }
        default:
            printf("  #%u: OTHER  | type=%d\n", i, par->codec_type);
            break;
        }
    }

    printf("=======================\n");
}

__attribute__((deprecated("imw: тестовая неотлаженная функция при передаче параметра наложения текста, использовать с осторожностью")))
__attribute__((hot, flatten)) static int
imws(rkcv_t *restrict ctx, rkcv_shot_t *restrict ctx_in, s_text_f *text_t)
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
        ctx->shot->frame->pts = AV_NOPTS_VALUE;
        ctx->shot->frame->pkt_dts = AV_NOPTS_VALUE;
        ctx->shot->frame->best_effort_timestamp = AV_NOPTS_VALUE;
        ctx->shot->frame->duration = 0;
        ctx->shot->frame->opaque = NULL;
    }

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
    ctx_in->c->s.mmuFlag = 1;
    ctx->shot->c->s.fd = fd_out;
    ctx->shot->c->s.mmuFlag = 1;

    int src_y_stride, src_hstride, dst_y_stride, dst_hstride;
    if (ctx_in->frame->format == AV_PIX_FMT_DRM_PRIME)
    {
        ctx_in->c->desc = (const AVDRMFrameDescriptor *)ctx_in->frame->data[0];
        ctx->shot->c->desc = (const AVDRMFrameDescriptor *)ctx->shot->frame->data[0];

        if (ctx_in->c->desc->layers[0].planes[1].pitch)
        {
            // YUV/NV12/NV21
            src_y_stride = ctx_in->c->desc->layers[0].planes[0].pitch;
            src_hstride = ctx_in->c->desc->layers[0].planes[1].offset / src_y_stride;
            dst_y_stride = ctx->shot->c->desc->layers[0].planes[0].pitch;
            dst_hstride = ctx->shot->c->desc->layers[0].planes[1].offset / dst_y_stride;
        }
        else
        {
            // RGB
            src_y_stride = ctx_in->c->desc->layers[0].planes[0].pitch;
            src_hstride = ctx_in->frame->height; // RGB одна плоскость
            dst_y_stride = ctx->shot->c->desc->layers[0].planes[0].pitch;
            dst_hstride = ctx->shot->frame->height;
        }
    }

    if (!ctx->shot->c->rga_init)
    {
        if (ctx->shot->c->desc->layers[0].planes[1].pitch)
            ctx->shot->c->frame_t->fmt = RK_PIX_FMT_YCbCr_420_SP;
        else
            ctx->shot->c->frame_t->fmt = RK_PIX_FMT_RGBA_8888;
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

    if (text_t)
    {
#ifdef USE_CPU_DRAW_TEXT
        // overlay_text_nv12_im2d(fd_out, ctx->shot->frame->width, ctx->shot->frame->height, "HELLO");

        // overlay_text_nv12_rga(drmprime_fd_from_frame(ctx->shot->frame), ctx->shot->frame->width, ctx->shot->frame->height, "ABC");
        // draw_text_rga(drmprime_fd_from_frame(ctx->shot->frame), ctx->shot->frame->width, ctx->shot->frame->height, 0, 0, "ABC");
        const int W = ctx->codec_ctx->width;
        const int H = ctx->codec_ctx->height;

        // 1) DRM_PRIME -> SW (NV12), получаем кадр с произвольными linesize
        AVFrame *sw = av_frame_alloc();
        if (!sw)
            return -1;
        sw->format = convert_pix_fmt(ctx->shot->c->frame_t->fmt, 0);
        sw->width = W;
        sw->height = H;

        if (av_hwframe_transfer_data(sw, ctx->shot->frame, 0) < 0)
        {
            av_frame_free(&sw);
            return -1;
        }
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

#endif