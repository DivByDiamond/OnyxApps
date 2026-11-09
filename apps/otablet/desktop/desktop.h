#ifndef ONYX_DESKTOP_H
#define ONYX_DESKTOP_H
#define DT_W 256
#define DT_H 160
#define DT_NOTE 217
#define DT_CLOCK 1
#define DT_NOTES 2
#define DT_SYSTEM 3
#define DT_SNAKE 4
#define DT_CONSOLE 5
#define DT_SAVE 201
struct dt_rect { int x, y, w, h; };
struct dt_state {
    int open, order[3], focus, dirty, note_dirty, saved, launch_error;
    unsigned long uptime, total_kb, free_kb;
    int memory_valid;
    char notes[DT_NOTE];
};
void dt_init(struct dt_state *);
void dt_open(struct dt_state *, int);
int dt_hit(const struct dt_state *, int, int);
int dt_action(struct dt_state *, int);
int dt_key(struct dt_state *, int);
struct dt_rect dt_window(int);
void dt_paint(const struct dt_state *, unsigned short *);
int dt_load(struct dt_state *, const char *);
int dt_save(struct dt_state *, const char *);
void dt_probe(struct dt_state *);
void dt_memory_line(struct dt_state *, const char *);
int desktop_device(void);
#endif
