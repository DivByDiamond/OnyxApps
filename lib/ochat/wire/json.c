/* json.c - small bounded JSON scanner. Works in place on a NUL-terminated
 * buffer, never allocates, nests at most 32 levels, and never reads past the
 * terminating NUL. Value pointers point at the first byte of a value. */
#include "../chat.h"
#include <string.h>

#define MAX_DEPTH 32

const char *ochat_j_skip(const char *p) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    return p;
}

const char *ochat_j_str_end(const char *p) {
    p++;
    while (*p) {
        if ((unsigned char)*p < 0x20) return NULL;
        if (*p == '\\') { if (!p[1]) return NULL; p += 2; continue; }
        if (*p == '"') return p + 1;
        p++;
    }
    return NULL;
}

/* Consume `"key" :` and return the position after the colon. */
static const char *key_colon(const char *p) {
    p = ochat_j_skip(p);
    if (*p != '"') return NULL;
    p = ochat_j_str_end(p);
    if (!p) return NULL;
    p = ochat_j_skip(p);
    return *p == ':' ? p + 1 : NULL;
}

const char *ochat_j_end(const char *v) {
    char stack[MAX_DEPTH];
    int depth = 0;
    if (!v) return NULL;
    for (;;) {
        v = ochat_j_skip(v);
        if (*v == '"') {
            v = ochat_j_str_end(v);
            if (!v) return NULL;
        } else if (*v == '{' || *v == '[') {
            char close = *v == '{' ? '}' : ']';
            v = ochat_j_skip(v + 1);
            if (*v == close) v++;
            else {
                if (depth >= MAX_DEPTH) return NULL;
                stack[depth++] = close;
                if (close == '}') { v = key_colon(v); if (!v) return NULL; }
                continue;
            }
        } else if (*v == '-' || (*v >= '0' && *v <= '9') || (*v >= 'a' && *v <= 'z')) {
            while (*v == '-' || *v == '+' || *v == '.' || (*v >= '0' && *v <= '9') ||
                   (*v >= 'a' && *v <= 'z') || (*v >= 'A' && *v <= 'Z')) v++;
        } else return NULL;
        for (;;) {           /* a value just ended: unwind or continue siblings */
            if (depth == 0) return v;
            v = ochat_j_skip(v);
            if (*v == ',') {
                v++;
                if (stack[depth - 1] == '}') { v = key_colon(v); if (!v) return NULL; }
                break;
            }
            if (*v == stack[depth - 1]) { depth--; v++; continue; }
            return NULL;
        }
    }
}

const char *ochat_j_first(const char *arr) {
    arr = ochat_j_skip(arr);
    if (*arr != '[') return NULL;
    arr = ochat_j_skip(arr + 1);
    return *arr == ']' ? NULL : arr;
}

const char *ochat_j_next(const char *elem) {
    const char *e = ochat_j_end(elem);
    if (!e) return NULL;
    e = ochat_j_skip(e);
    if (*e != ',') return NULL;
    e = ochat_j_skip(e + 1);
    return *e == ']' || !*e ? NULL : e;
}

static int key_equals(const char *k, const char *key) {
    long n;
    char tmp[64];
    n = ochat_j_string(k, tmp, sizeof(tmp));
    return n >= 0 && !strcmp(tmp, key);
}

const char *ochat_j_member(const char *obj, const char *key) {
    const char *p;
    if (!obj) return NULL;
    p = ochat_j_skip(obj);
    if (*p != '{') return NULL;
    p = ochat_j_skip(p + 1);
    while (*p == '"') {
        const char *ke = ochat_j_str_end(p), *val;
        if (!ke) return NULL;
        val = ochat_j_skip(ke);
        if (*val != ':') return NULL;
        val = ochat_j_skip(val + 1);
        if (key_equals(p, key)) return val;
        p = ochat_j_end(val);
        if (!p) return NULL;
        p = ochat_j_skip(p);
        if (*p != ',') return NULL;
        p = ochat_j_skip(p + 1);
    }
    return NULL;
}
