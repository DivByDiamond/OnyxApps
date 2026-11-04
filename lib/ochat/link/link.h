/*
 * link.h - RPC transport for ochat: one GPS tower device on /dev/hvc0.
 *
 * Only one process may own the RPC bus. A second opener cannot be detected
 * reliably by the kernel, so a bus that stays silent is reported as
 * "busy or unavailable". After any timeout the link is marked broken (the RPC
 * protocol has no request IDs, so late replies cannot be matched): every
 * further call fails until the program is restarted.
 */
#ifndef OCHAT_LINK_H
#define OCHAT_LINK_H
#include "../chat.h"
#include "../../oc2rpc/oc2rpc.h"

#define OCHAT_BUS_PATH "/dev/hvc0"
#define OCHAT_TOWER_TYPE "gps_tower"

struct ochat_link {
    char tower[37];
    char self[OCHAT_ADDR_SIZE];
    char error[256];
    int broken;
    oc2_bus bus;            /* large; keep last */
};

long ochat_now_ms(void);
void ochat_sleep_ms(int ms);

int ochat_link_open(struct ochat_link *l);   /* 0 ok, -1 see l->error */
void ochat_link_close(struct ochat_link *l);
/* text is cleaned and validated (strict); addr must be a normalised address. */
int ochat_link_send(struct ochat_link *l, const char *addr, const char *text);
int ochat_link_poll(struct ochat_link *l, struct ochat_msg *out, int max);
int ochat_link_ping_start(struct ochat_link *l, const char *addr, char request[OCHAT_UUID_SIZE]);
int ochat_link_ping_poll(struct ochat_link *l, const char *request, struct ochat_ping_event *ev);
/* The only place that depends on the mod's listRadioDevices() contract. */
int ochat_link_devices(struct ochat_link *l, struct ochat_device *out, int max);
int ochat_link_status(struct ochat_link *l, struct ochat_status *out);
int ochat_link_route(struct ochat_link *l, const char *addr, struct ochat_route *out);
#endif
