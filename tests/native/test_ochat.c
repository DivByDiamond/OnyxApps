/* Host tests for lib/ochat: addresses, framing, history, ping, wrap, JSON, wire. */
#include "chat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0, checks = 0;
#define CHECK(c) do { checks++; if (!(c)) { failures++; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

static void test_addr(void) {
    char out[OCHAT_ADDR_SIZE];
    struct ochat_device d[3];
    CHECK(ochat_addr_valid("7K3F:02Q9"));
    CHECK(ochat_addr_valid("7k3f:02q9"));
    CHECK(!ochat_addr_valid("7K3F-02Q9"));
    CHECK(!ochat_addr_valid("7K3F:02Q"));
    CHECK(!ochat_addr_valid("7K3F:02Q9X"));
    CHECK(!ochat_addr_valid("7K3I:02Q9"));      /* I is not in the alphabet */
    CHECK(!ochat_addr_valid("0000:02Q9"));      /* reserved */
    CHECK(!ochat_addr_valid("7K3F:0000"));
    CHECK(!ochat_addr_valid(NULL) && !ochat_addr_valid(""));
    CHECK(ochat_addr_normalize("7k3f:02q9", out) == 0 && !strcmp(out, "7K3F:02Q9"));
    CHECK(ochat_addr_encode(0x7A5F3, 0x0A, out) == 0 && ochat_addr_valid(out));
    CHECK(ochat_addr_encode(0, 1, out) == -1 && ochat_addr_encode(1, 0x100000, out) == -1);
    CHECK(ochat_addr_encode(1, 1, out) == 0 && !strcmp(out, "0001:0001"));
    CHECK(ochat_alias_valid("robot-1") && !ochat_alias_valid("bad name") && !ochat_alias_valid(""));
    CHECK(!ochat_alias_valid("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    memset(d, 0, sizeof(d));
    strcpy(d[0].address, "7K3F:02Q9"); strcpy(d[0].alias, "Miner");
    strcpy(d[1].address, "0001:0002"); strcpy(d[1].alias, "dup");
    strcpy(d[2].address, "0001:0003"); strcpy(d[2].alias, "DUP");
    CHECK(ochat_resolve(d, 3, "miner", out) == 0 && !strcmp(out, "7K3F:02Q9"));
    CHECK(ochat_resolve(d, 3, "0001:0002", out) == 0);
    CHECK(ochat_resolve(d, 3, "dup", out) == -2);
    CHECK(ochat_resolve(d, 3, "nobody", out) == -1);
    CHECK(ochat_resolve(NULL, 0, "nobody", out) == -1 && ochat_resolve(d, 3, "", out) == -1);
    CHECK(!strcmp(ochat_alias_of(d, 3, "7K3F:02Q9"), "Miner") && !ochat_alias_of(d, 3, "9999:9999"));
}

static void test_frame(void) {
    char out[OCHAT_MAX_TEXT + 1], big[3000], uuid[OCHAT_UUID_SIZE];
    unsigned char rnd[16];
    long n;
    int i;
    n = ochat_text_clean("  hello\tworld\n ", 15, out, sizeof(out), 1);
    CHECK(n == 11 && !strcmp(out, "hello world"));
    CHECK(ochat_text_clean("   ", 3, out, sizeof(out), 1) == OCHAT_E_EMPTY);
    CHECK(ochat_text_clean("", 0, out, sizeof(out), 1) == OCHAT_E_EMPTY);
    CHECK(ochat_text_clean("a\xff" "b", 3, out, sizeof(out), 1) == OCHAT_E_UTF8);
    CHECK(ochat_text_clean("\xc0\xaf", 2, out, sizeof(out), 1) == OCHAT_E_UTF8);     /* overlong */
    CHECK(ochat_text_clean("\xed\xa0\x80", 3, out, sizeof(out), 1) == OCHAT_E_UTF8); /* surrogate */
    CHECK(ochat_text_clean("\xf4\x90\x80\x80", 4, out, sizeof(out), 1) == OCHAT_E_UTF8);
    CHECK(ochat_text_clean("\xe2\x82", 2, out, sizeof(out), 1) == OCHAT_E_UTF8);     /* truncated */
    n = ochat_text_clean("caf\xc3\xa9 \xe2\x82\xac", 9, out, sizeof(out), 1);
    CHECK(n == 9 && !strcmp(out, "caf\xc3\xa9 \xe2\x82\xac"));
    memset(big, 'a', sizeof(big)); big[2999] = 0;
    CHECK(ochat_text_clean(big, 1025, out, sizeof(out), 1) == OCHAT_E_LONG);
    CHECK(ochat_text_clean(big, 1024, out, sizeof(out), 1) == 1024);
    n = ochat_text_clean(big, 2999, out, sizeof(out), 0);          /* inbound truncates */
    CHECK(n == 1024 && out[1024] == 0);
    /* escape injection: ESC and C0 controls never survive */
    n = ochat_text_clean("a\x1b[31mb\x07\x7f", 9, out, sizeof(out), 0);
    CHECK(strchr(out, 0x1b) == NULL && strchr(out, 7) == NULL && n == 6);
    CHECK(ochat_text_clean("x\xc2\x9b" "31m", 6, out, sizeof(out), 0) >= 0 && !strstr(out, "\xc2\x9b"));
    n = ochat_text_clean("ok\xff\xfe", 4, out, sizeof(out), 0);
    CHECK(n == 4 && !strcmp(out, "ok??"));
    /* truncation never splits a code point */
    memset(big, 'a', 1023); strcpy(big + 1023, "\xc3\xa9");
    n = ochat_text_clean(big, 1025, out, sizeof(out), 0);
    CHECK(n == 1023);
    CHECK(ochat_text_clean("x", 1, out, 1, 1) == OCHAT_E_LONG);   /* tiny buffer */
    for (i = 0; i < 16; i++) rnd[i] = 0xFF;
    ochat_uuid_v4(rnd, uuid);
    CHECK(!strcmp(uuid, "ffffffff-ffff-4fff-bfff-ffffffffffff"));
    for (i = 0; i < 16; i++) rnd[i] = (unsigned char)i;
    ochat_uuid_v4(rnd, uuid);
    CHECK(strlen(uuid) == 36 && uuid[14] == '4' && uuid[8] == '-');
    ochat_ascii_fold("h\xc3\xa9llo \xe2\x82\xac\xff", out, sizeof(out));
    CHECK(!strcmp(out, "h?llo ??"));
    ochat_ascii_fold("abcdef", out, 4);
    CHECK(!strcmp(out, "abc"));
}

static void test_line(void) {
    struct ochat_line l;
    const char *s;
    ochat_line_init(&l);
    for (s = "hi"; *s; s++) CHECK(ochat_line_feed(&l, (unsigned char)*s) == OCHAT_KEY_EDIT);
    CHECK(ochat_line_feed(&l, 0x7F) == OCHAT_KEY_EDIT && l.len == 1);
    CHECK(ochat_line_feed(&l, '\r') == OCHAT_KEY_SUBMIT && !strcmp(l.buf, "h"));
    ochat_line_init(&l);
    CHECK(ochat_line_feed(&l, 0x7F) == OCHAT_KEY_NONE);
    ochat_line_feed(&l, 0xC3); ochat_line_feed(&l, 0xA9);
    CHECK(l.len == 2);
    ochat_line_feed(&l, 0x7F);
    CHECK(l.len == 0);
    /* arrow key CSI swallowed */
    CHECK(ochat_line_feed(&l, 0x1B) == OCHAT_KEY_NONE && ochat_line_feed(&l, '[') == OCHAT_KEY_NONE);
    CHECK(ochat_line_feed(&l, 'A') == OCHAT_KEY_NONE && l.len == 0);
    CHECK(ochat_line_feed(&l, 0x04) == OCHAT_KEY_CANCEL);
    { int i; for (i = 0; i < OCHAT_LINE_MAX + 50; i++) ochat_line_feed(&l, 'x');
      CHECK(l.len == OCHAT_LINE_MAX && l.buf[OCHAT_LINE_MAX] == 0); }
}

static struct ochat_history hist;
static void test_history(void) {
    int i;
    char t[16];
    const struct ochat_entry *e;
    ochat_hist_init(&hist);
    CHECK(ochat_hist_count(&hist) == 0 && ochat_hist_get(&hist, 0) == NULL);
    ochat_hist_push(&hist, "7K3F:02Q9", 0, "first");
    e = ochat_hist_get(&hist, 0);
    CHECK(e && !e->outgoing && !strcmp(e->text, "first") && e->seq == 1);
    for (i = 0; i < OCHAT_HIST_CAP + 10; i++) {
        snprintf(t, sizeof(t), "m%d", i);
        ochat_hist_push(&hist, "7K3F:02Q9", i & 1, t);
    }
    CHECK(ochat_hist_count(&hist) == OCHAT_HIST_CAP);
    e = ochat_hist_get(&hist, 0);
    CHECK(e && !strcmp(e->text, "m10"));                       /* oldest evicted */
    e = ochat_hist_get(&hist, OCHAT_HIST_CAP - 1);
    CHECK(e && !strcmp(e->text, "m73") && e->seq == OCHAT_HIST_CAP + 11);
    CHECK(ochat_hist_get(&hist, OCHAT_HIST_CAP) == NULL && ochat_hist_get(&hist, -1) == NULL);
    { static char huge[OCHAT_MAX_TEXT * 2];
      memset(huge, 'z', sizeof(huge) - 1); huge[sizeof(huge) - 1] = 0;
      ochat_hist_push(&hist, "7K3F:02Q9", 0, huge);
      e = ochat_hist_get(&hist, OCHAT_HIST_CAP - 1);
      CHECK(strlen(e->text) == OCHAT_MAX_TEXT); }
    ochat_hist_push(&hist, NULL, 0, NULL);
    e = ochat_hist_get(&hist, OCHAT_HIST_CAP - 1);
    CHECK(e->peer[0] == 0 && e->text[0] == 0);
}

static void test_ping(void) {
    struct ochat_ping_stats s;
    char buf[32];
    ochat_ping_init(&s);
    CHECK(ochat_ping_loss_percent(&s) == 0 && ochat_ping_avg_us(&s) == 0);
    ochat_ping_reply(&s, 5);                                   /* reply without send is ignored */
    CHECK(s.received == 0);
    ochat_ping_sent(&s); ochat_ping_reply(&s, 1000);
    ochat_ping_sent(&s); ochat_ping_reply(&s, 3000);
    ochat_ping_sent(&s);                                       /* lost */
    ochat_ping_sent(&s); ochat_ping_reply(&s, -50);            /* clamps */
    CHECK(s.sent == 4 && s.received == 3);
    CHECK(s.min_us == 0 && s.max_us == 3000 && ochat_ping_avg_us(&s) == 1333);
    CHECK(ochat_ping_loss_percent(&s) == 25);
    ochat_ping_init(&s); ochat_ping_sent(&s);
    CHECK(ochat_ping_loss_percent(&s) == 100);
    ochat_ping_reply(&s, 99999999999999L);
    CHECK(s.max_us == 3600000000L);
    ochat_fmt_us(12345, buf, sizeof(buf)); CHECK(!strcmp(buf, "12.345 ms"));
    ochat_fmt_us(-5, buf, sizeof(buf)); CHECK(!strcmp(buf, "0.000 ms"));
    ochat_fmt_us(1000000, buf, 4); CHECK(strlen(buf) == 3);
}

static void test_wrap(void) {
    int st[8], ln[8], n;
    n = ochat_wrap("hello world foo", 8, st, ln, 8);
    CHECK(n == 3 && st[0] == 0 && ln[0] == 5 && st[1] == 6 && ln[1] == 5 && st[2] == 12 && ln[2] == 3);
    n = ochat_wrap("abcdefghij", 4, st, ln, 8);
    CHECK(n == 3 && ln[0] == 4 && ln[1] == 4 && ln[2] == 2);
    CHECK(ochat_wrap("", 4, st, ln, 8) == 0);
    CHECK(ochat_wrap("abc", 0, st, ln, 8) == 0);
    CHECK(ochat_wrap("a b c d e f g h i j k l", 2, st, ln, 3) == 3);   /* bounded */
    n = ochat_wrap("          ", 3, st, ln, 8);
    CHECK(n >= 1 && n <= 8);
}

static void test_json(void) {
    char out[64];
    long v;
    int b;
    const char *j = " {\"a\": [1, 2, {\"x\": \"y\\n\\u00e9\\ud83d\\ude00\"}], \"b\": true, \"c\": null, \"n\": -42.0} ";
    CHECK(ochat_j_end(j) != NULL);
    CHECK(ochat_j_long(ochat_j_member(j, "n"), &v) && v == -42);
    CHECK(ochat_j_bool(ochat_j_member(j, "b"), &b) && b == 1);
    CHECK(ochat_j_member(j, "zzz") == NULL);
    CHECK(ochat_j_string(ochat_j_member(j, "c"), out, sizeof(out)) == -1);
    { const char *e = ochat_j_first(ochat_j_member(j, "a")); int n = 0; while (e) { n++; e = ochat_j_next(e); } CHECK(n == 3); }
    { const char *e = ochat_j_first(ochat_j_member(j, "a")); e = ochat_j_next(ochat_j_next(e));
      CHECK(ochat_j_string(ochat_j_member(e, "x"), out, sizeof(out)) == 8 &&
            !strcmp(out, "y\n\xc3\xa9\xf0\x9f\x98\x80")); }
    CHECK(ochat_j_string("\"toolong\"", out, 4) == -1);
    CHECK(ochat_j_end("{\"a\":") == NULL && ochat_j_end("[1,") == NULL && ochat_j_end("{\"a\" 1}") == NULL);
    CHECK(ochat_j_end("[1 2]") == NULL && ochat_j_end("\"abc") == NULL && ochat_j_end("") == NULL);
    CHECK(ochat_j_end("[]") != NULL && ochat_j_end("{}") != NULL && ochat_j_end("[[],{}]") != NULL);
    CHECK(ochat_j_string("\"\\ud800\"", out, sizeof(out)) == -1);   /* lone surrogate */
    CHECK(ochat_j_string("\"\\q\"", out, sizeof(out)) == -1);
    CHECK(!ochat_j_long("99999999999999999999999", &v) && !ochat_j_long("abc", &v) && !ochat_j_long("1e5", &v));
    { char deep[200]; int i; for (i = 0; i < 100; i++) deep[i] = '['; deep[100] = 0;
      CHECK(ochat_j_end(deep) == NULL); }
    { char deep[80]; int i; for (i = 0; i < 20; i++) deep[i] = '[';
      for (i = 20; i < 40; i++) { deep[i] = ']'; }
      deep[40] = 0;
      CHECK(ochat_j_end(deep) == deep + 40); }
}

static void test_wire(void) {
    struct ochat_msg m[4];
    struct ochat_device d[4];
    struct ochat_ping_event ev;
    struct ochat_status st;
    struct ochat_route r;
    char out[OCHAT_ADDR_SIZE], buf[4096];
    int n;
    const char *msgs =
        "[{\"version\":1,\"source\":\"11111111-1111-1111-1111-111111111111\",\"sourceAddress\":{\"network\":7,\"device\":1},"
        "\"service\":\"chat\",\"channel\":\"msg\",\"requestId\":\"22222222-2222-2222-2222-222222222222\",\"type\":\"MESSAGE\","
        "\"payload\":[104,105,-61,-87]},"
        "{\"sourceAddress\":\"7k3f:02q9\",\"service\":\"chat\",\"channel\":\"msg\",\"payload\":\"plain \\u003c3\"},"
        "{\"sourceAddress\":\"7K3F:02Q9\",\"service\":\"telemetry\",\"channel\":\"msg\",\"payload\":[1]},"
        "{\"sourceAddress\":\"7K3F:02Q9\",\"service\":\"chat\",\"channel\":\"msg\",\"type\":\"PONG\",\"payload\":[1]},"
        "{\"sourceAddress\":\"garbage\",\"service\":\"chat\",\"channel\":\"msg\",\"payload\":[1]},"
        "{\"sourceAddress\":\"7K3F:02Q9\",\"service\":\"chat\",\"channel\":\"msg\",\"payload\":[1,\"x\"]},"
        "{\"sourceAddress\":\"7K3F:02Q9\",\"service\":\"chat\",\"channel\":\"msg\",\"payload\":[27,91,51,49,109,65]}]";
    n = ochat_parse_messages(msgs, m, 4);
    CHECK(n == 3);
    CHECK(!strcmp(m[0].from, "0007:0001") && !strcmp(m[0].text, "hi\xc3\xa9"));
    CHECK(!strcmp(m[1].from, "7K3F:02Q9") && !strcmp(m[1].text, "plain <3"));
    CHECK(strchr(m[2].text, 27) == NULL);
    CHECK(ochat_parse_messages(msgs, m, 1) == 1);
    CHECK(ochat_parse_messages("[]", m, 4) == 0);
    CHECK(ochat_parse_messages("{}", m, 4) == -1 && ochat_parse_messages("[{", m, 4) == -1);
    CHECK(ochat_parse_messages("", m, 4) == -1 && ochat_parse_messages(NULL, m, 4) == -1);
    CHECK(ochat_parse_messages("[1,2,\"x\",null,{}]", m, 4) == 0);

    CHECK(ochat_parse_ping("[{\"requestId\":\"AAAAAAAA-0000-4000-8000-000000000000\",\"state\":\"COMPLETED\",\"roundTripNanos\":1500000}]",
                           "aaaaaaaa-0000-4000-8000-000000000000", &ev) == 1 &&
          ev.state == OCHAT_PING_DONE && ev.rtt_us == 1500);
    CHECK(ochat_parse_ping("[{\"requestId\":\"x\",\"state\":\"COMPLETED\"}]", "y", &ev) == 0);
    CHECK(ochat_parse_ping("[{\"requestId\":\"y\",\"state\":\"ACCEPTED\"},{\"requestId\":\"y\",\"state\":\"TIMED_OUT\"}]", "y", &ev) == 1 &&
          ev.state == OCHAT_PING_TIMEOUT);
    CHECK(ochat_parse_ping("[{\"requestId\":\"y\",\"state\":\"ACCEPTED\"}]", "y", &ev) == 1 && ev.state == OCHAT_PING_PENDING);
    CHECK(ochat_parse_ping("[{\"requestId\":\"y\",\"state\":\"DISCONNECTED\"}]", "y", &ev) == 1 && ev.state == OCHAT_PING_FAILED);
    CHECK(ochat_parse_ping("[{\"requestId\":\"y\",\"state\":\"COMPLETED\"}]", "y", &ev) == 1 && ev.rtt_us == 0);
    CHECK(ochat_parse_ping("oops", "y", &ev) == -1 && ochat_parse_ping("[]", "y", &ev) == 0);

    n = ochat_parse_devices("[{\"address\":\"7K3F:02Q9\",\"alias\":\"Mi ner!\",\"online\":true},"
                            "{\"address\":\"bad\",\"alias\":\"x\",\"online\":true},"
                            "{\"address\":\"0001:0002\",\"alias\":null,\"online\":false},"
                            "{\"address\":\"0001:0003\"}]", d, 4);
    CHECK(n == 3 && !strcmp(d[0].alias, "Mi_ner_") && d[0].online == 1);
    CHECK(d[1].alias[0] == 0 && d[1].online == 0 && d[2].online == 0);
    CHECK(ochat_parse_devices("[]", d, 4) == 0 && ochat_parse_devices("nope", d, 4) == -1);
    CHECK(ochat_parse_devices("[{\"address\":\"0001:0002\"},{\"address\":\"0001:0003\"}]", d, 1) == 1);

    CHECK(ochat_parse_status("{\"endpoint\":\"e\",\"enrolled\":true,\"attached\":false,\"applicationSecurity\":\"tls\",\"security\":\"s\"}", &st) == 0);
    CHECK(st.enrolled == 1 && st.attached == 0 && !strcmp(st.app_security, "tls") && !strcmp(st.eth_security, "unknown"));
    CHECK(ochat_parse_status("[]", &st) == -1);
    CHECK(ochat_parse_route("{\"hops\":3,\"nominalTicks\":12}", &r) == 0 && r.hops == 3 && r.nominal_ticks == 12);
    CHECK(ochat_parse_route("{\"hops\":3}", &r) == -1 && ochat_parse_route("null", &r) == -1);
    CHECK(ochat_parse_address("\"7k3f:02q9\"", out) == 0 && !strcmp(out, "7K3F:02Q9"));
    CHECK(ochat_parse_address("null", out) == -1 && ochat_parse_address("\"zz\"", out) == -1);

    n = ochat_build_send("7K3F:02Q9", "uuid", "say \"hi\"\\\x01", buf, sizeof(buf));
    CHECK(n > 0 && !strcmp(buf, "[\"7K3F:02Q9\",\"chat\",\"msg\",\"uuid\",\"say \\\"hi\\\"\\\\\\u0001\"]"));
    CHECK(ochat_build_send("a", "b", "c", buf, 10) == -1);
    CHECK(ochat_build_send("a", "b", NULL, buf, sizeof(buf)) == -1);
    { int len = ochat_build_send("a", "b", "cdefgh", buf, 36);   /* exact-fit boundary scan */
      int c; for (c = 1; c < 40; c++) { int r2 = ochat_build_send("a", "b", "cdefgh", buf, (size_t)c);
          CHECK(r2 == -1 || (r2 == len && buf[r2] == 0 && (size_t)r2 < (size_t)c)); } }
    { static char big[OCHAT_MAX_TEXT + 1], wire[8192];
      memset(big, 1, OCHAT_MAX_TEXT); big[OCHAT_MAX_TEXT] = 0;
      CHECK(ochat_build_send("7K3F:02Q9", "u", big, wire, sizeof(wire)) > 0); }
    CHECK(ochat_build_args("x", NULL, buf, sizeof(buf)) == 5 && !strcmp(buf, "[\"x\"]"));
    CHECK(ochat_build_args("x", "y", buf, sizeof(buf)) > 0 && !strcmp(buf, "[\"x\",\"y\"]"));
    CHECK(ochat_build_args("x", "y", buf, 6) == -1 && ochat_build_args(NULL, "y", buf, 64) == -1);
}

int main(void) {
    test_addr(); test_frame(); test_line(); test_history(); test_ping();
    test_wrap(); test_json(); test_wire();
    printf("ochat: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
