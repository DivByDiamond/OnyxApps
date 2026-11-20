/*
 * jstest.h - shared types and module API for the joystick diagnostic.
 *
 * main.c       - program entry: args, open, the EAGAIN retry read loop
 * events.c     - pure /dev/jsN record decode and one-line formatting
 *
 * events.c is pure C99 with no I/O: it is exercised by the host tests
 * in tests/native/ (test_jstest) without OnyxOS or onyxcc, so the test
 * runs the exact decode code the app runs on the device.
 */
#ifndef JSTEST_H
#define JSTEST_H

#include <stddef.h>

/* joydev-compatible event types, mirrored from the kernel's /dev/jsN
 * records (OnyxKernel kernel/src/fs/devfs/js/rec.rs, linux/joystick.h). */
#define JS_EVENT_BUTTON 0x01 /* value: 0 = up, 1 = down */
#define JS_EVENT_AXIS   0x02 /* value: raw signed axis sample */
#define JS_EVENT_INIT   0x80 /* OR'ed into type by the open-time burst */
#define JS_REC_BYTES    8    /* u32 time_ms + i16 value + u8 type + u8 number */

/* One decoded /dev/jsN record. */
struct js_event {
    unsigned int time_ms; /* milliseconds since boot, wraps ~49.7 days */
    short value;         /* axis: raw sample; button: 0 up, 1 down */
    unsigned char type;  /* JS_EVENT_{BUTTON,AXIS}, OR'ed with JS_EVENT_INIT */
    unsigned char number; /* axis id or button index */
};

/* events.c - pure record decode and line formatting. */

/* Decode 8 little-endian wire bytes into *ev by explicit byte
 * composition (never a packed-struct reinterpretation of the raw
 * bytes). Returns 0 on a recognized record, -1 when the type byte
 * carries bits outside button/axis/init. */
int js_decode(const unsigned char bytes[JS_REC_BYTES], struct js_event *ev);

/* One-line human form of a decoded record, e.g.
 *   "axis 3 = 4660 (init)"  /  "axis 2 = -16"
 *   "button 5 down"         /  "button 5 up (init)"
 * Returns buf so callers can use it inline as an argument. */
char *js_format(const struct js_event *ev, char *buf, size_t cap);

#endif /* JSTEST_H */
