/*
 * chat.h - shared types and API of the portable ochat core.
 *
 * text/addr.c      - ORA address validation, normalisation, alias lookup
 * text/frame.c     - message text validation/sanitising, UUID text, ASCII fold
 * text/lineedit.c  - bounded one-line editor driven by raw key bytes
 * data/history.c   - fixed-size message history ring buffer
 * data/ping.c      - ping statistics and RTT formatting
 * data/wrap.c      - fixed-width text wrapping for the GUI message pane
 * wire/json.c      - bounded tolerant JSON scanner (no allocation)
 * wire/jval.c      - decode JSON scalars: strings, numbers, booleans
 * wire/wire.c      - parse RPC replies into structs
 * wire/build.c     - build RPC parameter arrays
 * link/link.c      - RPC transport (uses lib/oc2rpc; not host-tested)
 *
 * Everything except link/ is pure C99: no I/O, no allocation, every buffer
 * bounded by the caller-supplied capacity.
 */
#ifndef OCHAT_CHAT_H
#define OCHAT_CHAT_H
#include <stddef.h>

#define OCHAT_SERVICE "chat"
#define OCHAT_CHANNEL "msg"
#define OCHAT_MAX_TEXT 1024
#define OCHAT_ADDR_SIZE 10          /* "XXXX:XXXX" + NUL */
#define OCHAT_ALIAS_MAX 24
#define OCHAT_UUID_SIZE 37
#define OCHAT_MAX_DEVICES 32
#define OCHAT_MAX_POLL 16

/* ---- text/addr.c ---- */
int ochat_addr_valid(const char *s);
int ochat_addr_normalize(const char *s, char out[OCHAT_ADDR_SIZE]);
int ochat_addr_encode(long network, long device, char out[OCHAT_ADDR_SIZE]);
int ochat_alias_valid(const char *s);

struct ochat_device {
    char address[OCHAT_ADDR_SIZE];
    char alias[OCHAT_ALIAS_MAX + 1];
    int online;
};
/* 0 = resolved into out; -1 = unknown/invalid; -2 = alias is ambiguous. */
int ochat_resolve(const struct ochat_device *devs, int n, const char *target,
                  char out[OCHAT_ADDR_SIZE]);
/* Alias for an address, or NULL. */
const char *ochat_alias_of(const struct ochat_device *devs, int n, const char *addr);

/* ---- text/frame.c ---- */
#define OCHAT_E_UTF8 (-1)
#define OCHAT_E_LONG (-2)
#define OCHAT_E_EMPTY (-3)
/* Outbound: strict. Controls become spaces, ends trimmed. Returns length
 * (>0) or OCHAT_E_*. Inbound (strict==0): never fails on content; invalid
 * bytes become '?', truncated at a code point boundary. Returns length >= 0. */
long ochat_text_clean(const char *in, size_t len, char *out, size_t cap, int strict);
void ochat_uuid_v4(const unsigned char rnd[16], char out[OCHAT_UUID_SIZE]);
/* One '?' per non-ASCII code point; for the ASCII-only GUI font. */
void ochat_ascii_fold(const char *in, char *out, size_t cap);

/* ---- text/lineedit.c ---- */
#define OCHAT_LINE_MAX 256
enum { OCHAT_KEY_NONE, OCHAT_KEY_EDIT, OCHAT_KEY_SUBMIT, OCHAT_KEY_CANCEL };
struct ochat_line { char buf[OCHAT_LINE_MAX + 1]; int len; int esc; };
void ochat_line_init(struct ochat_line *l);
/* Feed one raw byte. Printable ASCII and UTF-8 bytes append; backspace
 * erases a whole code point; CR/LF submit (buffer kept); ESC-sequences are
 * swallowed; ^C / ^D on an empty line cancel. */
int ochat_line_feed(struct ochat_line *l, unsigned char c);

/* ---- data/history.c ---- */
#define OCHAT_HIST_CAP 64
struct ochat_entry {
    long seq; int outgoing;
    char peer[OCHAT_ADDR_SIZE];
    char text[OCHAT_MAX_TEXT + 1];
};
struct ochat_history { struct ochat_entry e[OCHAT_HIST_CAP]; int head, count; long next_seq; };
void ochat_hist_init(struct ochat_history *h);
void ochat_hist_push(struct ochat_history *h, const char *peer, int outgoing, const char *text);
int ochat_hist_count(const struct ochat_history *h);
/* index 0 = oldest retained entry; NULL if out of range. */
const struct ochat_entry *ochat_hist_get(const struct ochat_history *h, int index);

/* ---- data/ping.c ---- */
struct ochat_ping_stats { int sent, received; long min_us, max_us, sum_us; };
void ochat_ping_init(struct ochat_ping_stats *s);
void ochat_ping_sent(struct ochat_ping_stats *s);
void ochat_ping_reply(struct ochat_ping_stats *s, long rtt_us);
int ochat_ping_loss_percent(const struct ochat_ping_stats *s);
long ochat_ping_avg_us(const struct ochat_ping_stats *s);
void ochat_fmt_us(long us, char *out, size_t cap);   /* "12.345 ms" */

/* ---- data/wrap.c ---- */
/* Wrap text into at most max_lines lines of cols columns. Fills offsets and
 * lengths; returns the number of lines (0 for empty text). */
int ochat_wrap(const char *text, int cols, int *start, int *len, int max_lines);

/* ---- wire/json.c ---- */
const char *ochat_j_skip(const char *p);
const char *ochat_j_str_end(const char *quote);        /* just past a string literal */
const char *ochat_j_end(const char *v);                  /* end of value, NULL if malformed */
const char *ochat_j_member(const char *obj, const char *key);
const char *ochat_j_first(const char *arr);              /* first element or NULL */
const char *ochat_j_next(const char *elem);              /* next element or NULL */
long ochat_j_string(const char *v, char *out, size_t cap); /* decoded len, -1 if not a string */
int ochat_j_long(const char *v, long *out);
int ochat_j_bool(const char *v, int *out);

/* ---- wire/wire.c, wire/build.c ---- */
struct ochat_msg {
    char from[OCHAT_ADDR_SIZE];
    char text[OCHAT_MAX_TEXT + 1];
};
enum { OCHAT_PING_PENDING, OCHAT_PING_DONE, OCHAT_PING_FAILED, OCHAT_PING_TIMEOUT };
struct ochat_ping_event { int state; long rtt_us; };
struct ochat_status {
    char endpoint[OCHAT_UUID_SIZE];
    int enrolled, attached;
    char security[64], app_security[64], eth_security[64];
};
struct ochat_route { long hops; long nominal_ticks; };

/* Returns number of chat messages written (<= max) or -1 if malformed.
 * Messages of other services/channels/types and unparsable ones are skipped. */
int ochat_parse_messages(const char *json, struct ochat_msg *out, int max);
/* 1 = terminal/pending event for request found, 0 = none, -1 = malformed. */
int ochat_parse_ping(const char *json, const char *request, struct ochat_ping_event *ev);
int ochat_parse_devices(const char *json, struct ochat_device *out, int max);
int ochat_parse_status(const char *json, struct ochat_status *out);
int ochat_parse_route(const char *json, struct ochat_route *out);
int ochat_parse_address(const char *json, char out[OCHAT_ADDR_SIZE]);
/* Parameter array builders; return length or -1 when cap is too small. */
int ochat_build_send(const char *addr, const char *request, const char *text,
                     char *out, size_t cap);
int ochat_build_args(const char *a, const char *b, char *out, size_t cap);

#endif
