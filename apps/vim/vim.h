/*
 * vim.h - shared state and API for the Onyx modal editor.
 * Split of the original single-file vim.c into modules; see README.md
 * for the full key-binding reference and the module map.
 */
#ifndef VIM_H
#define VIM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

/* ── Limits ────────────────────────────────────────────────────────── */
#define MAX_LINES   4096
#define MAX_LINE    1024
#define TAB_STOP    4

/* ── Key codes above 0xFF for special keys ─────────────────────────── */
#define K_UP      0x100
#define K_DOWN    0x101
#define K_LEFT    0x102
#define K_RIGHT   0x103
#define K_HOME    0x104
#define K_END     0x105
#define K_PGUP    0x106
#define K_PGDN    0x107
#define K_DEL     0x108
#define K_CTRL_F  0x06
#define K_CTRL_B  0x02
#define K_CTRL_D  0x04
#define K_CTRL_U  0x15
#define K_CTRL_R  0x12

/* ── Modes ─────────────────────────────────────────────────────────── */
#define M_NORMAL   0
#define M_INSERT   1
#define M_COMMAND  2
#define M_SEARCH   3
#define M_SEARCH_B 4
#define M_VISUAL   5
#define M_VISUALL  6

/* ── Registers / undo ──────────────────────────────────────────────── */
#define UNDO_MAX 128

#define UNDO_DEL_TEXT 0    /* text deleted at pos - undo re-inserts it */
#define UNDO_INS_TEXT 1    /* text inserted at pos - undo deletes it */
#define UNDO_DEL_LINE 2    /* whole line removed - undo re-inserts line */

typedef struct {
    int pos_line, pos_col;   /* where the change started */
    char deleted[MAX_LINE];  /* removed text (one line's worth per op granularity) */
    int deleted_len;
    int lines_removed;
    int kind;
} undo_t;

/* ── State (state.c) ───────────────────────────────────────────────── */
extern char lines[MAX_LINES][MAX_LINE];  /* text buffer */
extern int  llen[MAX_LINES];             /* length of each line */
extern int  nlines;                      /* number of lines */

extern int  cx, cy;                      /* cursor col/row */
extern int  top;                         /* first visible row */
extern int  rows, cols;                  /* terminal size */
extern int  mode;                        /* M_* */
extern int  dirty;                       /* buffer modified */
extern int  show_num;                    /* :set nu */
extern char fname[256];                  /* current file */
extern char cmdbuf[128];                 /* : command line */
extern int  cmdlen;
extern char msg[160];                    /* status message */
extern int  quit_flag;
extern int  last_search[128];
extern int  have_search;

/* Registers: linewise and charwise */
extern char reg_line[MAX_LINE];
extern int  reg_line_len;
extern char reg_block[MAX_LINES][MAX_LINE];
extern int  reg_block_len[MAX_LINES];
extern int  reg_block_n;
extern int  reg_linewise;

/* Visual selection */
extern int  vsy, vsx;                    /* selection origin */

/* Undo */
extern undo_t undos[UNDO_MAX];
extern int    nundos;
extern undo_t redos[UNDO_MAX];
extern int    nredos;

/* Pending operator state (for dd/dw/yy with counts) */
extern int  pending_op;                  /* 'd', 'c', 'y', 'g' */
extern int  count;
extern int  find_char;                   /* f/F/t/T pending */
extern int  find_mode;                   /* 'f','F','t','T' */
extern int  last_find_mode, last_find_char;
extern int  replace_pending;
extern int  g_pending;

/* repeat (.) */
extern char last_cmd_seq[16];
extern int  last_cmd_len;

/* terminal */
extern struct termios orig_tio;
extern int  raw_on;

/* ── Terminal (main.c) ─────────────────────────────────────────────── */
void raw_enable(void);
void raw_disable(void);
void query_size(void);

/* ── Buffer helpers / shared shifts (state.c, buffer.c) ────────────── */
void ensure_buffer(void);
void clamp_cursor(void);
void scroll_view(void);
int  make_room(int at);   /* open a slot at `at`; 0 = buffer full */
void remove_at(int row);  /* close the gap at `row` */

/* ── Undo / primitives (buffer.c) ──────────────────────────────────── */
void undo_record(int line, int col, const char *deleted,
                 int dlen, int lines_removed);
void undo_record_k(int line, int col, const char *deleted,
                   int dlen, int lines_removed, int kind);
void do_undo(void);
void do_redo(void);
void insert_char(int c);
void split_line(void);
void del_char_at(int row, int col);
void del_line(int row);

/* ── Motions / search (motion.c) ───────────────────────────────────── */
int  is_word(int c);
void motion_word_forward(void);
void motion_word_back(void);
void motion_word_end(void);
void bracket_match(void);   /* % - jump to matching bracket */
int  find_str(const char *pat, int from_line, int from_col,
              int *out_line, int *out_col);
int  do_find_char(int dir, int til, int c);
void find_pending_key(int k);   /* consume key while f/F/t/T is pending */

/* ── Registers / operators (ops.c) ─────────────────────────────────── */
void yank_line_range(int from, int to);
void yank_chars(int len);
void paste(int after);
void op_delete_line(void);
void op_yank_line(void);
void op_delete_word(void);
void op_delete_to_eol(void);
void op_delete_to_bol(void);
void indent_line(int dir);

/* ── Rendering (render.c) ──────────────────────────────────────────── */
int  gut_width(void);
void draw(void);
void place_cursor(void);
void set_msg(const char *m);

/* ── File I/O (fileio.c) ───────────────────────────────────────────── */
void load_file(const char *path);
int  save_to(const char *path);

/* ── Input (input.c) ───────────────────────────────────────────────── */
int  kbread(unsigned char *out);
int  read_key(void);

/* ── Command mode (command.c) ──────────────────────────────────────── */
void run_command(char *cmd);

/* ── Normal-mode dispatch (mode/) ──────────────────────────────────── */
void normal_key(int k);
/* key handlers return: 0 = not consumed, 1 = consumed (continue),
 * 2 = consumed (return immediately) */
int  key_motion(int k, int n);
int  key_edit(int k, int n);

#endif /* VIM_H */
