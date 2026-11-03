#include "dashboard.h"
#include <string.h>
void dashboard_init(struct ow_view *v) {
    ow_init(v, 240, 112);
    ow_add(v, OW_LABEL, 8, 8, 224, 12, 0, 0, "ONYX TABLET / MOCK DATA");
    ow_add(v, OW_STATUS, 8, 26, 76, 10, 1, 0, "");
    ow_add(v, OW_LABEL, 88, 26, 144, 10, 2, 0, "WAITING");
    ow_add(v, OW_PROGRESS, 8, 44, 224, 12, 3, 0, "");
    ow_add(v, OW_BUTTON, 8, 66, 104, 16, 4, 1, "TOGGLE MACHINE");
    ow_add(v, OW_LABEL, 8, 94, 224, 10, 5, 0, "NO ACTION");
}
int dashboard_refresh(struct ow_view *v, const struct dashboard_backend *b,
                      unsigned long now) {
    struct dashboard_sample sample;
    int i;
    memset(&sample, 0, sizeof(sample));
    if (!v || !b || !b->fetch || !b->fetch(b->ctx, &sample)) return 0;
    if (sample.energy < 0 || sample.energy > 100) return 0;
    for (i = 0; i < OW_TEXT; i++) {
        unsigned char c = (unsigned char)sample.machine[i];
        if (!c) break;
        if (c < 32 || c > 126) return 0;
    }
    if (i == OW_TEXT) return 0;
    ow_update(v, 2, sample.machine, 0);
    ow_update(v, 3, "", sample.energy);
    ow_refresh(v, now);
    return 1;
}
int dashboard_click(struct ow_view *v, const struct dashboard_backend *b,
                    int x, int y, unsigned long request) {
    int action, result;
    if (!v || !v->online || !b || !b->action || !request) return 0;
    action = ow_pointer(v, x, y);
    if (!action) return 0;
    result = b->action(b->ctx, action, request);
    if (result < 0 || result > 2) result = 0;
    ow_update(v, 5, result == 2 ? "COMPLETED" : result == 1 ? "ACCEPTED / PENDING" : "REJECTED", 0);
    return result;
}
