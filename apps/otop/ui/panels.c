/*
 * panels.c - frame layout for otop: the Memory panel (real /proc
 * meminfo numbers), the Load panel (processes, harts, uptime) and the
 * Heap history panel with an ASCII sparkline.
 *
 * Screen map (default 24 rows): header 1, Memory 3..9, Load 11..15,
 * History 17..22, footer 23/24.
 */
#include "otop.h"

/* Format a kB pair as "cur / max kB" or "(n/a)" when absent. */
static void fmt_pair(char *out, int cap, long cur, long total) {
    if (total > 0) {
        snprintf(out, cap, "%ld / %ld kB", cur, total);
    } else {
        snprintf(out, cap, "(n/a)");
    }
}

void panel_memory(int row, int width, struct otop_info *oi) {
    char suf[48];

    draw_box(row, 1, row + 6, width, "Memory - /proc/meminfo");

    fmt_pair(suf, sizeof(suf), oi->mem_used_kb, oi->mem_total_kb);
    draw_bar(row + 1, "ram", permille(oi->mem_used_kb, oi->mem_total_kb), suf);

    fmt_pair(suf, sizeof(suf), oi->heap_used_kb, oi->heap_total_kb);
    draw_bar(row + 2, "heap", permille(oi->heap_used_kb, oi->heap_total_kb), suf);

    fmt_pair(suf, sizeof(suf), oi->heap_self_kb, oi->heap_total_kb);
    draw_bar(row + 3, "self", permille(oi->heap_self_kb, oi->heap_total_kb), suf);

    if (oi->mem_total_kb > 0) {
        printf("\x1b[%d;3H\x1b[90mfree ram: %ld kB   heap free: %ld kB\x1b[0m",
               row + 5, oi->mem_free_kb, oi->heap_free_kb);
    } else {
        printf("\x1b[%d;3H\x1b[90m/proc/meminfo not available\x1b[0m", row + 5);
    }
}

void panel_load(int row, int width, struct otop_info *oi) {
    char up[16];

    draw_box(row, 1, row + 4, width, "Load - /proc/load");
    fmt_uptime(oi->uptime_s, up, sizeof(up));

    printf("\x1b[%d;3Hprocesses: %ld   running: %ld   blocked: %ld",
           row + 1, oi->procs, oi->running, oi->blocked);
    printf("\x1b[%d;3Hharts (cpus): %ld%s", row + 2, oi->harts,
           oi->harts > 0 ? "" : "   (cpuinfo n/a)");
    printf("\x1b[%d;3Huptime: %s", row + 3,
           oi->uptime_s > 0 ? up : "(n/a)");
}

void panel_history(int row, int width, struct otop_hist *h) {
    char spark[OTOP_HIST + 1];
    int i;
    int vmin, vmax;

    draw_box(row, 1, row + 5, width, "Heap history - kB");
    if (h->n == 0) {
        printf("\x1b[%d;3H\x1b[90mcollecting samples...\x1b[0m", row + 2);
        return;
    }

    spark_line(h, spark, width - 4);
    printf("\x1b[%d;3H\x1b[36m%s\x1b[0m", row + 2, spark);

    vmin = h->v[0];
    vmax = h->v[0];
    for (i = 1; i < h->n; i++) {
        if (h->v[i] < vmin) vmin = h->v[i];
        if (h->v[i] > vmax) vmax = h->v[i];
    }
    printf("\x1b[%d;3Hcur: %d kB   min: %d   max: %d   samples: %d/%d",
           row + 4, h->v[h->n - 1], vmin, vmax, h->n, OTOP_HIST);
}
