/* actions.c - clicks, keys, sending, ping start, slash commands. */
#include "gui.h"
#include <stdio.h>
#include <string.h>

void gui_ping(struct gui *g, long now) {
    const char *peer = gui_selected(g);
    if (!peer) { gui_set_status(g, "select a contact first"); return; }
    if (g->ping_state == PING_BUSY) return;
    if (g->be.ping_start(g->be.ctx, peer, g->ping_request) != 0) {
        g->ping_state = PING_FAIL;
        snprintf(g->last_error, sizeof(g->last_error), "%s", g->be.error(g->be.ctx));
        gui_set_status(g, g->last_error);
        return;
    }
    ochat_ping_sent(&g->pstats);
    g->ping_state = PING_BUSY;
    g->ping_deadline = now + GUI_PING_TIMEOUT_MS;
    g->next_ping_poll = now;
}

static void command(struct gui *g, const char *text, long now) {
    char addr[OCHAT_ADDR_SIZE];
    if (!strncmp(text, "/to ", 4)) {
        int r = ochat_resolve(g->devs, g->ndevs, text + 4, addr);
        if (r == 0) gui_select(g, gui_ensure(g, addr));
        else gui_set_status(g, r == -2 ? "alias is ambiguous" : "unknown address or alias");
    } else if (!strcmp(text, "/ping")) gui_ping(g, now);
    else gui_set_status(g, "commands: /to <dest>  /ping");
}

void gui_submit(struct gui *g, long now) {
    char clean[OCHAT_MAX_TEXT + 1];
    const char *peer = gui_selected(g);
    long n;
    static const char *why[] = { "", "text is not valid UTF-8", "text is too long", "nothing to send" };
    if (g->line.len == 0) return;
    if (g->line.buf[0] == '/') { command(g, g->line.buf, now); ochat_line_init(&g->line); return; }
    n = ochat_text_clean(g->line.buf, (size_t)g->line.len, clean, sizeof(clean), 1);
    if (n < 0) { gui_set_status(g, why[-n]); return; }
    if (!peer) { gui_set_status(g, "select a contact first"); return; }
    if (g->be.send(g->be.ctx, peer, clean) != 0) {      /* keep the text for a retry */
        snprintf(g->last_error, sizeof(g->last_error), "send: %s", g->be.error(g->be.ctx));
        gui_set_status(g, g->last_error);
        return;
    }
    ochat_hist_push(&g->hist, peer, 1, clean);
    ochat_line_init(&g->line);
    g->status[0] = 0;
}

void gui_action(struct gui *g, int action, long now) {
    if (action >= 1 && action <= GUI_ROWS) gui_select(g, g->scroll + action - 1);
    else if (action == ACT_UP && g->scroll > 0) g->scroll--;
    else if (action == ACT_DOWN && g->scroll + GUI_ROWS < g->ndevs) g->scroll++;
    else if (action == ACT_PING) gui_ping(g, now);
    else if (action == ACT_REFRESH) { gui_refresh_devices(g, now); gui_set_status(g, "devices refreshed"); }
    else if (action == ACT_SEND) gui_submit(g, now);
    gui_sync(g);
}

int gui_click(struct gui *g, int x, int y, long now) {
    int a = ow_pointer(&g->view, x, y);
    if (a) gui_action(g, a, now);
    return a;
}

int gui_key(struct gui *g, unsigned char key, long now) {
    int k;
    if (key == '\t') {                      /* Tab: next contact */
        if (g->ndevs) gui_select(g, (g->sel + 1) % g->ndevs);
        gui_sync(g);
        return 0;
    }
    k = ochat_line_feed(&g->line, key);
    if (k == OCHAT_KEY_SUBMIT) gui_submit(g, now);
    gui_sync(g);
    return k == OCHAT_KEY_CANCEL;
}
