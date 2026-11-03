#ifndef ONYX_WIDGET_H
#define ONYX_WIDGET_H
/* Portable C99, no allocation or I/O. Coordinates are pixels. */
#define OW_MAX 16
#define OW_TEXT 48
#define OW_EXTENT 4096
enum ow_kind { OW_LABEL, OW_BUTTON, OW_PROGRESS, OW_STATUS };
struct ow_widget {
    int kind, x, y, w, h, binding, action, value;
    char text[OW_TEXT];
};
struct ow_view {
    int width, height, count, online, has_data;
    unsigned long refreshed;
    struct ow_widget widgets[OW_MAX];
};
int ow_init(struct ow_view *v, int width, int height);
/* Returns widget index, or -1; invalid text/geometry leaves view unchanged. */
int ow_add(struct ow_view *v, int kind, int x, int y, int w, int h,
           int binding, int action, const char *text);
/* Updates all widgets sharing a binding. Progress is 0..100. Atomic reject. */
int ow_update(struct ow_view *v, int binding, const char *text, int value);
void ow_refresh(struct ow_view *v, unsigned long now);
void ow_tick(struct ow_view *v, unsigned long now, unsigned long timeout);
/* Topmost visible button, positive opaque action or 0. Host debounces input. */
int ow_pointer(const struct ow_view *v, int x, int y);
/* Painter receives clipped fill rectangles. Text receives original origin and
 * a clip rectangle: the host must clip glyph pixels to that rectangle. */
struct ow_painter {
    void (*fill)(void *, int, int, int, int, unsigned short);
    void (*text)(void *, int, int, const char *, int, int, int, int, unsigned short);
};
void ow_paint(const struct ow_view *, const struct ow_painter *, void *);
#endif
