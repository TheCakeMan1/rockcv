#ifndef _RKDRAW_STRUCT_H
#define _RKDRAW_STRUCT_H

/**
 * @brief Image rectangle
 *
 */
typedef struct
{
    int left;
    int top;
    int right;
    int bottom;
} image_rect_t;

/**
 * @brief Image obb rectangle
 *
 */
typedef struct
{
    int x;
    int y;
    int w;
    int h;
    float angle;
} image_obb_box_t;

#endif