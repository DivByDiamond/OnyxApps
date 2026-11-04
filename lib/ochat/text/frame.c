/* frame.c - message text validation/sanitising, UUID text, ASCII fold. */
#include "../chat.h"

/* Length of the valid UTF-8 sequence at p (n bytes left), or 0 if invalid.
 * Rejects overlong forms, surrogates and values above U+10FFFF. */
static int utf8_len(const unsigned char *p, size_t n) {
    unsigned char c = p[0];
    if (c < 0x80) return 1;
    if (c >= 0xC2 && c <= 0xDF) return (n >= 2 && (p[1] & 0xC0) == 0x80) ? 2 : 0;
    if (c >= 0xE0 && c <= 0xEF) {
        if (n < 3 || (p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80) return 0;
        if (c == 0xE0 && p[1] < 0xA0) return 0;
        if (c == 0xED && p[1] > 0x9F) return 0;
        return 3;
    }
    if (c >= 0xF0 && c <= 0xF4) {
        if (n < 4 || (p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80 ||
            (p[3] & 0xC0) != 0x80) return 0;
        if (c == 0xF0 && p[1] < 0x90) return 0;
        if (c == 0xF4 && p[1] > 0x8F) return 0;
        return 4;
    }
    return 0;
}

long ochat_text_clean(const char *in, size_t len, char *out, size_t cap, int strict) {
    size_t i = 0, o = 0, limit;
    int k, l;
    if (!in || !out || cap < 2) return OCHAT_E_LONG;
    limit = cap - 1;
    if (limit > OCHAT_MAX_TEXT) limit = OCHAT_MAX_TEXT;
    while (i < len && in[i]) {
        const unsigned char *p = (const unsigned char *)in + i;
        l = utf8_len(p, len - i);
        if (l == 0) {
            if (strict) return OCHAT_E_UTF8;
            if (o + 1 > limit) break;
            out[o++] = '?'; i++;
            continue;
        }
        if (l == 1) {
            char c = in[i];
            if (c < 0x20 || c == 0x7F) c = (c == '\t' || c == '\n' || c == '\r') ? ' ' : 0;
            if (c) {
                if (o + 1 > limit) { if (strict) return OCHAT_E_LONG; break; }
                out[o++] = c;
            }
            i++;
            continue;
        }
        /* C1 controls (U+0080..U+009F) are 0xC2 0x80..0x9F: drop them. */
        if (p[0] == 0xC2 && p[1] < 0xA0) { i += 2; continue; }
        if (o + (size_t)l > limit) { if (strict) return OCHAT_E_LONG; break; }
        for (k = 0; k < l; k++) out[o++] = in[i + k];
        i += l;
    }
    if (strict && i < len && in[i]) return OCHAT_E_LONG;
    if (strict) {
        size_t s = 0;
        while (o > 0 && out[o - 1] == ' ') o--;
        while (s < o && out[s] == ' ') s++;
        if (s) { for (i = s; i < o; i++) out[i - s] = out[i]; o -= s; }
        if (o == 0) { out[0] = 0; return OCHAT_E_EMPTY; }
    }
    out[o] = 0;
    return (long)o;
}

void ochat_uuid_v4(const unsigned char rnd[16], char out[OCHAT_UUID_SIZE]) {
    static const char hex[] = "0123456789abcdef";
    unsigned char b[16];
    int i, o = 0;
    for (i = 0; i < 16; i++) b[i] = rnd[i];
    b[6] = (unsigned char)((b[6] & 0x0F) | 0x40);
    b[8] = (unsigned char)((b[8] & 0x3F) | 0x80);
    for (i = 0; i < 16; i++) {
        if (i == 4 || i == 6 || i == 8 || i == 10) out[o++] = '-';
        out[o++] = hex[b[i] >> 4];
        out[o++] = hex[b[i] & 15];
    }
    out[o] = 0;
}

void ochat_ascii_fold(const char *in, char *out, size_t cap) {
    size_t o = 0, i = 0, n;
    if (!out || cap == 0) return;
    if (in) {
        for (n = 0; in[n]; n++) {}
        while (i < n && o + 1 < cap) {
            unsigned char c = (unsigned char)in[i];
            if (c < 0x80) { out[o++] = (c < 0x20 || c == 0x7F) ? '?' : (char)c; i++; }
            else {
                int l = utf8_len((const unsigned char *)in + i, n - i);
                out[o++] = '?';
                i += l ? l : 1;
            }
        }
    }
    out[o] = 0;
}
