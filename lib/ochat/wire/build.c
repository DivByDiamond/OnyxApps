/* build.c - build JSON parameter arrays for radio RPC calls (bounded). */
#include "../chat.h"

/* Append a JSON string literal; returns new offset or -1. */
static int put_string(char *out, size_t cap, size_t o, const char *s) {
    static const char hex[] = "0123456789abcdef";
    if (o + 1 >= cap) return -1;
    out[o++] = '"';
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') {
            if (o + 2 >= cap) return -1;
            out[o++] = '\\'; out[o++] = (char)c;
        } else if (c < 0x20 || c == 0x7F) {
            if (o + 6 >= cap) return -1;
            out[o++] = '\\'; out[o++] = 'u'; out[o++] = '0'; out[o++] = '0';
            out[o++] = hex[c >> 4]; out[o++] = hex[c & 15];
        } else {
            if (o + 1 >= cap) return -1;
            out[o++] = (char)c;
        }
    }
    if (o + 1 >= cap) return -1;
    out[o++] = '"';
    return (int)o;
}

int ochat_build_send(const char *addr, const char *request, const char *text,
                     char *out, size_t cap) {
    const char *parts[5];
    int i, o = 1;
    parts[0] = addr; parts[1] = OCHAT_SERVICE; parts[2] = OCHAT_CHANNEL;
    parts[3] = request; parts[4] = text;
    if (!out || cap < 4) return -1;
    out[0] = '[';
    for (i = 0; i < 5; i++) {
        if (!parts[i]) return -1;
        if (i) { if ((size_t)o + 1 >= cap) return -1; out[o++] = ','; }
        o = put_string(out, cap, (size_t)o, parts[i]);
        if (o < 0) return -1;
    }
    if ((size_t)o + 2 > cap) return -1;
    out[o++] = ']'; out[o] = 0;
    return o;
}

int ochat_build_args(const char *a, const char *b, char *out, size_t cap) {
    int o = 1;
    if (!a || !out || cap < 4) return -1;
    out[0] = '[';
    o = put_string(out, cap, (size_t)o, a);
    if (o < 0) return -1;
    if (b) {
        if ((size_t)o + 1 >= cap) return -1;
        out[o++] = ',';
        o = put_string(out, cap, (size_t)o, b);
        if (o < 0) return -1;
    }
    if ((size_t)o + 2 > cap) return -1;
    out[o++] = ']'; out[o] = 0;
    return o;
}
