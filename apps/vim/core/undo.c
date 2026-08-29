/* undo.c — change recording, undo and redo application. */
#include "vim.h"

/* ── Undo ──────────────────────────────────────────────────────────── */
void undo_record(int line, int col, const char *deleted,
                        int dlen, int lines_removed) {
    undo_record_k(line, col, deleted, dlen, lines_removed,
                  lines_removed ? UNDO_DEL_LINE : UNDO_DEL_TEXT);
}

void undo_record_k(int line, int col, const char *deleted,
                          int dlen, int lines_removed, int kind) {
    undo_t *u;
    if (nundos >= UNDO_MAX) {
        int i;
        for (i = 0; i < UNDO_MAX - 1; i++) undos[i] = undos[i + 1];
        nundos = UNDO_MAX - 1;
    }
    u = &undos[nundos++];
    u->pos_line = line;
    u->pos_col = col;
    u->deleted_len = dlen < MAX_LINE ? dlen : MAX_LINE - 1;
    if (dlen > 0 && deleted) memcpy(u->deleted, deleted, u->deleted_len);
    u->deleted[u->deleted_len] = 0;
    u->lines_removed = lines_removed;
    u->kind = kind;
}


/* Undo the last recorded change. */
void do_undo(void) {
    if (nundos > 0) {
        undo_t *u = &undos[--nundos];
        if (nredos < UNDO_MAX) {
            memcpy(&redos[nredos], u, sizeof(undo_t));
            nredos++;
        }
        if (u->kind == UNDO_DEL_LINE) {
            if (nlines < MAX_LINES) {
                make_room(u->pos_line);
                memcpy(lines[u->pos_line], u->deleted, u->deleted_len);
                lines[u->pos_line][u->deleted_len] = 0;
                llen[u->pos_line] = u->deleted_len;
                nlines++;
            }
        } else if (u->kind == UNDO_DEL_TEXT) {
            /* re-insert the deleted text */
            int len = llen[u->pos_line];
            int i;
            for (i = len; i >= u->pos_col; i--)
                if (i + u->deleted_len < MAX_LINE)
                    lines[u->pos_line][i + u->deleted_len] =
                        lines[u->pos_line][i];
            for (i = 0; i < u->deleted_len; i++)
                lines[u->pos_line][u->pos_col + i] = u->deleted[i];
            llen[u->pos_line] += u->deleted_len;
        } else {
            /* UNDO_INS_TEXT: delete the inserted chars */
            int i, len = llen[u->pos_line];
            for (i = u->pos_col; i + u->deleted_len < len; i++)
                lines[u->pos_line][i] =
                    lines[u->pos_line][i + u->deleted_len];
            llen[u->pos_line] -= u->deleted_len;
            if (llen[u->pos_line] < 0) llen[u->pos_line] = 0;
            lines[u->pos_line][llen[u->pos_line]] = 0;
        }
        cy = u->pos_line;
        cx = u->pos_col;
        set_msg("Undo");
    } else {
        set_msg("Already at oldest change");
    }
}

/* Redo the last undone change. */
void do_redo(void) {
    if (nredos > 0) {
        undo_t *u = &redos[--nredos];
        if (nundos < UNDO_MAX) {
            memcpy(&undos[nundos], u, sizeof(undo_t));
            nundos++;
        }
        if (u->kind == UNDO_DEL_LINE) {
            remove_at(u->pos_line);
        } else if (u->kind == UNDO_DEL_TEXT) {
            /* re-delete the text */
            int i, len = llen[u->pos_line], dl = u->deleted_len;
            for (i = u->pos_col; i + dl < len; i++)
                lines[u->pos_line][i] = lines[u->pos_line][i + dl];
            llen[u->pos_line] -= dl;
            if (llen[u->pos_line] < 0) llen[u->pos_line] = 0;
            lines[u->pos_line][llen[u->pos_line]] = 0;
        } else {
            /* UNDO_INS_TEXT: re-insert */
            int len = llen[u->pos_line];
            int i;
            for (i = len; i >= u->pos_col; i--)
                if (i + u->deleted_len < MAX_LINE)
                    lines[u->pos_line][i + u->deleted_len] =
                        lines[u->pos_line][i];
            for (i = 0; i < u->deleted_len; i++)
                lines[u->pos_line][u->pos_col + i] = u->deleted[i];
            llen[u->pos_line] += u->deleted_len;
        }
        cy = u->pos_line;
        cx = u->pos_col;
        set_msg("Redo");
    } else {
        set_msg("Already at newest change");
    }
}
