#include "../core/widget.h"
void ow_paint(const struct ow_view *v, const struct ow_painter *p, void *ctx) {
    int i;
    if (!v || !p || !p->fill || !p->text) return;
    for (i = 0; i < v->count; i++) {
        const struct ow_widget *t = &v->widgets[i];
        int x = t->x < 0 ? 0 : t->x;
        int y = t->y < 0 ? 0 : t->y;
        int r = t->x + t->w < v->width ? t->x + t->w : v->width;
        int b = t->y + t->h < v->height ? t->y + t->h : v->height;
        unsigned short color = t->kind == OW_BUTTON ? 0x2457 : 0x1084;
        const char *text = t->kind == OW_STATUS ? (v->online ? "ONLINE" : "OFFLINE") : t->text;
        if (x >= r || y >= b) continue;
        p->fill(ctx, x, y, r - x, b - y, color);
        if (t->kind == OW_PROGRESS) {
            int end = t->x + t->w * t->value / 100;
            if (end > r) end = r;
            if (end > x) p->fill(ctx, x, y, end - x, b - y, 0x07e0);
        } else p->text(ctx, t->x, t->y, text, x, y, r - x, b - y, 0xffff);
    }
}
