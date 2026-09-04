/*
 * ui/error.c - error overlay for obrowse.
 *
 * The TUI shows a red-on-white banner with the kind ("fetch" / "reload")
 * and the error, the failed URL, and a short hint. Two cases:
 *   - http_status > 0: HTTP error (4xx/5xx). The banner shows the code
 *     and a one-line label (404 -> "Page not found", 5xx -> "Server-side
 *     error", etc.). Redirects (3xx) are also reported here because
 *     obrowse does not follow them yet.
 *   - http_status == 0: network error. The banner shows the errno
 *     (EAGAIN -> timeout, EIO -> DNS or I/O failure, EINVAL ->
 *     connection gone mid-fetch).
 *
 * The user is expected to call key_poll() in a loop after ui_show_error
 * and then dismiss the overlay (typically by clearing the screen and
 * re-rendering the page).
 *
 * Layout: max 4 files per folder, max 200 lines per file. This is the
 * only file in ui/, paired with the obrowse.h declaration of
 * ui_show_error.
 */
#include <stdio.h>
#include "../obrowse.h"

void ui_show_error(const char *kind, const char *url, int http_status, int my_errno) {
    printf("\x1b[2J\x1b[1;1H");
    if (http_status > 0) {
        printf("\x1b[7;31m %s: HTTP %d \x1b[0m\r\n", kind, http_status);
        printf("URL: %s\r\n", url);
        if (http_status == 404) {
            printf("Page not found.\r\n");
        } else if (http_status == 403) {
            printf("Forbidden.\r\n");
        } else if (http_status == 401) {
            printf("Authentication required.\r\n");
        } else if (http_status == 400) {
            printf("Bad request.\r\n");
        } else if (http_status >= 500 && http_status < 600) {
            printf("Server-side error.\r\n");
        } else if (http_status >= 300 && http_status < 400) {
            printf("Redirect (obrowse does not follow 3xx).\r\n");
        } else {
            printf("Unexpected HTTP status.\r\n");
        }
        return;
    }
    /* Network error: my_errno carries the kernel errno. */
    printf("\x1b[7;31m %s failed (errno %d) \x1b[0m\r\n", kind, my_errno);
    printf("URL: %s\r\n", url);
    /* OnyxKernel errno values (POSIX, translated by Errno::to_posix). */
    if (my_errno == 11 /* EAGAIN */) {
        printf("Connection refused or timed out.\r\n");
    } else if (my_errno == 5 /* EIO */) {
        printf("Network I/O error or DNS lookup failed.\r\n");
    } else if (my_errno == 22 /* EINVAL */) {
        printf("Connection gone (peer closed during fetch).\r\n");
    } else if (my_errno == 13 /* EACCES */) {
        printf("Permission denied.\r\n");
    } else if (my_errno == 12 /* ENOMEM */) {
        printf("Out of memory (kernel connection table full).\r\n");
    } else {
        printf("Network error.\r\n");
    }
}
