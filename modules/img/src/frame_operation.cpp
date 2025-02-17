#include "frame_operation.h"


// frame_t imread(const char* filename) {
//     frame_t outputFrame = {nullptr, nullptr};
//     avformat_network_init();

//     AVFormatContext* formatCtx = nullptr;
//     if (avformat_open_input(&formatCtx, filename, nullptr, nullptr) != 0) {
//         std::cerr << "Ошибка: Не удалось открыть изображение!" << std::endl;
//         return outputFrame;
//     }

//     if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
//         std::cerr << "Ошибка: Не удалось получить информацию о потоке!" << std::endl;
//         avformat_close_input(&formatCtx);
//         return outputFrame;
//     }

//     int videoStream = -1;
//     for (unsigned i = 0; i < formatCtx->nb_streams; i++) {
//         if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
//             videoStream = i;
//             break;
//         }
//     }
//     if (videoStream == -1) {
//         std::cerr << "Ошибка: Видео-поток не найден!" << std::endl;
//         avformat_close_input(&formatCtx);
//         return outputFrame;
//     }

//     AVCodecParameters* codecParams = formatCtx->streams[videoStream]->codecpar;
//     const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
//     if (!codec) {
//         std::cerr << "Ошибка: Декодер не найден!" << std::endl;
//         avformat_close_input(&formatCtx);
//         return outputFrame;
//     }

//     outputFrame.codecCtx = avcodec_alloc_context3(codec);
//     if (!outputFrame.codecCtx) {
//         std::cerr << "Ошибка: Не удалось создать контекст кодека!" << std::endl;
//         avformat_close_input(&formatCtx);
//         return outputFrame;
//     }

//     avcodec_parameters_to_context(outputFrame.codecCtx, codecParams);
//     if (avcodec_open2(outputFrame.codecCtx, codec, nullptr) < 0) {
//         std::cerr << "Ошибка: Не удалось открыть кодек!" << std::endl;
//         avcodec_free_context(&outputFrame.codecCtx);
//         avformat_close_input(&formatCtx);
//         return outputFrame;
//     }

//     AVFrame* decodedFrame = av_frame_alloc();
//     AVPacket* packet = av_packet_alloc();

//     while (av_read_frame(formatCtx, packet) >= 0) {
//         if (packet->stream_index == videoStream) {
//             if (avcodec_send_packet(outputFrame.codecCtx, packet) == 0) {
//                 if (avcodec_receive_frame(outputFrame.codecCtx, decodedFrame) == 0) {
//                     outputFrame.data = decodedFrame;
//                     av_packet_free(&packet);
//                     avformat_close_input(&formatCtx);
//                     return outputFrame;
//                 }
//             }
//         }
//         av_packet_unref(packet);
//     }

//     av_packet_free(&packet);
//     avformat_close_input(&formatCtx);
//     av_frame_free(&decodedFrame);
//     return outputFrame;
// }

// #ifdef WITH_OPENCV
// void frame2mat(frame* frame, cv::Mat mat){
//     SwsContext* sws_ctx = sws_getContext(
//         frame->codecCtx->width,                // Исходная ширина
//         frame->codecCtx->height,               // Исходная высота
//         frame->codecCtx->pix_fmt,              // Исходный формат (то, что возвращает декодер)
//         frame->codecCtx->width,                // Желаемая ширина
//         frame->codecCtx->height,               // Желаемая высота
//         AV_PIX_FMT_BGR24,                      // Формат, понятный OpenCV (BGR24)
//         SWS_BICUBIC,                           // Алгоритм масштабирования
//         nullptr, nullptr, nullptr
//     );

//     AVFrame* bgrFrame = av_frame_alloc();
//     bgrFrame->format = AV_PIX_FMT_BGR24;
//     bgrFrame->width  = frame->codecCtx->width;
//     bgrFrame->height = frame->codecCtx->height;

//     sws_scale(
//         sws_ctx,
//         frame->data->data,
//         frame->data->linesize,
//         0,
//         frame->data->height,
//         bgrFrame->data,
//         bgrFrame->linesize
//     );

//     // Заворачиваем bgrFrame->data[0] в cv::Mat
//     cv::Mat mat_old(
//         bgrFrame->height,
//         bgrFrame->width,
//         CV_8UC3,               // 8 бит на канал, 3 канала (BGR)
//         bgrFrame->data[0],
//         bgrFrame->linesize[0] // шаг (pitch) в байтах
//     );
//     mat_old.copyTo(mat);
// }
// #endif