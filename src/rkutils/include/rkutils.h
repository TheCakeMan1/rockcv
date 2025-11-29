#ifndef RKUTILS_H
#define RKUTILS_H

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <execinfo.h>
#include <signal.h>
#include "unistd.h"

#define likely(x) __builtin_expect(!!(x), 1)   // часто
#define unlikely(x) __builtin_expect(!!(x), 0) // нечасто

#define RKLOGX_ENABLE_COLOR 1

#if defined(__GNUC__) && !defined(__clang__)
#define OPT_O3 __attribute__((optimize("O3")))
#else
#define OPT_O3
#endif
#ifdef BUILD_DEV
static int START_VIDEO = 0;
static int END_VIDEO = 0;
#endif

// ============================ Конфигурация ============================
#ifndef RKLOGX_MAX_SINKS
#define RKLOGX_MAX_SINKS 8
#endif

#ifndef RKLOGX_DEFAULT_LEVEL
#define RKLOGX_DEFAULT_LEVEL 2 /* INFO */
#endif

#ifndef RKLOGX_ENABLE_COLOR
#define RKLOGX_ENABLE_COLOR 1
#endif

#ifndef RKLOGX_TIMESTAMP_LEN
#define RKLOGX_TIMESTAMP_LEN 32
#endif

#ifndef RKLOGX_MAX_TAG
#define RKLOGX_MAX_TAG 32
#endif

#ifndef RKLOGX_DEFAULT_TAG
#define RKLOGX_DEFAULT_TAG "APP"
#endif

// ============================ Типы и состояние ============================
typedef enum
{
    RKLOGX_TRACE = 0,
    RKLOGX_DEBUG = 1,
    RKLOGX_INFO = 2,
    RKLOGX_WARN = 3,
    RKLOGX_ERROR = 4,
    RKLOGX_FATAL = 5
} rklogx_level_t;

struct rklogx_event
{
    rklogx_level_t level;
    const char *tag;
    const char *file;
    int line;
    const char *fmt;
    va_list ap;
    struct tm tmval;
    uint64_t usec; // монотонная метка времени в мкс
    void *udata;   // пользовательские данные для приёмника
};

typedef void (*rklogx_sink_fn)(const struct rklogx_event *ev);

typedef struct
{
    rklogx_sink_fn fn;
    void *udata;
    rklogx_level_t level;
} rklogx_sink_t;

typedef void (*rklogx_lock_fn)(bool lock, void *udata);

typedef struct
{
    rklogx_lock_fn lock_fn;
    void *lock_udata;
    rklogx_level_t level;
    bool quiet;
    bool color;
    rklogx_sink_t sinks[RKLOGX_MAX_SINKS];
} rklogx_state_t;

static rklogx_state_t rklogx_g = {
    .lock_fn = NULL,
    .lock_udata = NULL,
    .level = (rklogx_level_t)RKLOGX_DEFAULT_LEVEL,
    .quiet = false,
    .color = (RKLOGX_ENABLE_COLOR != 0)};

// ============================ Вспомогательные ============================
static inline uint64_t rklogx_now_us(void)
{
#if defined(CLOCK_REALTIME)
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000ull + (uint64_t)ts.tv_nsec / 1000ull;
#else
    // Fallback на time()
    return (uint64_t)time(NULL) * 1000000ull;
#endif
}

static inline void rklogx_fill_tm(struct tm *out)
{
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS)
    time_t t = time(NULL);
    localtime_r(&t, out);
#else
    time_t t = time(NULL);
    struct tm *p = localtime(&t);
    if (p)
        *out = *p;
#endif
}

static inline const char *rklogx_level_str(rklogx_level_t lvl)
{
    switch (lvl)
    {
    case RKLOGX_TRACE:
        return "TRACE";
    case RKLOGX_DEBUG:
        return "DEBUG";
    case RKLOGX_INFO:
        return "INFO";
    case RKLOGX_WARN:
        return "WARN";
    case RKLOGX_ERROR:
        return "ERROR";
    case RKLOGX_FATAL:
        return "FATAL";
    default:
        return "?";
    }
}

static inline const char *rklogx_level_color(rklogx_level_t lvl)
{
#if RKLOGX_ENABLE_COLOR
    switch (lvl)
    {
    case RKLOGX_TRACE:
        return "\x1b[90m"; // ярко-серый
    case RKLOGX_DEBUG:
        return "\x1b[36m"; // циан
    case RKLOGX_INFO:
        return "\x1b[32m"; // зелёный
    case RKLOGX_WARN:
        return "\x1b[33m"; // жёлтый
    case RKLOGX_ERROR:
        return "\x1b[31m"; // красный
    case RKLOGX_FATAL:
        return "\x1b[35m"; // пурпурный
    default:
        return "\x1b[0m";
    }
#else
    (void)lvl;
    return "";
#endif
}

static inline void rklogx_lock(void)
{
    if (rklogx_g.lock_fn)
        rklogx_g.lock_fn(true, rklogx_g.lock_udata);
}
static inline void rklogx_unlock(void)
{
    if (rklogx_g.lock_fn)
        rklogx_g.lock_fn(false, rklogx_g.lock_udata);
}

// ============================ Приёмники (sinks) ============================
static void rklogx_stdout_sink(const struct rklogx_event *ev)
{
    char tbuf[RKLOGX_TIMESTAMP_LEN];
    strftime(tbuf, sizeof(tbuf), "%H:%M:%S", &ev->tmval); // время компактнее

    const char *level_str = rklogx_level_str(ev->level);
    const char *level_color = rklogx_level_color(ev->level);
    const char *tag = ev->tag ? ev->tag : RKLOGX_DEFAULT_TAG;
    const char *file = ev->file ? ev->file : "?";

#if RKLOGX_ENABLE_COLOR
    if (rklogx_g.color)
    {
        // Пример цветного и форматированного вывода:
        fprintf((FILE *)ev->udata,
                "\x1b[90m%s\x1b[0m "         // серое время
                "%s%-5s\x1b[0m "             // цветной уровень
                "\x1b[36m%-*s\x1b[0m "       // голубой тег
                "\x1b[2m%s:%d\x1b[0m\n    ", // тусклый файл:строка
                tbuf,
                level_color, level_str,
                RKLOGX_MAX_TAG, tag,
                file, ev->line);
    }
    else
#endif
    {
        fprintf((FILE *)ev->udata,
                "%s %-5s %-*s %s:%d\n    ",
                tbuf, level_str, RKLOGX_MAX_TAG, tag, file, ev->line);
    }

    // сам текст сообщения
    vfprintf((FILE *)ev->udata, ev->fmt, ev->ap);
    fputc('\n', (FILE *)ev->udata);
    fflush((FILE *)ev->udata);
}

// Внутренний udata для вращаемого файла
typedef struct
{
    FILE *fp;
    char *path;
    size_t max_bytes;
} rklogx_rotfile_t;

static void rklogx_file_sink(const struct rklogx_event *ev)
{
    char tbuf[RKLOGX_TIMESTAMP_LEN];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", &ev->tmval);
    FILE *fp = (FILE *)ev->udata;
    fprintf(fp, "%s %-5s %-*s %s:%d: ", tbuf, rklogx_level_str(ev->level), RKLOGX_MAX_TAG,
            ev->tag ? ev->tag : RKLOGX_DEFAULT_TAG, ev->file ? ev->file : "?", ev->line);
    vfprintf(fp, ev->fmt, ev->ap);
    fputc('\n', fp);
    fflush(fp);
}

static void rklogx_rotfile_sink(const struct rklogx_event *ev)
{
    rklogx_rotfile_t *rf = (rklogx_rotfile_t *)ev->udata;
    if (!rf || !rf->fp)
        return;
    // Проверка размера
    long pos = ftell(rf->fp);
    if (pos < 0)
        pos = 0;
    if ((size_t)pos > rf->max_bytes)
    {
        fclose(rf->fp);
        rf->fp = fopen(rf->path, "w"); // перезапись
        if (!rf->fp)
            return;
    }
    struct rklogx_event tmp = *ev;
    tmp.udata = rf->fp;
    rklogx_file_sink(&tmp);
}

// ============================ Публичный API ============================
static inline void rklogx_set_lock(rklogx_lock_fn fn, void *udata)
{
    rklogx_g.lock_fn = fn;
    rklogx_g.lock_udata = udata;
}
static inline void rklogx_set_level(rklogx_level_t lvl) { rklogx_g.level = lvl; }
static inline rklogx_level_t rklogx_get_level(void) { return rklogx_g.level; }
static inline void rklogx_set_quiet(bool q) { rklogx_g.quiet = q; }
static inline void rklogx_set_color(bool on) { rklogx_g.color = on; }

static inline int rklogx_add_sink(rklogx_sink_fn fn, void *udata, rklogx_level_t minlvl)
{
    for (int i = 0; i < RKLOGX_MAX_SINKS; i++)
    {
        if (!rklogx_g.sinks[i].fn)
        {
            rklogx_g.sinks[i].fn = fn;
            rklogx_g.sinks[i].udata = udata;
            rklogx_g.sinks[i].level = minlvl;
            return 0;
        }
    }
    return -1;
}

static inline int rklogx_add_stdout(rklogx_level_t minlvl)
{
    return rklogx_add_sink(rklogx_stdout_sink, stderr, minlvl);
}

static inline int rklogx_add_file(FILE *fp, rklogx_level_t minlvl)
{
    return rklogx_add_sink(rklogx_file_sink, fp, minlvl);
}

static inline rklogx_rotfile_t *rklogx_add_rotating_file(const char *path, size_t max_bytes, rklogx_level_t minlvl)
{
    rklogx_rotfile_t *rf = (rklogx_rotfile_t *)calloc(1, sizeof(*rf));
    if (!rf)
        return NULL;
    rf->path = strdup(path);
    rf->max_bytes = max_bytes;
    rf->fp = fopen(path, "a+");
    if (!rf->fp)
    {
        free(rf->path);
        free(rf);
        return NULL;
    }
    if (rklogx_add_sink(rklogx_rotfile_sink, rf, minlvl) != 0)
    {
        fclose(rf->fp);
        free(rf->path);
        free(rf);
        return NULL;
    }
    return rf;
}

static inline void rklogx_remove_all_sinks(void)
{
    for (int i = 0; i < RKLOGX_MAX_SINKS; i++)
        rklogx_g.sinks[i].fn = NULL;
}

static inline void rklogx_emit(rklogx_level_t lvl, const char *tag, const char *file, int line, const char *fmt, va_list ap)
{
    if (rklogx_g.quiet || lvl < rklogx_g.level)
        return;
    rklogx_lock();
    struct rklogx_event ev;
    ev.level = lvl;
    ev.tag = tag;
    ev.file = file;
    ev.line = line;
    ev.fmt = fmt;
    ev.udata = NULL;
    ev.usec = rklogx_now_us();
    rklogx_fill_tm(&ev.tmval);
    for (int i = 0; i < RKLOGX_MAX_SINKS; i++)
    {
        rklogx_sink_t *s = &rklogx_g.sinks[i];
        if (!s->fn)
            continue;
        if (lvl < s->level)
            continue;
        va_list cp;
        va_copy(cp, ap);
        ev.ap = cp;
        ev.udata = s->udata;
        s->fn(&ev);
        va_end(cp);
    }
    rklogx_unlock();
}

static inline void rklogx_log(rklogx_level_t lvl, const char *tag, const char *file, int line, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    rklogx_emit(lvl, tag, file, line, fmt, ap);
    va_end(ap);
}

// ============================ Утилиты: hexdump, timeit, rate-limit ============================
static inline void rklogx_hexdump_at(rklogx_level_t lvl, const char *tag, const char *file, int line,
                                     const void *data, size_t len, size_t width)
{
    const uint8_t *p = (const uint8_t *)data;
    char linebuf[256];
    if (width == 0)
        width = 16;
    for (size_t i = 0; i < len; i += width)
    {
        size_t n = (len - i < width) ? (len - i) : width;
        char *cur = linebuf;
        int off = snprintf(cur, sizeof(linebuf), "%08zx  ", i);
        (void)off;
        cur = linebuf + strlen(linebuf);
        for (size_t j = 0; j < width; j++)
        {
            if (j < n)
                cur += sprintf(cur, "%02X ", p[i + j]);
            else
                cur += sprintf(cur, "   ");
        }
        cur += sprintf(cur, " |");
        for (size_t j = 0; j < n; j++)
        {
            unsigned char c = p[i + j];
            *cur++ = (c >= 32 && c < 127) ? c : '.';
        }
        *cur++ = '|';
        *cur = '\0';
        rklogx_log(lvl, tag, file, line, "%s", linebuf);
    }
}

static inline uint64_t rklogx_timeit_us(void (*fn)(void *), void *arg)
{
    uint64_t t0 = rklogx_now_us();
    fn(arg);
    return rklogx_now_us() - t0;
}

// ============================ Макросы высокого уровня ============================
#define RKX_LOG(level, tag, fmt, ...) rklogx_log((level), (tag), __FILE__, __LINE__, (fmt), ##__VA_ARGS__)
#define RKX_T(fmt, ...) RKX_LOG(RKLOGX_TRACE, RKLOGX_DEFAULT_TAG, fmt, ##__VA_ARGS__)
#define RKX_D(fmt, ...) RKX_LOG(RKLOGX_DEBUG, RKLOGX_DEFAULT_TAG, fmt, ##__VA_ARGS__)
#define RKX_I(fmt, ...) RKX_LOG(RKLOGX_INFO, RKLOGX_DEFAULT_TAG, fmt, ##__VA_ARGS__)
#define RKX_W(fmt, ...) RKX_LOG(RKLOGX_WARN, RKLOGX_DEFAULT_TAG, fmt, ##__VA_ARGS__)
#define RKX_E(fmt, ...) RKX_LOG(RKLOGX_ERROR, RKLOGX_DEFAULT_TAG, fmt, ##__VA_ARGS__)
#define RKX_F(fmt, ...) RKX_LOG(RKLOGX_FATAL, RKLOGX_DEFAULT_TAG, fmt, ##__VA_ARGS__)

#define RKX_T_TAG(tag, fmt, ...) RKX_LOG(RKLOGX_TRACE, (tag), fmt, ##__VA_ARGS__)
#define RKX_D_TAG(tag, fmt, ...) RKX_LOG(RKLOGX_DEBUG, (tag), fmt, ##__VA_ARGS__)
#define RKX_I_TAG(tag, fmt, ...) RKX_LOG(RKLOGX_INFO, (tag), fmt, ##__VA_ARGS__)
#define RKX_W_TAG(tag, fmt, ...) RKX_LOG(RKLOGX_WARN, (tag), fmt, ##__VA_ARGS__)
#define RKX_E_TAG(tag, fmt, ...) RKX_LOG(RKLOGX_ERROR, (tag), fmt, ##__VA_ARGS__)
#define RKX_F_TAG(tag, fmt, ...) RKX_LOG(RKLOGX_FATAL, (tag), fmt, ##__VA_ARGS__)

#define RK_ERROR_CHECK(expr)                                                    \
    do                                                                          \
    {                                                                           \
        int __e = (int)(expr);                                                  \
        if (__e)                                                                \
        {                                                                       \
            RKX_E("Error check failed: %s=%d (%s)", #expr, __e, strerror(__e)); \
            abort();                                                            \
        }                                                                       \
    } while (0)

#define RK_RETURN_ON_ERROR(expr)                                   \
    do                                                             \
    {                                                              \
        int __e = (int)(expr);                                     \
        if (__e)                                                   \
        {                                                          \
            RKX_E("Error: %s=%d (%s)", #expr, __e, strerror(__e)); \
            return __e;                                            \
        }                                                          \
    } while (0)

// Варианты возврата/перехода при ошибке
#define RKX_RETURN_ON_ERROR(expr)                                   \
    do                                                              \
    {                                                               \
        int __e = (int)(expr);                                      \
        if (__e)                                                    \
        {                                                           \
            RKX_E("%s failed: %d (%s)", #expr, __e, strerror(__e)); \
            return __e;                                             \
        }                                                           \
    } while (0)
#define RKX_GOTO_ON_ERROR(label, expr)                              \
    do                                                              \
    {                                                               \
        int __e = (int)(expr);                                      \
        if (__e)                                                    \
        {                                                           \
            RKX_E("%s failed: %d (%s)", #expr, __e, strerror(__e)); \
            goto label;                                             \
        }                                                           \
    } while (0)
#define RKX_CHECK(cond, action, fmt, ...)                           \
    do                                                              \
    {                                                               \
        if (!(cond))                                                \
        {                                                           \
            RKX_E("CHECK failed: %s | " fmt, #cond, ##__VA_ARGS__); \
            action;                                                 \
        }                                                           \
    } while (0)
#define RKX_ASSERT(cond)                \
    do                                  \
    {                                   \
        if (!(cond))                    \
        {                               \
            RKX_E("ASSERT: %s", #cond); \
            abort();                    \
        }                               \
    } while (0)

// Ограничение частоты (rate limit) по месту вызова
#define RKX_RL_MS(interval_ms, level, fmt, ...)                          \
    do                                                                   \
    {                                                                    \
        static uint64_t __rkx_last = 0;                                  \
        uint64_t __rkx_now = rklogx_now_us();                            \
        if (__rkx_now - __rkx_last >= (uint64_t)(interval_ms) * 1000ull) \
        {                                                                \
            __rkx_last = __rkx_now;                                      \
            RKX_LOG((level), RKLOGX_DEFAULT_TAG, fmt, ##__VA_ARGS__);    \
        }                                                                \
    } while (0)

// Однократный лог (once)
#define RKX_ONCE(level, fmt, ...)                                     \
    do                                                                \
    {                                                                 \
        static int __rkx_once = 0;                                    \
        if (!__rkx_once)                                              \
        {                                                             \
            __rkx_once = 1;                                           \
            RKX_LOG((level), RKLOGX_DEFAULT_TAG, fmt, ##__VA_ARGS__); \
        }                                                             \
    } while (0)

// HEXDUMP
#define RKX_HEXDUMP(level, tag, ptr, len) rklogx_hexdump_at((level), (tag), __FILE__, __LINE__, (ptr), (len), 16)

// TIMEIT для произвольного блока (использует GCC statement expression, если доступен)
#if defined(__GNUC__)
#define RKX_TIMEIT_US(label, block)                              \
    do                                                           \
    {                                                            \
        uint64_t __t0 = rklogx_now_us();                         \
        do                                                       \
            block while (0);                                     \
        uint64_t __dt = rklogx_now_us() - __t0;                  \
        RKX_D("%s: %llu us", (label), (unsigned long long)__dt); \
    } while (0)
#else
// Портируемая версия требует вынести тело в функцию и измерить через rklogx_timeit_us
#endif

// ============================ Удобные пресеты ============================
static inline void rklogx_use_stderr_default(rklogx_level_t minlvl)
{
    rklogx_remove_all_sinks();
    rklogx_add_stdout(minlvl);
}

#ifdef BUILD_DEV
static inline void sigsegv_handler(int sig)
{
    void *array[32];
    size_t size = backtrace(array, 32);

    RKX_F("runtime", "Caught SIGSEGV (%s)", strsignal(sig));
    RKX_F("runtime", "Stack trace:");
    for (size_t i = 0; i < size; i++)
    {
        RKX_F("runtime", "  [%zu] %p", i, array[i]);
    }
    _exit(128 + sig);
}

__attribute__((constructor)) static inline void setup_signal_handlers(void)
{
    struct sigaction sa = {0};
    sa.sa_handler = sigsegv_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGSEGV, &sa, NULL);
}

__attribute__((constructor)) static void aaa(void)
{
    rklogx_use_stderr_default(RKLOGX_DEBUG);
    rklogx_set_level(RKLOGX_DEBUG);
}

__attribute__((destructor)) static void check_calls(void)
{
    RKX_D("Попытка выхода из программы.");
    if (START_VIDEO && !END_VIDEO)
        RKX_D("Вы вызвали video из rkvideo, но не вызвали release для ctx");
}
#endif
#endif // RKUTILS_H