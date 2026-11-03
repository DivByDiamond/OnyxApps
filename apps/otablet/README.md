# Onyx tablet widget foundation

A portable C99 pixel dashboard, with a **local mock backend**. Genuine OnyxOS
remains an RV64 operating system. This widget core can also run natively inside
a future lightweight tablet host without emulating the OS or its CPU. There is
no Minecraft integration, radio, network connection, TLS, authentication, Lua
binding, chart or production device frontend in this increment.

## Run

From OnyxApps:

```sh
make -C tests/native/tablet test
./tests/native/tablet/demo /tmp/tablet.ppm
./tests/native/tablet/demo /tmp/stopped.ppm 10 70
./tests/native/tablet/demo /tmp/offline.ppm 10 70 offline
python3 apps/otablet/tools/preview.py
make otablet ONYXCC=../OnyxCompiller/onyxcc
```

The optional preview needs Python with Tk. It displays the actual C-generated
RGB565 dashboard converted to PPM at 3x scale and forwards left mouse clicks
back into the same persistent native demo process. Click TOGGLE MACHINE;
Simulate timeout retains the last values and disables actions until Refresh.
The protocol advances logical ticks per command; it is a deterministic fixture,
not a real clock or transport. The snapshot CLI accepts pixel coordinates and
is usable without a GUI. Target `build/otablet.onx` writes the same PPM snapshot;
it does not draw to the real device or read its mouse yet.

Existing applications primarily use ANSI framebuffer terminals. The kernel
already provides framebuffer drivers and SYS_mouse_read, and Lua has a custom
runtime build; neither is needed by this standalone portable core. A future
OnyxOS frontend should adapt existing framebuffer/input facilities without
introducing another kernel ABI. Lua can bind the same bounded API later.

## Make widgets

```c
#include "core/widget.h"
struct ow_view view;
ow_init(&view, 240, 112);
ow_add(&view, OW_LABEL, 8, 8, 120, 12, 1, 0, "ENERGY");
ow_add(&view, OW_PROGRESS, 8, 26, 220, 10, 2, 0, "");
ow_add(&view, OW_BUTTON, 8, 44, 100, 16, 3, 7, "START");
ow_add(&view, OW_STATUS, 8, 66, 100, 10, 4, 0, "");
ow_update(&view, 2, "", 72);
ow_refresh(&view, 100);           /* complete validated sample, monotonic tick */
ow_tick(&view, 106, 5);          /* offline, preserves last data */
int action = ow_pointer(&view, 12, 48); /* 7: dispatch on press edge */
```

Check return values: init/update return boolean, add returns index or -1.
The initialized view and its widget array are API-owned: do not modify fields
or counts directly. All calls are single-threaded; hosts must serialize access.
Geometry is pixel based, half-open, clipped to the viewport. Later widgets paint
above earlier ones and occlude their input, even if they are labels. Dispatch
one click per press edge; pointer lookup itself is stateless. The dashboard
adapter also rejects actions when offline; raw lookup intentionally does not.

16 widgets maximum; 47 printable ASCII bytes per label; extent 1..4096 pixels;
binding/action IDs are nonnegative, action 0 means none. Text controls and
unterminated/overlong strings are rejected. Pass readable NUL-terminated strings
or arrays of at least OW_TEXT bytes. Shared-binding updates validate before
changing any widget. Progress rejects values outside 0..100. Caller owns all
storage; the core does not allocate, parse untrusted wire data, or perform I/O.

`ow_paint` emits clipped rectangle fills and text with an explicit glyph clip
rectangle. A host can implement those callbacks with its own graphics API.
`graphics/raster.h` provides bounded RGB565 painting to caller-owned memory,
with an injected 5x7 glyph callback; the demo font supports uppercase letters.
Allocate at least `stride * height` pixels. Raster and view sizes may differ:
both viewport and destination clipping are enforced. Clear the background before
repainting a changed layout. The raster paints supplied widget content only.

## Backend and next integration

`dashboard_backend` injects sample fetch and action callbacks, with host context.
Failed/malformed samples do not change data or freshness. The host must call tick regularly (even when fetch fails)
with a monotonic clock and timeout; backward time expires freshness safely.
Actions carry a positive request ID supplied by the host. Return 1 means
accepted/pending, 2 means completed, 0 means rejected. Accepted is never shown
as completed. The demo toggle finishes synchronously; it has no async result
queue, deduplication or request tracking. Those belong in a future adapter.

The next tablet adapter should deliver opaque short radio addresses, service,
channel, bounded payload and request ID, enforce permissions and bounded queues,
and match asynchronous acknowledgments/results before updating the UI. Preserve
existing Ethernet/TCP/IP separately. Authentication/TLS must use a real approved
transport stack, not custom cryptography. These interfaces implement none of
that transport. A future Minecraft host can map mouse coordinates to pixels,
forward actions, apply bounded samples, and paint via callbacks or upload RGB565.
