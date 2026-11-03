#include "dashboard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Local deterministic fixture only: never opens a network or radio socket. */
static int running = 1;
static int fetch(void *ctx, struct dashboard_sample *s) {
    s->energy = running ? 72 : 18;
    strcpy(s->machine, running ? "MACHINE RUNNING" : "MACHINE STOPPED");
    return 1;
}
static int action(void *ctx, int id, unsigned long request) {
    if (id != 1) return 0;
    running = !running;
    return 2;
}
int main(int argc, char **argv) {
    struct ow_view view;
    struct dashboard_backend backend;
    const char *path = argc > 1 ? argv[1] : "tablet.ppm";
    backend.ctx = 0; backend.fetch = fetch; backend.action = action;
    dashboard_init(&view);
    dashboard_refresh(&view, &backend, 1);
    if (argc > 3 && strcmp(argv[2], "--interactive")) {
        dashboard_click(&view, &backend, atoi(argv[2]), atoi(argv[3]), 1);
        dashboard_refresh(&view, &backend, 2);
    }
    if (argc > 4 && !strcmp(argv[4], "offline")) ow_tick(&view, 20, 5);
    if (argc > 2 && !strcmp(argv[2], "--interactive")) {
        char line[80];
        unsigned long request = 1;
        int x, y;
        for (;;) {
            if (!demo_snapshot(&view, path)) return 1;
            printf("READY\n");
            fflush(stdout);
            if (!fgets(line, sizeof(line), stdin)) break;
            if (!strcmp(line, "offline\n")) ow_tick(&view, request + 20, 5);
            else if (!strcmp(line, "refresh\n")) dashboard_refresh(&view, &backend, request);
            else if (sscanf(line, "%d %d", &x, &y) == 2) {
                dashboard_click(&view, &backend, x, y, request);
                if (view.online) dashboard_refresh(&view, &backend, request);
            }
            request++;
        }
        return 0;
    }
    if (!demo_snapshot(&view, path)) {
        printf("otablet: cannot write snapshot\n");
        return 1;
    }
    printf("MOCK dashboard: %s; pointer: otablet output.ppm x y [offline]\n", path);
    return 0;
}
