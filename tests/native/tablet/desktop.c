#include "desktop/desktop.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
static void preview(struct dt_state *s,const char *path) {
    static unsigned short pixels[DT_W*DT_H];
    FILE *f=fopen(path,"wb");int i;unsigned int c;unsigned char rgb[3];
    assert(f);dt_paint(s,pixels);fprintf(f,"P6\n256 160\n255\n");
    for(i=0;i<DT_W*DT_H;i++) {
        c=pixels[i];rgb[0]=((c>>11)&31)*255/31;rgb[1]=((c>>5)&63)*255/63;rgb[2]=(c&31)*255/31;
        assert(fwrite(rgb,1,3,f)==3);
    }
    assert(!fclose(f));
}
int main(void) {
    struct dt_state s,loaded;char path[128];int i,action;
    dt_init(&s);dt_probe(&s);preview(&s,"/tmp/onyx-desktop-preview.ppm");
    dt_memory_line(&s,"MemTotal\t: 16384 kB\n");
    dt_memory_line(&s,"MemFree\t\t: 4096 kB\n");
    assert(s.memory_valid&&s.total_kb==16384&&s.free_kb==4096);
    assert((s.open&(1<<DT_CLOCK))&&(s.open&(1<<DT_SYSTEM)));
    assert(dt_hit(&s,108,30)==101);dt_action(&s,101);assert(!(s.open&(1<<DT_CLOCK)));
    assert(dt_hit(&s,32,148)==DT_CLOCK);dt_action(&s,DT_CLOCK);assert(s.open&(1<<DT_CLOCK));
    assert(dt_hit(&s,80,148)==DT_NOTES);dt_action(&s,DT_NOTES);
    assert(s.focus==DT_NOTES&&dt_hit(&s,236,28)==102);
    for(i=0;"Hello tablet"[i];i++)assert(!dt_key(&s,"Hello tablet"[i]));
    assert(!dt_key(&s,'q')&&!strcmp(s.notes,"Hello tabletq"));dt_key(&s,127);
    snprintf(path,sizeof(path),"/tmp/onyx-desktop-note-%ld.txt",(long)getpid());
    assert(dt_hit(&s,216,119)==DT_SAVE);assert(dt_action(&s,DT_SAVE)==DT_SAVE);
    assert(dt_save(&s,path)&&s.saved==1&&!s.note_dirty);
    dt_init(&loaded);assert(dt_load(&loaded,path)&&!strcmp(loaded.notes,"Hello tablet"));
    unlink(path);preview(&s,"/tmp/onyx-desktop-notes-preview.ppm");
    dt_action(&s,102);assert(!(s.open&(1<<DT_NOTES)));
    dt_action(&s,DT_NOTES);assert(!strcmp(s.notes,"Hello tablet"));
    assert(dt_key(&s,27)==0&&!(s.open&(1<<DT_NOTES)));
    action=dt_hit(&s,176,148);assert(action==DT_SNAKE&&dt_action(&s,action)==DT_SNAKE);
    assert(dt_hit(&s,224,148)==DT_CONSOLE);
    assert(dt_hit(&s,65535,65535)==0);
    assert(dt_key(&s,'q')==DT_CONSOLE);
    dt_open(&loaded,DT_NOTES);loaded.notes[0]=0;
    for(i=0;i<5;i++)dt_key(&loaded,'\n');
    dt_key(&loaded,'\n');assert(strlen(loaded.notes)==5);
    for(i=0;i<36;i++)dt_key(&loaded,'A');
    dt_key(&loaded,'B');assert(strlen(loaded.notes)==41);
    puts("desktop: window close/reopen, dock launch, notes edit/persistence and exit passed");
    return 0;
}
