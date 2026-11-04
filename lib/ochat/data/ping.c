/* ping.c - ping statistics and RTT formatting. Integer only. */
#include "../chat.h"
#include <stdio.h>

#define MAX_RTT_US 3600000000L  /* clamp absurd values to one hour */

void ochat_ping_init(struct ochat_ping_stats *s) {
    s->sent = 0; s->received = 0; s->min_us = 0; s->max_us = 0; s->sum_us = 0;
}

void ochat_ping_sent(struct ochat_ping_stats *s) {
    if (s->sent < 1000000) s->sent++;
}

void ochat_ping_reply(struct ochat_ping_stats *s, long rtt_us) {
    if (rtt_us < 0) rtt_us = 0;
    if (rtt_us > MAX_RTT_US) rtt_us = MAX_RTT_US;
    if (s->received >= s->sent) return;  /* a reply needs a matching send */
    if (s->received == 0 || rtt_us < s->min_us) s->min_us = rtt_us;
    if (s->received == 0 || rtt_us > s->max_us) s->max_us = rtt_us;
    s->sum_us += rtt_us;
    s->received++;
}

int ochat_ping_loss_percent(const struct ochat_ping_stats *s) {
    if (s->sent <= 0) return 0;
    return (int)(((long)(s->sent - s->received) * 100) / s->sent);
}

long ochat_ping_avg_us(const struct ochat_ping_stats *s) {
    return s->received > 0 ? s->sum_us / s->received : 0;
}

void ochat_fmt_us(long us, char *out, size_t cap) {
    if (us < 0) us = 0;
    snprintf(out, cap, "%ld.%03ld ms", us / 1000, us % 1000);
}
