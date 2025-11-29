
#ifndef _RKDRAW_H
#define _RKDRAW_H

#include "rkframe.h"

#if defined(__ARM_NEON__) || defined(__ARM_NEON)
#include <arm_neon.h>
#endif

/**
 * @brief Image buffer
 *
 */
typedef struct
{
    int width;
    int height;
    int width_stride;
    int height_stride;
    int format;
    unsigned char *virt_addr;
    int size;
    int fd;
} image_buffer_t;

typedef enum
{
    RKCOLOR_GREEN = 0xFF00FF00,
    RKCOLOR_BLUE = 0xFF0000FF,
    RKCOLOR_RED = 0xFFFF0000,
    RKCOLOR_YELLOW = 0xFFFFFF00,
    RKCOLOR_ORANGE = 0xFFFF4500,
    RKCOLOR_BLACK = 0xFF000000,
    RKCOLOR_WHITE = 0xFFFFFFFF
} rkcolor;

void draw_text(image_buffer_t *image, const char *text, int x, int y, unsigned int color,
               int fontsize);

#endif