/* shell.c - interactive line chat: raw terminal, incoming messages shown live. */
#include "cli.h"
#include <termios.h>
#include <unistd.h>

#define POLL_MS 300
#define SHOW_HISTORY 20

static struct termios saved_tio;
static int raw_on;
static struct ochat_history hist;
static struct ochat_device devs[OCHAT_MAX_DEVICES];
static int ndevs;
static char peer[OCHAT_ADDR_SIZE];
static struct ochat_line line;
static struct ochat_msg inbox[OCHAT_MAX_POLL];

static void raw(int on) {
    struct termios t;
    if (on && !raw_on) {
        if (tcgetattr(0, &saved_tio) != 0) return;
        t = saved_tio;
        cfmakeraw_apply(&t);
        t.c_cc[VMIN] = 0;
        t.c_cc[VTIME] = 0;
        tcsetattr(0, 0, &t);
        raw_on = 1;
    } else if (!on && raw_on) {
        tcsetattr(0, 0, &saved_tio);
        raw_on = 0;
    }
}

static void prompt(void) {
    printf("\r\x1b[K[%s]> %s", peer[0] ? peer : "no peer", line.buf);
    fflush(stdout);
}

/* Print one message line above the prompt, then redraw the prompt. */
static void note(const char *fmt, const char *a, const char *b) {
    printf("\r\x1b[K");
    printf(fmt, a, b);
    printf("\r\n");
    prompt();
}

static void who_label(const char *addr, char *out, size_t cap) {
    const char *alias = ochat_alias_of(devs, ndevs, addr);
    if (alias) snprintf(out, cap, "%s (%s)", addr, alias);
    else snprintf(out, cap, "%s", addr);
}

static void refresh_devices(void) {
    int n = ochat_link_devices(&g_link, devs, OCHAT_MAX_DEVICES);
    if (n >= 0) ndevs = n;
    else { g_link.error[0] = 0; }          /* aliases are optional */
}

static void do_who(void) {
    int i;
    refresh_devices();
    for (i = 0; i < ndevs; i++) {
        printf("\r\x1b[K%s  %-7s %s\r\n", devs[i].address, devs[i].online ? "online" : "offline",
               devs[i].alias[0] ? devs[i].alias : "-");
    }
    if (!ndevs) printf("\r\x1b[K(no devices listed)\r\n");
}

static void do_ping(const char *target) {
    char addr[OCHAT_ADDR_SIZE], buf[32];
    long rtt;
    int s;
    if (ochat_resolve(devs, ndevs, target, addr) != 0) { note("unknown device '%s'%s", target, ""); return; }
    s = cli_ping_once(addr, 3000, &rtt);
    if (s < 0) note("ping error: %s%s", g_link.error, "");
    else if (s == OCHAT_PING_DONE) { ochat_fmt_us(rtt, buf, sizeof(buf)); note("reply from %s: %s", addr, buf); }
    else if (s == OCHAT_PING_TIMEOUT) note("ping %s: timed out%s", addr, "");
    else note("ping %s: failed%s", addr, "");
}

static void do_history(void) {
    int i, n = ochat_hist_count(&hist), from = n > SHOW_HISTORY ? n - SHOW_HISTORY : 0;
    for (i = from; i < n; i++) {
        const struct ochat_entry *e = ochat_hist_get(&hist, i);
        printf("\r\x1b[K%s %s: %s\r\n", e->outgoing ? ">>" : "<<", e->peer, e->text);
    }
    if (!n) printf("\r\x1b[K(no history)\r\n");
}

static int do_send(const char *text) {
    if (!peer[0]) { note("no peer: use /to <address|alias>%s%s", "", ""); return 0; }
    if (ochat_link_send(&g_link, peer, text) != 0) { note("send failed: %s%s", g_link.error, ""); return g_link.broken ? -1 : 0; }
    ochat_hist_push(&hist, peer, 1, text);
    return 0;
}

/* Returns 1 to quit, -1 on a broken link. */
static int submit(void) {
    char *t = line.buf;
    int r = 0;
    while (*t == ' ') t++;
    if (!*t) { ochat_line_init(&line); return 0; }
    printf("\r\x1b[K[%s]> %s\r\n", peer[0] ? peer : "no peer", t);
    if (t[0] == '/') {
        if (!strcmp(t, "/quit") || !strcmp(t, "/q")) return 1;
        else if (!strcmp(t, "/who")) do_who();
        else if (!strcmp(t, "/history")) do_history();
        else if (!strncmp(t, "/to ", 4)) {
            char a[OCHAT_ADDR_SIZE];
            refresh_devices();
            if (ochat_resolve(devs, ndevs, t + 4, a) == 0) memcpy(peer, a, OCHAT_ADDR_SIZE);
            else printf("\r\x1b[Kunknown or ambiguous device '%s'\r\n", t + 4);
        } else if (!strncmp(t, "/ping", 5) && (t[5] == 0 || t[5] == ' ')) {
            const char *target = t[5] ? t + 6 : peer;
            if (*target) do_ping(target); else printf("\r\x1b[Kusage: /ping <address|alias>\r\n");
        } else printf("\r\x1b[K/to <dest>  /who  /ping [dest]  /history  /quit   (plain text sends)\r\n");
    } else r = do_send(t);
    ochat_line_init(&line);
    prompt();
    return r;
}

static void incoming(void) {
    int n = ochat_link_poll(&g_link, inbox, OCHAT_MAX_POLL), i;
    char who[64];
    static char last_error[256];
    if (n < 0) {                       /* report a persistent error only once */
        if (strcmp(last_error, g_link.error)) {
            strcpy(last_error, g_link.error);
            note("receive error: %s%s", g_link.error, "");
        }
        return;
    }
    last_error[0] = 0;
    for (i = 0; i < n; i++) {
        ochat_hist_push(&hist, inbox[i].from, 0, inbox[i].text);
        who_label(inbox[i].from, who, sizeof(who));
        note("%s: %s", who, inbox[i].text);
        if (!peer[0]) memcpy(peer, inbox[i].from, OCHAT_ADDR_SIZE);
    }
}

int cmd_shell(int argc, char **argv) {
    long next_poll = 0;
    int quit = 0;
    unsigned char c;
    if (cli_connect()) return 1;
    ochat_hist_init(&hist);
    ochat_line_init(&line);
    peer[0] = 0;
    refresh_devices();
    printf("ochat shell as %s. /to <dest> picks a peer, /help lists commands.\n", g_link.self);
    raw(1);
    prompt();
    while (!quit) {
        long n = read(0, &c, 1);
        if (n == 1) {
            int k = ochat_line_feed(&line, c);
            if (k == OCHAT_KEY_EDIT) prompt();
            else if (k == OCHAT_KEY_CANCEL) quit = 1;
            else if (k == OCHAT_KEY_SUBMIT) quit = submit();
            continue;
        }
        if (ochat_now_ms() >= next_poll) {
            incoming();
            next_poll = ochat_now_ms() + POLL_MS;
            if (g_link.broken) { note("link lost: %s%s", g_link.error, ""); quit = -1; }
        }
        ochat_sleep_ms(20);
    }
    raw(0);
    printf("\r\n");
    ochat_link_close(&g_link);
    return quit < 0 ? 1 : 0;
}
