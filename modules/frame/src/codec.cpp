#include "codec.h"

const AVCodec* get_codec_by_id_rkmpp(AVCodecID id) {
    switch (id){
        case AV_CODEC_ID_H264:
        return avcodec_find_decoder_by_name("h264_rkmpp");
        break;
    case AV_CODEC_ID_HEVC:
        return avcodec_find_decoder_by_name("hevc_rkmpp");
        break;
    case AV_CODEC_ID_AV1:
        return avcodec_find_decoder_by_name("av1_rkmpp");
        break;
    case AV_CODEC_ID_H263:
        return avcodec_find_decoder_by_name("h263_rkmpp");
        break;
    case AV_CODEC_ID_MPEG1VIDEO:
        return avcodec_find_decoder_by_name("mpeg1_rkmpp");
        break;
    case AV_CODEC_ID_MPEG2VIDEO:
        return avcodec_find_decoder_by_name("mpeg2_rkmpp");
        break;
    case AV_CODEC_ID_MPEG4:
        return avcodec_find_decoder_by_name("mpeg4_rkmpp");
        break;
    case AV_CODEC_ID_VP8:
        return avcodec_find_decoder_by_name("vp8_rkmpp");
        break;
    case AV_CODEC_ID_VP9:
        return avcodec_find_decoder_by_name("vp9_rkmpp");
        break;
    default:
        return avcodec_find_decoder(id);
        break;
    }
}
