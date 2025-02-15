#include "gui.h"



// #include <iostream>
// #include <QPixmap>
// #include <QVBoxLayout>
// #include <QWidget>

// QApplication* appInstance = nullptr;

// ImageViewer::ImageViewer() {
//     label = new QLabel();
//     label->setWindowTitle("Qt + FFmpeg Viewer");
//     label->setAlignment(Qt::AlignCenter);
//     label->resize(800, 600);
// }

// // Деструктор
// ImageViewer::~ImageViewer() {
//     delete label;
// }

// // Запуск Qt в отдельном потоке
// void ImageViewer::run() {
//     label->show();
//     exec();  // Цикл событий Qt
// }

// // Обновление изображения
// void ImageViewer::showImage(const frame& imgFrame) {
//     std::lock_guard<std::mutex> lock(img_mutex);
//     QImage img = convert_frame_to_qimage(imgFrame);
//     if (!img.isNull()) {
//         QMetaObject::invokeMethod(label, "setPixmap", Qt::QueuedConnection, Q_ARG(QPixmap, QPixmap::fromImage(img)));
//     }
// }

// // Функция загрузки изображения с помощью FFmpeg
// frame imread(const char* filename) {
//     frame outputFrame = {nullptr, nullptr};
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

// // Преобразование FFmpeg кадра в QImage
// QImage convert_frame_to_qimage(const frame& inputFrame) {
//     if (!inputFrame.data || !inputFrame.codecCtx) {
//         std::cerr << "Ошибка: Нет данных для конвертации!" << std::endl;
//         return QImage();
//     }

//     struct SwsContext* swsCtx = sws_getContext(
//         inputFrame.codecCtx->width, inputFrame.codecCtx->height, inputFrame.codecCtx->pix_fmt,
//         inputFrame.codecCtx->width, inputFrame.codecCtx->height, AV_PIX_FMT_RGB24,
//         SWS_BILINEAR, nullptr, nullptr, nullptr);

//     int bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, inputFrame.codecCtx->width, inputFrame.codecCtx->height, 1);
//     uint8_t* buffer = (uint8_t*)av_malloc(bytes);
//     AVFrame* rgbFrame = av_frame_alloc();

//     av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer, AV_PIX_FMT_RGB24,
//                          inputFrame.codecCtx->width, inputFrame.codecCtx->height, 1);

//     sws_scale(swsCtx, inputFrame.data->data, inputFrame.data->linesize, 0, inputFrame.codecCtx->height,
//               rgbFrame->data, rgbFrame->linesize);

//     QImage img(rgbFrame->data[0], inputFrame.codecCtx->width, inputFrame.codecCtx->height, QImage::Format_RGB888);

//     sws_freeContext(swsCtx);
//     av_free(buffer);
//     av_frame_free(&rgbFrame);
//     return img;
// }


// // #include "gui.h"

// // frame imread(const char* filename) {
// //     frame outputFrame = {nullptr, nullptr};
// //     avformat_network_init();

// //     AVFormatContext* formatCtx = nullptr;
// //     if (avformat_open_input(&formatCtx, filename, nullptr, nullptr) != 0) {
// //         std::cerr << "Ошибка: Не удалось открыть изображение!" << std::endl;
// //         return outputFrame;
// //     }

// //     if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
// //         std::cerr << "Ошибка: Не удалось получить информацию о потоке!" << std::endl;
// //         avformat_close_input(&formatCtx);
// //         return outputFrame;
// //     }

// //     int videoStream = -1;
// //     for (unsigned i = 0; i < formatCtx->nb_streams; i++) {
// //         if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
// //             videoStream = i;
// //             break;
// //         }
// //     }
// //     if (videoStream == -1) {
// //         std::cerr << "Ошибка: Видео-поток не найден!" << std::endl;
// //         avformat_close_input(&formatCtx);
// //         return outputFrame;
// //     }

// //     AVCodecParameters* codecParams = formatCtx->streams[videoStream]->codecpar;
// //     const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
// //     if (!codec) {
// //         std::cerr << "Ошибка: Декодер не найден!" << std::endl;
// //         avformat_close_input(&formatCtx);
// //         return outputFrame;
// //     }

// //     outputFrame.codecCtx = avcodec_alloc_context3(codec);
// //     if (!outputFrame.codecCtx) {
// //         std::cerr << "Ошибка: Не удалось создать контекст кодека!" << std::endl;
// //         avformat_close_input(&formatCtx);
// //         return outputFrame;
// //     }

// //     avcodec_parameters_to_context(outputFrame.codecCtx, codecParams);
// //     if (avcodec_open2(outputFrame.codecCtx, codec, nullptr) < 0) {
// //         std::cerr << "Ошибка: Не удалось открыть кодек!" << std::endl;
// //         avcodec_free_context(&outputFrame.codecCtx);
// //         avformat_close_input(&formatCtx);
// //         return outputFrame;
// //     }

// //     AVFrame* decodedFrame = av_frame_alloc();
// //     AVPacket* packet = av_packet_alloc();

// //     while (av_read_frame(formatCtx, packet) >= 0) {
// //         if (packet->stream_index == videoStream) {
// //             if (avcodec_send_packet(outputFrame.codecCtx, packet) == 0) {
// //                 if (avcodec_receive_frame(outputFrame.codecCtx, decodedFrame) == 0) {
// //                     outputFrame.data = decodedFrame;
// //                     av_packet_free(&packet);
// //                     avformat_close_input(&formatCtx);
// //                     return outputFrame;
// //                 }
// //             }
// //         }
// //         av_packet_unref(packet);
// //     }

// //     av_packet_free(&packet);
// //     avformat_close_input(&formatCtx);
// //     av_frame_free(&decodedFrame);
// //     return outputFrame;
// // }

// // GdkPixbuf* convert_frame_to_pixbuf(const frame& inputFrame) {
// //     if (!inputFrame.data || !inputFrame.codecCtx) {
// //         std::cerr << "Ошибка: Нет данных для конвертации!" << std::endl;
// //         return nullptr;
// //     }

// //     std::cout << "Исходный формат пикселей: " << av_get_pix_fmt_name(inputFrame.codecCtx->pix_fmt) << std::endl;

// //     AVPixelFormat srcFormat = inputFrame.codecCtx->pix_fmt;
// //     if (srcFormat == AV_PIX_FMT_YUVJ420P) {
// //         srcFormat = AV_PIX_FMT_YUV420P;
// //     } else if (srcFormat == AV_PIX_FMT_RGBA || srcFormat == AV_PIX_FMT_BGRA) {
// //         srcFormat = AV_PIX_FMT_RGB24;
// //     }

// //     struct SwsContext* swsCtx = sws_getContext(
// //         inputFrame.codecCtx->width, inputFrame.codecCtx->height, srcFormat,
// //         inputFrame.codecCtx->width, inputFrame.codecCtx->height, AV_PIX_FMT_RGB24,
// //         SWS_BILINEAR, nullptr, nullptr, nullptr);

// //     int bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, inputFrame.codecCtx->width, inputFrame.codecCtx->height, 1);
// //     uint8_t* buffer = (uint8_t*)av_malloc(bytes * sizeof(uint8_t));
// //     AVFrame* rgbFrame = av_frame_alloc();
// //     av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer, AV_PIX_FMT_RGB24,
// //                          inputFrame.codecCtx->width, inputFrame.codecCtx->height, 1);

// //     sws_scale(swsCtx, inputFrame.data->data, inputFrame.data->linesize, 0, inputFrame.codecCtx->height,
// //               rgbFrame->data, rgbFrame->linesize);

// //     GdkPixbuf* pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, false, 8, inputFrame.codecCtx->width, inputFrame.codecCtx->height);
// //     if (pixbuf) {
// //         guchar* dest = gdk_pixbuf_get_pixels(pixbuf);
// //         int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
// //         for (int y = 0; y < inputFrame.codecCtx->height; ++y) {
// //             memcpy(dest + y * rowstride, rgbFrame->data[0] + y * rgbFrame->linesize[0], inputFrame.codecCtx->width * 3);
// //         }
// //     }

// //     sws_freeContext(swsCtx);
// //     av_free(buffer);
// //     av_frame_free(&rgbFrame);
// //     return pixbuf;
// // }

// // window create_window(char* title, window* win) {
// //     gtk_init(nullptr, nullptr);

// //     win->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
// //     gtk_window_set_title(GTK_WINDOW(win->window), title);
// //     gtk_window_set_default_size(GTK_WINDOW(win->window), 800, 600);
// //     g_signal_connect(win->window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);

// //     win->image = gtk_image_new();
// //     gtk_container_add(GTK_CONTAINER(win->window), win->image);

// //     return *win;
// // }

// // int8_t imshow(window* win, const frame& imgFrame) {
// //     if (!imgFrame.data) {
// //         std::cerr << "Ошибка: Пустой кадр, невозможно отобразить изображение!" << std::endl;
// //         return -1;
// //     }

// //     GdkPixbuf* pixbuf = convert_frame_to_pixbuf(imgFrame);
// //     if (!pixbuf) {
// //         std::cerr << "Ошибка преобразования изображения!" << std::endl;
// //         return -1;
// //     }

// //     gtk_image_set_from_pixbuf(GTK_IMAGE(win->image), pixbuf);
// //     g_object_unref(pixbuf);

// //     gtk_widget_show_all(win->window);
// //     gtk_main();

// //     return 0;
// // }