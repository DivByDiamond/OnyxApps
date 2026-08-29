/*
 * osysmon.c - OnyxOS system monitor (btop/htop-inspired).
 *
 * Program entry: terminal raw mode, framebuffer size probe and the
 * periodic refresh loop that lays out the panes. Data probes and the
 * drawing primitives live in monitor.c (see osysmon.h).
 *
 * Usage: osysmon [interval-ms]
 */
#include "osysmon.h"

int screen_rows = 24, screen_cols = 80;
static struct termios orig_termios;
static int raw_mode_on = 0;

void term_raw_on(void) {
    struct termios t;
    tcgetattr(0, &orig_termios);
    t = orig_termios;
    cfmakeraw_apply(&t);
    tcsetattr(0, 0, &t);
    raw_mode_on = 1;
}

void term_raw_off(void) {
    if (raw_mode_on) {
        tcsetattr(0, 0, &orig_termios);
        raw_mode_on = 0;
    }
}

int kbhit(void) {
    unsigned char c;
    long n = read(0, &c, 1);
    if (n <= 0) return 0;
    return (c == 'q' || c == 'Q' || c == 0x11) ? 1 : 0;
}

void get_size(void) {
    unsigned short ws[4] = {24, 80, 0, 0};
    if (_onyx_ioctl(0, 0x5413, (long)ws) == 0 && ws[0] > 2 && ws[1] > 8) {
        screen_rows = ws[0];
        screen_cols = ws[1];
    }
}

int main(int argc, char **argv) {
    int interval_ms = 1000;
    if (argc > 1) {
        interval_ms = atoi(argv[1]);
        if (interval_ms < 100) interval_ms = 100;
    }

    get_size();
    term_raw_on();

    struct utsname_l un;
    sysinfo_get(&un);

    long pid = getpid();
    char cwd[256] = "?";
    getcwd(cwd, sizeof(cwd));

    long boot_ms = uptime_ms();

    int frame = 0;
    char spin[] = "|/-\\";

    for (;;) {
        frame++;

        /* Header. */
        printf("\x1b[2J\x1b[1;1H");
        printf("\x1b[1;7m osysmon — OnyxOS system monitor \x1b[0m\r\n");

        int w = screen_cols < 78 ? screen_cols : 78;

        /* System pane. */
        box(3, 1, 9, w, "System");
        printf("\x1b[4;3Host: %s", un.nodename[0] ? un.nodename : "onyx");
        printf("\x1b[5;3HOS: %s %s (%s)",
               un.sysname[0] ? un.sysname : "OnyxOS",
               un.release[0] ? un.release : "",
               un.machine[0] ? un.machine : "riscv64");
        long up_s = uptime_ms() / 1000;
        printf("\x1b[6;3HUptime: %02ld:%02ld:%02ld",
               up_s / 3600, (up_s / 60) % 60, up_s % 60);
        printf("\x1b[7;3HPID: %ld   CWD: %s", pid, cwd);
        printf("\x1b[8;3HRefresh: %dms   Frame: %d %c",
               interval_ms, frame, spin[frame & 3]);
        (void)boot_ms;

        /* Memory pane (synthetic demo bars from heap usage). */
        box(11, 1, 16, w, "Memory");
        long heap_kb = heap_usage_kb();
        char mem1[48], mem2[48], mem3[48];
        snprintf(mem1, sizeof(mem1), "%6ld KiB", heap_kb);
        snprintf(mem2, sizeof(mem2), "%6ld KiB", heap_kb / 2 + 16);
        snprintf(mem3, sizeof(mem3), "%6ld KiB", heap_kb / 4 + 8);
        bar(12, "heap", (int)(heap_kb * 4), mem1);
        bar(13, "data", 320, mem2);
        bar(14, "bss ", 150, mem3);
        bar(15, "sys ", 240, "  96 MiB");

        /* CPU pane. */
        box(18, 1, 22, w, "CPU");
        /* Fake-but-moving load derived from frame parity: shows the UI
         * is alive (real per-CPU accounting arrives with SMP counters). */
        int load = 150 + (frame * 37) % 600;
        char cpu1[48];
        snprintf(cpu1, sizeof(cpu1), "%3d.%d%%", load / 10, load % 10);
        bar(19, "cpu0", load, cpu1);
        bar(20, "cpu1", (load * 3) % 1000, "");
        bar(21, "cpu2", (load * 7) % 1000, "");

        /* Disk / FS pane (right column if wide enough). */
        if (screen_cols >= 80) {
            box(3, w + 2, 16, screen_cols, "Disk");
            int files = count_dir(".");
            printf("\x1b[5;%dHCWD entries: %d", w + 4, files);
            struct stat st;
            if (stat("/", &st) == 0) {
                printf("\x1b[7;%dHRoot size: %ld KiB", w + 4,
                       (long)(st.st_size / 1024));
            } else {
                printf("\x1b[7;%dHRoot: (stat n/a)", w + 4);
            }
            printf("\x1b[9;%dHFS: OnyxFS v2", w + 4);
            printf("\x1b[11;%dHTools:", w + 4);
            printf("\x1b[12;%dH oed  editor", w + 4);
            printf("\x1b[13;%dH osh  shell", w + 4);
            printf("\x1b[14;%dH onyxcc cc", w + 4);

            box(18, w + 2, 22, screen_cols, "Net");
            printf("\x1b[19;%dHeth0: link n/a", w + 4);
            printf("\x1b[20;%dHtcp:  echo ready", w + 4);
            printf("\x1b[21;%dHudp:  -", w + 4);
        }

        /* Footer. */
        printf("\x1b[%d;1H\x1b[7m q: quit   r: refresh now \x1b[0m",
               screen_rows - 1);
        fflush(stdout);

        /* Sleep, checking for 'q' in small chunks. */
        int quit = 0;
        for (int i = 0; i < interval_ms / 100; i++) {
            struct timespec req = {0, 100 * 1000000};
            nanosleep(&req, NULL);
            if (kbhit()) {
                quit = 1;
                break;
            }
        }
        if (quit) break;
    }

    printf("\x1b[2J\x1b[1;1H\x1b[?25h");
    fflush(stdout);
    term_raw_off();
    printf("osysmon: bye\r\n");
    return 0;
}
