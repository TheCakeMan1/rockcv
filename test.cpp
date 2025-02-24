#include <iostream>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#include "rockcv/transform.h"
#include "rockcv/frame_operation.h"
#include <opencv2/opencv.hpp> // <-- Главное для отображения

#include <iostream>
#include <chrono>
#include <thread>
#include "rockcv/rtsp.h"
#include "rockcv/video.h"
#include "rockcv/codec.h"

class Timer
{
public:
    Timer() : running(false), elapsed_time(0) {}

    void start()
    {
        if (!running)
        {
            start_time = std::chrono::high_resolution_clock::now();
            running = true;
        }
    }

    void stop()
    {
        if (running)
        {
            auto end_time = std::chrono::high_resolution_clock::now();
            elapsed_time += std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            running = false;
        }
    }

    void reset()
    {
        running = false;
        elapsed_time = 0;
    }

    double elapsed() const
    {
        if (running)
        {
            auto current_time = std::chrono::high_resolution_clock::now();
            return (elapsed_time + std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count()) / 1000.0;
        }
        else
        {
            return elapsed_time / 1000.0;
        }
    }

private:
    bool running;
    long long elapsed_time; // в миллисекундах
    std::chrono::high_resolution_clock::time_point start_time;
};

int main()
{
    av_log_set_level(AV_LOG_QUIET);
    // const AVCodec *codec = NULL;
    // void *i = 0;
    // while ((codec = av_codec_iterate(&i))) {
    //     printf("Codec: %s, ID: %d\n", codec->name, codec->id);
    // }
    // Открываем файл
    // const char* input_filename = "321.mp4";
    const char *input_filename = "rtsp://192.168.6.53:554/user=admin_password=1UfX6Hen_channel=1_stream=0&protocol=unicast.sdp?real_stream";
    // const char* input_filename = "rtsp:/192.168.70.2:554/11";
    // const char* output_filename = "output.mp4";

    // const char* input_filename = "input.mp4";
    // const char* output_filename = "output.mp4";

    context_rtsp_t con;
    // context_video_t con;
    // context_video_t con_o;

    open_rtsp(input_filename, &con);
    // open_video(input_filename, con);

    AVPacket packet;

    con.frame = av_frame_alloc();

    print_stream_list(con);

    // AVFrame* frame = av_frame_alloc();
    frame_t test;
    frame_t temp;
    frame_t output;

    Timer timer;
    timer.start();
    int i = 0;
    double last_timestamp = 0;
    test.avframe = con.frame;
    double timestamp;
    cv::Mat mat;
    std::cout << "Offset of format: " << offsetof(frame_t, format) << "\n";
    std::cout << "Offset of width: " << offsetof(frame_t, width) << "\n";
    std::cout << "Offset of height: " << offsetof(frame_t, height) << "\n";
    std::cout << "Offset of buffer: " << offsetof(frame_t, buffer) << "\n";
    std::cout << "Offset of avframe: " << offsetof(frame_t, avframe) << "\n";
    std::cout << "Offset of avcodeccontext: " << offsetof(frame_t, avcodeccontext) << "\n";
    std::cout << "Offset of dmabuf_fd: " << offsetof(frame_t, dmabuf_fd) << "\n";
    while (1)
    {
        if (timer.elapsed() > 30)
        {
            break;
        }
        if (read_f(con, test))
        {
            // printf("%d\n", sizeof(con));
            // rockf::convert(&test, &temp, AV_PIX_FMT_RGB24);
            // rga_resize(src_addr, dst_addr, src_width, src_height, dst_width, dst_height);
            // rockf::rga_resize_dma(&test, &output, 640, 640, AV_PIX_FMT_RGB24);
            // rotate(&temp, &output, 90);
            rockf::resize(test, output, 640, 640, AV_PIX_FMT_RGB24, 1);
            // avframe_to_dmabuf(&test);
            // resize_frame(&test, &output, 640, 640);
            // rockf::convert(&temp, &output, AV_PIX_FMT_RGB24);

            frame2mat(output, mat);

            cv::imshow("Video", mat);
            // cv::imwrite("res.png", mat);
            // wait_per_frame(con.input_stream->codecpar->framerate);
            cv::waitKey(1);
            // cv::waitKey(600 / av_q2d(con.input_stream->codecpar->framerate));
            // delay(con);
        }
    }

    return 0;
}

// while (read_packet(&con)) {
//     if(timer.elapsed() > 60){
//         break;
//     }
//     while (read_frame(&con)) {
//         // printf("%d\n", sizeof(con));
//         // rockf::convert(&test, &temp, AV_PIX_FMT_RGB24);
//         rockf::resize(&test, &output, 640, 640, AV_PIX_FMT_RGB24, 0);
//         // rotate(&temp, &output, 90);
//         cv::Mat mat(
//             output.avframe->height,
//             output.avframe->width,
//             CV_8UC3,  // 8 бит на канал, 3 канала (BGR)
//             output.avframe->data[0],
//             output.avframe->linesize[0]  // Шаг (pitch) в байтах
//         );

//         // Отображение кадра
//         cv::imshow("Video", mat);
//         cv::waitKey(1);
//     }
// }

// int main() {
//     av_log_set_level(AV_LOG_QUIET);
//     // Открываем файл
//     // const char* input_filename = "321.mp4";
//     const char* input_filename = "rtsp://192.168.6.53:554/user=admin_password=1UfX6Hen_channel=1_stream=0&protocol=unicast.sdp?real_stream";
//     const char* output_filename = "output.mp4";

//     // Открываем входной файл
//     // const char* input_filename = "input.mp4";
//     // const char* output_filename = "output.mp4";

//     AVFormatContext* input_format_ctx = nullptr;
//     AVFormatContext* output_format_ctx = nullptr;

//     if (avformat_open_input(&input_format_ctx, input_filename, nullptr, nullptr) < 0) {
//         std::cerr << "Не удалось открыть входной файл." << std::endl;
//         return -1;
//     }

//     if (avformat_find_stream_info(input_format_ctx, nullptr) < 0) {
//         std::cerr << "Не удалось найти информацию о потоках." << std::endl;
//         return -1;
//     }

//     // Находим видеопоток в источнике
//     int video_stream_index = -1;
//     for (int i = 0; i < input_format_ctx->nb_streams; i++) {
//         if (input_format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
//             video_stream_index = i;
//             break;
//         }
//     }

//     if (video_stream_index == -1) {
//         std::cerr << "Входной файл не содержит видеопоток." << std::endl;
//         return -1;
//     }

//     AVStream* input_stream = input_format_ctx->streams[video_stream_index];
//     const AVCodec* input_codec = avcodec_find_decoder_by_name("h264_rkmpp");
//     if (!input_codec) {
//         std::cerr << "Не удалось найти декодер h264_rkmpp." << std::endl;
//         return -1;
//     }

//     AVCodecContext* input_codec_ctx = avcodec_alloc_context3(input_codec);
//     if (avcodec_parameters_to_context(input_codec_ctx, input_stream->codecpar) < 0) {
//         std::cerr << "Не удалось сконфигурировать контекст декодера." << std::endl;
//         return -1;
//     }

//     if (avcodec_open2(input_codec_ctx, input_codec, nullptr) < 0) {
//         std::cerr << "Не удалось открыть декодер h264_rkmpp." << std::endl;
//         return -1;
//     }

//     // Открываем выходной файл
//     if (avformat_alloc_output_context2(&output_format_ctx, nullptr, nullptr, output_filename) < 0) {
//         std::cerr << "Не удалось создать выходной формат." << std::endl;
//         return -1;
//     }

//     const AVCodec* output_codec = avcodec_find_encoder_by_name("hevc_rkmpp");
//     if (!output_codec) {
//         std::cerr << "Не удалось найти кодер hevc_rkmpp." << std::endl;
//         return -1;
//     }

//     AVStream* output_stream = avformat_new_stream(output_format_ctx, nullptr);
//     AVCodecContext* output_codec_ctx = avcodec_alloc_context3(output_codec);
//     if (!output_stream || !output_codec_ctx) {
//         std::cerr << "Не удалось создать выходной видеопоток." << std::endl;
//         return -1;
//     }

//     output_codec_ctx->height = input_stream->codecpar->height;
//     output_codec_ctx->width = input_stream->codecpar->width;
//     output_codec_ctx->pix_fmt = AV_PIX_FMT_NV12;
//     output_codec_ctx->time_base = input_stream->time_base; // Сохраняем time_base
//     output_codec_ctx->framerate = input_stream->r_frame_rate; // Используем ту же частоту кадров, что и у входного потока

//     if (avcodec_open2(output_codec_ctx, output_codec, nullptr) < 0) {
//         std::cerr << "Не удалось открыть кодер hevc_rkmpp." << std::endl;
//         return -1;
//     }

//     if (avcodec_parameters_from_context(output_stream->codecpar, output_codec_ctx) < 0) {
//         std::cerr << "Не удалось сконфигурировать параметры для выходного потока." << std::endl;
//         return -1;
//     }

//     // Открытие выходного файла
//     if (!(output_format_ctx->oformat->flags & AVFMT_NOFILE)) {
//         if (avio_open(&output_format_ctx->pb, output_filename, AVIO_FLAG_WRITE) < 0) {
//             std::cerr << "Не удалось открыть выходной файл." << std::endl;
//             return -1;
//         }
//     }

//     // Запись заголовков
//     if (avformat_write_header(output_format_ctx, nullptr) < 0) {
//         std::cerr << "Не удалось записать заголовки." << std::endl;
//         return -1;
//     }

//     AVPacket packet;
//     // AVFrame* frame = av_frame_alloc();
//     // if (!frame) {
//     //     std::cerr << "Не удалось выделить память для фрейма." << std::endl;
//     //     return -1;
//     // }
// // int i = 0;
//     SwsContext* sws_ctx = sws_getContext(
//         input_codec_ctx->width,                // Исходная ширина
//         input_codec_ctx->height,               // Исходная высота
//         input_codec_ctx->pix_fmt,              // Исходный формат (то, что возвращает декодер)
//         input_codec_ctx->width,                // Желаемая ширина
//         input_codec_ctx->height,               // Желаемая высота
//         AV_PIX_FMT_BGR24,                      // Формат, понятный OpenCV (BGR24)
//         SWS_BICUBIC,                           // Алгоритм масштабирования
//         nullptr, nullptr, nullptr
//     );

//     // Подготавливаем фрейм для BGR
//     AVFrame* bgrFrame = av_frame_alloc();
//     bgrFrame->format = AV_PIX_FMT_BGR24;
//     bgrFrame->width  = input_codec_ctx->width;
//     bgrFrame->height = input_codec_ctx->height;

//     // Выделяем память под bgrFrame
//     if (av_frame_get_buffer(bgrFrame, 32) < 0) {
//         std::cerr << "Не удалось выделить буфер для bgrFrame\n";
//         return -1;
//     }

//     // ------------------------------------

//     // Основной цикл чтения пакетов
//     // AVPacket packet;
//     AVFrame* frame = av_frame_alloc();
//     // AVFrame* output = av_frame_alloc();
//     frame_t test;
//     frame_t temp;
//     frame_t output;
//     test.avframe = frame;
//     // frame_t test;
//     // frame_t output;
//     // AVFrame* frame_out = av_frame_alloc();

//     Timer timer;
//     timer.start();
//     while (av_read_frame(input_format_ctx, &packet) >= 0) {
//         if (packet.stream_index == video_stream_index) {
//             if (avcodec_send_packet(input_codec_ctx, &packet) < 0) {
//                 std::cerr << "Ошибка отправки пакета в декодер.\n";
//                 break;
//             }
//             // if (timer.elapsed()> 300){
//             //     return 0;
//             // }

//             while (avcodec_receive_frame(input_codec_ctx, frame) >= 0) {
//                 // -------------------------------------------------------------
//                 // 1) Отображение кадра в OpenCV (добавляем sws_scale -> cv::imshow)
//                 //    Конвертируем frame -> bgrFrame
//                     // SwsContext* sws_ctx = sws_getContext(
//                     //     input_codec_ctx->width,                // Исходная ширина
//                     //     input_codec_ctx->height,               // Исходная высота
//                     //     input_codec_ctx->pix_fmt,              // Исходный формат (то, что возвращает декодер)
//                     //     input_codec_ctx->width,                // Желаемая ширина
//                     //     input_codec_ctx->height,               // Желаемая высота
//                     //     AV_PIX_FMT_BGR24,                      // Формат, понятный OpenCV (BGR24)
//                     //     SWS_BICUBIC,                           // Алгоритм масштабирования
//                     //     nullptr, nullptr, nullptr
//                     // );

//                 // test.data = frame;
//                 // test.codecCtx = input_codec_ctx;

//                 // convertAVFrameColor(test.avframe, test.avframe, AV_PIX_FMT_RGB24);
//                 //                 AVFrame* convertedFrame = convertToRGB24(test.avframe, test.avcodeccontext);
//                 // if (!convertedFrame) {
//                 //     std::cerr << "Ошибка: не удалось конвертировать кадр в RGB24!" << std::endl;
//                 //     return 1;
//                 // }

//                 // // Теперь вызываем RGA
//                 // // test.avframe = convertedFrame;
//                 rockf::resize(&test, &output, ALIGN_UP(400, 16), ALIGN_UP(400, 16), AV_PIX_FMT_RGB24);

//                 // rotate(&test, &output, 90, AV_PIX_FMT_RGB24);
//                 // av_frame_unref(frame);
//                 // printf("%d\n", output.avframe->height);

//                 // printf("Out: %d %d %d %d\n", output.avframe->width, output.avframe->height, output.avframe->linesize[0], test.avframe->linesize[0]);

//                 // sws_scale(
//                 //     sws_ctx,
//                 //     frame->data,
//                 //     frame->linesize,
//                 //     0,
//                 //     frame->height,
//                 //     bgrFrame->data,
//                 //     bgrFrame->linesize
//                 // );

//                 // Заворачиваем bgrFrame->data[0] в cv::Mat
//                 cv::Mat mat(
//                     output.avframe->height,
//                     output.avframe->width,
//                     CV_8UC3,               // 8 бит на канал, 3 канала (BGR)
//                     output.avframe->data[0],
//                     output.avframe->linesize[0] // шаг (pitch) в байтах
//                 );
//                 // av_freep(&output.avframe);
//                 // av_freep(&test.avframe);
//                 // av_freep(&output.avcodeccontext);
//                 // av_freep(&test.avcodeccontext);

//                 static cv::Mat img;
//                 // mirror(mat, img, 3);
//                 // scaleFrame(mat, img, 640, 640);
//                 // cv::resize(mat, img, cv::Size(640,640));
//                 // cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
//                 cv::imshow("Video", mat);
//                 cv::waitKey(1);  // Небольшая задержка, чтобы окно обновлялось

//                 // -------------------------------------------------------------
//                 // 2) Перекодирование (если вам всё ещё нужно):
//                 //    Обновляем временные метки, отправляем во второй кодек
//                 // frame->pts     = av_rescale_q(frame->pts,     input_stream->time_base, output_stream->time_base);
//                 // frame->pkt_dts = av_rescale_q(frame->pkt_dts, input_stream->time_base, output_stream->time_base);

//                 // if (avcodec_send_frame(output_codec_ctx, frame) < 0) {
//                 //     std::cerr << "Ошибка отправки фрейма в кодер.\n";
//                 //     break;
//                 // }

//                 // while (avcodec_receive_packet(output_codec_ctx, &packet) >= 0) {
//                 //     packet.stream_index = 0;
//                 //     if (av_write_frame(output_format_ctx, &packet) < 0) {
//                 //         std::cerr << "Ошибка записи пакета.\n";
//                 //         break;
//                 //     }
//                 //     av_packet_unref(&packet);
//                 // }
//             }
//         }
//         av_packet_unref(&packet);
//     }

//     // Завершение, освобождение ресурсов, av_write_trailer и т.д.
//     // ...
//     av_frame_free(&bgrFrame);
//     sws_freeContext(sws_ctx);
//     return 0;
// }

// // while (av_read_frame(input_format_ctx, &packet) >= 0) {
// //         if (packet.stream_index == video_stream_index) {
// //             if (avcodec_send_packet(input_codec_ctx, &packet) < 0) {
// //                 std::cerr << "Ошибка отправки пакета в декодер.\n";
// //                 break;
// //             }

// //             while (avcodec_receive_frame(input_codec_ctx, frame) >= 0) {

// //                 resize(&test, &output, ALIGN_UP(400, 16), ALIGN_UP(400, 16), AV_PIX_FMT_RGB24);

// //                 cv::Mat mat(
// //                     output.avframe->height,
// //                     output.avframe->width,
// //                     CV_8UC3,               // 8 бит на канал, 3 канала (BGR)
// //                     output.avframe->data[0],
// //                     output.avframe->linesize[0] // шаг (pitch) в байтах
// //                 );
// //                 static cv::Mat img;
// //                 cv::imshow("Video", mat);
// //                 cv::waitKey(1);

// //             }
// //         }
// //         av_packet_unref(&packet);
// //     }

// //     av_frame_free(&bgrFrame);
// //     sws_freeContext(sws_ctx);
// //     return 0;
// // }
