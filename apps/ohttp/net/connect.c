/*
 * net/connect.c - opens the TCP connection and sends the GET request.
 * Thin layer over libonyxc's net_connect/net_send (io/net.h), which wrap
 * OnyxKernel's outbound-only TCP syscalls (#80-83).
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <net.h>
#include "../ohttp.h"

int ohttp_connect(const ohttp_target *target) {
    int conn_id = net_connect(target->ip, target->port);
    if (conn_id < 0) {
        fprintf(stderr, "ohttp: connect to %d.%d.%d.%d:%d failed (errno %d)\n",
                target->ip[0], target->ip[1], target->ip[2], target->ip[3],
                target->port, errno);
        return -1;
    }
    return conn_id;
}

int ohttp_send_request(int conn_id, const ohttp_target *target) {
    char req[OHTTP_MAX_PATH + OHTTP_MAX_HOST + 128];
    int n = snprintf(req, sizeof(req),
                      "GET %s HTTP/1.1\r\n"
                      "Host: %s\r\n"
                      "User-Agent: ohttp/0.1 (OnyxOS)\r\n"
                      "Connection: close\r\n"
                      "\r\n",
                      target->path, target->host);
    if (n <= 0 || (size_t)n >= sizeof(req)) {
        fprintf(stderr, "ohttp: request too large to build\n");
        return -1;
    }

    size_t sent = 0;
    while (sent < (size_t)n) {
        long r = net_send(conn_id, req + sent, (size_t)n - sent);
        if (r < 0) {
            fprintf(stderr, "ohttp: send failed (errno %d)\n", errno);
            return -1;
        }
        if (r == 0) continue; /* non-blocking gap: no bytes accepted yet */
        sent += (size_t)r;
    }
    return 0;
}
