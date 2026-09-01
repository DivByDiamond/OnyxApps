/*
 * otop.h - shared types and module API for the btop-style monitor.
 *
 * main.c        - entry: args, refresh loop, keys (q, space, +/-)
 * probe/data.c  - syscall probes: /proc files, uname, own heap
 * probe/parse.c - pure parsers and formatting helpers (host-testable)
 * ui/draw.c     - box/bar/header/footer drawing primitives
 * ui/panels.c   - frame layout: memory, load, history panels
 */
#ifndef OTOP_H
#define OTOP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

/* Sample history for the heap sparkline. */
#define OTOP_HIST 48

/* Load bar width in characters. */
#define OTOP_BAR_W 30

/* utsname (kernel fills 5x65 fields, Linux layout). */
struct utsname_l {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
};

/* One system snapshot; missing data stays 0 and renders as "(n/a)". */
struct otop_info {
    long mem_total_kb, mem_free_kb, mem_used_kb;
    long heap_total_kb, heap_used_kb, heap_free_kb;
    long procs, running, blocked;
    long harts;
    long uptime_s;
    long heap_self_kb; /* own heap growth via sbrk(0) */
    struct utsname_l un;
};

struct otop_hist {
    int v[OTOP_HIST];
    int n;
};

/* probe/parse.c - pure, covered by tests/native/test_otop. */
int parse_num(const char *buf, const char *key, long *out);
long parse_uptime_s(const char *buf);
int permille(long used, long total);
void fmt_uptime(long s, char *out, int cap);
void hist_push(struct otop_hist *h, int val);
char spark_ch(int level_permille);
void spark_line(struct otop_hist *h, char *out, int width);

/* probe/data.c - syscall-backed probes. */
void probe_collect(struct otop_info *oi);
long heap_self_kb(void);

/* ui/draw.c */
void draw_box(int r1, int c1, int r2, int c2, const char *title);
void draw_bar(int row, const char *label, int permille_val, const char *suffix);
void draw_header(struct otop_info *oi, int frame);
void draw_footer(int row, const char *text);

/* ui/panels.c */
void panel_memory(int row, int width, struct otop_info *oi);
void panel_load(int row, int width, struct otop_info *oi);
void panel_history(int row, int width, struct otop_hist *h);

/* Terminal control (main.c). */
void term_raw_on(void);
void term_raw_off(void);
void get_size(void);
int key_poll(void);
void nap_ms(int ms);

extern int screen_rows;
extern int screen_cols;

#endif
