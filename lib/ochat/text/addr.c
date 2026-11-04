/* addr.c - ORA address ("7K3F:02Q9") and alias handling. Pure C99. */
#include "../chat.h"
#include <string.h>

/* Crockford-style base32 used by the mod: no I, L, O, U. */
static const char ALPHABET[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";

static int upper(int c) { return (c >= 'a' && c <= 'z') ? c - 32 : c; }

static int group_ok(const char *g) {
    int i, zero = 1;
    for (i = 0; i < 4; i++) {
        int c = upper((unsigned char)g[i]);
        if (c == 0 || !strchr(ALPHABET, c)) return 0;
        if (c != '0') zero = 0;
    }
    return !zero; /* network and device 0 are reserved */
}

int ochat_addr_valid(const char *s) {
    size_t i;
    if (!s) return 0;
    for (i = 0; i < 9; i++) if (!s[i]) return 0;
    return s[9] == 0 && s[4] == ':' && group_ok(s) && group_ok(s + 5);
}

int ochat_addr_normalize(const char *s, char out[OCHAT_ADDR_SIZE]) {
    int i;
    if (!ochat_addr_valid(s)) return -1;
    for (i = 0; i < 9; i++) out[i] = (char)upper((unsigned char)s[i]);
    out[9] = 0;
    return 0;
}

int ochat_addr_encode(long network, long device, char out[OCHAT_ADDR_SIZE]) {
    int i;
    long v[2];
    v[0] = network; v[1] = device;
    if (network < 1 || device < 1 || network > 0xFFFFF || device > 0xFFFFF) return -1;
    for (i = 0; i < 2; i++) {
        int k;
        for (k = 0; k < 4; k++) out[i * 5 + 3 - k] = ALPHABET[(v[i] >> (k * 5)) & 31];
    }
    out[4] = ':'; out[9] = 0;
    return 0;
}

int ochat_alias_valid(const char *s) {
    size_t i;
    if (!s || !*s) return 0;
    for (i = 0; s[i]; i++) {
        char c = s[i];
        int ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                 (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.';
        if (!ok || i >= OCHAT_ALIAS_MAX) return 0;
    }
    return 1;
}

static int ieq(const char *a, const char *b) {
    for (; *a && *b; a++, b++) if (upper((unsigned char)*a) != upper((unsigned char)*b)) return 0;
    return *a == *b;
}

int ochat_resolve(const struct ochat_device *devs, int n, const char *target,
                  char out[OCHAT_ADDR_SIZE]) {
    int i, hit = -1;
    if (!target || !*target) return -1;
    if (ochat_addr_normalize(target, out) == 0) return 0;
    if (!devs) return -1;
    for (i = 0; i < n; i++) {
        if (devs[i].alias[0] && ieq(devs[i].alias, target)) {
            if (hit >= 0 && strcmp(devs[hit].address, devs[i].address)) return -2;
            hit = i;
        }
    }
    if (hit < 0) return -1;
    memcpy(out, devs[hit].address, OCHAT_ADDR_SIZE);
    return 0;
}

const char *ochat_alias_of(const struct ochat_device *devs, int n, const char *addr) {
    int i;
    if (!devs || !addr) return NULL;
    for (i = 0; i < n; i++)
        if (devs[i].alias[0] && !strcmp(devs[i].address, addr)) return devs[i].alias;
    return NULL;
}
