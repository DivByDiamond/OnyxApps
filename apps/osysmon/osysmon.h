/*
 * osysmon.h - shared types and module API for the OnyxOS system monitor.
 *
 * osysmon.c  - program entry: terminal control, args, main refresh loop
 * monitor.c  - data probes via libonyxc syscalls (uname, uptime, heap,
 *              readdir) and the box/bar drawing primitives
 */
#ifndef OSYSMON_H
#define OSYSMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

/* Bar width in characters for the load bars. */
#define BAR_W 40

/* Screen grid, filled by get_size() and read by the layout code. */
extern int screen_rows;
extern int screen_cols;

/* utsname (kernel fills 5x65 fields, Linux layout). */
struct utsname_l {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
};

/* Terminal: raw mode, quit-key poll, framebuffer size (osysmon.c). */
void term_raw_on(void);
void term_raw_off(void);
int  kbhit(void);
void get_size(void);

/* Data collection via syscalls (monitor.c). */
void sysinfo_get(struct utsname_l *un);
long uptime_ms(void);
long heap_usage_kb(void);
int  count_dir(const char *path);

/* Drawing primitives (monitor.c). */
void bar(int row, const char *label, int permille, const char *suffix);
void box(int r1, int c1, int r2, int c2, const char *title);

#endif
