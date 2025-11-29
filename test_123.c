/* Минимальный RTSP/RTP interleaved клиент (TCP). Не production, но рабочий «скелет». */
#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <stdint.h>
#include <ctype.h>

#define MAX_HDR 65536
#define MAX_SDP 65536

/* ------------ утилиты сокета ------------ */
static int connect_host(const char *host, const char *port)
{
    struct addrinfo hints = {0}, *res, *rp;
    int sfd;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, port, &hints, &res) != 0)
        return -1;
    for (rp = res; rp; rp = rp->ai_next)
    {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd < 0)
            continue;
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            freeaddrinfo(res);
            return sfd;
        }
        close(sfd);
    }
    freeaddrinfo(res);
    return -1;
}
static ssize_t send_all(int fd, const void *buf, size_t len)
{
    size_t sent = 0;
    while (sent < len)
    {
        ssize_t r = send(fd, (const char *)buf + sent, len - sent, 0);
        if (r <= 0)
            return -1;
        sent += (size_t)r;
    }
    return (ssize_t)sent;
}
static ssize_t recv_n(int fd, void *buf, size_t n)
{
    size_t got = 0;
    while (got < n)
    {
        ssize_t r = recv(fd, (char *)buf + got, n - got, 0);
        if (r <= 0)
            return r;
        got += (size_t)r;
    }
    return (ssize_t)got;
}
static ssize_t recv_line(int fd, char *buf, size_t max)
{
    size_t i = 0;
    while (i + 1 < max)
    {
        char c;
        ssize_t r = recv(fd, &c, 1, 0);
        if (r <= 0)
            return r;
        buf[i++] = c;
        if (i >= 2 && buf[i - 2] == '\r' && buf[i - 1] == '\n')
            break;
    }
    buf[i] = '\0';
    return (ssize_t)i;
}

/* ------------ парсинг заголовков RTSP ------------ */
typedef struct
{
    int status_code;         /* 200 и т.п. */
    char headers[MAX_HDR];   /* «сырые» заголовки для поиска */
    int content_length;      /* -1 если нет */
    char content_base[1024]; /* Content-Base если есть */
    char session[256];       /* Session из ответа SETUP/PLAY */
} rtsp_resp_t;

static void strlower(char *s)
{
    for (; *s; ++s)
        *s = (char)tolower((unsigned char)*s);
}

static int hdr_get(const char *all, const char *key, char *out, size_t out_sz)
{
    size_t keylen = strlen(key);
    const char *p = all;
    char lkey[128];
    if (keylen >= sizeof(lkey))
        return 0;
    strcpy(lkey, key);
    strlower(lkey);

    while (*p)
    {
        const char *line_end = strstr(p, "\r\n");
        size_t L = line_end ? (size_t)(line_end - p) : strlen(p);
        if (L == 0)
            break;

        char *tmp = malloc(L + 1);
        if (!tmp)
            return 0;
        memcpy(tmp, p, L);
        tmp[L] = 0;

        char *colon = strchr(tmp, ':');
        if (colon)
        {
            *colon = 0;
            char *val = colon + 1;
            while (*val == ' ' || *val == '\t')
                ++val;
            strlower(tmp);
            if (strcmp(tmp, lkey) == 0)
            {
                // копируем всё значение до конца строки
                strncpy(out, val, out_sz - 1);
                out[out_sz - 1] = '\0';
                // удалить \r и \n в конце
                size_t n = strlen(out);
                while (n && (out[n - 1] == '\r' || out[n - 1] == '\n'))
                    out[--n] = '\0';
                free(tmp);
                return 1;
            }
        }
        free(tmp);
        if (!line_end)
            break;
        p = line_end + 2;
    }
    return 0;
}

/* Чтение RTSP-ответа: статус + заголовки (+ тело по Content-Length если нужно — отдельной функцией) */
static int read_rtsp_headers(int fd, rtsp_resp_t *resp)
{
    memset(resp, 0, sizeof(*resp));
    resp->status_code = -1;
    resp->content_length = -1;
    resp->headers[0] = 0;
    resp->content_base[0] = 0;
    resp->session[0] = 0;

    char line[2048];
    if (recv_line(fd, line, sizeof(line)) <= 0)
        return -1;
    // Первая строка: RTSP/1.0 200 OK
    int code = -1;
    if (sscanf(line, "RTSP/%*s %d", &code) == 1)
        resp->status_code = code;
    // Читаем заголовки до пустой строки
    size_t pos = 0;
    while (1)
    {
        ssize_t n = recv_line(fd, line, sizeof(line));
        if (n <= 0)
            return -1;
        if (strcmp(line, "\r\n") == 0)
            break;
        size_t L = strlen(line);
        if (pos + L < sizeof(resp->headers))
        {
            memcpy(resp->headers + pos, line, L);
            pos += L;
            resp->headers[pos] = 0;
        }
    }
    // Извлекаем интересное
    char tmp[1024];
    if (hdr_get(resp->headers, "Content-Length", tmp, sizeof(tmp)))
    {
        resp->content_length = atoi(tmp);
    }
    if (hdr_get(resp->headers, "Content-Base", resp->content_base, sizeof(resp->content_base)))
    {
        // обрежем \r\n
        size_t n = strlen(resp->content_base);
        while (n && (resp->content_base[n - 1] == '\r' || resp->content_base[n - 1] == '\n'))
            resp->content_base[--n] = 0;
    }
    if (hdr_get(resp->headers, "Session", resp->session, sizeof(resp->session)))
    {
        size_t n = strlen(resp->session);
        // безопасно убрать только управляющие символы (\r, \n, пробелы)
        while (n > 0 && (resp->session[n - 1] == '\r' || resp->session[n - 1] == '\n' || resp->session[n - 1] == ' '))
            resp->session[--n] = '\0';
        char *sc = strchr(resp->session, ';');
        if (sc)
            *sc = '\0';
    }

    return 0;
}

static int read_rtsp_body(int fd, int content_length, char *body, size_t body_sz)
{
    ssize_t total = 0;
    while (total + 1 < (ssize_t)body_sz)
    {
        ssize_t r = recv(fd, body + total, body_sz - 1 - total, 0);
        if (r <= 0)
            break;
        total += r;
        /* камера может послать SDP и сразу закрыть сокет — этого достаточно */
        if (strstr(body, "a=control:") || strstr(body, "m=video"))
            break;
    }
    body[total] = 0;
    printf("Read %zd bytes SDP:\n%.*s\n", total, (int)total, body);
    return 0;
}

/* ------------ простейший парсер SDP для video track ------------ */
static int sdp_find_video_control(const char *sdp, char *control, size_t control_sz, int *has_agg)
{
    /* ищем a=control:* (агрегат), и первый video m=... затем ближайший a=control:... */
    *has_agg = 0;
    const char *p = sdp;
    while ((p = strstr(p, "a=control:")) != NULL)
    {
        p += 10;
        if (strncmp(p, "*", 1) == 0)
        {
            *has_agg = 1;
            break;
        }
    }
    const char *sec_start = strstr(sdp, "m=video ");
    if (!sec_start)
        return 0;
    const char *sec_end = strstr(sec_start + 1, "\n m="); /* редко встречается, fallback ниже */
    if (!sec_end)
        sec_end = sdp + strlen(sdp);
    /* в секции video ищем a=control: */
    const char *q = sec_start;
    while ((q = strstr(q, "a=control:")) && q < sec_end)
    {
        q += 10;
        const char *eol = strchr(q, '\n');
        if (!eol)
            eol = sdp + strlen(sdp);
        while (*q == ' ' || *q == '\t')
            ++q;
        size_t L = (size_t)(eol - q);
        if (L && L < control_sz)
        {
            /* убрать \r */
            while (L && (q[L - 1] == '\r' || q[L - 1] == '\n'))
                L--;
            snprintf(control, control_sz, "%.*s", (int)L, q);
            return 1;
        }
        break;
    }
    return 0;
}

/* Слияние Content-Base (или базового URL) и control (если он относительный) */
static void join_rtsp_url(const char *base, const char *control, char *out, size_t out_sz)
{
    if (!control || !*control)
    {
        snprintf(out, out_sz, "%s", base);
        return;
    }
    if (strstr(control, "rtsp://") == control)
    {
        snprintf(out, out_sz, "%s", control);
        return;
    }
    /* относительный: аккуратно склеить */
    size_t blen = strlen(base);
    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "%s", base);
    if (blen && tmp[blen - 1] != '/')
    {
        strncat(tmp, "/", sizeof(tmp) - strlen(tmp) - 1);
    }
    snprintf(out, out_sz, "%s%s", tmp, control);
}

/* Удалить последний сегмент пути (для аггрегатного PLAY) */
static void strip_last_segment(const char *url, char *out, size_t out_sz)
{
    snprintf(out, out_sz, "%s", url);
    char *p = strrchr(out, '/');
    if (p && p != out)
        *p = 0;
}

/* ------------ RTP ------------ */
static uint8_t *parse_rtp(uint8_t *pkt, uint32_t pkt_len, uint16_t *seq, uint32_t *ts, uint8_t *pt, uint32_t *payload_len)
{
    if (pkt_len < 12)
        return NULL;
    uint8_t v_p_x_cc = pkt[0];
    uint8_t m_pt = pkt[1];
    if ((v_p_x_cc >> 6) != 2)
        return NULL;
    uint8_t cc = v_p_x_cc & 0x0F;
    uint32_t header_len = 12 + cc * 4;
    int has_ext = (v_p_x_cc & 0x10) != 0;
    *pt = m_pt & 0x7F;
    *seq = (uint16_t)((pkt[2] << 8) | pkt[3]);
    *ts = ((uint32_t)pkt[4] << 24) | ((uint32_t)pkt[5] << 16) | ((uint32_t)pkt[6] << 8) | pkt[7];
    if (pkt_len < header_len)
        return NULL;
    if (has_ext)
    {
        if (pkt_len < header_len + 4)
            return NULL;
        uint16_t ext_len = (uint16_t)((pkt[header_len + 2] << 8) | pkt[header_len + 3]);
        header_len += 4 + ext_len * 4;
        if (pkt_len < header_len)
            return NULL;
    }
    *payload_len = pkt_len - header_len;
    return pkt + header_len;
}

/* Чтение interleaved фрейма ($). Возвращает 1 — ок, 0 — пропущено (например RTSP-текст), <0 — ошибка */
static int read_interleaved_frame(int fd, uint8_t **payload, uint16_t *channel, uint32_t *len)
{
    uint8_t b;
    ssize_t r = recv(fd, &b, 1, 0);
    if (r <= 0)
        return -1;
    if (b != '$')
    {
        /* возможно пришёл RTSP-текст (ответ keepalive) — дочитаем заголовки и пропустим */
        char first[2048];
        first[0] = b;
        first[1] = 0;
        if (recv_line(fd, first + 1, sizeof(first) - 1) <= 0)
            return -1;
        if (strncmp(first, "RTSP/", 5) == 0)
        {
            rtsp_resp_t tmp;
            if (read_rtsp_headers(fd, &tmp) < 0)
                return -1;
            if (tmp.content_length > 0)
            {
                char dummy[2048];
                int toread = tmp.content_length;
                while (toread > 0)
                {
                    int chunk = toread > (int)sizeof(dummy) ? (int)sizeof(dummy) : toread;
                    if (recv_n(fd, dummy, (size_t)chunk) <= 0)
                        return -1;
                    toread -= chunk;
                }
            }
            return 0;
        }
        return 0;
    }
    uint8_t hdr[3];
    if (recv_n(fd, hdr, 3) <= 0)
        return -1;
    *channel = hdr[0];
    uint16_t len16 = ((uint16_t)hdr[1] << 8) | hdr[2];
    *len = len16;
    uint8_t *buf = (uint8_t *)malloc(len16);
    if (!buf)
        return -1;
    if (recv_n(fd, buf, len16) <= 0)
    {
        free(buf);
        return -1;
    }
    *payload = buf;
    return 1;
}

/* ------------ main ------------ */
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s rtsp://host[:port]/path\n", argv[0]);
        return 1;
    }

    /* разбор URL */
    char host[256] = {0}, port[8] = "554", path[1024] = {0};
    if (sscanf(argv[1], "rtsp://%255[^:/]:%7[^/]/%1023[^\n]", host, port, path) < 2)
    {
        if (sscanf(argv[1], "rtsp://%255[^/]/%1023[^\n]", host, path) < 1)
        {
            fprintf(stderr, "bad url\n");
            return 1;
        }
        strcpy(port, "554");
    }
    char baseurl[1600];
    if (strlen(path))
        snprintf(baseurl, sizeof(baseurl), "rtsp://%s:%s/%s", host, port, path);
    else
        snprintf(baseurl, sizeof(baseurl), "rtsp://%s:%s/", host, port);

    int fd = connect_host(host, port);
    if (fd < 0)
    {
        perror("connect");
        return 1;
    }

    int cseq = 1;
    char req[4096];
    rtsp_resp_t resp;
    char sdp[MAX_SDP] = {0};
    char track_ctrl[512] = {0};
    int has_agg = 0;

    /* ---- DESCRIBE ---- */
    snprintf(req, sizeof(req),
             "DESCRIBE %s RTSP/1.0\r\nCSeq: %d\r\nAccept: application/sdp\r\nUser-Agent: rtsp-min/0.1\r\n\r\n",
             baseurl, cseq++);
    if (send_all(fd, req, strlen(req)) < 0)
    {
        perror("send");
        return 1;
    }
    if (read_rtsp_headers(fd, &resp) < 0)
    {
        fprintf(stderr, "DESCRIBE: no response\n");
        return 1;
    }
    if (resp.status_code != 200)
    {
        fprintf(stderr, "DESCRIBE: status %d\n", resp.status_code);
        return 1;
    }
    if (read_rtsp_body(fd, resp.content_length, sdp, sizeof(sdp)) < 0)
    {
        fprintf(stderr, "DESCRIBE: body read fail\n");
        return 1;
    }
    printf("DESCRIBE headers:\n%s\n", resp.headers);
    printf("SDP:\n%s\n", sdp);

    /* базовый URL для media */
    char content_base[1024];
    if (resp.content_base[0])
        snprintf(content_base, sizeof(content_base), "%s", resp.content_base);
    else
        snprintf(content_base, sizeof(content_base), "%s", baseurl);

    if (!sdp_find_video_control(sdp, track_ctrl, sizeof(track_ctrl), &has_agg))
    {
        fprintf(stderr, "No video control in SDP\n");
        return 1;
    }
    /* полный URL трека для SETUP */
    char setup_url[1600];
    join_rtsp_url(content_base, track_ctrl, setup_url, sizeof(setup_url));

    /* ---- SETUP ---- */
    snprintf(req, sizeof(req),
             "SETUP %s RTSP/1.0\r\nCSeq: %d\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\nUser-Agent: rtsp-min/0.1\r\n\r\n",
             setup_url, cseq++);
    if (send_all(fd, req, strlen(req)) < 0)
    {
        perror("send");
        return 1;
    }
    if (read_rtsp_headers(fd, &resp) < 0)
    {
        fprintf(stderr, "SETUP: no response\n");
        return 1;
    }
    if (resp.status_code != 200)
    {
        fprintf(stderr, "SETUP: status %d\n", resp.status_code);
        return 1;
    }
    if (!resp.session[0])
    {
        fprintf(stderr, "SETUP: no Session header\n");
        return 1;
    }
    printf("SETUP headers:\n%s\nSession: %s\n", resp.headers, resp.session);

    /* ---- PLAY ---- */
    /* ---- PLAY ---- */
    char play_url[1600];
    if (has_agg)
    {
        strip_last_segment(content_base, play_url, sizeof(play_url));
    }
    else
    {
        snprintf(play_url, sizeof(play_url), "%s", setup_url);
    }

    snprintf(req, sizeof(req),
             "PLAY %s RTSP/1.0\r\n"
             "CSeq: %d\r\n"
             "Session: %s\r\n"
             "Range: npt=0.000-\r\n"
             "User-Agent: rtsp-min/0.1\r\n\r\n",
             play_url, cseq++, resp.session);

    if (send_all(fd, req, strlen(req)) < 0)
    {
        perror("send PLAY");
        return 1;
    }
    if (read_rtsp_headers(fd, &resp) < 0)
    {
        fprintf(stderr, "PLAY: no response\n");
        return 1;
    }
    if (resp.status_code != 200)
    {
        fprintf(stderr, "PLAY: status %d\n", resp.status_code);
        return 1;
    }
    printf("PLAY headers:\n%s\n", resp.headers);

    /* ---- чтение interleaved RTP ---- */
    FILE *fout = fopen("stream.h264", "wb");
    if (!fout)
    {
        perror("stream.h264");
        return 1;
    }

    uint8_t fu_buffer[1024 * 1024];
    size_t fu_len = 0;
    int fu_active = 0;
    uint8_t fu_type = 0;

    for (;;)
    {
        uint8_t *payload = NULL;
        uint16_t channel = 0;
        uint32_t len = 0;
        int r = read_interleaved_frame(fd, &payload, &channel, &len);
        if (r < 0)
        {
            fprintf(stderr, "stream closed\n");
            break;
        }
        if (r == 0)
            continue; /* пропустить keepalive */

        if ((channel % 2) == 0)
        {
            uint16_t seq;
            uint32_t ts;
            uint8_t pt;
            uint32_t pl_len;
            uint8_t *pl = parse_rtp(payload, len, &seq, &ts, &pt, &pl_len);
            if (!pl)
            {
                free(payload);
                continue;
            }

            if (pt != 96)
            {
                /* не H264, можно игнорировать */
                free(payload);
                continue;
            }

            uint8_t nal_type = pl[0] & 0x1F;
            if (nal_type >= 1 && nal_type <= 23)
            {
                /* одиночный NAL */
                const uint8_t startcode[4] = {0, 0, 0, 1};
                fwrite(startcode, 1, 4, fout);
                fwrite(pl, 1, pl_len, fout);
            }
            else if (nal_type == 28)
            {
                /* FU-A */
                uint8_t fu_header = pl[1];
                uint8_t start = fu_header & 0x80;
                uint8_t end = fu_header & 0x40;
                uint8_t nal = (pl[0] & 0xE0) | (fu_header & 0x1F);

                if (start)
                {
                    /* начать новый фрагмент */
                    fu_len = 0;
                    const uint8_t startcode[4] = {0, 0, 0, 1};
                    memcpy(fu_buffer + fu_len, startcode, 4);
                    fu_len += 4;
                    fu_buffer[fu_len++] = nal;
                    memcpy(fu_buffer + fu_len, pl + 2, pl_len - 2);
                    fu_len += pl_len - 2;
                    fu_active = 1;
                    fu_type = nal & 0x1F;
                }
                else if (fu_active)
                {
                    memcpy(fu_buffer + fu_len, pl + 2, pl_len - 2);
                    fu_len += pl_len - 2;
                    if (end)
                    {
                        fwrite(fu_buffer, 1, fu_len, fout);
                        fu_len = 0;
                        fu_active = 0;
                    }
                }
            }
            else
            {
                /* другие типы (STAP-A, MTAP и т.п.) можно добавить позже */
            }
        }
        free(payload);
    }
    fclose(fout);

    // char play_url[1600];
    // if (has_agg)
    // {
    //     /* агрегатный контроль — PLAY по Content-Base (или базовому пути без /trackN) */
    //     snprintf(play_url, sizeof(play_url), "%s", content_base);
    // }
    // else
    // {
    //     /* fallback: PLAY по треку тоже обычно работает */
    //     snprintf(play_url, sizeof(play_url), "%s", setup_url);
    // }
    // /* убедимся, что нет лишнего / после host */
    // size_t Lp = strlen(play_url);
    // if (Lp && play_url[Lp - 1] == '\r')
    //     play_url[Lp - 1] = 0;

    // snprintf(req, sizeof(req),
    //          "PLAY %s RTSP/1.0\r\nCSeq: %d\r\nSession: %s\r\nRange: npt=0.000-\r\nUser-Agent: rtsp-min/0.1\r\n\r\n",
    //          play_url, cseq++, resp.session);
    // if (send_all(fd, req, strlen(req)) < 0)
    // {
    //     perror("send");
    //     return 1;
    // }
    // if (read_rtsp_headers(fd, &resp) < 0)
    // {
    //     fprintf(stderr, "PLAY: no response\n");
    //     return 1;
    // }
    // if (resp.status_code != 200)
    // {
    //     fprintf(stderr, "PLAY: status %d\n", resp.status_code);
    //     return 1;
    // }
    // printf("PLAY headers:\n%s\n", resp.headers);

    // /* ---- чтение interleaved RTP ---- */
    // for (;;)
    // {
    //     uint8_t *payload = NULL;
    //     uint16_t channel = 0;
    //     uint32_t len = 0;
    //     int r = read_interleaved_frame(fd, &payload, &channel, &len);
    //     if (r < 0)
    //     {
    //         fprintf(stderr, "stream closed\n");
    //         break;
    //     }
    //     if (r == 0)
    //         continue; /* пропущено (RTSP keepalive и т.п.) */
    //     if ((channel % 2) == 0)
    //     {
    //         uint16_t seq;
    //         uint32_t ts;
    //         uint8_t pt;
    //         uint32_t pl_len;
    //         uint8_t *pl = parse_rtp(payload, len, &seq, &ts, &pt, &pl_len);
    //         if (pl)
    //         {
    //             /* Пример: просто лог и запись полезной нагрузки RTP (для H.264 сборка NAL делается отдельно) */
    //             printf("RTP[ch=%u] seq=%u ts=%u pt=%u payload=%u\n", channel, seq, ts, pt, pl_len);
    //             FILE *f = fopen("payload.bin", "ab");
    //             if (f)
    //             {
    //                 fwrite(pl, 1, pl_len, f);
    //                 fclose(f);
    //             }
    //         }
    //     }
    //     else
    //     {
    //         /* RTCP, можно игнорировать */
    //     }
    //     free(payload);
    // }

    close(fd);
    return 0;
}
