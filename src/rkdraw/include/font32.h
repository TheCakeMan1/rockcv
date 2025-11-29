#ifndef FONT32
#define FONT32

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "im2d.h"
#include <RgaApi.h>
#include "im2d_buffer.h"
#include "im2d_single.h"

typedef struct
{
    uint32_t codepoint;
    uint16_t width;
    uint16_t height;
    int16_t bearing_x;
    int16_t bearing_y;
    int16_t advance;
    const uint32_t *bitmap;
} Glyph;

extern const Glyph font32[];
extern const uint32_t font32_count;

static inline const Glyph *find_glyph(uint32_t cp)
{
    for (uint32_t i = 0; i < font32_count; i++)
        if (font32[i].codepoint == cp)
            return &font32[i];
    return NULL;
}

static inline void blit_glyph_argb(
    rga_buffer_t dst, // полный ARGB кадр
    int frame_w,
    int frame_h,
    int x, int y,
    const Glyph *g)
{
    int gw = g->width;
    int gh = g->height;

    // создаём фулл-сайз ARGB с прозрачностью
    int size = frame_w * frame_h * 4;
    uint32_t *full = calloc(1, size);
    if (!full)
        return;

    // кладём глиф в нужное место
    for (int j = 0; j < gh; j++)
    {
        memcpy(full + (y + j) * frame_w + x,
               g->bitmap + j * gw,
               gw * 4);
    }

    // оборачиваем как буфер
    rga_buffer_t src = wrapbuffer_virtualaddr(
        full, frame_w, frame_h, RK_FORMAT_ARGB_8888);

    // прямое наложение всего кадра
    int ret = imblend(src, dst, IM_ALPHA_BLEND_SRC_OVER);
    if (ret != IM_STATUS_SUCCESS)
        fprintf(stderr, "imblend failed: %d\n", ret);

    free(full);
}
static void draw_text_argb(
    rga_buffer_t dst,
    int w, int h,
    int start_x,
    int baseline_y,
    const char *text)
{
    int px = start_x;
    int py = baseline_y;

    while (*text)
    {
        uint8_t cp = *text++;
        const Glyph *g = find_glyph(cp);
        if (!g)
            continue;

        int gx = px + g->bearing_x;
        int gy = py - g->bearing_y;

        if (gx >= 0 && gy >= 0 &&
            gx + g->width <= w &&
            gy + g->height <= h)
        {
            blit_glyph_argb(dst, w, h, gx, gy, g);
        }

        px += g->advance;
    }
}
static void overlay_text_nv12_im2d(
    int fd_nv12,
    int w, int h,
    const char *text)
{
    int pixels = w * h;
    int argb_size = pixels * 4;

    uint32_t *argb = malloc(argb_size);
    if (!argb)
        return;

    rga_buffer_t src_nv12 = wrapbuffer_fd(fd_nv12, w, h, RK_FORMAT_YCbCr_420_SP);
    rga_buffer_t dst_argb = wrapbuffer_virtualaddr(argb, w, h, RK_FORMAT_ARGB_8888);

    // NV12 → ARGB
    if (imcvtcolor(src_nv12, dst_argb,
                   RK_FORMAT_ABGR_8888,
                   0, 0) != IM_STATUS_SUCCESS)
        fprintf(stderr, "NV12->ARGB fail\n");

    // draw text
    draw_text_argb(dst_argb, w, h, 50, h - 40, text);

    // ARGB → NV12
    if (imcvtcolor(dst_argb, src_nv12,
                   RK_FORMAT_YCbCr_420_SP,
                   0, 0) != IM_STATUS_SUCCESS)
        fprintf(stderr, "ARGB->NV12 fail\n");

    free(argb);
}

// static void overlay_text_nv12_im2d(
//     int fd_nv12,    // NV12 framebuffer (dst)
//     int fd_rgba,    // RGBA8888 overlay (src)
//     int w, int h,   // overlay width/height
//     int ws, int hs) // NV12 strides: ws=y_stride, hs=height_stride
// {
//     rga_info_t src;
//     rga_info_t dst;
//     int ret;

//     memset(&src, 0, sizeof(src));
//     memset(&dst, 0, sizeof(dst));

//     /* ============================================================
//        SRC: RGBA8888 overlay
//        ============================================================ */
//     src.fd = fd_rgba;
//     src.mmuFlag = 1;
//     src.blend = 1; // включить простое альфа-смешивание

//     src.rect.xoffset = 0;
//     src.rect.yoffset = 0;
//     src.rect.width = w;
//     src.rect.height = h;
//     src.rect.wstride = w * 4; // stride для RGBA8888
//     src.rect.hstride = h;
//     src.rect.format = RK_FORMAT_RGBA_8888;

//     /* ============================================================
//        DST: NV12 framebuffer
//        ============================================================ */
//     dst.fd = fd_nv12;
//     dst.mmuFlag = 1;

//     dst.rect.xoffset = 0;
//     dst.rect.yoffset = 0;
//     dst.rect.width = w;
//     dst.rect.height = h;
//     dst.rect.wstride = ws;                    // y_stride NV12
//     dst.rect.hstride = hs;                    // height_stride NV12
//     dst.rect.format = RK_FORMAT_YCbCr_420_SP; // NV12

//     /* ============================================================
//        RGA BLIT
//        ============================================================ */
//     ret = RgaBlit(&src, &dst, NULL);
//     if (ret)
//     {
//         printf("RGA overlay error = %d\n", ret);
//     }
// }

#endif
