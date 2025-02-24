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


const AVCodec* get_codec_by_name_rkmpp(const char* name) {
    if (strcmp(name, "h264") == 0){
        return avcodec_find_encoder_by_name("h264_rkmpp");
    } else if (strcmp(name, "hevc") == 0) {
        return avcodec_find_encoder_by_name("hevc_rkmpp");
    } else if (strcmp(name, "av1") == 0) {
        return avcodec_find_encoder_by_name("av1_rkmpp");
    } else if (strcmp(name, "h263") == 0) {
        return avcodec_find_encoder_by_name("h263_rkmpp");
    } else if (strcmp(name, "mpeg1") == 0) {
        return avcodec_find_encoder_by_name("mpeg1_rkmpp");
    } else if (strcmp(name, "mpeg2") == 0) {
        return avcodec_find_encoder_by_name("mpeg2_rkmpp");
    } else if (strcmp(name, "mpeg4") == 0) {
        return avcodec_find_encoder_by_name("mpeg4_rkmpp");
    } else if (strcmp(name, "vp8") == 0) {
        return avcodec_find_encoder_by_name("vp8_rkmpp");
    } else if (strcmp(name, "vp9") == 0) {
        return avcodec_find_encoder_by_name("vp9_rkmpp");
    } else {
        avcodec_find_encoder_by_name(name);
    }
}
