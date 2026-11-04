/* link.c - ochat RPC transport on top of lib/oc2rpc. */
#include "link.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

long ochat_now_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void ochat_sleep_ms(int ms) {
    struct timespec ts;
    if (ms < 0) ms = 0;
    ts.tv_sec = ms / 1000; ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

/* Request IDs only need to be unique, not secret: urandom when present,
 * otherwise a clock/counter mix. */
static void ochat_random16(unsigned char out[16]) {
    static unsigned long counter;
    int fd = open("/dev/urandom", O_RDONLY, 0), i;
    long got = 0;
    if (fd >= 0) { got = read(fd, out, 16); close(fd); }
    if (got != 16) {
        unsigned long x = (unsigned long)ochat_now_ms() * 2654435761UL + (++counter) * 40503UL;
        for (i = 0; i < 16; i++) {
            x ^= x << 13; x ^= x >> 7; x ^= x << 17;
            out[i] = (unsigned char)(x >> 24);
        }
    }
}

static int fail(struct ochat_link *l, const char *what, const char *detail) {
    snprintf(l->error, sizeof(l->error), "%s%s%s", what, detail ? ": " : "", detail ? detail : "");
    return -1;
}

/* Run one device method; NULL on failure with l->error set. */
static const char *rpc(struct ochat_link *l, const char *method, const char *params) {
    const char *r;
    if (l->broken) {
        fail(l, "RPC bus out of sync after an earlier timeout; restart the program", NULL);
        return NULL;
    }
    r = oc2_call(&l->bus, l->tower, method, params);
    if (!r) {
        fail(l, method, l->bus.error);
        if (strstr(l->bus.error, "timeout")) l->broken = 1;
        return NULL;
    }
    return r;
}

int ochat_link_open(struct ochat_link *l) {
    const char *r;
    memset(l, 0, sizeof(*l));
    l->bus.fd = -1;
    if (oc2_open(&l->bus, OCHAT_BUS_PATH) != 0) {
        if (errno == EBUSY)
            return fail(l, "RPC bus busy: another program (oc2ctl, ochat, ochat-gui) owns " OCHAT_BUS_PATH, NULL);
        return fail(l, "cannot open " OCHAT_BUS_PATH, l->bus.error);
    }
    if (oc2_find(&l->bus, OCHAT_TOWER_TYPE, l->tower) != 0) {
        const char *e = l->bus.error;
        oc2_close(&l->bus);
        if (strstr(e, "timeout"))
            return fail(l, "no reply from the RPC bus: it may be owned by another program "
                           "(oc2ctl, ochat, ochat-gui) or the machine has no power", e);
        return fail(l, "no gps_tower device is attached to this machine", e);
    }
    r = rpc(l, "registerRadioService", "[\"" OCHAT_SERVICE "\"]");
    if (r) r = rpc(l, "getRadioAddress", "[]");
    if (r && ochat_parse_address(r, l->self) != 0) {
        fail(l, "tower returned an invalid radio address", r);
        r = NULL;
    }
    if (!r) { oc2_close(&l->bus); return -1; }
    return 0;
}

void ochat_link_close(struct ochat_link *l) { oc2_close(&l->bus); }

int ochat_link_send(struct ochat_link *l, const char *addr, const char *text) {
    char clean[OCHAT_MAX_TEXT + 1], uuid[OCHAT_UUID_SIZE], params[8192];
    unsigned char rnd[16];
    long n;
    static const char *why[] = { "", "text is not valid UTF-8", "text exceeds 1024 bytes", "text is empty" };
    if (!ochat_addr_valid(addr)) return fail(l, "invalid address", addr);
    n = ochat_text_clean(text, strlen(text), clean, sizeof(clean), 1);
    if (n < 0) return fail(l, why[-n], NULL);
    ochat_random16(rnd);
    ochat_uuid_v4(rnd, uuid);
    if (ochat_build_send(addr, uuid, clean, params, sizeof(params)) < 0)
        return fail(l, "message does not fit the RPC request", NULL);
    return rpc(l, "sendRadioMessage", params) ? 0 : -1;
}

int ochat_link_poll(struct ochat_link *l, struct ochat_msg *out, int max) {
    const char *r = rpc(l, "pollRadioMessages", "[]");
    int n;
    if (!r) return -1;
    n = ochat_parse_messages(r, out, max);
    if (n < 0) return fail(l, "malformed pollRadioMessages reply", NULL);
    return n;
}

int ochat_link_ping_start(struct ochat_link *l, const char *addr, char request[OCHAT_UUID_SIZE]) {
    char params[128];
    unsigned char rnd[16];
    if (!ochat_addr_valid(addr)) return fail(l, "invalid address", addr);
    ochat_random16(rnd);
    ochat_uuid_v4(rnd, request);
    if (ochat_build_args(addr, request, params, sizeof(params)) < 0) return fail(l, "bad ping arguments", NULL);
    return rpc(l, "pingRadio", params) ? 0 : -1;
}

int ochat_link_ping_poll(struct ochat_link *l, const char *request, struct ochat_ping_event *ev) {
    const char *r = rpc(l, "pollRadioEvents", "[]");
    int f;
    if (!r) return -1;
    f = ochat_parse_ping(r, request, ev);
    if (f < 0) return fail(l, "malformed pollRadioEvents reply", NULL);
    return f && ev->state != OCHAT_PING_PENDING;
}

/* Contract (mod side, pending): listRadioDevices() -> [{address, alias, online}]. */
int ochat_link_devices(struct ochat_link *l, struct ochat_device *out, int max) {
    const char *r = rpc(l, "listRadioDevices", "[]");
    int n;
    if (!r) return -1;
    n = ochat_parse_devices(r, out, max);
    if (n < 0) return fail(l, "malformed listRadioDevices reply", NULL);
    return n;
}

int ochat_link_status(struct ochat_link *l, struct ochat_status *out) {
    const char *r = rpc(l, "getRadioStatus", "[]");
    if (!r) return -1;
    return ochat_parse_status(r, out) ? fail(l, "malformed getRadioStatus reply", NULL) : 0;
}

int ochat_link_route(struct ochat_link *l, const char *addr, struct ochat_route *out) {
    char params[64];
    const char *r;
    if (!ochat_addr_valid(addr)) return fail(l, "invalid address", addr);
    if (ochat_build_args(addr, NULL, params, sizeof(params)) < 0) return fail(l, "bad arguments", NULL);
    r = rpc(l, "estimateRadioRoute", params);
    if (!r) return -1;
    return ochat_parse_route(r, out) ? fail(l, "malformed estimateRadioRoute reply", NULL) : 0;
}
