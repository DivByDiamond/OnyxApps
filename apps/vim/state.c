/* state.c — editor state: buffer, cursor, registers, undo, pending ops. */
#include "vim.h"
char lines[MAX_LINES][MAX_LINE];
int llen[MAX_LINES];
int nlines = 0;

int cx = 0, cy = 0;              /* cursor col/row */
int top = 0;                     /* first visible row */
int rows = 24, cols = 80;
int mode = M_NORMAL;
int dirty = 0;
int show_num = 1;                /* :set nu default on */
char fname[256] = "";
char cmdbuf[128];
int cmdlen = 0;
char msg[160] = "";
int quit_flag = 0;
int last_search[128];
int have_search = 0;

/* Registers: linewise and charwise */
char reg_line[MAX_LINE];
int reg_line_len = 0;
char reg_block[MAX_LINES][MAX_LINE];
int reg_block_len[MAX_LINES];
int reg_block_n = 0;
int reg_linewise = 0;

/* Visual selection */
int vsy = 0, vsx = 0;            /* selection origin */

/* Undo */
undo_t undos[UNDO_MAX];
int nundos = 0;
undo_t redos[UNDO_MAX];
int nredos = 0;

/* Pending operator state (for dd/dw/yy with counts) */
int pending_op = 0;              /* 'd', 'c', 'y', 'g' */
int count = 0;
int find_char = 0;               /* f/F/t/T pending */
int find_mode = 0;               /* 'f','F','t','T' */
int last_find_mode = 0, last_find_char = 0;
int replace_pending = 0;
int g_pending = 0;

/* repeat (.) */
char last_cmd_seq[16];
int last_cmd_len = 0;

struct termios orig_tio;
int raw_on = 0;

/* ── Buffer helpers ────────────────────────────────────────────────── */
void ensure_buffer(void) {
    if (nlines == 0) {
        nlines = 1;
        llen[0] = 0;
        lines[0][0] = 0;
    }
}

void clamp_cursor(void) {
    if (cy >= nlines) cy = nlines - 1;
    if (cy < 0) cy = 0;
    if (cx > llen[cy]) cx = llen[cy];
    if (cx < 0) cx = 0;
}

void scroll_view(void) {
    int vis = rows - 2;   /* status + command lines */
    if (vis < 1) vis = 1;
    if (cy < top) top = cy;
    if (cy >= top + vis) top = cy - vis + 1;
    if (top < 0) top = 0;
}
