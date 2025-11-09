#ifndef _RKVIDEO_H
#define _RKVIDEO_H

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include "logs.h"
#include "rkdrm.h"
#include "rkframe.h"
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/opt.h>

#include <rga/RgaApi.h>
#include <rga/RgaUtils.h>
#include <CL/cl.h>

// TODO сделать в коде вывод списка стримов нормально
// TODO написать выбор стрима в видео файле, если их несколько

#define RK_TRANSFORM_ROT_0 0x00000000
#define RK_TRANSFORM_ROT_90 HAL_TRANSFORM_ROT_90
#define RK_TRANSFORM_ROT_180 HAL_TRANSFORM_ROT_180
#define RK_TRANSFORM_ROT_270 HAL_TRANSFORM_ROT_270

#define RK_TRANSFORM_FLIP_MASK 0x00000003
#define RK_TRANSFORM_FLIP_H HAL_TRANSFORM_FLIP_H
#define RK_TRANSFORM_FLIP_V HAL_TRANSFORM_FLIP_V

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
    const AVDRMFrameDescriptor *desc_in;
    const AVDRMFrameDescriptor *desc_out;
} rga_rect;

typedef struct __attribute__((aligned(64)))
{
    char *source;
    _Bool check_rtsp;
    AVFormatContext *format_ctx;
    uint8_t video_stream_index;
    AVStream *stream;
    AVCodecContext *codec_ctx;
    const AVCodec *codec;
    AVPacket *packet;
    // AVFrame *frame;
    // int max_stream_v;
    AVDictionary *opts;
    int frame_count;
    int fps;
    rkcv_shot_t *shot;
} rkcv_t;

rkcv_t *openv(char *source);
int readf(rkcv_t *ctx);
// rkcv_t *video(rkcv_t *ctx_in, const char *filename, int codec, int width, int height, rk_pix_fmt_t pix_fmt);
rkcv_t *video(rkcv_t *ctx_in, const char *filename, int codec, info_frame_t *frame_t);
// __attribute__((hot, flatten)) int imw(rkcv_t *restrict ctx, rkcv_shot_t *restrict ctx_in);
__attribute__((hot, flatten, warning("imw имеет тестовый вызов функции отображения текста на кадре"))) int imw(rkcv_t *restrict ctx, rkcv_shot_t *restrict ctx_in, s_text_f *text_t);
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

#endif