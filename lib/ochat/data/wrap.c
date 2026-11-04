/* wrap.c - wrap text to fixed columns, preferring breaks at spaces. */
#include "../chat.h"

int ochat_wrap(const char *text, int cols, int *start, int *len, int max_lines) {
    int pos = 0, n = 0, total;
    if (!text || cols < 1 || max_lines < 1 || !start || !len) return 0;
    for (total = 0; text[total]; total++) {}
    while (pos < total && n < max_lines) {
        int end = pos + cols, brk;
        if (end >= total) end = total;
        else if (text[end] != ' ') {
            for (brk = end; brk > pos && text[brk - 1] != ' '; brk--) {}
            if (brk > pos) end = brk;
        }
        start[n] = pos;
        len[n] = end - pos;
        /* drop the trailing blanks of a soft-wrapped line */
        while (len[n] > 1 && text[pos + len[n] - 1] == ' ' && end < total) len[n]--;
        n++;
        pos = end;
        while (pos < total && text[pos] == ' ' && pos > 0 && end < total) pos++;
    }
    return n;
}
