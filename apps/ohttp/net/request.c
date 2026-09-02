/*
 * net/request.c - parses the "host[:port][/path]" target syntax.
 *
 * A raw IPv4 is used as-is; anything else is resolved via the kernel's
 * net_resolve() (OnyxKernel/kernel/src/syscall/net_sys.rs, #89), a
 * blocking DNS A-record lookup against the DHCP-learned server.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net.h>
#include "../ohttp.h"

static int parse_ip(const char *s, unsigned char ip[4], const char **rest) {
    int part = 0;
    const char *p = s;
    for (int i = 0; i < 4; i++) {
        if (i > 0) {
            if (*p != '.') return -1;
            p++;
        }
        if (*p < '0' || *p > '9') return -1;
        long v = strtol(p, (char **)&p, 10);
        if (v < 0 || v > 255) return -1;
        ip[i] = (unsigned char)v;
        part++;
    }
    (void)part;
    *rest = p;
    return 0;
}

/* Extracts the leading "host" component (up to '/', ':', or NUL) into
 * out (size cap), and returns a pointer to the byte in arg right after it. */
static const char *split_host(const char *arg, char *out, size_t cap) {
    size_t n = 0;
    while (arg[n] != '\0' && arg[n] != '/' && arg[n] != ':') n++;
    if (n == 0 || n >= cap) return NULL;
    memcpy(out, arg, n);
    out[n] = '\0';
    return arg + n;
}

int ohttp_parse_target(const char *arg, ohttp_target *out) {
    if (strncmp(arg, "https://", 8) == 0) {
        fprintf(stderr, "ohttp: https:// is not supported (no TLS in-kernel)\n");
        return -1;
    }
    if (strncmp(arg, "http://", 7) == 0) arg += 7;

    const char *rest = NULL;
    int had_hostname = 0;
    if (parse_ip(arg, out->ip, &rest) != 0) {
        /* Not a raw IPv4 — treat the leading component as a hostname. */
        char hostbuf[OHTTP_MAX_HOST];
        rest = split_host(arg, hostbuf, sizeof(hostbuf));
        if (rest == NULL) {
            fprintf(stderr, "ohttp: bad target '%s'\n", arg);
            return -1;
        }
        if (net_resolve(hostbuf, out->ip) != 0) {
            fprintf(stderr, "ohttp: could not resolve '%s'\n", hostbuf);
            return -1;
        }
        strcpy(out->host, hostbuf);
        had_hostname = 1;
    }

    out->port = 80;
    if (*rest == ':') {
        rest++;
        char *end = NULL;
        long v = strtol(rest, &end, 10);
        if (v <= 0 || v > 65535) {
            fprintf(stderr, "ohttp: bad port in '%s'\n", arg);
            return -1;
        }
        out->port = (int)v;
        rest = end;
    }

    if (*rest == '\0') {
        strcpy(out->path, "/");
    } else if (*rest == '/') {
        strncpy(out->path, rest, OHTTP_MAX_PATH - 1);
        out->path[OHTTP_MAX_PATH - 1] = '\0';
    } else {
        fprintf(stderr, "ohttp: unexpected trailer '%s' in target\n", rest);
        return -1;
    }

    if (!had_hostname) {
        snprintf(out->host, OHTTP_MAX_HOST, "%d.%d.%d.%d",
                  out->ip[0], out->ip[1], out->ip[2], out->ip[3]);
    }
    return 0;
}
