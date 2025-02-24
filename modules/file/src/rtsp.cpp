#include "rtsp.h"

int8_t open_rtsp(const char *input, context_rtsp_t *context)
{
    if (avformat_open_input(&context->input_format_ctx, input, nullptr, nullptr) < 0)
    {
        std::cerr << "Не удалось открыть входной файл." << std::endl;
        return -1;
    }
    if (avformat_find_stream_info(context->input_format_ctx, nullptr) < 0)
    {
        std::cerr << "Не удалось найти информацию о потоках." << std::endl;
        return -1;
    }
    context->video_stream_index = -1;
    for (int i = 0; i < context->input_format_ctx->nb_streams; i++)
    {
        if (context->input_format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            context->video_stream_index = i;
            break;
        }
    }
    if (context->video_stream_index == -1)
    {
        std::cerr << "Входной файл не содержит видеопоток." << std::endl;
        return -1;
    }

    context->input_stream = context->input_format_ctx->streams[context->video_stream_index];
    context->input_codec = get_codec_by_id_rkmpp(context->input_stream->codecpar->codec_id);

    if (!context->input_codec)
    {
        std::cerr << "Не удалось найти декодер " << context->input_codec->id << " " << context->input_codec->name << "." << std::endl;
        return -1;
    }

    context->input_codec_ctx = avcodec_alloc_context3(context->input_codec);
    if (avcodec_parameters_to_context(context->input_codec_ctx, context->input_stream->codecpar) < 0)
    {
        std::cerr << "Не удалось сконфигурировать контекст декодера." << std::endl;
        return -1;
    }

    if (avcodec_open2(context->input_codec_ctx, context->input_codec, nullptr) < 0)
    {
        std::cerr << "Не удалось открыть декодер " << context->input_codec->name << "." << std::endl;
        return -1;
    }

    return 0;
}

void print_stream_list(context_rtsp_t &context)
{
    printf("Stream list:\n");
    for (int i = 0; i < context.input_format_ctx->nb_streams; i++)
    {
        AVStream *stream = context.input_format_ctx->streams[i];
        AVCodecParameters *codecpar = stream->codecpar;
        const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);

        printf("ID: %d ", i);

        if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            printf("Video Stream\n");
            printf("   Codec: %s (%s)\n", avcodec_get_name(codecpar->codec_id), codec ? codec->long_name : "Unknown");
            printf("   H x W: %d x %d\n", codecpar->height, codecpar->width);
            printf("   BitRate: %ld\n", codecpar->bit_rate);
            printf("   FPS: %.2f\n", av_q2d(stream->r_frame_rate));
            printf("   Format: %s\n", av_get_pix_fmt_name((AVPixelFormat)codecpar->format));
            printf("   Profile: %d\n", codecpar->profile);
            printf("   Level: %d\n", codecpar->level);
        }
        else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            printf("Audio Stream\n");
            printf("   Codec: %s (%s)\n", avcodec_get_name(codecpar->codec_id), codec ? codec->long_name : "Unknown");
            printf("   Sample Rate: %d Hz\n", codecpar->sample_rate);
            printf("   Channels: %d\n", codecpar->channels);
            printf("   BitRate: %ld\n", codecpar->bit_rate);
            printf("   Channel layout: %" PRIx64 "\n", codecpar->channel_layout);
        }
        else if (codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE)
        {
            printf("Subtitle Stream\n");
            printf("   Codec: %s (%s)\n", avcodec_get_name(codecpar->codec_id), codec ? codec->long_name : "Unknown");
            printf("   Codec Tag: 0x%X\n", codecpar->codec_tag);
        }
        else
        {
            printf("Unknown Stream\n");
        }
        printf("-------------------------------------\n");
    }
}

bool read_f(context_rtsp_t &context, frame_t &frame)
{
    if (avcodec_receive_frame(context.input_codec_ctx, context.frame) >= 0)
    {
        frame.height = context.frame->height;
        frame.width = context.frame->width;
        frame.format = (AVPixelFormat)context.frame->format;
        buffer_free(frame);
        convert_avframe_to_buffer(frame);
        return true;
    }
    else
    {
        if (av_read_frame(context.input_format_ctx, &context.packet) >= 0)
        {
            if (context.packet.stream_index == context.video_stream_index)
            {
                if (avcodec_send_packet(context.input_codec_ctx, &context.packet) < 0)
                {
                    av_packet_unref(&context.packet);
                    std::cerr << "Ошибка отправки пакета в декодер.\n";
                    return false;
                }
                av_packet_unref(&context.packet);
            }
        }
    }
    if (avcodec_receive_frame(context.input_codec_ctx, context.frame) >= 0)
    {
        // frame.avframe = context.frame;
        frame.height = context.frame->height;
        frame.width = context.frame->width;
        frame.format = (AVPixelFormat)context.frame->format;
        buffer_free(frame);
        convert_avframe_to_buffer(frame);
        return true;
    }
    else
    {
        return false;
    }
}
