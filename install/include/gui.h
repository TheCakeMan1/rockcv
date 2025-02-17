#ifndef ROCKCV_GUI_H
#define ROCKCV_GUI_H

#include <cstdint>
#include <cstdio>
#include <ostream>
#include <iostream>
// #include "img_op.h"

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
}
#include <iostream>

// class ImageViewer : public QThread {
//     Q_OBJECT
// public:
//     ImageViewer();
//     ~ImageViewer();
//     void run() override;
//     void showImage(const frame& imgFrame);

// private:
//     QLabel* label;
//     std::mutex img_mutex;
// };

// #ifdef __cplusplus
// extern "C" {
// #endif

// extern window win;

// frame imread(const char* filename);
// QImage convert_frame_to_qimage(const frame& inputFrame);
// #ifdef __cplusplus
// }
// #endif

#endif