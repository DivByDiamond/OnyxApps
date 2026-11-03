#ifndef OW_PANEL_H
#define OW_PANEL_H
#include "../core/widget.h"
/* One bounded line: kind x y width height binding action |printable text.
 * Types: label, button, progress, status. Atomic replacement on success. */
int ow_panel_load(struct ow_view *, const char *);
#endif
