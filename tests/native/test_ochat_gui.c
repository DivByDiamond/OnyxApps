/* Host tests for apps/ochat-gui model, actions and drawing with a fake radio. */
#include "gui/gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0, checks = 0;
#define CHECK(c) do { checks++; if (!(c)) { failures++; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

/* ---- fake radio ---- */
static struct ochat_device f_devs[8];
static int f_ndevs, f_devices_fail, f_send_fail, f_poll_fail, f_ping_mode, f_ping_polls;
static struct ochat_msg f_in[4];
static int f_nin, f_sent;
static char f_last_to[OCHAT_ADDR_SIZE], f_last_text[OCHAT_MAX_TEXT + 1];
static const char *f_err = "";

static int b_send(void *c, const char *a, const char *t) {
    if (f_send_fail) { f_err = "bus busy"; return -1; }
    strcpy(f_last_to, a); strcpy(f_last_text, t); f_sent++; return 0;
}
static int b_poll(void *c, struct ochat_msg *o, int m) {
    int n = f_nin;
    if (f_poll_fail) { f_err = "poll failed"; return -1; }
    memcpy(o, f_in, sizeof(f_in[0]) * (size_t)n); f_nin = 0; return n;
}
static int b_devices(void *c, struct ochat_device *o, int m) {
    if (f_devices_fail) { f_err = "no listRadioDevices"; return -1; }
    memcpy(o, f_devs, sizeof(f_devs[0]) * (size_t)f_ndevs); return f_ndevs;
}
static int b_ping_start(void *c, const char *a, char r[OCHAT_UUID_SIZE]) {
    if (f_ping_mode == 9) { f_err = "ping refused"; return -1; }
    strcpy(r, "req"); f_ping_polls = 0; return 0;
}
static int b_ping_poll(void *c, const char *r, struct ochat_ping_event *e) {
    f_ping_polls++;
    if (f_ping_mode == 0) { e->state = OCHAT_PING_DONE; e->rtt_us = 12300; return 1; }
    if (f_ping_mode == 1) return 0;                        /* never answers */
    if (f_ping_mode == 2) { e->state = OCHAT_PING_FAILED; e->rtt_us = 0; return 1; }
    f_err = "poll broke"; return -1;
}
static const char *b_error(void *c) { return f_err; }

static struct gui g;
static unsigned short px[GUI_W * GUI_H];

static void dev(int i, const char *addr, const char *alias, int on) {
    strcpy(f_devs[i].address, addr); strcpy(f_devs[i].alias, alias); f_devs[i].online = on;
}
static void reset(void) {
    struct gui_backend be;
    memset(&be, 0, sizeof(be));
    be.send = b_send; be.poll = b_poll; be.devices = b_devices;
    be.ping_start = b_ping_start; be.ping_poll = b_ping_poll; be.error = b_error;
    f_devices_fail = f_send_fail = f_poll_fail = f_ping_mode = f_nin = f_sent = 0;
    dev(0, "0001:0001", "me", 1); dev(1, "7K3F:02Q9", "miner", 1); dev(2, "0001:0003", "", 0);
    f_ndevs = 3; f_err = "";
    gui_init(&g, &be, "0001:0001");
    gui_refresh_devices(&g, 0);
}
static const char *label(int i) { return g.view.widgets[2 + i].text; }
static void type(const char *s) { for (; *s; s++) gui_key(&g, (unsigned char)*s, 0); }
static int lit(int x0, int y0, int x1, int y1) {
    int x, y, n = 0;
    for (y = y0; y < y1; y++) for (x = x0; x < x1; x++) if (px[y * GUI_W + x] == 0xFFFF || px[y * GUI_W + x] == 0x07E0) n++;
    return n;
}

static void test_contacts(void) {
    reset();
    CHECK(g.ndevs == 3 && g.sel == 1);                          /* first device that is not self */
    CHECK(!strcmp(label(0), " *me") && !strcmp(label(1), ">*miner") && !strcmp(label(2), " -0001:0003"));
    CHECK(label(3)[0] == 0);
    CHECK(gui_click(&g, 5, 11 + 2 * 11 + 2, 0) == 3 && g.sel == 2);   /* click third contact */
    gui_key(&g, '\t', 0);
    CHECK(g.sel == 0);
    gui_key(&g, '\t', 0); gui_key(&g, '\t', 0); gui_key(&g, '\t', 0);
    CHECK(g.sel == 0);
    CHECK(gui_click(&g, 100, 50, 0) == 0);                       /* pane click: no action */
    f_devices_fail = 1;
    gui_action(&g, ACT_REFRESH, 0);
    CHECK(g.ndevs == 3 && strstr(g.status, "devices"));          /* list kept, error shown */
    f_devices_fail = 0;
    dev(1, "7K3F:02Q9", "miner", 0);
    gui_select(&g, 1);
    gui_refresh_devices(&g, 0);
    CHECK(g.sel == 1 && !strcmp(label(1), ">-miner"));
    f_ndevs = 1;                                                  /* selected device vanished */
    gui_refresh_devices(&g, 0);
    CHECK(g.ndevs == 2 && g.sel >= 0 && !strcmp(gui_selected(&g), "7K3F:02Q9"));
}

static void test_scroll(void) {
    int i;
    reset();
    for (i = 0; i < 8; i++) { char a[10]; snprintf(a, sizeof(a), "0002:000%d", i + 1); dev(i, a, "", 1); }
    f_ndevs = 8; g.sel = -1; gui_refresh_devices(&g, 0);
    CHECK(g.ndevs == 8 && g.scroll == 0);
    gui_action(&g, ACT_UP, 0); CHECK(g.scroll == 0);
    gui_action(&g, ACT_DOWN, 0); gui_action(&g, ACT_DOWN, 0); gui_action(&g, ACT_DOWN, 0);
    CHECK(g.scroll == 2);
    CHECK(!strcmp(label(0), " *0002:0003"));
    gui_action(&g, 1, 0);
    CHECK(g.sel == 2);
    gui_action(&g, 6, 0);                                         /* beyond the list is ignored */
    f_ndevs = 2; gui_refresh_devices(&g, 0);
    CHECK(g.scroll == 0);
    gui_action(&g, 5, 0); gui_action(&g, 0, 0); gui_action(&g, 999, 0);
}

static void test_chat(void) {
    reset();
    type("hello \"world\"");
    CHECK(gui_click(&g, 210, 105, 0) == ACT_SEND);               /* SEND button */
    CHECK(f_sent == 1 && !strcmp(f_last_to, "7K3F:02Q9") && !strcmp(f_last_text, "hello \"world\""));
    CHECK(g.line.len == 0 && ochat_hist_count(&g.hist) == 1);
    gui_key(&g, '\r', 0);                                         /* empty line: nothing */
    CHECK(f_sent == 1);
    f_send_fail = 1;
    type("again");
    gui_key(&g, '\r', 0);
    CHECK(f_sent == 1 && !strcmp(g.line.buf, "again") && strstr(g.status, "bus busy"));
    f_send_fail = 0;
    gui_key(&g, '\r', 0);
    CHECK(f_sent == 2 && g.status[0] == 0);
    gui_key(&g, '\r', 0);
    type("\xff\xfe");
    gui_key(&g, '\r', 0);
    CHECK(f_sent == 2 && strstr(g.status, "UTF-8"));
    ochat_line_init(&g.line);
    { int i; for (i = 0; i < 300; i++) gui_key(&g, 'x', 0); }
    CHECK(g.line.len == OCHAT_LINE_MAX);
    gui_key(&g, '\r', 0);
    CHECK(f_sent == 3);
    ochat_line_init(&g.line);
    type("/to 7k3f:02q9");
    gui_key(&g, '\r', 0);
    CHECK(g.sel == 1);
    type("/to 0abc:0def"); gui_key(&g, '\r', 0);
    CHECK(g.ndevs == 4 && !strcmp(gui_selected(&g), "0ABC:0DEF"));
    type("/to nobody"); gui_key(&g, '\r', 0);
    CHECK(strstr(g.status, "unknown"));
    type("/bogus"); gui_key(&g, '\r', 0);
    CHECK(strstr(g.status, "commands"));
    CHECK(gui_key(&g, 0x03, 0) == 1);                             /* ^C quits */
}

static void test_incoming(void) {
    reset();
    strcpy(f_in[0].from, "0001:0003"); strcpy(f_in[0].text, "ping me");
    strcpy(f_in[1].from, "0009:0009"); strcpy(f_in[1].text, "stranger");
    f_nin = 2;
    gui_tick(&g, 1000);
    CHECK(g.unread[2] == 1 && g.ndevs == 4 && g.unread[3] == 1);
    CHECK(label(2)[0] == '!');
    gui_action(&g, 3, 1000);
    CHECK(g.unread[2] == 0 && label(2)[0] == '>');
    f_poll_fail = 1;
    gui_tick(&g, 2000);
    CHECK(strstr(g.status, "receive"));
    gui_set_status(&g, "");
    gui_tick(&g, 3000);                                            /* same error: reported once */
    CHECK(g.status[0] == 0);
    f_poll_fail = 0;
    memset(&g.hist, 0, sizeof(g.hist)); ochat_hist_init(&g.hist);
    gui_select(&g, 2);
    {   /* no selection yet: first incoming message selects its sender */
        struct gui_backend be = g.be;
        gui_init(&g, &be, "0001:0001");
        f_devices_fail = 1; gui_refresh_devices(&g, 0); f_devices_fail = 0;
        CHECK(g.sel == -1);
        strcpy(f_in[0].from, "7K3F:02Q9"); strcpy(f_in[0].text, "x"); f_nin = 1;
        gui_tick(&g, 0);
        CHECK(g.sel == 0 && g.ndevs == 1);
    }
}

static void test_ping(void) {
    reset();
    gui_ping(&g, 0);
    CHECK(g.ping_state == PING_BUSY);
    gui_tick(&g, 0);
    CHECK(g.ping_state == PING_OK && g.ping_us == 12300);
    CHECK(!strcmp(g.view.widgets[1].text, "PING 12.3MS 0%"));
    f_ping_mode = 1; gui_ping(&g, 1000);
    gui_tick(&g, 1000); CHECK(g.ping_state == PING_BUSY);
    gui_ping(&g, 1500); CHECK(g.pstats.sent == 2);                /* already busy: no second ping */
    gui_tick(&g, 1000 + GUI_PING_TIMEOUT_MS + 1);
    CHECK(g.ping_state == PING_LOST && !strcmp(g.view.widgets[1].text, "PING LOST"));
    f_ping_mode = 2; gui_action(&g, ACT_PING, 5000); gui_tick(&g, 5000);
    CHECK(g.ping_state == PING_FAIL);
    f_ping_mode = 3; gui_action(&g, ACT_PING, 6000); gui_tick(&g, 6000);
    CHECK(g.ping_state == PING_FAIL && strstr(g.status, "ping"));
    f_ping_mode = 9; gui_action(&g, ACT_PING, 7000);
    CHECK(g.ping_state == PING_FAIL && strstr(g.status, "refused"));
    gui_select(&g, 2);
    CHECK(g.ping_state == PING_IDLE && g.pstats.sent == 0);
    g.sel = -1; gui_ping(&g, 8000);
    CHECK(strstr(g.status, "select"));
}

static void test_draw(void) {
    int i;
    reset();
    gui_draw(&g, px);
    CHECK(lit(74, 11, 240, 101) == 0);                            /* empty chat: nothing lit */
    type("hi");
    gui_key(&g, '\r', 0);
    strcpy(f_in[0].from, "7K3F:02Q9"); strcpy(f_in[0].text, "a long reply that wraps over several lines of the pane ok");
    f_nin = 1; gui_tick(&g, 1000);
    gui_draw(&g, px);
    CHECK(lit(74, 11, 240, 101) > 50);
    CHECK(lit(0, 102, 200, 110) == 0);                            /* input row empty so far */
    type("typing");
    gui_draw(&g, px);
    CHECK(lit(0, 102, 200, 110) > 20);
    for (i = 0; i < 64; i++) {                                    /* history ring overflow with huge messages */
        char big[OCHAT_MAX_TEXT + 1];
        memset(big, 'w', OCHAT_MAX_TEXT); big[OCHAT_MAX_TEXT] = 0;
        ochat_hist_push(&g.hist, "7K3F:02Q9", i & 1, big);
    }
    gui_set_status(&g, "a very long status line that must be clipped by the pane width");
    gui_draw(&g, px);
    ochat_hist_push(&g.hist, "7K3F:02Q9", 0, "\xe2\x82\xac\xff\x01 tab\there");
    gui_draw(&g, px);
    for (i = 0; i < 300; i++) gui_key(&g, 'q', 0);                  /* input tail display bounded */
    gui_draw(&g, px);
    if (getenv("OCHAT_GUI_PPM")) {
        FILE *f = fopen(getenv("OCHAT_GUI_PPM"), "wb");
        if (f) {
            int k;
            fprintf(f, "P6\n%d %d\n255\n", GUI_W, GUI_H);
            for (k = 0; k < GUI_W * GUI_H; k++) {
                unsigned c = px[k];
                fputc((int)((c >> 11) * 255 / 31), f); fputc((int)(((c >> 5) & 63) * 255 / 63), f);
                fputc((int)((c & 31) * 255 / 31), f);
            }
            fclose(f);
        }
    }
}

static void test_font(void) {
    int r, any = 0;
    unsigned char c;
    for (c = 0; c < 255; c++) for (r = 0; r < 9; r++) {
        unsigned int b = gui_glyph8(c, r);
        CHECK(b < 32);
        if (b) any = 1;
    }
    CHECK(any && gui_glyph8(' ', 3) == 0 && gui_glyph8('A', 0) != 0 && gui_glyph8('A', 8) == 0);
    CHECK(gui_glyph('a', 3) == gui_glyph('A', 3) && gui_glyph('A', 7) == 0);
    CHECK(gui_glyph8('g', 7) != 0);                                 /* descender row */
}

int main(void) {
    test_contacts(); test_scroll(); test_chat(); test_incoming(); test_ping(); test_draw(); test_font();
    printf("ochat-gui: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
