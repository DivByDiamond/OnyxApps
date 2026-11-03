#ifndef OW_DASHBOARD_H
#define OW_DASHBOARD_H
#include "core/widget.h"
struct dashboard_sample { int energy; char machine[OW_TEXT]; };
/* Inject trusted local adapter; success must mean a complete validated sample.
 * action return 1 is accepted, 2 is completed, 0 is rejected/offline. */
struct dashboard_backend {
    void *ctx;
    int (*fetch)(void *, struct dashboard_sample *);
    int (*action)(void *, int, unsigned long);
};
void dashboard_init(struct ow_view *);
int dashboard_refresh(struct ow_view *, const struct dashboard_backend *, unsigned long);
int dashboard_click(struct ow_view *, const struct dashboard_backend *, int, int, unsigned long);
unsigned int demo_glyph(unsigned char, int);
int demo_snapshot(struct ow_view *, const char *);
#endif
