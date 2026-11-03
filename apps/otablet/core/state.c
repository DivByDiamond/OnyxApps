#include "widget.h"
#include <string.h>
static int valid_text(const char *s) {
    int i;
    if (!s) return 0;
    for (i = 0; i < OW_TEXT; i++) {
        unsigned char c = (unsigned char)s[i];
        if (!c) return 1;
        if (c < 32 || c > 126) return 0;
    }
    return 0;
}
int ow_init(struct ow_view *v, int width, int height) {
    if (!v || width < 1 || height < 1 || width > OW_EXTENT || height > OW_EXTENT)
        return 0;
    memset(v, 0, sizeof(*v));
    v->width = width;
    v->height = height;
    return 1;
}
int ow_add(struct ow_view *v, int kind, int x, int y, int w, int h,
           int binding, int action, const char *text) {
    struct ow_widget *t;
    if (!v || v->count >= OW_MAX || kind < OW_LABEL || kind > OW_STATUS ||
        x < -OW_EXTENT || y < -OW_EXTENT || x > OW_EXTENT || y > OW_EXTENT ||
        w < 1 || h < 1 || w > OW_EXTENT || h > OW_EXTENT ||
        binding < 0 || action < 0 || !valid_text(text)) return -1;
    t = &v->widgets[v->count];
    memset(t, 0, sizeof(*t));
    t->kind = kind; t->x = x; t->y = y; t->w = w; t->h = h;
    t->binding = binding; t->action = action;
    strcpy(t->text, text);
    return v->count++;
}
int ow_update(struct ow_view *v, int binding, const char *text, int value) {
    int i, found = 0;
    char copy[OW_TEXT];
    if (!v || binding < 0 || !valid_text(text)) return 0;
    for (i = 0; i < v->count; i++) {
        if (v->widgets[i].binding == binding) {
            found = 1;
            if (v->widgets[i].kind == OW_PROGRESS && (value < 0 || value > 100))
                return 0;
        }
    }
    strcpy(copy, text);
    for (i = 0; i < v->count; i++) {
        if (v->widgets[i].binding == binding) {
            strcpy(v->widgets[i].text, copy);
            v->widgets[i].value = value;
        }
    }
    return found;
}
void ow_refresh(struct ow_view *v, unsigned long now) {
    if (!v) return;
    v->refreshed = now; v->online = 1; v->has_data = 1;
}
void ow_tick(struct ow_view *v, unsigned long now, unsigned long timeout) {
    if (v) v->online = v->has_data && now >= v->refreshed &&
                       now - v->refreshed < timeout;
}
