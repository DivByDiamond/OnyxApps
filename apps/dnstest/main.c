#include <stdio.h>
#include <errno.h>
#include <net.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <hostname>\n", argv[0]);
        return 1;
    }
    unsigned char ip[4];
    if (net_resolve(argv[1], ip) != 0) {
        fprintf(stderr, "resolve failed errno=%d\n", errno);
        return 1;
    }
    printf("%s -> %d.%d.%d.%d\n", argv[1], ip[0], ip[1], ip[2], ip[3]);
    return 0;
}
