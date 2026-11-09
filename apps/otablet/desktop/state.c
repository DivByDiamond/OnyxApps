#define _POSIX_C_SOURCE 200809L
#include "desktop.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static int contains(struct dt_rect r, int x, int y) {
    return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}
struct dt_rect dt_window(int id) {
    struct dt_rect r;
    r.x = id == DT_SYSTEM ? 126 : id == DT_NOTES ? 10 : 8;
    r.y = id == DT_NOTES ? 22 : 24;
    r.w = id == DT_SYSTEM ? 122 : id == DT_NOTES ? 236 : 110;
    r.h = id == DT_NOTES ? 110 : 102;
    return r;
}
void dt_open(struct dt_state *s, int id) {
    int i, j;
    if (id < 1 || id > 3) return;
    for (i = 0; i < 3; i++) if (s->order[i] == id) {
        for (j = i; j < 2; j++) s->order[j] = s->order[j + 1];
        break;
    }
    s->order[2] = id; s->open |= 1 << id; s->focus = id; s->dirty = 1;
}
void dt_init(struct dt_state *s) {
    memset(s, 0, sizeof(*s));
    s->order[0] = 1; s->order[1] = 2; s->order[2] = 3;
    dt_open(s, DT_CLOCK); dt_open(s, DT_SYSTEM);
}
int dt_hit(const struct dt_state *s, int x, int y) {
    int i, id;
    struct dt_rect r;
    if (x < 0 || y < 0 || x >= DT_W || y >= DT_H) return 0;
    if (y >= 136 && x >= 8 && x < 248) return (x - 8) / 48 + 1;
    for (i = 2; i >= 0; i--) {
        id = s->order[i];
        if (!(s->open & (1 << id))) continue;
        r = dt_window(id);
        if (!contains(r, x, y)) continue;
        if (x >= r.x + r.w - 16 && y < r.y + 15) return 100 + id;
        if (id == DT_NOTES && x >= 196 && y >= 113) return DT_SAVE;
        return 10 + id;
    }
    return 0;
}
int dt_action(struct dt_state *s, int action) {
    int id, i;
    if (action >= 1 && action <= 3) dt_open(s, action);
    else if (action >= 11 && action <= 13) dt_open(s, action - 10);
    else if (action >= 101 && action <= 103) {
        id = action - 100; s->open &= ~(1 << id); s->focus = 0;
        for (i = 2; i >= 0; i--) if (s->open & (1 << s->order[i])) {
            s->focus = s->order[i]; break;
        }
        s->dirty = 1;
    }
    return action == DT_SNAKE || action == DT_CONSOLE || action == DT_SAVE ? action : 0;
}
int dt_key(struct dt_state *s, int key) {
    size_t n = strlen(s->notes);
    int row=0,col=0;size_t i;
    if (key == 27) {
        if (s->focus) { dt_action(s, 100 + s->focus); return 0; }
        return DT_CONSOLE;
    }
    if (s->focus != DT_NOTES) return key == 'q' || key == 'Q' ? DT_CONSOLE : 0;
    if ((key == 8 || key == 127) && n) s->notes[--n] = 0;
    else if (n < DT_NOTE - 1 && (key == 10 || key == 13 || (key >= 32 && key <= 126))) {
        for(i=0;i<n;i++) {
            if(s->notes[i]=='\n'){row++;col=0;}
            else if(++col==36){row++;col=0;}
        }
        if(row>=6||((key==10||key==13)&&row>=5))return 0;
        s->notes[n++] = key == 13 ? 10 : key; s->notes[n] = 0;
    } else return 0;
    s->dirty = s->note_dirty = 1; s->saved = 0;
    return 0;
}
int dt_load(struct dt_state *s, const char *path) {
    FILE *file = fopen(path, "rb");
    unsigned long n;
    if (!file) return 0;
    n = fread(s->notes, 1, DT_NOTE - 1, file); s->notes[n] = 0;
    if (ferror(file)) { fclose(file); return 0; }
    fclose(file); s->saved = 1; s->note_dirty = 0; s->dirty = 1;
    return 1;
}
int dt_save(struct dt_state *s, const char *path) {
    FILE *file;
    size_t n = strlen(s->notes);
    int okay;
    file = fopen(path, "wb");
    if (!file) { s->saved = -1; s->dirty = 1; return 0; }
    okay = fwrite(s->notes, 1, n, file) == n && !ferror(file);
    if (fclose(file) != 0) okay = 0;
    s->saved = okay ? 1 : -1; s->dirty = 1;
    if (okay) s->note_dirty = 0;
    return okay;
}
void dt_memory_line(struct dt_state *s,const char *line) {
    const char *colon=strchr(line,':');unsigned long value;
    if(!colon)return;
    value=strtoul(colon+1,0,10);
    if(!strncmp(line,"MemTotal",8)) {s->total_kb=value;s->memory_valid=value!=0;}
    if(!strncmp(line,"MemFree",7))s->free_kb=value;
}
void dt_probe(struct dt_state *s) {
    struct timespec now;
    FILE *file;
    char line[128];
    unsigned long old = s->uptime;
    if (clock_gettime(CLOCK_MONOTONIC, &now) == 0) s->uptime = now.tv_sec;
    if (old != s->uptime) s->dirty = 1;
    file = fopen("/proc/meminfo", "rb");
    if (!file) { s->memory_valid = 0; return; }
    s->memory_valid = 0;
    while (fgets(line,sizeof(line),file))dt_memory_line(s,line);
    fclose(file);
}
