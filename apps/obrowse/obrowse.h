#ifndef OBROWSE_H
#define OBROWSE_H

#include <stddef.h>

#define OBROWSE_BUF      16384
#define OBROWSE_MAX_LINKS 64
#define OBROWSE_HISTORY   8

struct obrowse_link {
    char href[256];
};

struct obrowse_page {
    char  text[OBROWSE_BUF];
    size_t text_len;
    struct obrowse_link links[OBROWSE_MAX_LINKS];
    int  link_count;
    char url[256];
};

int obrowse_html_parse(const char *body, size_t body_len, struct obrowse_page *page);

long obrowse_fetch(const char *url, char *out_buf, size_t out_cap);

int obrowse_url_resolve(const char *base, const char *href, char *out, size_t out_cap);

void ui_show_error(const char *kind, const char *url, int http_status, int my_errno);

#endif /* OBROWSE_H */
