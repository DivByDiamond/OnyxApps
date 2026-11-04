/* lineedit.c - bounded single-line editor fed one raw byte at a time. */
#include "../chat.h"

void ochat_line_init(struct ochat_line *l) {
    l->len = 0; l->buf[0] = 0; l->esc = 0;
}

int ochat_line_feed(struct ochat_line *l, unsigned char c) {
    if (l->esc == 1) { l->esc = (c == '[' || c == 'O') ? 2 : 0; return OCHAT_KEY_NONE; }
    if (l->esc == 2) {
        /* CSI: parameters 0x30-0x3F, final byte 0x40-0x7E ends the sequence. */
        if (c >= 0x40 && c <= 0x7E) l->esc = 0;
        return OCHAT_KEY_NONE;
    }
    if (c == 0x1B) { l->esc = 1; return OCHAT_KEY_NONE; }
    if (c == '\r' || c == '\n') return OCHAT_KEY_SUBMIT;
    if (c == 0x03 || (c == 0x04 && l->len == 0)) return OCHAT_KEY_CANCEL;
    if (c == 0x7F || c == 0x08) {
        if (l->len == 0) return OCHAT_KEY_NONE;
        do { l->len--; } while (l->len > 0 && ((unsigned char)l->buf[l->len] & 0xC0) == 0x80);
        l->buf[l->len] = 0;
        return OCHAT_KEY_EDIT;
    }
    if (c == 0x15) { l->len = 0; l->buf[0] = 0; return OCHAT_KEY_EDIT; }
    if (c < 0x20) return OCHAT_KEY_NONE;
    if (l->len >= OCHAT_LINE_MAX) return OCHAT_KEY_NONE;
    l->buf[l->len++] = (char)c;
    l->buf[l->len] = 0;
    return OCHAT_KEY_EDIT;
}
