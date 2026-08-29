/* command.c - COMMAND mode (:) implementation: w/q/e/r/set/s. */
#include "vim.h"
/* ── Command mode (:cmd) ───────────────────────────────────────────── */
void run_command(char *cmd) {
    char arg[128];
    char *p;
    arg[0] = 0;
    p = cmd;
    while (*p && *p != ' ') p++;
    if (*p == ' ') {
        *p = 0;
        p++;
        while (*p == ' ') p++;
        strncpy(arg, p, sizeof(arg) - 1);
        arg[sizeof(arg) - 1] = 0;
    }

    if (strcmp(cmd, "w") == 0 || strcmp(cmd, "write") == 0) {
        char *target = arg[0] ? arg : fname;
        if (!arg[0] && !fname[0]) {
            set_msg("E32: No file name");
        } else if (save_to(target) == 0) {
            if (arg[0] && arg[0] != fname[0]) {
                strncpy(fname, arg, sizeof(fname) - 1);
                fname[sizeof(fname) - 1] = 0;
            }
            set_msg("Written.");
        }
    } else if (strcmp(cmd, "q") == 0 || strcmp(cmd, "quit") == 0) {
        if (dirty) {
            set_msg("E37: No write since last change (use :q!)");
        } else {
            quit_flag = 1;
        }
    } else if (strcmp(cmd, "q!") == 0 || strcmp(cmd, "quit!") == 0) {
        quit_flag = 1;
    } else if (strcmp(cmd, "wq") == 0 || strcmp(cmd, "x") == 0) {
        if (fname[0] || arg[0]) {
            if (save_to(arg[0] ? arg : fname) == 0) quit_flag = 1;
        } else {
            set_msg("E32: No file name");
        }
    } else if (strcmp(cmd, "wq!") == 0) {
        save_to(arg[0] ? arg : fname);
        quit_flag = 1;
    } else if (strcmp(cmd, "e") == 0 || strcmp(cmd, "edit") == 0) {
        if (arg[0]) {
            load_file(arg);
        } else {
            set_msg("E32: No file name");
        }
    } else if (strcmp(cmd, "enew") == 0) {
        nlines = 1; llen[0] = 0; lines[0][0] = 0;
        fname[0] = 0; cx = cy = top = 0; dirty = 0;
        set_msg("New buffer");
    } else if (strcmp(cmd, "r") == 0 || strcmp(cmd, "read") == 0) {
        FILE *f;
        static char buf[MAX_LINE + 4];
        if (!arg[0]) { set_msg("E32: file name required"); return; }
        f = fopen(arg, "r");
        if (!f) { set_msg("E484: Can't open file"); return; }
        while (nlines < MAX_LINES && fgets(buf, sizeof(buf), f)) {
            int len = (int)strlen(buf), i;
            while (len > 0 && (buf[len-1]=='\n'||buf[len-1]=='\r')) buf[--len]=0;
            for (i = nlines; i > cy + 1; i--) {
                memcpy(lines[i], lines[i-1], MAX_LINE);
                llen[i] = llen[i-1];
            }
            memcpy(lines[cy+1], buf, len);
            lines[cy+1][len] = 0;
            llen[cy+1] = len;
            nlines++;
            cy++;
        }
        fclose(f);
        dirty = 1;
    } else if (strcmp(cmd, "set") == 0) {
        if (strcmp(arg, "nu") == 0 || strcmp(arg, "number") == 0) show_num = 1;
        else if (strcmp(arg, "nonu") == 0 || strcmp(arg, "nonumber") == 0) show_num = 0;
        else set_msg("Options: nu nonu");
    } else if (cmd[0] >= '0' && cmd[0] <= '9') {
        int n = atoi(cmd);
        if (n > 0) { cy = n - 1; if (cy >= nlines) cy = nlines - 1; cx = 0; }
    } else if (strcmp(cmd, "$") == 0) {
        cy = nlines - 1; cx = 0;
    } else if (strcmp(cmd, "h") == 0 || strcmp(cmd, "help") == 0) {
        raw_disable();
        printf("\x1b[2J\x1b[1;1H");
        printf("vim (OnyxOS) - modal editor\r\n\r\n");
        printf("hjkl w b 0 $ gg G - move      x dd dw - delete\r\n");
        printf("yy p P   - yank/paste        u Ctrl+r - undo/redo\r\n");
        printf("i a I A o O - insert modes    v V - visual\r\n");
        printf("/pat n N  - search            :w :q :wq :q! :e f\r\n");
        printf(":s/old/new/g :%s///  5j 3dd - counts\r\n\r\n");
        printf("Press any key...\r\n");
        fflush(stdout);
        read_key();
        raw_enable();
    } else if (strcmp(cmd, "ver") == 0) {
        set_msg("vim 1.0 (OnyxOS, OnyxCC)");
    } else if (strncmp(cmd, "s/", 2) == 0 || strncmp(cmd, "%s/", 3) == 0) {
        /* :s/old/new/[g] and :%s/old/new/[g] */
        char s_old[64], s_new[64];
        char *body;
        int global = 0, whole = 0, oi = 0, ni = 0, phase = 0;
        body = cmd;
        if (cmd[0] == '%') { whole = 1; body = cmd + 1; }
        body++; /* skip 's' */
        if (*body == '/') body++;
        s_old[0] = s_new[0] = 0;
        {
            char *slash;
            slash = strchr(body, '/');
            if (slash) {
                int n = (int)(slash - body);
                if (n > 63) n = 63;
                memcpy(s_old, body, n);
                s_old[n] = 0;
                body = slash + 1;
                slash = strchr(body, '/');
                if (slash) {
                    n = (int)(slash - body);
                    if (n > 63) n = 63;
                    memcpy(s_new, body, n);
                    s_new[n] = 0;
                    if (strchr(slash + 1, 'g')) global = 1;
                } else {
                    n = (int)strlen(body);
                    if (n > 63) n = 63;
                    memcpy(s_new, body, n);
                    s_new[n] = 0;
                }
            }
        }
        (void)oi; (void)ni; (void)phase;
        if (s_old[0]) {
            int from = whole ? 0 : cy;
            int to = whole ? nlines - 1 : cy;
            int y, reps = 0;
            for (y = from; y <= to && y < nlines; y++) {
                char work[MAX_LINE];
                int wl, off = 0, line_reps = 0;
                memcpy(work, lines[y], MAX_LINE);
                work[llen[y]] = 0;
                wl = llen[y];
                for (;;) {
                    char *hit = strstr(work + off, s_old);
                    int ol = (int)strlen(s_old), nl = (int)strlen(s_new);
                    int pos;
                    if (!hit) break;
                    pos = (int)(hit - work);
                    /* build: work[0..pos) + new + work[pos+ol..] */
                    {
                        char tmp[MAX_LINE];
                        int tl = 0, i;
                        for (i = 0; i < pos; i++) tmp[tl++] = work[i];
                        for (i = 0; i < nl && tl < MAX_LINE - 1; i++) tmp[tl++] = s_new[i];
                        for (i = pos + ol; i < wl && tl < MAX_LINE - 1; i++) tmp[tl++] = work[i];
                        tmp[tl] = 0;
                        memcpy(work, tmp, tl + 1);
                        wl = tl;
                        reps++;
                        line_reps++;
                        off = pos + nl;
                        if (off >= wl) break;
                    }
                    if (!global) break;
                }
                if (line_reps > 0) {
                    memcpy(lines[y], work, wl);
                    lines[y][wl] = 0;
                    llen[y] = wl;
                    dirty = 1;
                }
            }
            {
                char m[64];
                snprintf(m, sizeof(m), "%d substitution%s", reps,
                         reps == 1 ? "" : "s");
                set_msg(m);
            }
        }
    } else {
        set_msg("E492: Not an editor command");
    }
}
