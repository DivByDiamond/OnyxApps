/*
 * http/response.c - reads the response, splits header/body on the first
 * blank line, and streams the body to a file.
 *
 * OnyxKernel's net_recv (kernel/src/net/tcp/sock.rs:tcp_recv) has no
 * "peer closed" signal distinct from "no data yet": both come back as
 * -1/ENOENT (it never returns 0), and the connection slot only vanishes
 * (-1/EINVAL) after TIMEWAIT expires, well after the response finished.
 * So this reader trusts Content-Length to know when the body is done,
 * and treats slot-gone (EINVAL) as "done, whatever we got" for the rare
 * server that closes without one. ENOENT just means "poll again".
 *
 * Deliberately minimal beyond that: no chunked transfer-encoding, no
 * redirects. Good enough for the plain-file GETs ohttp targets today;
 * obrowse will need more.
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <net.h>
#include "../ohttp.h"

/* Retry budget for the busy-wait below. Not time-based: usleep()/nanosleep()
 * hangs on this kernel (confirmed live — see OnyxKernel/todo.md), so this
 * spins CPU cycles between attempts instead of actually sleeping. The count
 * is just "enough spins to not busy-loop forever on a truly dead peer". */
#define OHTTP_RECV_RETRIES 200000

static const char *find_header_end(const char *buf, size_t len) {
    for (size_t i = 0; i + 3 < len; i++) {
        if (buf[i] == '\r' && buf[i + 1] == '\n' &&
            buf[i + 2] == '\r' && buf[i + 3] == '\n') {
            return buf + i + 4;
        }
    }
    return NULL;
}

static long content_length(const char *headers, size_t header_len) {
    const char *cl = strstr(headers, "Content-Length:");
    if (!cl || (size_t)(cl - headers) >= header_len) return -1;
    const char *v = cl + 15;
    while (*v == ' ') v++;
    long n = 0;
    int any = 0;
    while (*v >= '0' && *v <= '9') { n = n * 10 + (*v - '0'); v++; any = 1; }
    return any ? n : -1;
}

static void print_status_and_length(const char *headers, long body_len) {
    const char *line_end = memchr(headers, '\n', OHTTP_MAX_HEADER);
    size_t first_len = line_end ? (size_t)(line_end - headers) : strlen(headers);
    printf("%.*s\n", (int)first_len, headers);
    if (body_len >= 0) printf("Content-Length: %ld\n", body_len);
}

/* Blocks (busy-polling net_recv) until at least one byte arrives, the
 * connection slot is gone, or too many empty attempts pass. Returns bytes
 * read (> 0), 0 for "treat as done", or -1 on a real error. */
static long recv_wait(int conn_id, void *buf, size_t len) {
    for (int attempt = 0; attempt < OHTTP_RECV_RETRIES; attempt++) {
        long r = net_recv(conn_id, buf, len);
        if (r > 0) return r;
        if (r < 0 && errno == EINVAL) return 0;   /* slot gone: peer long closed */
        if (r < 0 && errno != ENOENT) return -1;  /* real error */
        for (volatile int spin = 0; spin < 2000; spin++) {} /* usleep() hangs here; spin instead */
    }
    fprintf(stderr, "ohttp: no data from peer after %d attempts, giving up\n",
            OHTTP_RECV_RETRIES);
    return -1;
}

/* Static, not stack-local: OnyxOS's initial user stack pointer sits only a
 * few KB below USER_TOP (kernel/src/syscall/handler/dispatch.rs), so an
 * 8KB local array here pushes the syscall's user_ptr_ok(buf, len) check
 * past USER_TOP and every net_recv into it fails with EINVAL — confirmed
 * live: OnyxKernel/todo.md has the trace (sys_net_recv logged ptr_ok=0 for
 * this exact buffer/length). */
static char g_header_buf[OHTTP_MAX_HEADER];

int ohttp_read_response(int conn_id, const char *out_path) {
    char *buf = g_header_buf;
    size_t have = 0;
    const char *body_start = NULL;

    while (!body_start) {
        if (have >= OHTTP_MAX_HEADER) {
            fprintf(stderr, "ohttp: response header exceeds %d bytes, giving up\n",
                    OHTTP_MAX_HEADER);
            return -1;
        }
        long r = recv_wait(conn_id, buf + have, OHTTP_MAX_HEADER - have);
        if (r < 0) return -1;
        if (r == 0) {
            fprintf(stderr, "ohttp: connection closed before headers finished\n");
            return -1;
        }
        have += (size_t)r;
        body_start = find_header_end(buf, have);
    }

    size_t header_len = (size_t)(body_start - buf);
    long want = content_length(buf, header_len); /* -1 = unknown, read until closed */
    print_status_and_length(buf, want);

    FILE *out = fopen(out_path, "wb");
    if (!out) {
        fprintf(stderr, "ohttp: cannot open '%s' for writing (errno %d)\n", out_path, errno);
        return -1;
    }

    long got = (long)(have - header_len);
    if (got > 0) fwrite(body_start, 1, (size_t)got, out);

    while (want < 0 || got < want) {
        size_t chunk = OHTTP_MAX_HEADER;
        if (want >= 0 && (long)chunk > want - got) chunk = (size_t)(want - got);
        long r = recv_wait(conn_id, buf, chunk);
        if (r < 0) { fclose(out); return -1; }
        if (r == 0) break; /* peer gone; short body if Content-Length disagreed */
        fwrite(buf, 1, (size_t)r, out);
        got += r;
    }

    fclose(out);
    return 0;
}
