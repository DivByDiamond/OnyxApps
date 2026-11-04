/* jval.c - decode JSON scalar values: strings (UTF-8, \u escapes), numbers, booleans. */
#include "../chat.h"
#include <string.h>

static int hex4(const char *p) {
    int i, v = 0;
    for (i = 0; i < 4; i++) {
        char c = p[i];
        v <<= 4;
        if (c >= '0' && c <= '9') v |= c - '0';
        else if (c >= 'a' && c <= 'f') v |= c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') v |= c - 'A' + 10;
        else return -1;
    }
    return v;
}

static int put_utf8(unsigned long cp, char *out, size_t room) {
    if (cp < 0x80) { if (room < 1) return 0; out[0] = (char)cp; return 1; }
    if (cp < 0x800) { if (room < 2) return 0;
        out[0] = (char)(0xC0 | (cp >> 6)); out[1] = (char)(0x80 | (cp & 63)); return 2; }
    if (cp < 0x10000) { if (room < 3) return 0;
        out[0] = (char)(0xE0 | (cp >> 12)); out[1] = (char)(0x80 | ((cp >> 6) & 63));
        out[2] = (char)(0x80 | (cp & 63)); return 3; }
    if (room < 4) return 0;
    out[0] = (char)(0xF0 | (cp >> 18)); out[1] = (char)(0x80 | ((cp >> 12) & 63));
    out[2] = (char)(0x80 | ((cp >> 6) & 63)); out[3] = (char)(0x80 | (cp & 63));
    return 4;
}

long ochat_j_string(const char *v, char *out, size_t cap) {
    size_t o = 0;
    const char *end;
    if (!v || !out || cap == 0) return -1;
    v = ochat_j_skip(v);
    if (*v != '"') return -1;
    end = ochat_j_str_end(v);
    if (!end) return -1;
    v++;
    while (v < end - 1) {
        unsigned long cp;
        int n;
        char c = *v++;
        if (c == '\\') {
            c = *v++;
            switch (c) {
            case 'n': c = '\n'; break;
            case 't': c = '\t'; break;
            case 'r': c = '\r'; break;
            case 'b': c = '\b'; break;
            case 'f': c = '\f'; break;
            case '"': case '\\': case '/': break;
            case 'u': {
                int h = hex4(v);
                if (h < 0 || v + 4 > end - 1) return -1;
                v += 4;
                cp = (unsigned long)h;
                if (cp >= 0xD800 && cp <= 0xDBFF) {
                    int lo;
                    if (v + 6 > end - 1 || v[0] != '\\' || v[1] != 'u') return -1;
                    lo = hex4(v + 2);
                    if (lo < 0xDC00 || lo > 0xDFFF) return -1;
                    v += 6;
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (unsigned long)(lo - 0xDC00);
                } else if (cp >= 0xDC00 && cp <= 0xDFFF) return -1;
                n = put_utf8(cp, out + o, cap - 1 - o);
                if (!n) return -1;
                o += (size_t)n;
                continue;
            }
            default: return -1;
            }
        }
        if (o + 1 >= cap) return -1;
        out[o++] = c;
    }
    out[o] = 0;
    return (long)o;
}

int ochat_j_long(const char *v, long *out) {
    long n = 0;
    int neg = 0, digits = 0;
    if (!v) return 0;
    v = ochat_j_skip(v);
    if (*v == '-') { neg = 1; v++; }
    while (*v >= '0' && *v <= '9') {
        if (n > 100000000000000000L) return 0; /* bound before overflow */
        n = n * 10 + (*v - '0'); v++; digits++;
    }
    if (!digits) return 0;
    if (*v == '.') {       /* integral part of a JSON float, e.g. 12.0 */
        v++;
        while (*v >= '0' && *v <= '9') v++;
    }
    if (*v == 'e' || *v == 'E') return 0;
    *out = neg ? -n : n;
    return 1;
}

int ochat_j_bool(const char *v, int *out) {
    if (!v) return 0;
    v = ochat_j_skip(v);
    if (!strncmp(v, "true", 4) && !(v[4] >= 'a' && v[4] <= 'z')) { *out = 1; return 1; }
    if (!strncmp(v, "false", 5) && !(v[5] >= 'a' && v[5] <= 'z')) { *out = 0; return 1; }
    return 0;
}
