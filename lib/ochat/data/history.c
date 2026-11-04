/* history.c - fixed-capacity ring of chat messages; oldest entry overwritten. */
#include "../chat.h"
#include <string.h>

void ochat_hist_init(struct ochat_history *h) {
    h->head = 0; h->count = 0; h->next_seq = 1;
}

void ochat_hist_push(struct ochat_history *h, const char *peer, int outgoing, const char *text) {
    struct ochat_entry *e;
    size_t n;
    int slot = (h->head + h->count) % OCHAT_HIST_CAP;
    if (h->count == OCHAT_HIST_CAP) {
        slot = h->head;
        h->head = (h->head + 1) % OCHAT_HIST_CAP;
    } else h->count++;
    e = &h->e[slot];
    e->seq = h->next_seq++;
    e->outgoing = outgoing != 0;
    memset(e->peer, 0, sizeof(e->peer));
    if (peer) strncpy(e->peer, peer, OCHAT_ADDR_SIZE - 1);
    n = text ? strlen(text) : 0;
    if (n > OCHAT_MAX_TEXT) n = OCHAT_MAX_TEXT;
    if (n) memcpy(e->text, text, n);
    e->text[n] = 0;
}

int ochat_hist_count(const struct ochat_history *h) { return h->count; }

const struct ochat_entry *ochat_hist_get(const struct ochat_history *h, int index) {
    if (index < 0 || index >= h->count) return NULL;
    return &h->e[(h->head + index) % OCHAT_HIST_CAP];
}
