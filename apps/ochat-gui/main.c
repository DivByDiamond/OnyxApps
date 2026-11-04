/*
 * ochat-gui - graphical chat for the OC2R radio network (framebuffer + mouse).
 *
 * Left: device list (click to talk, UP/DOWN scroll, PING, REFR). Right: chat
 * history with the selected device. Bottom: input line (type on the keyboard,
 * Enter or SEND to send). Top right: ping indicator. Tab cycles contacts;
 * "/to ADDRESS" opens a chat with an address not in the list. Right click or
 * Ctrl-C exits. Owns the RPC bus (/dev/hvc0) while running: one program at a time.
 */
#include "gui/gui.h"
#include "link/link.h"
#include <stdio.h>
#include <string.h>

static struct ochat_link g_link;
static struct gui gui;

int screen_run(struct gui *g);

static int be_send(void *c, const char *a, const char *t) { return ochat_link_send(&g_link, a, t); }
static int be_poll(void *c, struct ochat_msg *o, int m) { return ochat_link_poll(&g_link, o, m); }
static int be_devices(void *c, struct ochat_device *o, int m) { return ochat_link_devices(&g_link, o, m); }
static int be_ping_start(void *c, const char *a, char r[OCHAT_UUID_SIZE]) { return ochat_link_ping_start(&g_link, a, r); }
static int be_ping_poll(void *c, const char *r, struct ochat_ping_event *e) { return ochat_link_ping_poll(&g_link, r, e); }
static const char *be_error(void *c) { return g_link.error; }

int main(int argc, char **argv) {
    struct gui_backend be;
    int rc;
    if (argc > 1) {
        puts("usage: ochat-gui   (needs /dev/fb0, the mouse and the RPC bus; see ochat-gui docs)");
        return strcmp(argv[1], "--help") ? 2 : 0;
    }
    if (ochat_link_open(&g_link) != 0) { printf("ochat-gui: %s\n", g_link.error); return 1; }
    be.ctx = 0; be.send = be_send; be.poll = be_poll; be.devices = be_devices;
    be.ping_start = be_ping_start; be.ping_poll = be_ping_poll; be.error = be_error;
    gui_init(&gui, &be, g_link.self);
    gui_refresh_devices(&gui, ochat_now_ms());
    rc = screen_run(&gui);
    ochat_link_close(&g_link);
    if (rc < 0) { printf("ochat-gui: framebuffer or mouse unavailable (/dev/fb0)\n"); return 1; }
    return 0;
}
