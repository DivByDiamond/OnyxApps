/*
 * test_obrowse.c - host tests for the pure obrowse modules
 * (apps/obrowse/html/parser.c + the url-resolve helper in main.c).
 *
 *   make -C tests/native test_obrowse && ./tests/native/test_obrowse
 *
 * The parser has no I/O, so no terminal is needed. Interactive
 * behavior (raw input, ANSI output, real fetches) is verified
 * manually in QEMU and built for real by CI with onyxcc.
 *
 * url_resolve is declared in obrowse.h but defined in main.c. main.c
 * is not pure (it has terminal I/O), so we copy the small resolve
 * function into a host-side shim here and test it against the same
 * contract. If the contract drifts, this test will need the same fix
 * in both places — kept that way to keep main.c under 200 lines.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "obrowse.h"

static int failures = 0;
static int checks = 0;

#define CHECK(cond)                                                      \
    do {                                                                 \
        checks++;                                                        \
        if (cond) {                                                      \
            printf("  ok  %s\n", #cond);                                 \
        } else {                                                         \
            printf("  FAIL %s (%s:%d)\n", #cond, __FILE__, __LINE__);    \
            failures++;                                                  \
        }                                                                \
    } while (0)

/* Local copy of obrowse_url_resolve, kept in sync with main.c. */
static int url_resolve(const char *base, const char *href, char *out, size_t out_cap) {
    if (href[0] == 'h' && href[1] == 't' && href[2] == 't' && href[3] == 'p' && href[4] == ':' && href[5] == '/' && href[6] == '/') {
        strncpy(out, href, out_cap - 1); out[out_cap - 1] = '\0'; return 0;
    }
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

static void test_plain_text(void) {
    struct obrowse_page p;
    const char *html = "Hello, world!";
    printf("plain text passes through:\n");
    obrowse_html_parse(html, strlen(html), &p);
    CHECK(strcmp(p.text, "Hello, world!") == 0);
    CHECK(p.link_count == 0);
}

static void test_strips_tags(void) {
    struct obrowse_page p;
    const char *html = "<html><body><p>One</p><p>Two</p></body></html>";
    printf("tags stripped, block tags emit newlines:\n");
    obrowse_html_parse(html, strlen(html), &p);
    /* Each <p> emits a '\n'; the result should contain One\nTwo\n. */
    CHECK(strstr(p.text, "One") != NULL);
    CHECK(strstr(p.text, "Two") != NULL);
    CHECK(strchr(p.text, '\n') != NULL);
    CHECK(p.link_count == 0);
}

static void test_link_emission(void) {
    struct obrowse_page p;
    const char *html = "<a href=\"/about.html\">About</a> and <a href=\"http://foo.com/\">Foo</a>";
    printf("anchors become [N] text with href recorded:\n");
    obrowse_html_parse(html, strlen(html), &p);
    CHECK(p.link_count == 2);
    CHECK(strstr(p.text, "[1] About") != NULL);
    CHECK(strstr(p.text, "[2] Foo") != NULL);
    CHECK(strcmp(p.links[0].href, "/about.html") == 0);
    CHECK(strcmp(p.links[1].href, "http://foo.com/") == 0);
}

static void test_entities(void) {
    struct obrowse_page p;
    const char *html = "&amp;&lt;&gt;&quot;&nbsp;&apos; &unknown;";
    printf("named entities decode:\n");
    obrowse_html_parse(html, strlen(html), &p);
    /* Expected: &<>\" ' &unknown; */
    CHECK(strstr(p.text, "&<>\" '") != NULL);
    CHECK(strstr(p.text, "&unknown;") != NULL);
}

static void test_comments(void) {
    struct obrowse_page p;
    const char *html = "Before<!-- this is a comment -->After";
    printf("HTML comments are dropped:\n");
    obrowse_html_parse(html, strlen(html), &p);
    CHECK(strstr(p.text, "BeforeAfter") != NULL);
    CHECK(p.link_count == 0);
}

static void test_script_style_dropped(void) {
    struct obrowse_page p;
    const char *html = "<style>.x{color:red}</style>Visible<script>alert(1)</script>Done";
    printf("script/style content dropped, surrounding text kept:\n");
    obrowse_html_parse(html, strlen(html), &p);
    CHECK(strstr(p.text, "Visible") != NULL);
    CHECK(strstr(p.text, "Done") != NULL);
    CHECK(strstr(p.text, "color:red") == NULL);
    CHECK(strstr(p.text, "alert") == NULL);
}

static void test_whitespace_collapse(void) {
    struct obrowse_page p;
    const char *html = "One\n  two\t\tthree   four";
    printf("whitespace runs collapse to a single space:\n");
    obrowse_html_parse(html, strlen(html), &p);
    CHECK(strstr(p.text, "One two three four") != NULL);
}

static void test_truncation(void) {
    struct obrowse_page p;
    /* Build a body larger than OBROWSE_BUF. */
    size_t n = OBROWSE_BUF + 1024;
    char *big = malloc(n);
    memset(big, 'X', n);
    printf("body larger than OBROWSE_BUF truncates cleanly:\n");
    int rc = obrowse_html_parse(big, n, &p);
    free(big);
    CHECK(rc == -1);
    CHECK(p.text_len == OBROWSE_BUF - 1);
    CHECK(p.text[OBROWSE_BUF - 1] == '\0');
}

static void test_too_many_links(void) {
    struct obrowse_page p;
    /* 100 anchors, only OBROWSE_MAX_LINKS become navigable. */
    char html[8192];
    size_t off = 0;
    for (int i = 0; i < 100; i++) {
        off += snprintf(html + off, sizeof(html) - off, "<a href=\"/p%d\">L%d</a> ", i, i);
    }
    printf("anchors beyond OBROWSE_MAX_LINKS are dropped silently:\n");
    obrowse_html_parse(html, off, &p);
    CHECK(p.link_count == OBROWSE_MAX_LINKS);
}

static void test_url_resolve(void) {
    char out[256];
    printf("url_resolve handles absolute, root-relative, relative:\n");
    CHECK(url_resolve("http://example.com/foo/bar", "/baz", out, sizeof(out)) == 0);
    CHECK(strcmp(out, "http://example.com/baz") == 0);
    CHECK(url_resolve("http://example.com/foo/bar", "qux", out, sizeof(out)) == 0);
    CHECK(strcmp(out, "http://example.com/foo/qux") == 0);
    CHECK(url_resolve("http://example.com/foo/bar", "http://other.com/x", out, sizeof(out)) == 0);
    CHECK(strcmp(out, "http://other.com/x") == 0);
    /* No-scheme base, root-relative href. */
    CHECK(url_resolve("example.com/foo/bar", "/baz", out, sizeof(out)) == 0);
    CHECK(strcmp(out, "example.com/baz") == 0);
    /* No-path base, relative href. */
    CHECK(url_resolve("example.com", "page", out, sizeof(out)) == 0);
    CHECK(strcmp(out, "example.com/page") == 0);
}

int main(void) {
    test_plain_text();
    test_strips_tags();
    test_link_emission();
    test_entities();
    test_comments();
    test_script_style_dropped();
    test_whitespace_collapse();
    test_truncation();
    test_too_many_links();
    test_url_resolve();
    printf("\ntest_obrowse: %d checks, %d failures\n", checks, failures);
    return failures > 0;
}
