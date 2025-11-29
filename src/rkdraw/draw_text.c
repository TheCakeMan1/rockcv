#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "rkdraw.h"
#include "font.h"

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

// Преобразует UTF-8 в Unicode (UCS-4)

static int clamp(float val, int min, int max)
{
    return val > min ? (val < max ? val : max) : min;
}

__attribute__((pure)) static unsigned int utf8_next(const char **s)
{
    const unsigned char *p = (const unsigned char *)(*s);
    unsigned int ch;

    if (p[0] < 0x80)
    {
        ch = p[0];
        (*s)++;
    }
    else if ((p[0] & 0xE0) == 0xC0)
    {
        ch = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
        (*s) += 2;
    }
    else if ((p[0] & 0xF0) == 0xE0)
    {
        ch = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
        (*s) += 3;
    }
    else
    {
        ch = '?';
        (*s)++;
    }

    return ch;
}
// src color format(ARGB888) To dest format color
static unsigned int convert_color(unsigned int src_color, int dst_fmt)
{
    // printf("sizeof(int)=%d\n", sizeof(int));
    unsigned int dst_color = 0x0;
    unsigned char *p_src_color = (unsigned char *)&src_color;
    unsigned char *p_dst_color = (unsigned char *)&dst_color;
    char r = p_src_color[2];
    char g = p_src_color[1];
    char b = p_src_color[0];
    char a = p_src_color[3];

    switch (dst_fmt)
    {
    case RK_PIX_FMT_RGB_888:
        p_dst_color[0] = r;
        p_dst_color[1] = g;
        p_dst_color[2] = b;
        break;
    case RK_PIX_FMT_RGBA_8888:
        p_dst_color[0] = r;
        p_dst_color[1] = g;
        p_dst_color[2] = b;
        p_dst_color[3] = a;
        break;
    case RK_PIX_FMT_YCbCr_420_SP:
        p_dst_color[0] = (0.299 * r + 0.587 * g + 0.114 * b);
        p_dst_color[1] = clamp(128 + (-0.14713 * r - 0.28886 * g + 0.436 * b), 0, 255);
        p_dst_color[2] = clamp(128 + (0.615 * r - 0.51499 * g - 0.10001 * b), 0, 255);
        break;
    case RK_PIX_FMT_YCrCb_420_SP:
        p_dst_color[0] = 0.299 * r + 0.587 * g + 0.114 * b;
        p_dst_color[1] = 0.877 * (r - p_dst_color[0]);
        p_dst_color[2] = 0.492 * (b - p_dst_color[0]);
        break;
    default:
        break;
    }
    return dst_color;
}

static inline int distance_lessequal(int x0, int y0, int x1, int y1, int r2)
{
    int dx = x0 - x1;
    int dy = y0 - y1;
    int q = dx * dx + dy * dy;
    return q <= r2;
}

static inline int distance_inrange(int x0, int y0, int x1, int y1, float r0, float r1)
{
    int dx = x0 - x1;
    int dy = y0 - y1;
    int q = dx * dx + dy * dy;
    return q >= r0 * r0 && q < r1 * r1;
}

static inline int distance_lessthan(
    int x, int y,
    int x0, int y0,
    int x1, int y1,
    int t2 // квадрат допустимого расстояния
)
{
    int vx = x1 - x0;
    int vy = y1 - y0;
    int wx = x - x0;
    int wy = y - y0;

    int c1 = wx * vx + wy * vy;
    if (c1 <= 0)
        return 0;

    int c2 = vx * vx + vy * vy;
    if (c1 >= c2)
        return 0;

    // cross = |w × v|
    int cross = wx * vy - wy * vx;
    long long cross2 = (long long)cross * cross;

    // Сравниваем без деления
    return cross2 < (long long)t2 * c2;
}

static void get_text_drawing_size(const char *text, int fontpixelsize, int *w, int *h)
{
    *w = 0;
    *h = 0;

    const int n = strlen(text);

    int line_w = 0;
    for (int i = 0; i < n; i++)
    {
        char ch = text[i];

        if (ch == '\n')
        {
            // newline
            *w = max(*w, line_w);
            *h += fontpixelsize * 2;
            line_w = 0;
        }

        if (isprint(ch) != 0)
        {
            line_w += fontpixelsize;
        }
    }

    *w = max(*w, line_w);
    *h += fontpixelsize * 2;
}

static inline int resize_bilinear_c1(const unsigned char *src_pixels, int w, int h,
                                     unsigned char *dst_pixels, int w2, int h2)
{
    if (!src_pixels || !dst_pixels || w < 2 || h < 2 || w2 < 1 || h2 < 1)
        return -1;

    float x_ratio = (float)(w - 1) / (w2 - 1);
    float y_ratio = (float)(h - 1) / (h2 - 1);

    for (int i = 0; i < h2; i++)
    {
        float sy = i * y_ratio;
        int y = (int)sy;
        float y_diff = sy - y;

        for (int j = 0; j < w2; j++)
        {
            float sx = j * x_ratio;
            int x = (int)sx;
            float x_diff = sx - x;

            int index = y * w + x;

            int A = src_pixels[index];
            int B = src_pixels[index + 1];
            int C = src_pixels[index + w];
            int D = src_pixels[index + w + 1];

            float grayf = A * (1 - x_diff) * (1 - y_diff) +
                          B * (x_diff) * (1 - y_diff) +
                          C * (y_diff) * (1 - x_diff) +
                          D * (x_diff * y_diff);

            dst_pixels[i * w2 + j] = (unsigned char)(grayf + 0.5f);
        }
    }

    return 0;
}
static inline void blend_row_neon(
    unsigned char *pout,
    const unsigned char *palpha,
    int count,
    uint8_t pen)
{
    uint8x16_t vpen = vdupq_n_u8(pen);
    uint8x16_t v255 = vdupq_n_u8(255);

    while (count >= 16)
    {
        uint8x16_t vp = vld1q_u8(pout);   // destination
        uint8x16_t va = vld1q_u8(palpha); // alpha

        uint8x16_t vinva = vsubq_u8(v255, va); // inv_alpha = 255 - a

        uint16x8_t lo1 = vmull_u8(vget_low_u8(vp), vget_low_u8(vinva));
        uint16x8_t hi1 = vmull_u8(vget_high_u8(vp), vget_high_u8(vinva));

        uint16x8_t lo2 = vmlal_u8(lo1, vget_low_u8(vpen), vget_low_u8(va));
        uint16x8_t hi2 = vmlal_u8(hi1, vget_high_u8(vpen), vget_high_u8(va));

        uint8x16_t vres = vcombine_u8(
            vshrn_n_u16(lo2, 8),
            vshrn_n_u16(hi2, 8));

        vst1q_u8(pout, vres);

        pout += 16;
        palpha += 16;
        count -= 16;
    }

    // остаток (<16)
    while (count--)
    {
        uint8_t a = *palpha++;
        uint8_t ia = 255 - a;
        *pout = ((*pout) * ia + pen * a) >> 8;
        pout++;
    }
}
static void draw_text_c1(unsigned char *pixels, int w, int h,
                         const char *text, int x, int y,
                         int fontpixelsize, unsigned int color)
{
    const unsigned char *pen_color = (const unsigned char *)&color;
    int stride = w;
    const uint8_t pen = (uint8_t)(color & 0xFF); // один канал (C1)

    unsigned char *resized_font_bitmap = (unsigned char *)malloc(fontpixelsize * fontpixelsize * 2);
    if (!resized_font_bitmap)
        return;

    int cursor_x = x;
    int cursor_y = y;
    int w_b, h_b;

    const char *p = text;
    while (*p)
    {
        unsigned int code = utf8_next(&p); // читаем символ UTF-8 → Unicode

        if (code == '\n')
        {
            cursor_x = x;
            cursor_y += fontpixelsize * 2;
            continue;
        }

        const unsigned char *font_bitmap = NULL;

        // ---- Выбор таблицы ----
        if (code >= 32 && code <= 126)
        {
            // ASCII
            font_bitmap = mono_font_data[code - 32];
            w_b = 20;
            h_b = 40;
        }
        else if (code == 0x401)
        {
            // Ё (между Е и Ж)
            font_bitmap = mono_font_data_rus[6]; // после Е
            w_b = 13;
            h_b = 26;
        }
        else if (code == 0x451)
        {
            // ё (между е и ж)
            font_bitmap = mono_font_data_rus[39]; // после е
            w_b = 13;
            h_b = 26;
        }
        else if (code >= 0x410 && code <= 0x44F)
        {
            // Кириллица А–я
            int idx = code - 0x410;

            if (code > 0x415) // после Е
                idx++;        // сдвиг из-за вставки Ё
            if (code > 0x435) // после е
                idx++;        // сдвиг из-за вставки ё

            font_bitmap = mono_font_data_rus[idx];
            w_b = 13;
            h_b = 26;
        }
        else
        {
            continue;
        }

        // ---- Масштабируем ----
        resize_bilinear_c1(font_bitmap, w_b, h_b, resized_font_bitmap,
                           fontpixelsize, fontpixelsize * 2);

// ---- Отрисовка ----
#if defined(__ARM_NEON__) || defined(__ARM_NEON)
        for (int j = 0; j < fontpixelsize * 2; j++)
        {
            int Y = cursor_y + j;
            if (Y < 0 || Y >= h)
                continue;

            const unsigned char *palpha =
                resized_font_bitmap + j * fontpixelsize;
            unsigned char *pout = pixels + Y * stride;

            int startX = cursor_x;
            int endX = cursor_x + fontpixelsize;

            if (startX < 0)
                startX = 0;
            if (endX > w)
                endX = w;

            int len = endX - startX;
            if (len <= 0)
                continue;

            blend_row_neon(
                pout + startX,
                palpha + (startX - cursor_x),
                len,
                pen);
        }

#else
        for (int j = cursor_y; j < cursor_y + fontpixelsize * 2; j++)
        {
            if (j < 0)
                continue;
            if (j >= h)
                break;

            const unsigned char *palpha = resized_font_bitmap + (j - cursor_y) * fontpixelsize;
            unsigned char *pout = pixels + stride * j;

            for (int k = cursor_x; k < cursor_x + fontpixelsize; k++)
            {
                if (k < 0)
                    continue;
                if (k >= w)
                    break;

                unsigned char alpha = palpha[k - cursor_x];
                pout[k] = (pout[k] * (255 - alpha) + pen_color[0] * alpha) / 255;
            }
        }
#endif

        cursor_x += fontpixelsize + 2;
    }

    free(resized_font_bitmap);
}

static void draw_text_c2(unsigned char *pixels, int w, int h,
                         const char *text, int x, int y,
                         int fontpixelsize, unsigned int color)
{
    const unsigned char *pen_color = (const unsigned char *)&color;
    int stride = w * 2; // два канала на пиксель

    unsigned char *resized_font_bitmap = (unsigned char *)malloc(fontpixelsize * fontpixelsize * 2);
    if (!resized_font_bitmap)
        return;

    int cursor_x = x;
    int cursor_y = y;
    int w_b, h_b;

    const char *ptext = text;
    while (*ptext)
    {
        unsigned int code = utf8_next(&ptext);

        if (code == '\n')
        {
            cursor_x = x;
            cursor_y += fontpixelsize * 2;
            continue;
        }

        const unsigned char *font_bitmap = NULL;

        // ---- Выбор таблицы ----
        if (code >= 32 && code <= 126)
        {
            // ASCII
            font_bitmap = mono_font_data[code - 32];
            w_b = 20;
            h_b = 40;
        }
        else if (code == 0x401)
        {
            // Ё (между Е и Ж)
            font_bitmap = mono_font_data_rus[6]; // после Е
            w_b = 13;
            h_b = 26;
        }
        else if (code == 0x451)
        {
            // ё (между е и ж)
            font_bitmap = mono_font_data_rus[6 + (0x416 - 0x410)]; // после е
            w_b = 13;
            h_b = 26;
        }
        else if (code >= 0x410 && code <= 0x44F)
        {
            // Кириллица А–я
            int idx = code - 0x410;

            if (code > 0x415) // после Е
                idx++;        // сдвиг из-за вставки Ё
            if (code > 0x435) // после е
                idx++;        // сдвиг из-за вставки ё

            font_bitmap = mono_font_data_rus[idx];
            w_b = 13;
            h_b = 26;
        }
        else
        {
            continue;
        }

        // --- масштабируем символ ---

        resize_bilinear_c1(font_bitmap, w_b, h_b,
                           resized_font_bitmap,
                           fontpixelsize, fontpixelsize * 2);
        // printf("U+%04X w_b=%d h_b=%d font_bitmap=%p resized_font_bitmap=%p\n", code, w_b, h_b, font_bitmap, resized_font_bitmap);

        // --- рендер ---
        for (int j = cursor_y; j < cursor_y + fontpixelsize * 2; j++)
        {
            if (j < 0)
                continue;
            if (j >= h)
                break;

            const unsigned char *palpha = resized_font_bitmap + (j - cursor_y) * fontpixelsize;
            unsigned char *p = pixels + stride * j;

            for (int k = cursor_x; k < cursor_x + fontpixelsize; k++)
            {
                if (k < 0)
                    continue;
                if (k >= w)
                    break;

                unsigned char alpha = palpha[k - cursor_x];
                int px = k * 2;

                p[px + 0] = (p[px + 0] * (255 - alpha) + pen_color[0] * alpha) / 255;
                p[px + 1] = (p[px + 1] * (255 - alpha) + pen_color[1] * alpha) / 255;
            }
        }

        cursor_x += fontpixelsize + 2;
    }

    free(resized_font_bitmap);
}

static void draw_text_c3(unsigned char *pixels, int w, int h, const char *text, int x, int y, int fontpixelsize,
                         unsigned int color)
{
    const unsigned char *pen_color = (const unsigned char *)&color;
    int stride = w * 3;

    unsigned char *resized_font_bitmap = (unsigned char *)malloc(fontpixelsize * fontpixelsize * 2);

    const int n = strlen(text);

    int cursor_x = x;
    int cursor_y = y;
    for (int i = 0; i < n; i++)
    {
        char ch = text[i];

        if (ch == '\n')
        {
            // newline
            cursor_x = x;
            cursor_y += fontpixelsize * 2;
        }

        if (isprint(ch) != 0)
        {
            int font_bitmap_index = ch - ' ';
            const unsigned char *font_bitmap = mono_font_data[font_bitmap_index];

            // draw resized character
            resize_bilinear_c1(font_bitmap, 20, 40, resized_font_bitmap, fontpixelsize, fontpixelsize * 2);

            for (int j = cursor_y; j < cursor_y + fontpixelsize * 2; j++)
            {
                if (j < 0)
                    continue;

                if (j >= h)
                    break;

                const unsigned char *palpha = resized_font_bitmap + (j - cursor_y) * fontpixelsize;
                unsigned char *p = pixels + stride * j;

                for (int k = cursor_x; k < cursor_x + fontpixelsize; k++)
                {
                    if (k < 0)
                        continue;

                    if (k >= w)
                        break;

                    unsigned char alpha = palpha[k - cursor_x];

                    p[k * 3 + 0] = (p[k * 3 + 0] * (255 - alpha) + pen_color[0] * alpha) / 255;
                    p[k * 3 + 1] = (p[k * 3 + 1] * (255 - alpha) + pen_color[1] * alpha) / 255;
                    p[k * 3 + 2] = (p[k * 3 + 2] * (255 - alpha) + pen_color[2] * alpha) / 255;
                }
            }

            cursor_x += fontpixelsize;
        }
    }

    free(resized_font_bitmap);
}

static void draw_text_c4(unsigned char *pixels, int w, int h, const char *text, int x, int y, int fontpixelsize,
                         unsigned int color)
{
    const unsigned char *pen_color = (const unsigned char *)&color;
    int stride = w * 4;

    unsigned char *resized_font_bitmap = (unsigned char *)malloc(fontpixelsize * fontpixelsize * 2);

    const int n = strlen(text);

    int cursor_x = x;
    int cursor_y = y;
    for (int i = 0; i < n; i++)
    {
        char ch = text[i];

        if (ch == '\n')
        {
            // newline
            cursor_x = x;
            cursor_y += fontpixelsize * 2;
        }

        if (isprint(ch) != 0)
        {
            const unsigned char *font_bitmap = mono_font_data[ch - ' '];

            // draw resized character
            resize_bilinear_c1(font_bitmap, 20, 40, resized_font_bitmap, fontpixelsize, fontpixelsize * 2);

            for (int j = cursor_y; j < cursor_y + fontpixelsize * 2; j++)
            {
                if (j < 0)
                    continue;

                if (j >= h)
                    break;

                const unsigned char *palpha = resized_font_bitmap + (j - cursor_y) * fontpixelsize;
                unsigned char *p = pixels + stride * j;

                for (int k = cursor_x; k < cursor_x + fontpixelsize; k++)
                {
                    if (k < 0)
                        continue;

                    if (k >= w)
                        break;

                    unsigned char alpha = palpha[k - cursor_x];

                    p[k * 4 + 0] = (p[k * 4 + 0] * (255 - alpha) + pen_color[0] * alpha) / 255;
                    p[k * 4 + 1] = (p[k * 4 + 1] * (255 - alpha) + pen_color[1] * alpha) / 255;
                    p[k * 4 + 2] = (p[k * 4 + 2] * (255 - alpha) + pen_color[2] * alpha) / 255;
                    p[k * 4 + 3] = (p[k * 4 + 3] * (255 - alpha) + pen_color[3] * alpha) / 255;
                }
            }

            cursor_x += fontpixelsize;
        }
    }

    free(resized_font_bitmap);
}

static void draw_text_yuv420sp(unsigned char *yuv420sp, int w, int h, const char *text, int x, int y, int fontpixelsize,
                               unsigned int color)
{
    const unsigned char *pen_color = (const unsigned char *)&color;

    unsigned int v_y;
    unsigned int v_uv;
    unsigned char *pen_color_y = (unsigned char *)&v_y;
    unsigned char *pen_color_uv = (unsigned char *)&v_uv;
    pen_color_y[0] = pen_color[0];
    pen_color_uv[0] = pen_color[1];
    pen_color_uv[1] = pen_color[2];

    unsigned char *Y = yuv420sp;
    draw_text_c1(Y, w, h, text, x, y, fontpixelsize, v_y);

    unsigned char *UV = yuv420sp + w * h;
    draw_text_c2(UV, w / 2, h / 2, text, x / 2, y / 2, max(fontpixelsize / 2, 1), v_uv);
}

void rbbox_to_corners(const float *in_rbbox, int *out_rbbox)
{
    // generate clockwise corners and rotate it clockwise
    // 顺时针方向返回角点位置
    float cx = in_rbbox[0] + in_rbbox[2] / 2;
    float cy = in_rbbox[1] + in_rbbox[3] / 2;
    float x_d = in_rbbox[2];
    float y_d = in_rbbox[3];
    float angle = in_rbbox[4];
    float a_cos = cos(angle);
    float a_sin = sin(angle);
    float corners_x[4] = {-x_d / 2, -x_d / 2, x_d / 2, x_d / 2};
    float corners_y[4] = {-y_d / 2, y_d / 2, y_d / 2, -y_d / 2};
    for (int i = 0; i < 4; ++i)
    {
        out_rbbox[2 * i] = (int)(a_cos * corners_x[i] - a_sin * corners_y[i] + cx);
        out_rbbox[2 * i + 1] = (int)(a_sin * corners_x[i] + a_cos * corners_y[i] + cy);
    }
}

__attribute__((hot, fluten)) void draw_text(image_buffer_t *image, const char *text, int x, int y, unsigned int color,
                                            int fontsize)
{
    unsigned char *pixels = image->virt_addr;
    int w = image->width;
    int h = image->height;
    unsigned int draw_color = convert_color(color, image->format);

    switch (image->format)
    {
    case RK_PIX_FMT_RGB_888:
        draw_text_c3(pixels, w, h, text, x, y, fontsize, draw_color);
        break;
    case RK_PIX_FMT_RGBA_8888:
        draw_text_c4(pixels, w, h, text, x, y, fontsize, draw_color);
        break;
    case RK_PIX_FMT_YCbCr_420_SP:
    case RK_PIX_FMT_YCrCb_420_SP:
        draw_text_yuv420sp(pixels, w, h, text, x, y, fontsize, draw_color);
        break;
    default:
        printf("no support format %d", image->format);
        break;
    }
}
