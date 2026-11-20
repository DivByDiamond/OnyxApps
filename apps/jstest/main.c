/*
 * main.c - program entry and the /dev/jsN read loop for jstest.
 *
 * Usage: jstest [/dev/jsN]     (default /dev/js0)
 *
 * Opens the joydev-compatible node, drains the open-time init burst
 * and then prints one line per live event. An empty ring makes
 * read() fail with EAGAIN (the kernel's Errno::Again reaches
 * userspace as POSIX errno 11): sleep 10 ms and retry, like the other
 * nonblocking readers in this repo (mux, obrowse, lib/oc2rpc).
 *
 * Exit codes: 0 clean exit (--help; the event loop itself never ends),
 * 2 usage error, 3 device error (open failure or a non-EAGAIN read).
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "jstest.h"

#define DEFAULT_DEVICE "/dev/js0"
#define POLL_US 10000UL /* 10 ms retry after EAGAIN */
#define LINE_CAP 48     /* longest line: "axis 255 = -32768 (init)" */

static const char *const USAGE =
    "usage: jstest [/dev/jsN]    print joystick events (default /dev/js0)\r\n"
    "exit codes: 0 clean, 2 usage, 3 device error\r\n";

static int is_help(const char *arg) {
    return strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0;
}

/* Report a hard failure on the device path; returns the exit code. */
static int device_error(const char *what, const char *device) {
    fprintf(stderr, "jstest: %s %s failed (errno %d)\r\n",
            what, device, errno);
    return 3;
}

/* Open the node and loop: decode every 8-byte record, sleep through
 * EAGAIN, bail out on any other read failure. Never returns on the
 * happy path. */
static int run(const char *device) {
    int fd = open(device, O_RDONLY, 0);

    if (fd < 0) return device_error("open", device);
    for (;;) {
        unsigned char bytes[JS_REC_BYTES];
        long n = read(fd, bytes, JS_REC_BYTES);

        if (n == JS_REC_BYTES) {
            struct js_event ev;
            char line[LINE_CAP];
            if (js_decode(bytes, &ev) != 0) {
                close(fd);
                fprintf(stderr, "jstest: %s: bad record type 0x%02x\r\n",
                        device, (unsigned int)bytes[6]);
                return 3;
            }
            printf("%s\r\n", js_format(&ev, line, sizeof(line)));
        } else if (n < 0 && errno == EAGAIN) {
            usleep(POLL_US); /* empty ring: poll again shortly */
        } else if (n < 0 && errno == EINTR) {
            /* interrupted: just retry the read */
        } else {
            close(fd);
            return device_error("read", device);
        }
    }
}

int main(int argc, char **argv) {
    if (argc == 2 && is_help(argv[1])) {
        fputs(USAGE, stderr);
        return 0;
    }
    if (argc > 2 || (argc == 2 && argv[1][0] == '-')) {
        fputs(USAGE, stderr);
        return 2;
    }
    return run((argc == 2) ? argv[1] : DEFAULT_DEVICE);
}
