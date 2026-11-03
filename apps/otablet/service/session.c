#include "session.h"
#include <string.h>
static int active(int state) { return state >= OW_QUEUED && state <= OW_RUNNING; }
void ow_session_init(struct ow_session *s, const struct ow_transport *t) {
    if (!s) return;
    memset(s, 0, sizeof(*s));
    s->next_id = 1;
    if (t) s->transport = *t;
}
void ow_session_online(struct ow_session *s, int online) {
    int i;
    if (!s) return;
    s->online = online != 0;
    if (!online) for (i = 0; i < OW_REQUESTS; i++)
        if (s->requests[i].id && active(s->requests[i].state)) s->requests[i].state = OW_DISCONNECTED;
}
const struct ow_request *ow_session_find(const struct ow_session *s, unsigned long id) {
    int i;
    if (!s || !id) return 0;
    for (i = 0; i < OW_REQUESTS; i++) if (s->requests[i].id == id) return &s->requests[i];
    return 0;
}
unsigned long ow_session_send(struct ow_session *s, int action, unsigned long now, unsigned long deadline) {
    struct ow_request *r = 0;
    unsigned long id;
    int i;
    if (!s || !s->online || !s->next_id || action <= 0 || deadline <= now || !s->transport.enqueue) return 0;
    for (i = 0; i < OW_REQUESTS; i++) {
        if (!s->requests[i].id || !active(s->requests[i].state)) { r = &s->requests[i]; break; }
    }
    if (!r) return 0;
    id = s->next_id++;
    /* Adapter must enqueue only; it cannot invoke result callbacks reentrantly. */
    if (!s->transport.enqueue(s->transport.ctx, id, action, deadline)) return 0;
    r->id = id; r->action = action; r->deadline = deadline; r->state = OW_QUEUED;
    return id;
}
int ow_session_result(struct ow_session *s, unsigned long id, int state) {
    int i;
    if (!s || !s->online || !id || state < OW_ACCEPTED || state > OW_FAILED) return 0;
    for (i = 0; i < OW_REQUESTS; i++) {
        struct ow_request *r = &s->requests[i];
        if (r->id == id && active(r->state)) {
            if (state < r->state) return 0;
            r->state = state; return 1;
        }
    }
    return 0;
}
void ow_session_tick(struct ow_session *s, unsigned long now) {
    int i;
    if (!s) return;
    for (i = 0; i < OW_REQUESTS; i++) {
        struct ow_request *r = &s->requests[i];
        if (r->id && active(r->state) && now >= r->deadline) r->state = OW_TIMED_OUT;
    }
}
