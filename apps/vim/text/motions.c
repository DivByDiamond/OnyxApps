/* motions.c — word-motion primitives and bracket matching. */
#include "vim.h"

/* ── Word motions ──────────────────────────────────────────────────── */
int is_word(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_';
}

void motion_word_forward(void) {
    int started_new;
    ensure_buffer();
    if (cy >= nlines) return;
    started_new = 0;
    for (;;) {
        if (cx >= llen[cy]) {
            if (cy < nlines - 1) {
                cy++;
                cx = 0;
                started_new = 1;
                continue;
            }
            return;
        }
        if (is_word(lines[cy][cx])) {
            if (started_new) return;
            while (cx < llen[cy] && is_word(lines[cy][cx])) cx++;
            /* skip one separator */
            if (cx < llen[cy]) cx++;
            return;
        }
        if (!started_new && cx == 0 && llen[cy] > 0 && is_word(lines[cy][0])) {
            /* at sep at line start */
        }
        cx++;
    }
}

void motion_word_back(void) {
    ensure_buffer();
    for (;;) {
        if (cx == 0) {
            if (cy > 0) {
                cy--;
                cx = llen[cy];
                continue;
            }
            cx = 0;
            return;
        }
        cx--;
        if (cx < llen[cy] && is_word(lines[cy][cx])) {
            while (cx > 0 && is_word(lines[cy][cx - 1])) cx--;
            return;
        }
    }
}

void motion_word_end(void) {
    ensure_buffer();
    for (;;) {
        if (cx + 1 >= llen[cy]) {
            if (cy < nlines - 1) {
                cy++;
                cx = 0;
                continue;
            }
            return;
        }
        cx++;
        if (cx < llen[cy] && is_word(lines[cy][cx])) {
            if (cx + 1 <= llen[cy] && (cx + 1 >= llen[cy] ||
                                       !is_word(lines[cy][cx + 1]))) {
                if (cx + 1 < llen[cy] && is_word(lines[cy][cx + 1])) continue;
                return;
            }
        }
    }
}

/* Jump to the bracket matching the one under the cursor. */
void bracket_match(void) {
            static const char open_b[] = "([{";
            static const char close_b[] = ")]}";
            int i, depth = 0, dir = 0;
            char me = 0, mate = 0;
            for (i = 0; i < 3; i++) {
                if (cx < llen[cy] && lines[cy][cx] == open_b[i]) {
                    me = open_b[i]; mate = close_b[i]; dir = 1; break;
                }
                if (cx < llen[cy] && lines[cy][cx] == close_b[i]) {
                    me = close_b[i]; mate = open_b[i]; dir = -1; break;
                }
            }
            if (me) {
                int y2 = cy, x2 = cx;
                for (;;) {
                    x2 += dir;
                    if (x2 < 0) {
                        if (y2 == 0) break;
                        y2--; x2 = llen[y2] - 1;
                        if (x2 < 0) x2 = 0;
                    } else if (x2 >= llen[y2]) {
                        if (y2 >= nlines - 1) break;
                        y2++; x2 = 0;
                    }
                    if (x2 < llen[y2]) {
                        char c2 = lines[y2][x2];
                        if (c2 == me) depth++;
                        else if (c2 == mate) {
                            if (depth == 0) { cy = y2; cx = x2; break; }
                            depth--;
                        }
                    }
                }
            }
        }
