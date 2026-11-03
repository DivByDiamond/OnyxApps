#include "widget.h"
static int contains(const struct ow_widget *t, int x, int y) {
    return x >= t->x && y >= t->y && x - t->x < t->w && y - t->y < t->h;
}
int ow_pointer(const struct ow_view *v, int x, int y) {
    int i;
    if (!v || x < 0 || y < 0 || x >= v->width || y >= v->height) return 0;
    for (i = v->count - 1; i >= 0; i--) {
        if (contains(&v->widgets[i], x, y))
            return v->widgets[i].kind == OW_BUTTON ? v->widgets[i].action : 0;
    }
    return 0;
}
