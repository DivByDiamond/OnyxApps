/*
 * oed.h - shared state and module API for the OnyxOS text editor (oed).
 * Split of the original single-file oed.c into modules; see README.md
 * for the project structure.
 */
#ifndef OED_H
#define OED_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

/* Limits */
#define OED_MAX_LINES   2048
#define OED_MAX_LINE    512
#define TAB_WIDTH       4

/* Key codes above 0xFF for special keys */
#define KEY_UP      0x100
#define KEY_DOWN    0x101
#define KEY_LEFT    0x102
#define KEY_RIGHT   0x103
#define KEY_HOME    0x104
#define KEY_END     0x105
#define KEY_PGUP    0x106
#define KEY_PGDN    0x107
#define KEY_DEL     0x108

/* State (edit.c) */
extern char lines[OED_MAX_LINES][OED_MAX_LINE];  /* text buffer */
extern int  line_len[OED_MAX_LINES];             /* length of each line */
extern int  nlines;                              /* number of lines */

extern int  cx, cy;                              /* cursor col/row (text space) */
extern int  dirty;                               /* buffer modified */
extern char filename[256];                       /* current file */
extern char status_msg[128];                     /* status bar message */

/* State (screen.c) */
extern int  screen_rows, screen_cols;            /* terminal size */

/* Terminal (screen.c) */
void term_raw_on(void);
void term_raw_off(void);
void get_size(void);

/* Screen drawing (screen.c) */
void draw_all(void);
void set_status(const char *msg);
void place_cursor(void);
void scroll_into_view(void);
void show_help(void);

/* Editing and file I/O (edit.c) */
void insert_char(int c);
void insert_newline(void);
void backspace(void);
void delete_key(void);
void load_file(const char *path);
int  save_file(void);

/* Input (main.c) */
int  read_key(void);

#endif /* OED_H */
