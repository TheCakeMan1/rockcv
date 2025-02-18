#include "rtsp.h"

int8_t open_rtsp(const char* input, context_rtsp_t* context){
    if (avformat_open_input(&context->input_format_ctx, input, nullptr, nullptr) < 0) {
        std::cerr << "Не удалось открыть входной файл." << std::endl;
        return -1;
    }
    if (avformat_find_stream_info(context->input_format_ctx, nullptr) < 0) {
        std::cerr << "Не удалось найти информацию о потоках." << std::endl;
        return -1;
    }
    context->video_stream_index = -1;
    for (int i = 0; i < context->input_format_ctx->nb_streams; i++) {
        if (context->input_format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            context->video_stream_index = i;
            break;
        }
    }
    if (context->video_stream_index == -1) {
        std::cerr << "Входной файл не содержит видеопоток." << std::endl;
        return -1;
    }

    context->input_stream = context->input_format_ctx->streams[context->video_stream_index];
    if (context->input_stream->codecpar->codec_id == AV_CODEC_ID_H264) {
        context->input_codec = avcodec_find_decoder_by_name("h264_rkmpp");
    } else if (context->input_stream->codecpar->codec_id == AV_CODEC_ID_HEVC) {
        context->input_codec = avcodec_find_decoder_by_name("hevc_rkmpp");
    } else {
        context->input_codec = avcodec_find_decoder(context->input_stream->codecpar->codec_id);
    }

    if (!context->input_codec) {
        std::cerr << "Не удалось найти декодер " << context->input_codec->name << "." << std::endl;
        return -1;
    }

    context->input_codec_ctx = avcodec_alloc_context3(context->input_codec);
    if (avcodec_parameters_to_context(context->input_codec_ctx, context->input_stream->codecpar) < 0) {
        std::cerr << "Не удалось сконфигурировать контекст декодера." << std::endl;
        return -1;
    }

    if (avcodec_open2(context->input_codec_ctx, context->input_codec, nullptr) < 0) {
        std::cerr << "Не удалось открыть декодер " << context->input_codec->name << "." << std::endl;
        return -1;
    }
    return 0;
}

bool read_packet(context_rtsp_t* context) {
    if (av_read_frame(context->input_format_ctx, &context->packet) >= 0) {
        if (context->packet.stream_index == context->video_stream_index) {
            int ret = avcodec_send_packet(context->input_codec_ctx, &context->packet);
            if (ret < 0) {
                if (ret == AVERROR(EAGAIN)) {
                    std::cerr << "Буфер декодера заполнен, необходимо сначала получить кадры.\n";
                } else {
                    std::cerr << "Ошибка отправки пакета в декодер: " << ret << std::endl;
                }
                return false;
            }
        }
        return true;
    }
    return false;
}

bool read_frame(context_rtsp_t* context) {
    int ret = avcodec_receive_frame(context->input_codec_ctx, context->frame);
    
    if (ret >= 0) {
        return true;  // Успешно получили один кадр
    }
    
    if (ret == AVERROR(EAGAIN)) {
        av_packet_unref(&context->packet);
        return false;  // Кадров больше нет, нужен новый пакет
    }

    std::cerr << "Ошибка получения кадра: " << ret << std::endl;
    return false;
}
