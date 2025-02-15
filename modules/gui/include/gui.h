#ifndef ROCKCV_GUI_H
#define ROCKCV_GUI_H

#include <cstdint>
#include <cstdio>
#include "SDL2/SDL.h"
#include <ostream>
#include <iostream>

    #include <SDL2/SDL.h>
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif

int8_t create_win(const int SCREEN_WIDTH, const int SCREEN_HEIGHT);
int he();

#ifdef __cplusplus
}
#endif

#endif