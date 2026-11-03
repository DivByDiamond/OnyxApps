#ifndef OW_SESSION_H
#define OW_SESSION_H
#define OW_REQUESTS 8
enum ow_outcome {
    OW_QUEUED, OW_ACCEPTED, OW_RUNNING, OW_COMPLETED,
    OW_FAILED, OW_TIMED_OUT, OW_DISCONNECTED
};
struct ow_request { unsigned long id, deadline; int action, state; };
/* Inject an already authenticated service adapter. Enqueue is transport-only;
 * successful enqueue is never evidence of acceptance or task completion. */
struct ow_transport {
    void *ctx;
    int (*enqueue)(void *, unsigned long, int, unsigned long);
};
struct ow_session {
    struct ow_transport transport;
    struct ow_request requests[OW_REQUESTS];
    unsigned long next_id;
    int online;
};
void ow_session_init(struct ow_session *, const struct ow_transport *);
void ow_session_online(struct ow_session *, int);
/* Caller supplies monotonic timestamps; deadlines must be strictly in future. */
unsigned long ow_session_send(struct ow_session *, int, unsigned long, unsigned long);
/* Only the trusted endpoint adapter supplies results, after identity validation. */
int ow_session_result(struct ow_session *, unsigned long, int);
void ow_session_tick(struct ow_session *, unsigned long);
const struct ow_request *ow_session_find(const struct ow_session *, unsigned long);
#endif
