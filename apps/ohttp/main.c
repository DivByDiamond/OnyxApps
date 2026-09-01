/*
 * main.c - CLI entry point for ohttp.
 *
 *   ohttp <ip[:port]/path> <outfile>
 *   ohttp 93.184.216.34/index.html page.html
 *   ohttp 10.0.2.2:8080/api/data.json data.json
 *
 * No DNS in userspace yet (todo.md): the target is an IPv4, not a
 * hostname. Fetches over plain HTTP/1.1 and writes the body to outfile.
 */
#include <stdio.h>
#include <net.h>
#include "ohttp.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <ip[:port]/path> <outfile>\n", argv[0]);
        return 1;
    }

    ohttp_target target;
    if (ohttp_parse_target(argv[1], &target) != 0) return 1;

    int conn_id = ohttp_connect(&target);
    if (conn_id < 0) return 1;

    int rc = ohttp_send_request(conn_id, &target);
    if (rc == 0) rc = ohttp_read_response(conn_id, argv[2]);

    net_close(conn_id);
    return rc == 0 ? 0 : 1;
}
