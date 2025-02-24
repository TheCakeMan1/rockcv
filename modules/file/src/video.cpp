#include "video.h"

using namespace std::chrono;


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


// Глобальная переменная для хранения времени начала воспроизведения
static steady_clock::time_point start_time = steady_clock::now();
static int64_t first_pts = AV_NOPTS_VALUE;

int8_t open_video(const char* input, context_video_t& context){
    if (avformat_open_input(&context.format_ctx, input, nullptr, nullptr) < 0) {
        std::cerr << "Не удалось открыть входной файл." << std::endl;
        return -1;
    }
    if (avformat_find_stream_info(context.format_ctx, nullptr) < 0) {
        std::cerr << "Не удалось найти информацию о потоках." << std::endl;
        return -1;
    }
    context.video_stream_index = -1;
    context.max_stream_v = context.format_ctx->nb_streams;
    for (int i = 0; i < context.format_ctx->nb_streams; i++) {
        if (context.format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            context.video_stream_index = i;
            break;
        }
    }
    if (context.video_stream_index == -1) {
        std::cerr << "Входной файл не содержит видеопоток." << std::endl;
        return -1;
    }
    
    context.stream = context.format_ctx->streams[context.video_stream_index];
    context.codec = get_codec_by_id_rkmpp(context.stream->codecpar->codec_id);
    
    if (!context.codec) {
        std::cerr << "Не удалось найти декодер h264_rkmpp." << std::endl;
        return -1;
    }

    context.codec_ctx = avcodec_alloc_context3(context.codec);
    if (avcodec_parameters_to_context(context.codec_ctx, context.stream->codecpar) < 0) {
        std::cerr << "Не удалось сконфигурировать контекст декодера." << std::endl;
        return -1;
    }

    if (avcodec_open2(context.codec_ctx, context.codec, nullptr) < 0) {
        std::cerr << "Не удалось открыть декодер h264_rkmpp." << std::endl;
        return -1;
    }
    return 0;
}

int8_t create_video(const char* output, context_video_t& context, context_video_t& input_context, AVPixelFormat fmt, const char* codec){
    if (avformat_alloc_output_context2(&context.format_ctx, nullptr, nullptr, output) < 0) {
        std::cerr << "Не удалось создать выходной формат." << std::endl;
        return -1;
    }
    
    context.codec = get_codec_by_name_rkmpp(codec);

    if (!context.codec) {
        std::cerr << "Не удалось найти кодер " << context.codec->name << "." << std::endl;
        return -1;
    }

    context.stream = avformat_new_stream(context.format_ctx, nullptr);
    context.codec_ctx = avcodec_alloc_context3(context.codec);
    if (!context.stream || !context.codec_ctx) {
        std::cerr << "Не удалось создать выходной видеопоток." << std::endl;
        return -1;
    }

    context.codec_ctx->height = input_context.stream->codecpar->height;
    context.codec_ctx->width = input_context.stream->codecpar->width;
    context.codec_ctx->pix_fmt = fmt;
    context.codec_ctx->time_base = input_context.stream->time_base;
    context.codec_ctx->framerate = input_context.stream->r_frame_rate; 

    if (avcodec_open2(context.codec_ctx, context.codec, nullptr) < 0) {
        std::cerr << "Не удалось открыть кодер hevc_rkmpp." << std::endl;
        return -1;
    }

    if (avcodec_parameters_from_context(context.stream->codecpar, context.codec_ctx) < 0) {
        std::cerr << "Не удалось сконфигурировать параметры для выходного потока." << std::endl;
        return -1;
    }

    // Открытие выходного файла
    if (!(context.format_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&context.format_ctx->pb, output, AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Не удалось открыть выходной файл." << std::endl;
            return -1;
        }
    }

    // Запись заголовков
    if (avformat_write_header(context.format_ctx, nullptr) < 0) {
        std::cerr << "Не удалось записать заголовки." << std::endl;
        return -1;
    }

    return 0;
}

void print_stream_list(context_video_t& context){
    printf("Stream list:\n");
    for (int i = 0; i < context.format_ctx->nb_streams; i++) {
        AVStream *stream = context.format_ctx->streams[i];
        AVCodecParameters *codecpar = stream->codecpar;
        const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);

        printf("ID: %d ", i);

        if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            printf("Video Stream\n");
            printf("   Codec: %s (%s)\n", avcodec_get_name(codecpar->codec_id), codec ? codec->long_name : "Unknown");
            printf("   H x W: %d x %d\n", codecpar->height, codecpar->width);
            printf("   BitRate: %ld\n", codecpar->bit_rate);
            printf("   FPS: %.2f\n", av_q2d(stream->r_frame_rate));
            printf("   Format: %s\n", av_get_pix_fmt_name((AVPixelFormat)codecpar->format));
            printf("   Profile: %d\n", codecpar->profile);
            printf("   Level: %d\n", codecpar->level);

        } else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            printf("Audio Stream\n");
            printf("   Codec: %s (%s)\n", avcodec_get_name(codecpar->codec_id), codec ? codec->long_name : "Unknown");
            printf("   Sample Rate: %d Hz\n", codecpar->sample_rate);
            printf("   Channels: %d\n", codecpar->channels);
            printf("   BitRate: %ld\n", codecpar->bit_rate);
            printf("   Channel layout: %" PRIx64 "\n", codecpar->channel_layout);

        } else if (codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) {
            printf("Subtitle Stream\n");
            printf("   Codec: %s (%s)\n", avcodec_get_name(codecpar->codec_id), codec ? codec->long_name : "Unknown");
            printf("   Codec Tag: 0x%X\n", codecpar->codec_tag);

        } else {
            printf("Unknown Stream\n");
        }
        printf("-------------------------------------\n");
    }
}

void sync_by_pts(AVFrame* frame, AVRational time_base) {
    if (frame->pts == AV_NOPTS_VALUE) return; 
    
    double pts_time = frame->pts * av_q2d(time_base);

    if (first_pts == AV_NOPTS_VALUE) {
        first_pts = frame->pts;
        start_time = steady_clock::now();
    }

    double elapsed_time = duration<double>(steady_clock::now() - start_time).count();

    if (pts_time > elapsed_time) {
        std::this_thread::sleep_for(duration<double>(pts_time - elapsed_time));
    }
}

bool read_f(context_video_t& context){
    AVRational time_base = context.stream->time_base;
    if(avcodec_receive_frame(context.codec_ctx, context.frame) >= 0){
        sync_by_pts(context.frame, time_base);
        return true;
    } else {
        if (av_read_frame(context.format_ctx, &context.packet) >= 0){
            if (context.packet.stream_index == context.video_stream_index){
                if (avcodec_send_packet( context.codec_ctx, & context.packet) < 0) {
                    
                    av_packet_unref(&context.packet);
                    std::cerr << "Ошибка отправки пакета в декодер.\n";
                    return false;
                }
                av_packet_unref(&context.packet);
            }
        }
    }
    if(avcodec_receive_frame(context.codec_ctx, context.frame) >= 0){
        sync_by_pts(context.frame, time_base);
        return true;
    } else {
        return false;
    }
}

bool read_f(context_video_t& context, context_video_t& output){
    AVRational time_base = context.stream->time_base;
    if(avcodec_receive_frame(context.codec_ctx, context.frame) >= 0){
        av_write_frame(output.format_ctx, &output.packet);
        sync_by_pts(context.frame, time_base);
        return true;
    } else {
        if (av_read_frame(context.format_ctx, &context.packet) >= 0){
            if (context.packet.stream_index == context.video_stream_index){
                if (avcodec_send_packet( context.codec_ctx, & context.packet) < 0) {
                    av_packet_unref(&context.packet);
                    std::cerr << "Ошибка отправки пакета в декодер.\n";
                    return false;
                }
                av_packet_unref(&output.packet);
                avcodec_receive_packet(output.codec_ctx, &output.packet);
                av_packet_unref(&context.packet);
            }
        }
    }
    if(avcodec_receive_frame(context.codec_ctx, context.frame) >= 0){
        av_write_frame(output.format_ctx, &output.packet);
        sync_by_pts(context.frame, time_base);
        return true;
    } else {
        return false;
    }
}
