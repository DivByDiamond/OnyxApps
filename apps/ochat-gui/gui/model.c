/* model.c - ochat-gui state: contacts, polling, ping progress, widget sync. */
#include "gui.h"
#include <stdio.h>
#include <string.h>

enum { B_TITLE = 1, B_PING = 2, B_CONTACT = 10 };

void gui_set_status(struct gui *g, const char *text) {
    size_t i;
    for (i = 0; text[i] && i < GUI_STATUS_MAX; i++)
        g->status[i] = (text[i] >= 32 && text[i] < 127) ? text[i] : '?';
    g->status[i] = 0;
}

int gui_find(const struct gui *g, const char *addr) {
    int i;
    for (i = 0; i < g->ndevs; i++) if (!strcmp(g->devs[i].address, addr)) return i;
    return -1;
}

int gui_ensure(struct gui *g, const char *addr) {
    int i = gui_find(g, addr);
    if (i >= 0 || g->ndevs >= OCHAT_MAX_DEVICES) return i;
    memset(&g->devs[g->ndevs], 0, sizeof(g->devs[0]));
    memcpy(g->devs[g->ndevs].address, addr, OCHAT_ADDR_SIZE);
    g->devs[g->ndevs].online = 1;
    g->unread[g->ndevs] = 0;
    return g->ndevs++;
}

const char *gui_selected(const struct gui *g) {
    return g->sel >= 0 && g->sel < g->ndevs ? g->devs[g->sel].address : NULL;
}

void gui_select(struct gui *g, int index) {
    if (index < 0 || index >= g->ndevs) return;
    g->sel = index;
    g->unread[index] = 0;
    g->status[0] = 0;
    g->ping_state = PING_IDLE;
    ochat_ping_init(&g->pstats);
    if (index < g->scroll) g->scroll = index;
    if (index >= g->scroll + GUI_ROWS) g->scroll = index - GUI_ROWS + 1;
}

static void report(struct gui *g, const char *prefix) {
    const char *e = g->be.error ? g->be.error(g->be.ctx) : "";
    char text[GUI_STATUS_MAX + 1];
    snprintf(text, sizeof(text), "%s: %s", prefix, e);
    if (strcmp(g->last_error, text)) {      /* show a persisting error only once */
        snprintf(g->last_error, sizeof(g->last_error), "%s", text);
        gui_set_status(g, text);
    }
}

void gui_refresh_devices(struct gui *g, long now) {
    struct ochat_device fresh[OCHAT_MAX_DEVICES];
    int unread[OCHAT_MAX_DEVICES], i, n, old;
    char keep[OCHAT_ADDR_SIZE];
    const char *cur = gui_selected(g);
    g->next_devices = now + GUI_DEVICES_MS;
    n = g->be.devices(g->be.ctx, fresh, OCHAT_MAX_DEVICES);
    if (n < 0) { report(g, "devices"); gui_sync(g); return; }
    keep[0] = 0;
    if (cur) memcpy(keep, cur, OCHAT_ADDR_SIZE);
    for (i = 0; i < n; i++) {
        old = gui_find(g, fresh[i].address);
        unread[i] = old >= 0 ? g->unread[old] : 0;
    }
    memcpy(g->devs, fresh, sizeof(fresh[0]) * (size_t)n);
    memcpy(g->unread, unread, sizeof(unread[0]) * (size_t)n);
    g->ndevs = n;
    g->sel = keep[0] ? gui_ensure(g, keep) : -1;     /* selection survives a refresh */
    if (!keep[0])                                    /* first selection: first other device */
        for (i = 0; i < n && g->sel < 0; i++)
            if (strcmp(g->devs[i].address, g->self)) g->sel = i;
    if (g->scroll > g->ndevs - GUI_ROWS) g->scroll = g->ndevs > GUI_ROWS ? g->ndevs - GUI_ROWS : 0;
    g->last_error[0] = 0;
    gui_sync(g);
}

static void poll_messages(struct gui *g) {
    struct ochat_msg in[OCHAT_MAX_POLL];
    int n = g->be.poll(g->be.ctx, in, OCHAT_MAX_POLL), i, idx;
    if (n < 0) { report(g, "receive"); return; }
    for (i = 0; i < n; i++) {
        ochat_hist_push(&g->hist, in[i].from, 0, in[i].text);
        idx = gui_ensure(g, in[i].from);
        if (idx < 0) continue;
        if (g->sel < 0) gui_select(g, idx);
        else if (idx != g->sel && g->unread[idx] < 99) g->unread[idx]++;
    }
}

static void ping_progress(struct gui *g, long now) {
    struct ochat_ping_event ev;
    int r;
    if (g->ping_state != PING_BUSY || now < g->next_ping_poll) return;
    g->next_ping_poll = now + 100;
    r = g->be.ping_poll(g->be.ctx, g->ping_request, &ev);
    if (r < 0) { g->ping_state = PING_FAIL; report(g, "ping"); }
    else if (r == 1) {
        if (ev.state == OCHAT_PING_DONE) { g->ping_state = PING_OK; g->ping_us = ev.rtt_us;
            ochat_ping_reply(&g->pstats, ev.rtt_us); }
        else g->ping_state = ev.state == OCHAT_PING_TIMEOUT ? PING_LOST : PING_FAIL;
    } else if (now >= g->ping_deadline) g->ping_state = PING_LOST;
}

void gui_tick(struct gui *g, long now) {
    if (now >= g->next_poll) { g->next_poll = now + GUI_POLL_MS; poll_messages(g); }
    if (now >= g->next_devices) gui_refresh_devices(g, now);
    ping_progress(g, now);
    gui_sync(g);
}

static void ping_label(const struct gui *g, char *out, size_t cap) {
    static const char *names[] = { "PING --", "PING ..", "", "PING LOST", "PING FAIL" };
    if (g->ping_state == PING_OK)
        snprintf(out, cap, "PING %ld.%ldMS %d%%", g->ping_us / 1000, (g->ping_us % 1000) / 100,
                 ochat_ping_loss_percent(&g->pstats));
    else snprintf(out, cap, "%s", names[g->ping_state]);
}

void gui_sync(struct gui *g) {
    char text[OW_TEXT], name[OCHAT_ALIAS_MAX + 1];
    int i;
    snprintf(text, sizeof(text), "ME %s", g->self[0] ? g->self : "?");
    ow_update(&g->view, B_TITLE, text, 0);
    ping_label(g, text, sizeof(text));
    ow_update(&g->view, B_PING, text, 0);
    for (i = 0; i < GUI_ROWS; i++) {
        int k = g->scroll + i;
        if (k < g->ndevs) {
            const struct ochat_device *d = &g->devs[k];
            snprintf(name, sizeof(name), "%s", d->alias[0] ? d->alias : d->address);
            name[9] = 0;                              /* 72 px button, 2 marker columns */
            snprintf(text, sizeof(text), "%c%c%s", k == g->sel ? '>' : (g->unread[k] ? '!' : ' '),
                     d->online ? '*' : '-', name);
        } else text[0] = 0;
        ow_update(&g->view, B_CONTACT + i, text, 0);
    }
}

void gui_init(struct gui *g, const struct gui_backend *be, const char *self) {
    int i;
    memset(g, 0, sizeof(*g));
    g->be = *be;
    g->sel = -1;
    snprintf(g->self, sizeof(g->self), "%s", self ? self : "");
    ochat_hist_init(&g->hist);
    ochat_line_init(&g->line);
    ochat_ping_init(&g->pstats);
    ow_init(&g->view, GUI_W, GUI_H);
    ow_add(&g->view, OW_LABEL, 0, 1, 130, 9, B_TITLE, 0, "");
    ow_add(&g->view, OW_LABEL, 146, 1, 94, 9, B_PING, 0, "");
    for (i = 0; i < GUI_ROWS; i++)
        ow_add(&g->view, OW_BUTTON, 0, 11 + i * 11, 72, 10, B_CONTACT + i, 1 + i, "");
    ow_add(&g->view, OW_BUTTON, 0, 78, 35, 10, 30, ACT_UP, "UP");
    ow_add(&g->view, OW_BUTTON, 37, 78, 35, 10, 31, ACT_DOWN, "DOWN");
    ow_add(&g->view, OW_BUTTON, 0, 90, 35, 10, 32, ACT_PING, "PING");
    ow_add(&g->view, OW_BUTTON, 37, 90, 35, 10, 33, ACT_REFRESH, "REFR");
    ow_add(&g->view, OW_BUTTON, 202, 102, 38, 10, 34, ACT_SEND, "SEND");
    gui_sync(g);
}
