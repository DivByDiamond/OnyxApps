/*
 * net/request.c - parses the "ip[:port][/path]" target syntax.
 *
 * No DNS resolver is exposed to userspace yet (todo.md), so the caller
 * gives ohttp a raw IPv4 instead of a hostname; the Host: header still
 * gets a name via a separate --host override in main.c.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int ohttp_parse_target(const char *arg, ohttp_target *out) {
    if (strncmp(arg, "https://", 8) == 0) {
        fprintf(stderr, "ohttp: https:// is not supported (no TLS in-kernel)\n");
        return -1;
    }
    if (strncmp(arg, "http://", 7) == 0) arg += 7;

    const char *rest = NULL;
    if (parse_ip(arg, out->ip, &rest) != 0) {
        fprintf(stderr, "ohttp: expected an IPv4 target, e.g. 93.184.216.34/ (got '%s')\n", arg);
        return -1;
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

    snprintf(out->host, OHTTP_MAX_HOST, "%d.%d.%d.%d",
              out->ip[0], out->ip[1], out->ip[2], out->ip[3]);
    return 0;
}
