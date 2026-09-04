/*
 * net/fetch.c - HTTP/1.1 GET for obrowse.
 *
 * Reuses the ohttp patterns (apps/ohttp/net/{request,connect,response}.c)
 * but writes the body into a single in-memory buffer instead of a file,
 * so the obrowse TUI can render it directly. Plain HTTP over IPv4 (or
 * DNS-via-net_resolve); no TLS, no listen/accept (kernel limits).
 *
 * Same kernel quirks ohttp works around (see apps/ohttp/README.md):
 *   - tcp_recv has no clean EOF; trust Content-Length, fall back to
 *     "read until EINVAL (slot gone)" for chunked responses.
 *   - usleep()/nanosleep() hangs on this kernel; busy-spin between
 *     recv attempts instead (OBROWSE_RECV_RETRIES budget).
 *   - 8 KB header staging buffer is static (in .bss): USER_TOP
 *     proximity on the initial user stack pointer would push a stack
 *     array past the syscall user_ptr_ok check.
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <net.h>
#include "../obrowse.h"

#define OBROWSE_RECV_RETRIES 200000
#define OBROWSE_MAX_HOST    256
#define OBROWSE_MAX_PATH   1024
#define OBROWSE_MAX_HEADER  8192
#define OBROWSE_RECV_CHUNK  4096

/* .bss: see header comment about USER_TOP proximity. */
static char g_header_buf[OBROWSE_MAX_HEADER];

static int parse_ip(const char *s, unsigned char ip[4], const char **rest) {
    const char *p = s;
    for (int i = 0; i < 4; i++) {
        if (i > 0) { if (*p != '.') return -1; p++; }
        if (*p < '0' || *p > '9') return -1;
        long v = 0;
        while (*p >= '0' && *p <= '9') { v = v * 10 + (*p - '0'); p++; }
        if (v < 0 || v > 255) return -1;
        ip[i] = (unsigned char)v;
    }
    *rest = p;
    return 0;
}

static int parse_target(const char *arg, char *host_out, size_t host_cap,
                        int *port_out, char *path_out, size_t path_cap,
                        unsigned char ip_out[4]) {
    const char *rest;
    int had_host = 0;
    if (parse_ip(arg, ip_out, &rest) != 0) {
        size_t n = 0;
        while (arg[n] != '\0' && arg[n] != '/' && arg[n] != ':') n++;
        if (n == 0 || n >= host_cap) return -1;
        memcpy(host_out, arg, n);
        host_out[n] = '\0';
        rest = arg + n;
        if (net_resolve(host_out, ip_out) != 0) return -1;
        had_host = 1;
    }
    *port_out = 80;
    if (*rest == ':') {
        rest++;
        long v = 0;
        const char *end = rest;
        while (*end >= '0' && *end <= '9') { v = v * 10 + (*end - '0'); end++; }
        if (end == rest || v <= 0 || v > 65535) return -1;
        *port_out = (int)v;
        rest = end;
    }
    if (*rest == '\0') { strncpy(path_out, "/", path_cap - 1); path_out[path_cap - 1] = '\0'; }
    else if (*rest == '/') {
        strncpy(path_out, rest, path_cap - 1);
        path_out[path_cap - 1] = '\0';
    } else return -1;
    if (!had_host) snprintf(host_out, host_cap, "%d.%d.%d.%d",
                            ip_out[0], ip_out[1], ip_out[2], ip_out[3]);
    return 0;
}

/* Busy-poll recv_wait: returns >0 bytes, 0 for "slot gone (done)", -1 err. */
static long recv_wait(int conn_id, void *buf, size_t len) {
    for (int attempt = 0; attempt < OBROWSE_RECV_RETRIES; attempt++) {
        long r = net_recv(conn_id, buf, len);
        if (r > 0) return r;
        if (r < 0 && errno == EINVAL) return 0;
        if (r < 0 && errno != ENOENT) return -1;
        for (volatile int spin = 0; spin < 2000; spin++) {}
    }
    return -1;
}

static const char *find_header_end(const char *buf, size_t len) {
    for (size_t i = 0; i + 3 < len; i++) {
        if (buf[i] == '\r' && buf[i + 1] == '\n' && buf[i + 2] == '\r' && buf[i + 3] == '\n')
            return buf + i + 4;
    }
    return NULL;
}

static long content_length(const char *headers, size_t header_len) {
    /* Case-insensitive find: "Content-Length:" */
    const char needle[] = "content-length:";
    for (size_t i = 0; i + sizeof(needle) - 1 <= header_len; i++) {
        char a = headers[i];
        if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
        if (a != needle[0]) continue;
        size_t j;
        for (j = 1; j < sizeof(needle) - 1; j++) {
            char b = headers[i + j];
            if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
            if (b != needle[j]) break;
        }
        if (j == sizeof(needle) - 1) {
            const char *v = headers + i + sizeof(needle) - 1;
            while (v < headers + header_len && *v == ' ') v++;
            long n = 0; int any = 0;
            while (v < headers + header_len && *v >= '0' && *v <= '9') {
                n = n * 10 + (*v - '0'); v++; any = 1;
            }
            return any ? n : -1;
        }
    }
    return -1;
}

long obrowse_fetch(const char *url, char *out_buf, size_t out_cap) {
    char host[OBROWSE_MAX_HOST];
    char path[OBROWSE_MAX_PATH];
    unsigned char ip[4];
    int port;
    if (url[0] != '\0' && url[0] == 'h' && url[1] == 't' && url[2] == 't' && url[3] == 'p' && url[4] == 's') {
        fprintf(stderr, "obrowse: https:// not supported (no TLS in-kernel)\n");
        return -1;
    }
    const char *arg = url;
    if (arg[0] == 'h' && arg[1] == 't' && arg[2] == 't' && arg[3] == 'p' && arg[4] == ':' && arg[5] == '/' && arg[6] == '/')
        arg += 7;
    if (parse_target(arg, host, sizeof(host), &port, path, sizeof(path), ip) != 0) {
        fprintf(stderr, "obrowse: bad URL '%s'\n", url);
        return -1;
    }
    int conn_id = net_connect(ip, port);
    if (conn_id < 0) {
        fprintf(stderr, "obrowse: connect to %s:%d failed (errno %d)\n", host, port, errno);
        return -1;
    }
    char req[OBROWSE_MAX_PATH + OBROWSE_MAX_HOST + 128];
    int n = snprintf(req, sizeof(req),
                     "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: obrowse/0.1 (OnyxOS)\r\nConnection: close\r\n\r\n",
                     path, host);
    if (n <= 0 || (size_t)n >= sizeof(req)) { net_close(conn_id); return -1; }
    size_t sent = 0;
    while (sent < (size_t)n) {
        long r = net_send(conn_id, req + sent, (size_t)n - sent);
        if (r < 0) { net_close(conn_id); return -1; }
        if (r == 0) continue;
        sent += (size_t)r;
    }
    char *buf = g_header_buf;
    size_t have = 0;
    const char *body_start = NULL;
    while (!body_start) {
        if (have >= OBROWSE_MAX_HEADER) { net_close(conn_id); return -1; }
        long r = recv_wait(conn_id, buf + have, OBROWSE_MAX_HEADER - have);
        if (r < 0) { net_close(conn_id); return -1; }
        if (r == 0) { net_close(conn_id); return -1; }
        have += (size_t)r;
        body_start = find_header_end(buf, have);
    }
    size_t header_len = (size_t)(body_start - buf);
    long want = content_length(buf, header_len);
    size_t out_used = 0;
    long got = (long)(have - header_len);
    if (got > 0) {
        size_t copy = (size_t)got;
        if (copy > out_cap - 1) copy = out_cap - 1;
        memcpy(out_buf, body_start, copy);
        out_used = copy;
    }
    while (want < 0 || (long)out_used < want) {
        size_t chunk = OBROWSE_RECV_CHUNK;
        if (out_used + chunk > out_cap - 1) chunk = out_cap - 1 - out_used;
        if (chunk == 0) break; /* out_buf full */
        long r = recv_wait(conn_id, buf, chunk);
        if (r < 0) break;
        if (r == 0) break;
        memcpy(out_buf + out_used, buf, (size_t)r);
        out_used += (size_t)r;
    }
    net_close(conn_id);
    if (out_used < out_cap) out_buf[out_used] = '\0';
    else out_buf[out_cap - 1] = '\0';
    return (long)out_used;
}
-- 