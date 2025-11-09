/**
 * Copyright (c) 2020 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `log.c` for details.
 */

#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>

#define likely(x) __builtin_expect(!!(x), 1)   // часто
#define unlikely(x) __builtin_expect(!!(x), 0) // нечасто

#ifdef BUILD_DEV
static int START_VIDEO = 0;
static int END_VIDEO = 0;
#endif

#define LOG_VERSION "0.1.0"

typedef struct
{
    va_list ap;
    const char *fmt;
    const char *file;
    struct tm *time;
    void *udata;
    int line;
    int level;
} log_Event;

typedef void (*log_LogFn)(log_Event *ev);
typedef void (*log_LockFn)(bool lock, void *udata);

enum
{
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
};

#define log_trace(...) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...) log_log(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...) log_log(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

const char *log_level_string(int level);
void log_set_lock(log_LockFn fn, void *udata);
void log_set_level(int level);
void log_set_quiet(bool enable);
int log_add_callback(log_LogFn fn, void *udata, int level);
int log_add_fp(FILE *fp, int level);

void log_log(int level, const char *file, int line, const char *fmt, ...);

#ifdef BUILD_DEV
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
static int init_freetype_once(const char *font_path, int px)
{
    static FT_Library g_ft = NULL;
    static FT_Face g_face = NULL;
    static int inited = 0;
    if (inited)
        return 0;

    if (FT_Init_FreeType(&g_ft))
        return -1;
    if (FT_New_Face(g_ft, font_path, 0, &g_face))
        return -2;
    FT_Set_Pixel_Sizes(g_face, 0, px);
    inited = 1;
    return 0;
}
__attribute__((constructor)) static void aaa(void)
{
    init_freetype_once("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 36);
}
__attribute__((destructor)) static void check_calls(void)
{
    log_debug("Попытка выхода из программы.");
    if (START_VIDEO && !END_VIDEO)
        log_debug("Вы вызвали video из rkvideo, но не вызвали release для ctx");
}
#endif
#endif