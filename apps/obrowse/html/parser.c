/*
 * html/parser.c - pure HTML-to-text parser for obrowse.
 *
 * Walks the byte stream, copies non-tag bytes to page->text (collapsing
 * whitespace runs), decodes a small entity subset, emits [N] text for
 * <a href="...">...</a> elements while recording the href. No I/O, no
 * globals - host-testable as-is (see tests/native/test_obrowse.c).
 *
 * Limits (KISS): <script>/<style> blocks dropped entirely; comments
 * skipped; entities decoded: &amp; &lt; &gt; &quot; &apos; &nbsp; (no
 * numeric &#NN;); unknown tags stripped (their text still renders).
 * Whitespace runs collapse to a single space; block tags emit '\n'.
 *
 * Returns 0 on success, -1 if the body did not fit in OBROWSE_BUF; in
 * that case page->text is NUL-terminated at OBROWSE_BUF-1 and the link
 * list reflects what was parsed up to that truncation point.
 */
#include <string.h>
#include <stdio.h>
#include "../obrowse.h"

static int is_ws(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

/* Decode a known entity at s[0]='&'. Writes decoded byte to *out and
 * returns bytes consumed (including the leading & and trailing ;).
 * Returns 0 for unknown entities (caller passes the raw '&' through). */
static int decode_entity(const char *s, size_t len, char *out) {
    if (len < 3 || s[1] == ' ' || s[1] == '&' || s[1] == ';') return 0;
    size_t i = 1;
    while (i < len && s[i] != ';' && s[i] != '&' && i < 8) i++;
    if (i >= len || s[i] != ';') return 0;
    size_t n = i + 1;
    if (n == 5 && !memcmp(s, "&amp;", 5))  { *out = '&';  return 5; }
    if (n == 4 && !memcmp(s, "&lt;", 4))   { *out = '<';  return 4; }
    if (n == 4 && !memcmp(s, "&gt;", 4))   { *out = '>';  return 4; }
    if (n == 6 && !memcmp(s, "&quot;", 6)) { *out = '"';  return 6; }
    if (n == 6 && !memcmp(s, "&apos;", 6)) { *out = '\''; return 6; }
    if (n == 6 && !memcmp(s, "&nbsp;", 6)) { *out = ' ';  return 6; }
    return 0;
}

static int name_eq(const char *body, size_t off, const char *lower, size_t n) {
    for (size_t k = 0; k < n; k++) {
        char a = body[off + k];
        if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
        if (a != lower[k]) return 0;
    }
    return 1;
}

static int is_block_name(const char *s, size_t n) {
    if (n == 1 && s[0] == 'p') return 1;
    if (n == 2 && (s[0] == 'b' || s[0] == 'l' || s[0] == 't' ||
                   (s[0] == 'h' && s[1] >= '1' && s[1] <= '6'))) return 1;
    if (n == 3 && (s[0] == 'd' || s[0] == 'p')) return 1;   /* div, pre */
    if (n == 6 && !memcmp(s, "option", 6)) return 1;
    return 0;
}

static int is_dropped_name(const char *s, size_t n) {
    if (n == 6 && !memcmp(s, "script", 6)) return 1;
    if (n == 5 && !memcmp(s, "style", 5)) return 1;
    return 0;
}

/* Skip past the matching </NAME> tag (case-insensitive). On entry *i
 * points just past the dropped-tag's '>'; on return *i points past the
 * matching close '>' (or at len if none is found). */
static void skip_dropped_block(const char *body, size_t len, size_t *i, const char *lower, size_t n) {
    while (*i + n + 3 <= len) {
        if (body[*i] == '<' && body[*i + 1] == '/' && name_eq(body, *i + 2, lower, n)) {
            size_t j = *i + 2 + n;
            while (j < len && body[j] != '>') j++;
            *i = (j < len) ? j + 1 : len;
            return;
        }
        (*i)++;
    }
    *i = len;
}

/* Parse href="..." attribute from an <a ...> tag. *i is just past '<a';
 * on return *i points just past the closing '>'. Writes a NUL-terminated
 * copy (truncated to 255 bytes) into link->href. */
static void extract_href(const char *body, size_t len, size_t *i, struct obrowse_link *link) {
    link->href[0] = '\0';
    while (*i < len && body[*i] != '>') {
        if (body[*i] == 'h' && body[*i + 1] == 'r' && body[*i + 2] == 'e' &&
            body[*i + 3] == 'f' && *i + 4 < len && body[*i + 4] == '=') {
            size_t j = *i + 5;
            while (j < len && body[j] == ' ') j++;
            if (j < len && (body[j] == '"' || body[j] == '\'')) {
                char q = body[j++];
                size_t start = j;
                while (j < len && body[j] != q) j++;
                size_t hlen = j - start;
                if (hlen > sizeof(link->href) - 1) hlen = sizeof(link->href) - 1;
                memcpy(link->href, body + start, hlen);
                link->href[hlen] = '\0';
                *i = (j < len) ? j + 1 : len;
                return;
            }
        }
        (*i)++;
    }
}

int obrowse_html_parse(const char *body, size_t len, struct obrowse_page *page) {
    page->text_len = 0;
    page->link_count = 0;
    size_t i = 0;
    int ws_pending = 0;
    int anchor_open = 0;
    while (i < len) {
        char c = body[i];
        if (c == '<') {
            if (i + 4 <= len && !memcmp(body + i, "<!--", 4)) {
                i += 4;
                while (i + 3 <= len && memcmp(body + i, "-->", 3) != 0) i++;
                if (i + 3 <= len) i += 3;
                continue;
            }
            int is_close = (i + 1 < len && body[i + 1] == '/');
            size_t name_start = i + (is_close ? 2 : 1);
            size_t name_end = name_start;
            while (name_end < len) {
                char b = body[name_end];
                if (b == ' ' || b == '>' || b == '\t' || b == '\n' || b == '/') break;
                name_end++;
            }
            size_t name_n = name_end - name_start;
            char lower[8];
            if (name_n > sizeof(lower)) name_n = sizeof(lower);
            for (size_t k = 0; k < name_n; k++) {
                char b = body[name_start + k];
                lower[k] = (b >= 'A' && b <= 'Z') ? (char)(b - 'A' + 'a') : b;
            }
            if (is_dropped_name(lower, name_n)) {
                size_t j = name_end;
                while (j < len && body[j] != '>') j++;
                i = (j < len) ? j + 1 : len;
                skip_dropped_block(body, len, &i, lower, name_n);
                continue;
            }
            if (is_close && name_n == 1 && lower[0] == 'a') anchor_open = 0;
            if (!is_close && name_n == 1 && lower[0] == 'a' &&
                page->link_count < OBROWSE_MAX_LINKS) {
                struct obrowse_link *link = &page->links[page->link_count];
                size_t tag_body = name_end;
                extract_href(body, len, &tag_body, link);
                /* Skip any remaining tag bytes up to and past '>'. */
                while (tag_body < len && body[tag_body] != '>') tag_body++;
                if (tag_body < len) tag_body++;
                i = tag_body;
                if (page->text_len + 8 < OBROWSE_BUF) {
                    int n = snprintf(page->text + page->text_len, OBROWSE_BUF - page->text_len, "[%d] ", page->link_count + 1);
                    if (n > 0) page->text_len += (size_t)n;
                }
                page->link_count++;
                anchor_open = 1;
                continue;
            }
            if (is_block_name(lower, name_n) && page->text_len < OBROWSE_BUF) {
                page->text[page->text_len++] = '\n';
                ws_pending = 0;
            }
            while (i < len && body[i] != '>') i++;
            if (i < len) i++;
            continue;
        }
        if (is_ws(c)) { ws_pending = 1; i++; continue; }
        if (ws_pending && page->text_len < OBROWSE_BUF &&
            (page->text_len == 0 || page->text[page->text_len - 1] != '\n') &&
            (page->text_len == 0 || page->text[page->text_len - 1] != ' ')) {
            page->text[page->text_len++] = ' ';
        }
        ws_pending = 0;
        if (c == '&') {
            char dec;
            int n = decode_entity(body + i, len - i, &dec);
            if (n > 0) {
                if (page->text_len < OBROWSE_BUF) page->text[page->text_len++] = dec;
                i += (size_t)n;
                continue;
            }
        }
        if (page->text_len + 1 < OBROWSE_BUF) {
            page->text[page->text_len++] = c;
        } else {
            page->text[OBROWSE_BUF - 1] = '\0';
            return -1;
        }
        i++;
        (void)anchor_open;
    }
    if (page->text_len < OBROWSE_BUF) page->text[page->text_len] = '\0';
    else page->text[OBROWSE_BUF - 1] = '\0';
    return 0;
}