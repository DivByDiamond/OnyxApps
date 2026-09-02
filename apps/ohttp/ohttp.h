/*
 * ohttp.h - shared state and module API for ohttp, a minimal HTTP/1.1
 * GET client. First step toward obrowse (see README.md): fetches one URL
 * to a local file over plain TCP.
 *
 * No TLS: hostnames are resolved via the kernel's net_resolve() DNS
 * syscall (kernel/src/syscall/net_sys.rs, #89), then fetched over plain
 * outbound TCP (#80-83). http:// only; an https:// URL is rejected up
 * front with a clear error.
 */
#ifndef OHTTP_H
#define OHTTP_H

#include <stddef.h>

#define OHTTP_MAX_HOST   256
#define OHTTP_MAX_PATH   1024
#define OHTTP_RECV_BUF   4096
#define OHTTP_MAX_HEADER 8192   /* header section must fit this before body starts */

/* Parsed request target (net/request.c). */
typedef struct {
    unsigned char ip[4];   /* resolved/given IPv4, network order */
    int           port;    /* default 80 */
    char          host[OHTTP_MAX_HOST];  /* for the Host: header */
    char          path[OHTTP_MAX_PATH];  /* request path, defaults to "/" */
} ohttp_target;

/* Parses "host:port/path" or "host/path" (host = IPv4 or a DNS name, no
 * scheme) into *out. Returns 0 on success, -1 on a malformed or
 * unresolvable target (message on stderr). */
int ohttp_parse_target(const char *arg, ohttp_target *out);

/* Opens a TCP connection to target->ip:port. Returns conn_id >= 0, or -1
 * with errno set (net/connect.c). */
int ohttp_connect(const ohttp_target *target);

/* Builds and sends a minimal HTTP/1.1 GET request with Host and
 * Connection: close. Returns 0 on success, -1 on a short/failed send
 * (net/connect.c). */
int ohttp_send_request(int conn_id, const ohttp_target *target);

/* Reads the response from conn_id, splits status line / headers / body,
 * and writes the body to out_path. Prints the status line and
 * Content-Length (if present) to stdout. Returns 0 on success, -1 on a
 * network error or a response that doesn't fit OHTTP_MAX_HEADER
 * (http/response.c). */
int ohttp_read_response(int conn_id, const char *out_path);

#endif /* OHTTP_H */
