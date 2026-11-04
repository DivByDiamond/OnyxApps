/*
 * gui.h - ochat-gui: chat window on the otablet widget toolkit.
 *
 * gui/model.c    - state: contacts, history, polling, ping progress, view sync
 * gui/actions.c  - clicks, keys, send, slash commands
 * gui/draw.c     - paints the 240x112 logical screen (message pane, input row)
 * font/font.c    - 5x7 ASCII font
 * device/screen.c- /dev/fb0 + mouse + keyboard loop (OnyxOS only)
 *
 * The model talks to the radio only through struct gui_backend, so it runs
 * (and is tested) on a host with a fake backend. Single-threaded.
 */
#ifndef OCHAT_GUI_H
#define OCHAT_GUI_H
#include "core/widget.h"
#include "chat.h"

#define GUI_W 240
#define GUI_H 112
#define GUI_ROWS 6              /* contact buttons */
#define GUI_PANE_LINES 10
#define GUI_PANE_COLS 27
#define GUI_POLL_MS 300
#define GUI_DEVICES_MS 5000
#define GUI_PING_TIMEOUT_MS 3000
#define GUI_STATUS_MAX 47

enum { ACT_UP = 20, ACT_DOWN, ACT_PING, ACT_REFRESH, ACT_SEND };
enum { PING_IDLE, PING_BUSY, PING_OK, PING_LOST, PING_FAIL };

struct gui_backend {
    void *ctx;
    int (*send)(void *, const char *addr, const char *text);              /* 0 ok */
    int (*poll)(void *, struct ochat_msg *out, int max);                  /* n or -1 */
    int (*devices)(void *, struct ochat_device *out, int max);            /* n or -1 */
    int (*ping_start)(void *, const char *addr, char request[OCHAT_UUID_SIZE]);
    int (*ping_poll)(void *, const char *request, struct ochat_ping_event *);  /* 1 done, 0, -1 */
    const char *(*error)(void *);
};

struct gui {
    struct gui_backend be;
    char self[OCHAT_ADDR_SIZE];
    struct ochat_device devs[OCHAT_MAX_DEVICES];
    int unread[OCHAT_MAX_DEVICES];
    int ndevs, scroll, sel;
    struct ochat_history hist;
    struct ochat_line line;
    char status[GUI_STATUS_MAX + 1];
    struct ochat_ping_stats pstats;
    char ping_request[OCHAT_UUID_SIZE];
    int ping_state;
    long ping_us, ping_deadline, next_ping_poll, next_poll, next_devices;
    char last_error[128];
    struct ow_view view;
};

void gui_init(struct gui *g, const struct gui_backend *be, const char *self);
void gui_tick(struct gui *g, long now_ms);           /* poll radio, advance ping */
void gui_refresh_devices(struct gui *g, long now_ms);
void gui_set_status(struct gui *g, const char *text);
void gui_sync(struct gui *g);                         /* model -> widgets */
int gui_find(const struct gui *g, const char *addr);
int gui_ensure(struct gui *g, const char *addr);      /* contact index, adds if missing */
void gui_select(struct gui *g, int index);
const char *gui_selected(const struct gui *g);        /* address or NULL */

void gui_action(struct gui *g, int action, long now_ms);
int gui_click(struct gui *g, int x, int y, long now_ms);   /* returns action fired */
/* 0 = continue, 1 = quit requested. */
int gui_key(struct gui *g, unsigned char key, long now_ms);
void gui_submit(struct gui *g, long now_ms);
void gui_ping(struct gui *g, long now_ms);

unsigned int gui_glyph(unsigned char ch, int row);   /* 5 low bits of row 0..6 */
unsigned int gui_glyph8(unsigned char ch, int row);  /* row 0..7, descenders */
void gui_draw(const struct gui *g, unsigned short *pixels);
#endif
