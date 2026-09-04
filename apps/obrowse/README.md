 ## Kernel limits inherited from `ohttp`
 
 - Plain HTTP only. No TLS, no `listen/accept`.
-- `tcp_recv` has no clean EOF signal (returns `ENOENT` for "no data"
-  and `EINVAL` after TIMEWAIT); `obrowse` trusts `Content-Length`
-  and falls back to "read until slot gone" for chunked/legacy.
- `tcp_recv` has a clean EOF signal as of OnyxKernel 2026-09-04
  (`Ok(0)` when the peer has sent FIN and the recv buffer is drained,
  see OnyxKernel/todo.md). `obrowse/net/fetch.c` reads until `Ok(0)`
  for responses without `Content-Length` (chunked/legacy) and falls
  back to "read until EINVAL (slot gone)" only as a safety net for
  pre-fix kernels. `ohttp` (older sibling) still trusts
  `Content-Length` for the body length, but that workaround is no
  longer required.
 - `usleep()`/`nanosleep()` hangs on this kernel (confirmed live,
   see `OnyxKernel/todo.md`); `net/fetch.c` busy-spins between recv
   attempts.
   `USER_TOP` proximity on the initial user stack pointer would
   push a stack array past the syscall `user_ptr_ok` check.
 
-## Not done (stages C+)
## Not done (stages D+)
 
-- Bigger recv buffers / clean EOF (kernel-side, separate repo)
- Bigger recv buffers (kernel-side: BUF_SIZE 2 KiB → 16 KiB+ would
  let a single recv() pull more of the body per call) — separate
  OnyxKernel commit if anyone gets to it
 - Chunked transfer-encoding (currently any page without
-  `Content-Length` falls back to "read until EINVAL")
  `Content-Length` falls back to "read until Ok(0)" — works on the
  fixed kernel, but does not parse the chunk framing)
 - Form submission, redirects (3xx), cookies
 - TLS / https:// (would need kernel TLS — unlikely for v0.6)
