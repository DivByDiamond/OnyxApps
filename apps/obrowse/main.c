/*
 * main.c - obrowse entry point and TUI loop.
 *
 * Usage: obrowse <url>          - fetch and render one page
 *
 * Non-blocking read loop (raw termios, VMIN=0/VTIME=0): PgUp/PgDn/arrows
 * scroll, digits + Enter follow links, 'b' back, 'r' reload, 'q' quit.
 *
 * Layout: max 4 files per folder, max 200 lines per file. History is
 * static state in main.c. Stage A (render+scroll) and Stage B (link
 * navigation + back) are both in this file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include "obrowse.h"
#define K_UP    1000
#define K_DOWN  1001
#define K_PGUP  1002
#define K_PGDN  1003

static struct termios g_orig;
static int g_raw_on = 0;
static int g_rows = 24, g_cols = 80;
static struct obrowse_page g_page;
static struct obrowse_page g_history[OBROWSE_HISTORY];
static int g_history_len = 0;
static int g_top = 0;
static char g_body_buf[OBROWSE_BUF]; /* .bss: USER_TOP proximity workaround */
static void term_raw_on(void) {
    struct termios t;
    tcgetattr(0, &g_orig);
    t = g_orig;
    cfmakeraw_apply(&t);
    t.c_cc[VMIN] = 0; t.c_cc[VTIME] = 0;
    tcsetattr(0, 0, &t);
    g_raw_on = 1;
}
static void term_raw_off(void) { if (g_raw_on) { tcsetattr(0, 0, &g_orig); g_raw_on = 0; } }
static void get_size(void) {
    unsigned short ws[4] = {24, 80, 0, 0};
    if (_onyx_ioctl(0, 0x5413, (long)ws) == 0 && ws[0] > 4 && ws[1] > 16) {
        g_rows = ws[0]; g_cols = ws[1];
    }
    if (g_rows < 8) g_rows = 8;
    if (g_cols < 32) g_cols = 32;
}
static int read_byte(unsigned char *c) { return read(0, c, 1) == 1; }

static int key_poll(void) {
    unsigned char c, s1, s2;
    if (!read_byte(&c)) return 0;
    if (c != 0x1b) return c;
    if (!read_byte(&s1)) return 0x1b;
    if (s1 != '[') return 0;
    if (!read_byte(&s2)) return 0;
    if (s2 == 'A') return K_UP;
    if (s2 == 'B') return K_DOWN;
    if (!read_byte(&s2)) return 0;
    if (s2 == '5' && read_byte(&s2)) { if (s2 == '~') return K_PGUP; return 0; }
    if (s2 == '6' && read_byte(&s2)) { if (s2 == '~') return K_PGDN; return 0; }
    return 0;
}
static void render(void) {
    printf("\x1b[2J\x1b[1;1H");
    printf("\x1b[7m obrowse - %s \x1b[0m\r\n", g_page.url[0] ? g_page.url : "(no url)");
    int max_lines = g_rows - 3;
    if (max_lines < 1) max_lines = 1;
    int line = 0, skipped = 0;
    size_t pos = 0;
    while (skipped < g_top && pos < g_page.text_len) {
        if (g_page.text[pos] == '\n') skipped++;
        pos++;
    }
    while (line < max_lines && pos < g_page.text_len) {
        size_t end = pos, col = 0;
        while (end < g_page.text_len && g_page.text[end] != '\n' && col + 1 < (size_t)g_cols) { end++; col++; }
        printf("%.*s\r\n", (int)(end - pos), g_page.text + pos);
        pos = (end < g_page.text_len && g_page.text[end] == '\n') ? end + 1 : end;
        line++;
        if (pos >= g_page.text_len) break;
    }
    printf("\x1b[33mlinks: %d  history: %d  top: %d  size: %zu\x1b[0m\r\n",
           g_page.link_count, g_history_len, g_top, g_page.text_len);
    printf("\x1b[2mPgUp/PgDn scroll  [N] Enter follow link  b back  r reload  q quit\x1b[0m");
    fflush(stdout);
}
/* Fetch and render a new URL, pushing the current page into history. */
static int go(const char *url) {
    if (g_history_len < OBROWSE_HISTORY) {
        memcpy(&g_history[g_history_len++], &g_page, sizeof(g_page));
    } else {
        memmove(&g_history[0], &g_history[1], sizeof(g_history[0]) * (OBROWSE_HISTORY - 1));
        memcpy(&g_history[OBROWSE_HISTORY - 1], &g_page, sizeof(g_page));
    }
    long n = obrowse_fetch(url, g_body_buf, sizeof(g_body_buf));
    if (n < 0) {
        printf("\x1b[2J\x1b[1;1Hfetch failed (errno %d): %s\r\npress any key to continue\r\n", errno, url);
        fflush(stdout);
        return -1;
    }
    memset(&g_page, 0, sizeof(g_page));
    obrowse_html_parse(g_body_buf, (size_t)n, &g_page);
    strncpy(g_page.url, url, sizeof(g_page.url) - 1);
    g_page.url[sizeof(g_page.url) - 1] = '\0';
    g_top = 0;
    render();
    return 0;
}
static void go_back(void) {
    if (g_history_len == 0) return;
    memcpy(&g_page, &g_history[--g_history_len], sizeof(g_page));
    g_top = 0; render();
}
static void follow_link(int n) {
    if (n < 1 || n > g_page.link_count) return;
    char next_url[256];
    if (obrowse_url_resolve(g_page.url, g_page.links[n - 1].href, next_url, sizeof(next_url)) != 0) return;
    go(next_url);
}
int obrowse_url_resolve(const char *base, const char *href, char *out, size_t out_cap) {
    if (href[0] == 'h' && href[1] == 't' && href[2] == 't' && href[3] == 'p' && href[4] == ':' && href[5] == '/' && href[6] == '/') {
        strncpy(out, href, out_cap - 1); out[out_cap - 1] = '\0'; return 0;
    }
    /* Preserve the scheme from the base if present. */
    const char *scheme_end = NULL;
    if (base[0] == 'h' && base[1] == 't' && base[2] == 't' && base[3] == 'p' && base[4] == ':' && base[5] == '/' && base[6] == '/') {
        scheme_end = base + 7;
    }
    const char *b = scheme_end ? scheme_end : base;
    size_t scheme_len = scheme_end ? 7 : 0;
    const char *path_start = strchr(b, '/');
    if (href[0] == '/') {
        if (!path_start) { snprintf(out, out_cap, "%s%s", base, href); return 0; }
        size_t host_len = (size_t)(path_start - b);
        if (scheme_len + host_len + strlen(href) + 1 > out_cap) return -1;
        memcpy(out, base, scheme_len);
        memcpy(out + scheme_len, b, host_len);
        out[scheme_len + host_len] = '\0';
        strncat(out, href, out_cap - scheme_len - host_len - 1);
        return 0;
    }
    if (!path_start) { snprintf(out, out_cap, "%s/%s", base, href); return 0; }
    const char *last_slash = path_start;
    for (const char *p = path_start + 1; *p; p++) if (*p == '/') last_slash = p;
    size_t prefix = scheme_len + (size_t)(last_slash - b) + 1;
    if (prefix + strlen(href) + 1 > out_cap) return -1;
    memcpy(out, base, prefix); out[prefix] = '\0';
    strncat(out, href, out_cap - prefix - 1);
    return 0;
}
int main(int argc, char **argv) {
    if (argc != 2) { fprintf(stderr, "usage: %s <url>\n", argv[0]); return 1; }
    get_size();
    term_raw_on();
    printf("\x1b[?25l");
    long n = obrowse_fetch(argv[1], g_body_buf, sizeof(g_body_buf));
    if (n < 0) {
        printf("\x1b[2J\x1b[1;1Hfetch failed (errno %d): %s\r\npress q to exit\r\n", errno, argv[1]);
        fflush(stdout);
        int k; while ((k = key_poll()) == 0) {} (void)k;
        printf("\x1b[2J\x1b[1;1H\x1b[?25h"); fflush(stdout); term_raw_off(); return 1;
    }
    memset(&g_page, 0, sizeof(g_page));
    obrowse_html_parse(g_body_buf, (size_t)n, &g_page);
    strncpy(g_page.url, argv[1], sizeof(g_page.url) - 1);
    g_page.url[sizeof(g_page.url) - 1] = '\0';
    render();
    int quit = 0, link_buf = 0;
    while (!quit) {
        int k = key_poll();
        if (k == 0) continue;
        if (k == 'q' || k == 'Q' || k == 0x11 || k == 0x03) { quit = 1; continue; }
        if (k == K_PGUP || k == K_UP) { g_top = g_top > 5 ? g_top - 5 : 0; render(); continue; }
        if (k == K_PGDN || k == K_DOWN) { g_top += 5; render(); continue; }
        if (k == 'b' || k == 'B') { go_back(); continue; }
        if (k == 'r' || k == 'R') {
            long rn = obrowse_fetch(g_page.url, g_body_buf, sizeof(g_body_buf));
            if (rn >= 0) {
                char saved_url[256]; strncpy(saved_url, g_page.url, sizeof(saved_url) - 1);
                saved_url[sizeof(saved_url) - 1] = '\0';
                memset(&g_page, 0, sizeof(g_page));
                obrowse_html_parse(g_body_buf, (size_t)rn, &g_page);
                strncpy(g_page.url, saved_url, sizeof(g_page.url) - 1);
                g_page.url[sizeof(g_page.url) - 1] = '\0';
                g_top = 0; render();
            }
            continue;
        }
        if (k >= '0' && k <= '9') { link_buf = link_buf * 10 + (k - '0'); if (link_buf > 999) link_buf = k - '0'; continue; }
        if (k == '\n' || k == '\r' || k == 'g' || k == 'G') {
            if (link_buf > 0 && link_buf <= g_page.link_count) { follow_link(link_buf); link_buf = 0; }
        }
    }
    printf("\x1b[2J\x1b[1;1H\x1b[?25h"); fflush(stdout); term_raw_off();
    return 0;
}