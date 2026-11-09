# Onyx tablet widget foundation

A portable C99 pixel dashboard, with a **local mock backend**. Genuine OnyxOS
remains an RV64 operating system. This widget core can also run natively inside
a future lightweight tablet host without emulating the OS or its CPU. There is
no Minecraft integration, radio, network connection, TLS, authentication, Lua
binding or chart. The graphical OnyxOS frontend uses the existing framebuffer
and mouse ABI; its service data is still explicitly local mock data.

## Run

From OnyxApps:

```sh
make -C tests/native/tablet test
make -C tests/native/tablet target-test
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
is usable without a GUI. Target `build/otablet.onx` writes the same PPM snapshot by default. On OnyxOS:

```sh
otablet --device
# Or load a declarative panel, then open the framebuffer:
otablet --panel /path/to/energy.panel --device
```

The device frontend opens `/dev/fb0`, reads its existing `0x4600` geometry ioctl,
and writes centered integer-scaled pixels through the framebuffer device. It
supports RGB565 and 32-bit RGB, pitch padding and clipping. Existing syscall 86
reads absolute mouse coordinates. Press and release the left button over the
same action to invoke it once; right click exits. A white square marks the
pointer. It occupies the framebuffer until exit; there is no window manager or
background application lifetime. Unsupported/missing devices return failure.
The compiler sibling checkout supplies the existing syscall header; no new
kernel ABI or compiler changes are required. OnyxOS image assembly already
includes every source application, including otablet. No bundled OC2R image is
updated by building this app.

`make -C tests/native/tablet test` includes a clearly isolated mock-kernel ABI
test of the actual device frontend, including writes, pointer release and error
cleanup. This test does not establish real framebuffer/mouse hardware operation.
The RV64 snapshot path can run under `OnyxCompiller/tools/onx-run`; that runner
does not provide framebuffer hardware. Lua remains deferred until real runtime
instruction/memory limits and a restricted host API are integrated.

## Declarative panels

`presets/energy.panel` is a complete editable example. Each line is:

```text
kind x y width height binding action |PRINTABLE TEXT
```

Kinds are `label`, `button`, `progress`, `status`. Maximum 16 lines, 2048 bytes,
47 printable ASCII text bytes per line and dimensions up to 4096. Load errors
leave the existing view unchanged. Blank lines/comments are not supported.
`--panel FILE` renders a PPM snapshot; add `--device` for OnyxOS graphics.
Bindings in the local demo are 1: connection, 2: machine label, 3: energy percent,
5: last command result. Action 1 toggles the local mock machine. These IDs are
local bindings, not service addresses, permissions or radio credentials.
Custom hosts can assign their own IDs through the portable C API.

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

`service/session.h` defines a separate portable bounded asynchronous adapter
contract for future trusted service transports. Up to eight requests can be
pending. Enqueue means QUEUED; endpoint events explicitly advance ACCEPTED,
RUNNING, COMPLETED or FAILED. Host ticks expire deadlines; disconnect cancels
pending requests, and reconnect never replays them. IDs never wrap/reuse within
a session, and late terminal events are rejected. Reinitializing a session
requires the adapter to discard all prior connections and replies. Completed
slots can be reused; retain external history if needed. Adapters must serialize
callbacks, validate endpoint identities, enforce their own payload/rate bounds,
and call tick before applying results. This interface supplies no network,
authentication, TLS, discovery or live telemetry. The graphical fixture still
uses its synchronous local backend, independently of this future contract.

## Tablet device mode

Run `otablet --device` from the guest console, then select the tablet screen.
This mode displays real monotonic uptime and a local click counter. ADD CLICK
increments that counter; the progress bar shows the counter modulo 101. It does
not report remote energy or radio telemetry. Snapshot/demo modes retain their
explicit mock fixture.

The framebuffer must support GET_INFO (0x4600), process-owned graphics claim
(0x46f0), and release (0x46f1). Device mode accepts RGB565 little endian and
32-bit RGB displays, and centers its 240x112 canvas. On the 256x160 tablet the
canvas starts at (8,24). Pointer syscall 86 supplies absolute pixel coordinates.
Press Q, Escape or the right mouse button to return to the console. All normal
and failed device exits restore terminal settings and release the console;
the kernel also releases ownership when the process exits.
