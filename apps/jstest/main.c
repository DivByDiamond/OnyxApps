/*
 * main.c - program entry and the /dev/jsN read loop for jstest.
 *
 * Usage: jstest [-t SECS] [/dev/jsN]     (default /dev/js0)
 * -t SECS makes the loop end cleanly after SECS seconds (scripted runs,
 * the QEMU smoke test); without it the loop never ends.
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
#include <time.h>
#include <unistd.h>

#include "jstest.h"

#define POLL_US 10000UL /* 10 ms retry after EAGAIN */
#define LINE_CAP 48     /* longest line: "axis 255 = -32768 (init)" */

static const char *const USAGE =
    "usage: jstest [-t SECS] [/dev/jsN]   print joystick events (default /dev/js0)\r\n"
    "  -t SECS   exit 0 after SECS seconds (1..86400)\r\n"
    "exit codes: 0 clean, 2 usage, 3 device error\r\n";

/* Report a hard failure on the device path; returns the exit code. */
static int device_error(const char *what, const char *device) {
    fprintf(stderr, "jstest: %s %s failed (errno %d)\r\n",
            what, device, errno);
    return 3;
}

/* Open the node and loop: decode every 8-byte record, sleep through
 * EAGAIN, bail out on any other read failure. Returns 0 only at the
 * -t deadline; without -t it never returns on the happy path. */
static int run(const char *device, unsigned int secs) {
    int fd = open(device, O_RDONLY, 0);
    time_t deadline = time(NULL) + (time_t)secs;

    if (fd < 0) return device_error("open", device);
    for (;;) {
        if (secs != 0 && time(NULL) >= deadline) {
            close(fd);
            return 0;
        }
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
    struct js_opts opts;
    int rc = js_parse_args(argc, argv, &opts);

    if (rc != JS_ARGS_OK) {
        fputs(USAGE, stderr);
        return rc == JS_ARGS_HELP ? 0 : 2;
    }
    return run(opts.device, opts.secs);
}
