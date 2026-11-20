/*
 * events.c - pure /dev/jsN record decode and one-line formatting.
 *
 * The wire layout is joydev-compatible and fixed: time_ms u32 LE,
 * value i16 LE, type u8, number u8 (JS_REC_BYTES = 8). Bytes are
 * composed explicitly - no packed struct is ever reinterpreted from
 * the raw buffer - so the decode is identical under gcc (host tests)
 * and onyxcc (the .onx build).
 */
#include <stdio.h>

#include "jstest.h"

int js_decode(const unsigned char b[JS_REC_BYTES], struct js_event *ev) {
    unsigned char base = (unsigned char)(b[6] & ~JS_EVENT_INIT);

    if (base != JS_EVENT_BUTTON && base != JS_EVENT_AXIS) return -1;
    ev->time_ms = (unsigned int)b[0]
                | ((unsigned int)b[1] << 8)
                | ((unsigned int)b[2] << 16)
                | ((unsigned int)b[3] << 24);
    ev->value = (short)((unsigned short)b[4] | ((unsigned short)b[5] << 8));
    ev->type = b[6];
    ev->number = b[7];
    return 0;
}

char *js_format(const struct js_event *ev, char *buf, size_t cap) {
    const char *init = (ev->type & JS_EVENT_INIT) ? " (init)" : "";
    unsigned char base = (unsigned char)(ev->type & ~JS_EVENT_INIT);

    if (base == JS_EVENT_BUTTON) {
        snprintf(buf, cap, "button %u %s%s",
                 (unsigned int)ev->number, ev->value ? "down" : "up", init);
    } else {
        snprintf(buf, cap, "axis %u = %d%s",
                 (unsigned int)ev->number, (int)ev->value, init);
    }
    return buf;
}
