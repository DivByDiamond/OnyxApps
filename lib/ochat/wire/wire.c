/* wire.c - parse OC2R radio RPC replies and build parameter arrays.
 * All shapes assumed here are listed in OnyxOS docs/dev/ochat.md ("RPC contract"). */
#include "../chat.h"
#include <string.h>
#include <stdio.h>

#define RAW_MAX 2048

static int str_is(const char *v, const char *want) {
    char tmp[48];
    long n = ochat_j_string(v, tmp, sizeof(tmp));
    return n >= 0 && !strcmp(tmp, want);
}

static int icase_eq(const char *a, const char *b) {
    for (; *a && *b; a++, b++) {
        char x = *a, y = *b;
        if (x >= 'A' && x <= 'Z') x += 32;
        if (y >= 'A' && y <= 'Z') y += 32;
        if (x != y) return 0;
    }
    return *a == *b;
}

/* Address as "XXXX:XXXX" text or {"network":N,"device":N}. */
static int read_address(const char *v, char out[OCHAT_ADDR_SIZE]) {
    char tmp[32];
    long a, b;
    if (!v) return -1;
    if (ochat_j_string(v, tmp, sizeof(tmp)) >= 0) return ochat_addr_normalize(tmp, out);
    if (ochat_j_long(ochat_j_member(v, "network"), &a) &&
        ochat_j_long(ochat_j_member(v, "device"), &b)) return ochat_addr_encode(a, b, out);
    return -1;
}

/* Payload as a JSON array of byte values (-128..255) or plain string. */
static long read_payload(const char *v, char *raw, size_t cap) {
    const char *e;
    size_t n = 0;
    long s = ochat_j_string(v, raw, cap);
    if (s >= 0) return s;
    for (e = ochat_j_first(v); e; e = ochat_j_next(e)) {
        long b;
        if (!ochat_j_long(e, &b) || b < -128 || b > 255) return -1;
        if (n + 1 < cap) raw[n++] = (char)(b & 0xFF);
    }
    raw[n] = 0;
    return (long)n;
}

int ochat_parse_messages(const char *json, struct ochat_msg *out, int max) {
    const char *e;
    int n = 0;
    if (!json || !out || max < 1 || !ochat_j_end(json)) return -1;
    json = ochat_j_skip(json);
    if (*json != '[') return -1;
    for (e = ochat_j_first(json); e && n < max; e = ochat_j_next(e)) {
        char raw[RAW_MAX];
        const char *v;
        long len;
        if (!ochat_j_end(e) || *ochat_j_skip(e) != '{') continue;
        v = ochat_j_member(e, "service");
        if (!v || !str_is(v, OCHAT_SERVICE)) continue;
        v = ochat_j_member(e, "channel");
        if (!v || !str_is(v, OCHAT_CHANNEL)) continue;
        v = ochat_j_member(e, "type");
        if (v && !str_is(v, "MESSAGE")) continue;
        if (read_address(ochat_j_member(e, "sourceAddress"), out[n].from)) continue;
        len = read_payload(ochat_j_member(e, "payload"), raw, sizeof(raw));
        if (len < 0) continue;
        ochat_text_clean(raw, (size_t)len, out[n].text, sizeof(out[n].text), 0);
        n++;
    }
    return n;
}

int ochat_parse_ping(const char *json, const char *request, struct ochat_ping_event *ev) {
    const char *e;
    int found = 0;
    if (!json || !request || !ev) return -1;
    json = ochat_j_skip(json);
    if (*json != '[' || !ochat_j_end(json)) return -1;
    for (e = ochat_j_first(json); e; e = ochat_j_next(e)) {
        char id[48], state[24];
        long nanos = 0;
        if (ochat_j_string(ochat_j_member(e, "requestId"), id, sizeof(id)) < 0) continue;
        if (!icase_eq(id, request)) continue;
        if (ochat_j_string(ochat_j_member(e, "state"), state, sizeof(state)) < 0) continue;
        found = 1;
        ev->rtt_us = 0;
        if (!strcmp(state, "COMPLETED")) {
            ev->state = OCHAT_PING_DONE;
            if (ochat_j_long(ochat_j_member(e, "roundTripNanos"), &nanos) && nanos > 0)
                ev->rtt_us = nanos / 1000;
        } else if (!strcmp(state, "TIMED_OUT")) ev->state = OCHAT_PING_TIMEOUT;
        else if (!strcmp(state, "FAILED") || !strcmp(state, "DISCONNECTED")) ev->state = OCHAT_PING_FAILED;
        else ev->state = OCHAT_PING_PENDING;
    }
    return found;
}

int ochat_parse_devices(const char *json, struct ochat_device *out, int max) {
    const char *e;
    int n = 0;
    if (!json || !out || max < 1) return -1;
    json = ochat_j_skip(json);
    if (*json != '[' || !ochat_j_end(json)) return -1;
    for (e = ochat_j_first(json); e && n < max; e = ochat_j_next(e)) {
        struct ochat_device *d = &out[n];
        char alias[96];
        int on = 0, i;
        if (read_address(ochat_j_member(e, "address"), d->address)) continue;
        d->alias[0] = 0;
        if (ochat_j_string(ochat_j_member(e, "alias"), alias, sizeof(alias)) > 0) {
            for (i = 0; alias[i] && i < OCHAT_ALIAS_MAX; i++) {
                char c = alias[i];
                int ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                         (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.';
                d->alias[i] = ok ? c : '_';
            }
            d->alias[i] = 0;
        }
        ochat_j_bool(ochat_j_member(e, "online"), &on);
        d->online = on;
        n++;
    }
    return n;
}

static void copy_str(const char *v, char *out, size_t cap) {
    if (ochat_j_string(v, out, cap) < 0) strcpy(out, "unknown");
}

int ochat_parse_status(const char *json, struct ochat_status *out) {
    if (!json || !out || *ochat_j_skip(json) != '{' || !ochat_j_end(json)) return -1;
    memset(out, 0, sizeof(*out));
    copy_str(ochat_j_member(json, "endpoint"), out->endpoint, sizeof(out->endpoint));
    ochat_j_bool(ochat_j_member(json, "enrolled"), &out->enrolled);
    ochat_j_bool(ochat_j_member(json, "attached"), &out->attached);
    copy_str(ochat_j_member(json, "security"), out->security, sizeof(out->security));
    copy_str(ochat_j_member(json, "applicationSecurity"), out->app_security, sizeof(out->app_security));
    copy_str(ochat_j_member(json, "ethernetSecurity"), out->eth_security, sizeof(out->eth_security));
    return 0;
}

int ochat_parse_route(const char *json, struct ochat_route *out) {
    if (!json || !out || *ochat_j_skip(json) != '{' || !ochat_j_end(json)) return -1;
    if (!ochat_j_long(ochat_j_member(json, "hops"), &out->hops)) return -1;
    if (!ochat_j_long(ochat_j_member(json, "nominalTicks"), &out->nominal_ticks)) return -1;
    return 0;
}

int ochat_parse_address(const char *json, char out[OCHAT_ADDR_SIZE]) {
    return read_address(json, out);
}
