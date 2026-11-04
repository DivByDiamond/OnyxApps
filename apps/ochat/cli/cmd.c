/* cmd.c - one-shot ochat commands. */
#include "cli.h"
#include <errno.h>

#define PING_TIMEOUT_MS 3000
#define PING_MAX_COUNT 100

struct ochat_link g_link;
static struct ochat_device devs[OCHAT_MAX_DEVICES];
static struct ochat_msg inbox[OCHAT_MAX_POLL];

int cli_connect(void) {
    if (ochat_link_open(&g_link) != 0) {
        printf("ochat: %s\n", g_link.error);
        return 1;
    }
    return 0;
}

int cli_resolve(const char *target, char out[OCHAT_ADDR_SIZE]) {
    int n = 0, r;
    if (ochat_addr_normalize(target, out) == 0) return 0;
    if (!ochat_alias_valid(target)) {
        printf("ochat: '%s' is neither an address (XXXX:XXXX) nor a valid alias\n", target);
        return 1;
    }
    n = ochat_link_devices(&g_link, devs, OCHAT_MAX_DEVICES);
    if (n < 0) { printf("ochat: cannot look up alias: %s\n", g_link.error); return 1; }
    r = ochat_resolve(devs, n, target, out);
    if (r == -2) printf("ochat: alias '%s' is ambiguous; use the address\n", target);
    else if (r) printf("ochat: unknown device '%s' (see: ochat who)\n", target);
    return r != 0;
}

int cmd_send(int argc, char **argv) {
    char text[OCHAT_MAX_TEXT * 2 + 2], addr[OCHAT_ADDR_SIZE];
    size_t used = 0;
    int i;
    if (argc < 4) { printf("usage: ochat send <address|alias> <text>\n"); return 2; }
    for (i = 3; i < argc; i++) {
        size_t n = strlen(argv[i]);
        if (used + n + 2 > sizeof(text)) { printf("ochat: text exceeds 1024 bytes\n"); return 1; }
        if (i > 3) text[used++] = ' ';
        memcpy(text + used, argv[i], n);
        used += n;
    }
    text[used] = 0;
    if (cli_connect()) return 1;
    if (cli_resolve(argv[2], addr) || ochat_link_send(&g_link, addr, text) != 0) {
        if (g_link.error[0]) printf("ochat: %s\n", g_link.error);
        ochat_link_close(&g_link);
        return 1;
    }
    printf("sent to %s\n", addr);
    ochat_link_close(&g_link);
    return 0;
}

static void show_inbox(int n) {
    int i;
    for (i = 0; i < n; i++) {
        const char *alias = ochat_alias_of(devs, OCHAT_MAX_DEVICES, inbox[i].from);
        printf("%s%s%s%s: %s\n", inbox[i].from, alias ? " (" : "", alias ? alias : "",
               alias ? ")" : "", inbox[i].text);
    }
    fflush(stdout);
}

int cmd_recv(int argc, char **argv) {
    int follow = 0, n, got = 0;
    if (argc == 3 && !strcmp(argv[2], "--follow")) follow = 1;
    else if (argc != 2) { printf("usage: ochat recv [--follow]\n"); return 2; }
    if (cli_connect()) return 1;
    memset(devs, 0, sizeof(devs));
    /* Aliases are cosmetic; a missing device list must not stop receiving. */
    if (ochat_link_devices(&g_link, devs, OCHAT_MAX_DEVICES) < 0) g_link.error[0] = 0;
    for (;;) {
        n = ochat_link_poll(&g_link, inbox, OCHAT_MAX_POLL);
        if (n < 0) { printf("ochat: %s\n", g_link.error); ochat_link_close(&g_link); return 1; }
        show_inbox(n);
        got += n;
        if (!follow && n < OCHAT_MAX_POLL) break;
        if (follow) ochat_sleep_ms(250);
    }
    if (!follow && got == 0) printf("no new messages\n");
    ochat_link_close(&g_link);
    return 0;
}

int cmd_who(int argc, char **argv) {
    int n, i;
    if (cli_connect()) return 1;
    n = ochat_link_devices(&g_link, devs, OCHAT_MAX_DEVICES);
    if (n < 0) { printf("ochat: %s\n", g_link.error); ochat_link_close(&g_link); return 1; }
    printf("this device: %s\n", g_link.self);
    printf("ADDRESS    STATE    ALIAS\n");
    for (i = 0; i < n; i++)
        printf("%s  %-7s  %s%s\n", devs[i].address, devs[i].online ? "online" : "offline",
               devs[i].alias[0] ? devs[i].alias : "-",
               strcmp(devs[i].address, g_link.self) ? "" : "  (you)");
    if (n == 0) printf("(no devices listed)\n");
    ochat_link_close(&g_link);
    return 0;
}

int cli_ping_once(const char *addr, long timeout_ms, long *rtt_us) {
    char request[OCHAT_UUID_SIZE];
    struct ochat_ping_event ev;
    long deadline;
    int r;
    if (ochat_link_ping_start(&g_link, addr, request) != 0) return -1;
    deadline = ochat_now_ms() + timeout_ms;
    for (;;) {
        r = ochat_link_ping_poll(&g_link, request, &ev);
        if (r < 0) return -1;
        if (r == 1) { *rtt_us = ev.rtt_us; return ev.state; }
        if (ochat_now_ms() >= deadline) return OCHAT_PING_TIMEOUT;
        ochat_sleep_ms(50);
    }
}

int cmd_ping(int argc, char **argv) {
    struct ochat_ping_stats st;
    char addr[OCHAT_ADDR_SIZE], buf[32];
    const char *target = NULL;
    long count = 4, rtt;
    int i, state;
    for (i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "-c") && i + 1 < argc) {
            char *end;
            errno = 0;
            count = strtol(argv[++i], &end, 10);
            if (errno || *end || count < 1 || count > PING_MAX_COUNT) {
                printf("ochat: -c needs a number from 1 to %d\n", PING_MAX_COUNT);
                return 2;
            }
        } else if (!target && argv[i][0] != '-') target = argv[i];
        else { printf("usage: ochat ping <address|alias> [-c N]\n"); return 2; }
    }
    if (!target) { printf("usage: ochat ping <address|alias> [-c N]\n"); return 2; }
    if (cli_connect()) return 1;
    if (cli_resolve(target, addr)) { ochat_link_close(&g_link); return 1; }
    ochat_ping_init(&st);
    for (i = 1; i <= count; i++) {
        ochat_ping_sent(&st);
        state = cli_ping_once(addr, PING_TIMEOUT_MS, &rtt);
        if (state < 0) { printf("ochat: %s\n", g_link.error); break; }
        if (state == OCHAT_PING_DONE) {
            ochat_ping_reply(&st, rtt);
            ochat_fmt_us(rtt, buf, sizeof(buf));
            printf("reply from %s: seq=%d time=%s\n", addr, i, buf);
        } else if (state == OCHAT_PING_TIMEOUT) printf("request timed out: seq=%d\n", i);
        else printf("ping failed (unreachable or rejected): seq=%d\n", i);
        if (i < count) ochat_sleep_ms(1000);
    }
    printf("--- %s ping statistics ---\n%d sent, %d received, %d%% loss\n",
           addr, st.sent, st.received, ochat_ping_loss_percent(&st));
    if (st.received) {
        char a[32], b[32], c[32];
        ochat_fmt_us(st.min_us, a, sizeof(a)); ochat_fmt_us(ochat_ping_avg_us(&st), b, sizeof(b));
        ochat_fmt_us(st.max_us, c, sizeof(c));
        printf("rtt min/avg/max = %s / %s / %s\n", a, b, c);
    }
    ochat_link_close(&g_link);
    return st.received > 0 ? 0 : 1;
}

int cmd_status(int argc, char **argv) {
    struct ochat_status s;
    struct ochat_route r;
    char target[OCHAT_ADDR_SIZE];
    int n, i;
    if (argc > 3) { printf("usage: ochat status [address|alias]\n"); return 2; }
    if (cli_connect()) return 1;
    if (ochat_link_status(&g_link, &s) != 0) goto error;
    printf("address:    %s\n", g_link.self);
    printf("endpoint:   %s\n", s.endpoint);
    printf("enrolled:   %s   attached: %s\n", s.enrolled ? "yes" : "no", s.attached ? "yes" : "no");
    printf("security:   %s\napplication: %s\nethernet:   %s\n", s.security, s.app_security, s.eth_security);
    target[0] = 0;
    if (argc == 3) { if (cli_resolve(argv[2], target)) goto done; }
    else if ((n = ochat_link_devices(&g_link, devs, OCHAT_MAX_DEVICES)) > 0) {
        for (i = 0; i < n && !target[0]; i++)
            if (devs[i].online && strcmp(devs[i].address, g_link.self))
                memcpy(target, devs[i].address, OCHAT_ADDR_SIZE);
    } else g_link.error[0] = 0;
    if (!target[0]) { printf("route:      (no target; try: ochat status <address>)\n"); goto done; }
    if (ochat_link_route(&g_link, target, &r) != 0) goto error;
    printf("route:      %s: %ld hop(s), ~%ld ms nominal\n", target, r.hops, r.nominal_ticks * 50);
done:
    ochat_link_close(&g_link);
    return 0;
error:
    printf("ochat: %s\n", g_link.error);
    ochat_link_close(&g_link);
    return 1;
}
