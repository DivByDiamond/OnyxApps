/*
 * test_jstest.c - host tests for the pure jstest modules
 * (apps/jstest/events.c: /dev/jsN record decode + line format).
 * Runs on any Linux with gcc:
 *
 *   make -C tests/native test_jstest && ./tests/native/test_jstest
 *
 * The tested code has no I/O, so no terminal is needed. The live read
 * loop (open, EAGAIN retry) needs a real OnyxOS with /dev/js0 and is
 * verified manually in QEMU; CI builds the .onx with onyxcc.
 */
#include <stdio.h>
#include <string.h>
#include "jstest.h"

static int failures = 0;
static int checks = 0;

#define CHECK(cond)                                                      \
    do {                                                                 \
        checks++;                                                        \
        if (cond) {                                                      \
            printf("  ok  %s\n", #cond);                                 \
        } else {                                                         \
            printf("  FAIL %s (%s:%d)\n", #cond, __FILE__, __LINE__);    \
            failures++;                                                  \
        }                                                                \
    } while (0)

/* The exact wire bytes one /dev/jsN read() hands back (joydev layout,
 * little-endian: u32 time_ms, i16 value, u8 type, u8 number). */
static struct js_event decode_bytes(const unsigned char *b) {
    struct js_event ev;
    memset(&ev, 0, sizeof(ev));
    (void)js_decode(b, &ev);
    return ev;
}

static void test_axis_decode(void) {
    /* {e8 03 00 00, 34 12, 02, 03}: time=1000, value=0x1234, axis 3.
     * All fields little-endian on the wire (kernel rec.rs to_le_bytes);
     * 1000 = 0x000003e8 and 0x1234 read as LE bytes. */
    const unsigned char b[JS_REC_BYTES] = {0xe8, 0x03, 0x00, 0x00,
                                           0x34, 0x12, 0x02, 0x03};
    struct js_event ev = decode_bytes(b);
    char line[64];

    printf("axis record decode:\n");
    CHECK(js_decode(b, &ev) == 0);
    CHECK(ev.time_ms == 1000);
    CHECK(ev.value == 4660); /* 0x1234 */
    CHECK(ev.type == JS_EVENT_AXIS);
    CHECK(ev.number == 3);
    CHECK((ev.type & JS_EVENT_INIT) == 0);
    CHECK(strcmp(js_format(&ev, line, sizeof(line)), "axis 3 = 4660") == 0);
}

static void test_axis_init(void) {
    /* Same record with the init bit set in the type byte (0x82). */
    const unsigned char b[JS_REC_BYTES] = {0xe8, 0x03, 0x00, 0x00,
                                           0x34, 0x12, 0x82, 0x03};
    struct js_event ev = decode_bytes(b);
    char line[64];

    printf("axis init record decode:\n");
    CHECK(ev.type == (JS_EVENT_AXIS | JS_EVENT_INIT));
    CHECK((ev.type & JS_EVENT_INIT) != 0); /* the init bit is recognized */
    CHECK(ev.value == 4660);
    CHECK(ev.number == 3);
    CHECK(strcmp(js_format(&ev, line, sizeof(line)),
                 "axis 3 = 4660 (init)") == 0);
}

static void test_button_decode(void) {
    /* {..., 01, 05} with value 1: button 5 pressed. */
    const unsigned char b[JS_REC_BYTES] = {0x40, 0x0d, 0x03, 0x00,
                                           0x01, 0x00, 0x01, 0x05};
    struct js_event ev = decode_bytes(b);
    char line[64];

    printf("button record decode:\n");
    CHECK(js_decode(b, &ev) == 0);
    CHECK(ev.time_ms == 200000); /* 0x00030d40 */
    CHECK(ev.type == JS_EVENT_BUTTON);
    CHECK(ev.number == 5);
    CHECK(ev.value == 1);
    CHECK(strcmp(js_format(&ev, line, sizeof(line)), "button 5 down") == 0);
}

static void test_button_init_up(void) {
    /* Init burst for an unpressed button (type 0x81, value 0). */
    const unsigned char b[JS_REC_BYTES] = {0x00, 0x00, 0x00, 0x00,
                                           0x00, 0x00, 0x81, 0x05};
    struct js_event ev = decode_bytes(b);
    char line[64];

    printf("button init record decode:\n");
    CHECK(ev.type == (JS_EVENT_BUTTON | JS_EVENT_INIT));
    CHECK((ev.type & JS_EVENT_INIT) != 0);
    CHECK(ev.number == 5);
    CHECK(ev.value == 0);
    CHECK(strcmp(js_format(&ev, line, sizeof(line)),
                 "button 5 up (init)") == 0);
}

static void test_negative_value(void) {
    /* Axis at the negative rail: value bytes F0 FF = -16. */
    const unsigned char b[JS_REC_BYTES] = {0x00, 0x00, 0x00, 0x00,
                                           0xf0, 0xff, 0x02, 0x00};
    struct js_event ev = decode_bytes(b);

    printf("negative axis value (i16 sign):\n");
    CHECK(ev.value == -16);
}

static void test_bad_type(void) {
    /* Type bits outside button/axis/init are rejected, not misread. */
    const unsigned char b[JS_REC_BYTES] = {0x00, 0x00, 0x00, 0x00,
                                           0x00, 0x00, 0x03, 0x00};
    struct js_event ev;

    printf("malformed type byte:\n");
    CHECK(js_decode(b, &ev) == -1);
}

static void test_args(void) {
    struct js_opts o;
    char *none[] = {"jstest"};
    char *dev[] = {"jstest", "/dev/js2"};
    char *timed[] = {"jstest", "-t", "5", "/dev/js1"};
    char *timed_first[] = {"jstest", "/dev/js3", "-t", "30"};
    char *help[] = {"jstest", "--help"};
    char *no_secs[] = {"jstest", "-t"};
    char *zero[] = {"jstest", "-t", "0"};
    char *junk[] = {"jstest", "-t", "5x"};
    char *huge[] = {"jstest", "-t", "86401"};
    char *two_dev[] = {"jstest", "/dev/js0", "/dev/js1"};
    char *unknown[] = {"jstest", "-x"};

    printf("argument parsing:\n");
    CHECK(js_parse_args(1, none, &o) == JS_ARGS_OK);
    CHECK(strcmp(o.device, "/dev/js0") == 0 && o.secs == 0);
    CHECK(js_parse_args(2, dev, &o) == JS_ARGS_OK);
    CHECK(strcmp(o.device, "/dev/js2") == 0 && o.secs == 0);
    CHECK(js_parse_args(4, timed, &o) == JS_ARGS_OK);
    CHECK(strcmp(o.device, "/dev/js1") == 0 && o.secs == 5);
    CHECK(js_parse_args(4, timed_first, &o) == JS_ARGS_OK);
    CHECK(strcmp(o.device, "/dev/js3") == 0 && o.secs == 30);
    CHECK(js_parse_args(2, help, &o) == JS_ARGS_HELP);
    CHECK(js_parse_args(2, no_secs, &o) == JS_ARGS_USAGE);
    CHECK(js_parse_args(3, zero, &o) == JS_ARGS_USAGE);
    CHECK(js_parse_args(3, junk, &o) == JS_ARGS_USAGE);
    CHECK(js_parse_args(3, huge, &o) == JS_ARGS_USAGE);
    CHECK(js_parse_args(3, two_dev, &o) == JS_ARGS_USAGE);
    CHECK(js_parse_args(2, unknown, &o) == JS_ARGS_USAGE);
}

int main(void) {
    test_axis_decode();
    test_axis_init();
    test_button_decode();
    test_button_init_up();
    test_negative_value();
    test_bad_type();
    test_args();
    printf("\ntest_jstest: %d checks, %d failures\n", checks, failures);
    return failures > 0;
}
