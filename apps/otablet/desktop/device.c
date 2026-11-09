#include "desktop.h"
#include "../device/display.h"
#include <stdio.h>
#include <string.h>
#ifdef __onyx__
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include "../../../../OnyxCompiller/include/sys/syscalls.h"
#define NOTE_PATH "/users/tablet-notes.txt"
struct dt_pointer { unsigned short x,y; unsigned char buttons,pad; };
static unsigned short pixels[DT_W*DT_H];
static unsigned char scanline[4096*4];
static struct termios original;
static int raw_begin(void) {
    struct termios raw;
    if(tcgetattr(0,&original))return 0;
    raw=original;cfmakeraw_apply(&raw);raw.c_cc[VMIN]=0;raw.c_cc[VTIME]=0;
    return tcsetattr(0,0,&raw)==0;
}
static int position(const struct ow_screen *s,int x,int y,int *px,int *py) {
    int scale=s->width/DT_W,left,top;
    if(scale>(int)s->height/DT_H)scale=s->height/DT_H;
    left=((int)s->width-DT_W*scale)/2;top=((int)s->height-DT_H*scale)/2;
    if(x<left||y<top||x>=left+DT_W*scale||y>=top+DT_H*scale)return 0;
    *px=(x-left)/scale;*py=(y-top)/scale;return 1;
}
static int present(int fd,const struct ow_screen *screen,struct dt_state *s,int mx,int my) {
    static const unsigned char arrow[7]={16,24,28,30,28,10,2};
    int x,y,px,py,k,bytes=screen->bpp/8;unsigned int color;
    if(s->dirty)dt_paint(s,pixels);
    if(screen->width==DT_W&&screen->height==DT_H&&screen->bpp==16&&screen->pitch==DT_W*2) {
        unsigned short saved[35];int index=0;long written;unsigned long offset=0;
        for(y=0;y<7;y++)for(x=0;x<5;x++) {
            saved[index++]=(mx+x<DT_W&&my+y<DT_H)?pixels[(my+y)*DT_W+mx+x]:0;
            if(mx+x<DT_W&&my+y<DT_H&&(arrow[y]&(1<<(4-x))))pixels[(my+y)*DT_W+mx+x]=65535;
        }
        if(lseek(fd,0,0)<0)return 0;
        while(offset<sizeof(pixels)) {
            written=write(fd,(unsigned char *)pixels+offset,sizeof(pixels)-offset);
            if(written<=0)return 0;
            offset+=(unsigned long)written;
        }
        index=0;
        for(y=0;y<7;y++)for(x=0;x<5;x++) {
            if(mx+x<DT_W&&my+y<DT_H)pixels[(my+y)*DT_W+mx+x]=saved[index];
            index++;
        }
        s->dirty=0;return 1;
    }
    for(y=0;y<(int)screen->height;y++) {
        memset(scanline,0,sizeof(scanline));
        for(x=0;x<(int)screen->width;x++) {
            color=0;
            if(position(screen,x,y,&px,&py))color=pixels[py*DT_W+px];
            if(x>=mx&&x<mx+5&&y>=my&&y<my+7&&
               (arrow[y-my]&(1<<(4-(x-mx)))))color=65535;
            if(screen->bpp==32)color=ow_screen_rgb(color);
            for(k=0;k<bytes;k++)scanline[x*bytes+k]=color>>(k*8);
        }
        if(lseek(fd,y*screen->pitch,0)<0||
           write(fd,scanline,screen->width*bytes)!=(long)(screen->width*bytes))return 0;
    }
    s->dirty=0;return 1;
}
static void save_notes(struct dt_state *s) {
    if(s->note_dirty&&dt_save(s,NOTE_PATH)) {
        printf("OTABLET_NOTES_SAVED=%lu\n",(unsigned long)strlen(s->notes));fflush(stdout);
    }
}
static int launch_snake(int fd,struct dt_state *s) {
    char *argv[65];int i,pid,status=0,result;
    for(i=0;i<65;i++)argv[i]=0;
    argv[0]="osnake";
    tcsetattr(0,0,&original);
    _onyx_ioctl(fd,OW_FB_RELEASE_GRAPHICS,0);
    pid=(int)_onyx_spawn("/bin/osnake",argv,1);
    if(pid>=0) {
        printf("OTABLET_APP_STARTED=osnake\n");fflush(stdout);
        result=(int)_onyx_waitpid(pid,&status,0);
        s->launch_error=result<0||status!=0;
        printf("OTABLET_APP_RETURN=osnake STATUS=%d\n",status);fflush(stdout);
    } else s->launch_error=1;
    if(_onyx_ioctl(fd,OW_FB_CLAIM_GRAPHICS,0)<0||!raw_begin())return 0;
    s->dirty=1;return 1;
}
int desktop_device(void) {
    struct ow_screen screen;struct dt_state state;struct dt_pointer pointer;
    int fd,raw=0,result=0,down=0,armed=0,x,y,action,key_action;
    int lastx=-1,lasty=-1,tick=0,count,first=1;unsigned char key;
    struct timespec start,end;long elapsed;
    fd=open("/dev/fb0",O_RDWR);if(fd<0)return 0;
    if(_onyx_ioctl(fd,0x4600,(long)&screen)<0||!ow_screen_valid(&screen)||
       screen.width<DT_W||screen.height<DT_H) {close(fd);return 0;}
    if(_onyx_ioctl(fd,OW_FB_CLAIM_GRAPHICS,0)<0){close(fd);return 0;}
    raw=raw_begin();if(!raw)goto done;
    dt_init(&state);dt_load(&state,NOTE_PATH);dt_probe(&state);
    printf("OTABLET_READY %ux%u\nOTABLET_DESKTOP_READY\n",screen.width,screen.height);fflush(stdout);
    for(;;) {
        if(_onyx_syscall1(SYS_mouse_read,(long)&pointer)<0)break;
        action=0;
        if(pointer.buttons&2){result=1;break;}
        if((pointer.buttons&1)&&!down)
            armed=position(&screen,pointer.x,pointer.y,&x,&y)?dt_hit(&state,x,y):0;
        if(!(pointer.buttons&1)&&down) {
            if(armed&&position(&screen,pointer.x,pointer.y,&x,&y)&&dt_hit(&state,x,y)==armed) {
                action=dt_action(&state,armed);
                printf("OTABLET_ACTION=%d\n",armed);fflush(stdout);
                if(armed==102)save_notes(&state);
            }
            armed=0;
        }
        down=pointer.buttons&1;
        for(count=0;count<64&&read(0,&key,1)==1;count++) {
            key_action=dt_key(&state,key);if(key_action)action=key_action;
        }
        if(action==DT_CONSOLE){result=1;break;}
        if(action==DT_SAVE){state.note_dirty=1;save_notes(&state);}
        if(action==DT_SNAKE) {
            save_notes(&state);
            if(!launch_snake(fd,&state))break;
            down=armed=0;lastx=lasty=-1;
        }
        if(++tick>=25) {dt_probe(&state);save_notes(&state);tick=0;}
        if(state.dirty||lastx!=pointer.x||lasty!=pointer.y) {
            if(first)clock_gettime(CLOCK_MONOTONIC,&start);
            if(!present(fd,&screen,&state,pointer.x,pointer.y))break;
            if(first) {
                clock_gettime(CLOCK_MONOTONIC,&end);
                elapsed=(end.tv_sec-start.tv_sec)*1000+(end.tv_nsec-start.tv_nsec)/1000000;
                printf("OTABLET_DESKTOP_PAINTED MS=%ld\n",elapsed);fflush(stdout);first=0;
            }
            lastx=pointer.x;lasty=pointer.y;
        }
        usleep(20000);
    }
    save_notes(&state);
done:
    if(raw)tcsetattr(0,0,&original);
    _onyx_ioctl(fd,OW_FB_RELEASE_GRAPHICS,0);close(fd);
    puts("OTABLET_EXIT");fflush(stdout);return result;
}
#else
int desktop_device(void) { return 0; }
#endif
