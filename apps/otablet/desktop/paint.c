#include "desktop.h"
#include "../dashboard.h"
#include <stdio.h>
#include <string.h>
#define RGB(r,g,b) (((r)>>3)<<11 | ((g)>>2)<<5 | ((b)>>3))
#define NAVY RGB(22,38,49)
#define SLATE RGB(43,64,77)
#define LINE RGB(57,83,95)
#define IVORY RGB(239,235,220)
#define PAPER RGB(255,250,232)
#define INK RGB(31,49,58)
#define MUTED RGB(101,118,118)
#define TEAL RGB(62,165,157)
#define AMBER RGB(231,176,78)
static unsigned short *canvas;
static void box(int x,int y,int w,int h,unsigned short c) {
    int endx=x+w,endy=y+h,row,count;unsigned short *dst;
    if(x<0)x=0;
    if(y<0)y=0;
    if(endx>DT_W)endx=DT_W;
    if(endy>DT_H)endy=DT_H;
    for(row=y;row<endy;row++) {
        dst=canvas+row*DT_W+x;count=endx-x;
        while(count-->0)*dst++=c;
    }
}
static void text(int x,int y,const char *s,unsigned short c,int scale,int cap) {
    int i,row,col; unsigned int bits;
    for(i=0;s[i]&&i<cap;i++)for(row=0;row<7;row++) {
        bits=demo_glyph((unsigned char)s[i],row);
        for(col=0;col<5;col++) {
            if(!(bits&(1u<<(4-col)))) continue;
            if(scale==1)canvas[(y+row)*DT_W+x+i*6+col]=c;
            else box(x+(i*6+col)*scale,y+row*scale,scale,scale,c);
        }
    }
}
static void frame(struct dt_rect r,const char *title,int active) {
    box(r.x+2,r.y+3,r.w,r.h,NAVY);
    box(r.x,r.y,r.w,r.h,IVORY);
    box(r.x,r.y,r.w,15,active?TEAL:SLATE);
    text(r.x+6,r.y+4,title,active?NAVY:IVORY,1,24);
    box(r.x+r.w-15,r.y+2,12,11,active?NAVY:LINE);
    text(r.x+r.w-12,r.y+4,"X",IVORY,1,1);
}
static void clock_window(const struct dt_state *s) {
    struct dt_rect r=dt_window(DT_CLOCK); char line[48];
    frame(r,"CLOCK",s->focus==DT_CLOCK);
    text(r.x+9,r.y+24,"UPTIME",MUTED,1,14);
    snprintf(line,sizeof(line),"%02lu:%02lu",s->uptime/60%60,s->uptime%60);
    text(r.x+9,r.y+40,line,INK,2,5);
    snprintf(line,sizeof(line),"%lu HOURS",s->uptime/3600);
    text(r.x+9,r.y+63,line,INK,1,15);
    box(r.x+9,r.y+81,92,3,RGB(207,212,198));
    box(r.x+9,r.y+81,(int)(s->uptime%60)*92/60,3,TEAL);
    text(r.x+9,r.y+90,"SINCE BOOT",MUTED,1,15);
}
static void system_window(const struct dt_state *s) {
    struct dt_rect r=dt_window(DT_SYSTEM);char line[48];
    unsigned long used=s->total_kb>=s->free_kb?s->total_kb-s->free_kb:0;
    frame(r,"SYSTEM",s->focus==DT_SYSTEM);
    text(r.x+8,r.y+24,"ONYX / RISC-V",INK,1,18);
    text(r.x+8,r.y+39,"MEMORY",MUTED,1,17);
    if(s->memory_valid) {
        snprintf(line,sizeof(line),"%lu.%lu / %lu.%lu MB",used/1024,(used%1024)*10/1024,s->total_kb/1024,(s->total_kb%1024)*10/1024);
        text(r.x+8,r.y+51,line,INK,1,18);
        box(r.x+8,r.y+65,106,5,RGB(207,212,198));
        box(r.x+8,r.y+65,(int)(used*106/s->total_kb),5,AMBER);
        snprintf(line,sizeof(line),"FREE %lu.%lu MB",s->free_kb/1024,(s->free_kb%1024)*10/1024);
        text(r.x+8,r.y+81,line,MUTED,1,18);
    } else text(r.x+8,r.y+55,"UNAVAILABLE",MUTED,1,18);
}
static void notes_window(const struct dt_state *s) {
    struct dt_rect r=dt_window(DT_NOTES);int i,col=0,row=0;char c[2];
    frame(r,"NOTES",s->focus==DT_NOTES);
    box(18,42,220,66,PAPER);
    for(i=0;s->notes[i]&&row<6;i++) {
        if(s->notes[i]=='\n') {row++;col=0;continue;}
        c[0]=s->notes[i];c[1]=0;text(20+col*6,45+row*10,c,INK,1,1);
        if(++col==36){row++;col=0;}
    }
    if(s->focus==DT_NOTES&&row<6)box(20+col*6,52+row*10,5,1,TEAL);
    text(20,116,s->saved<0?"SAVE FAILED":s->saved?"SAVED":"TYPE YOUR NOTE",MUTED,1,27);
    box(196,112,42,14,TEAL);text(203,116,"SAVE",NAVY,1,4);
}
static void dock(const struct dt_state *s) {
    static const char *labels[5]={"CLOCK","NOTES","SYSTEM","SNAKE","SHELL"};
    static const unsigned char icons[5][7]={
        {28,34,41,41,33,34,28},{62,34,46,34,46,34,62},
        {4,4,20,20,21,21,21},{30,16,30,2,30,16,30},
        {0,16,8,4,8,16,31}};
    int app,row,col,x;
    box(0,134,256,26,RGB(14,28,37));box(0,134,256,1,LINE);
    for(app=0;app<5;app++) {
        x=8+app*48;
        if(app<3&&(s->open&(1<<(app+1))))box(x+4,157,40,2,TEAL);
        for(row=0;row<7;row++)for(col=0;col<6;col++)
            if(icons[app][row]&(1<<(5-col)))box(x+21+col,138+row,1,1,app==3?AMBER:IVORY);
        text(x+(48-(int)strlen(labels[app])*6)/2,148,labels[app],IVORY,1,7);
    }
}
void dt_paint(const struct dt_state *s,unsigned short *pixels) {
    int x,y,i,id;char line[32];canvas=pixels;
    box(0,0,256,160,NAVY);
    for(y=22;y<133;y+=12)for(x=4;x<256;x+=12)box(x,y,1,1,LINE);
    box(0,0,256,16,SLATE);box(0,15,256,1,LINE);
    text(8,4,"ONYX",IVORY,1,4);box(38,5,3,5,TEAL);
    text(48,4,"DESKTOP",RGB(160,181,181),1,10);
    snprintf(line,sizeof(line),"UP %luS",s->uptime);text(176,4,line,IVORY,1,12);
    if(!s->open) {
        text(47,64,"YOUR WORKSPACE",IVORY,1,26);
        text(38,80,"OPEN AN APP BELOW",RGB(160,181,181),1,30);
    }
    for(i=0;i<3;i++) {
        id=s->order[i];if(!(s->open&(1<<id)))continue;
        if(id==DT_CLOCK)clock_window(s);
        else if(id==DT_SYSTEM)system_window(s);
        else notes_window(s);
    }
    if(s->launch_error) {box(22,118,212,13,AMBER);text(28,121,"SNAKE COULD NOT START",NAVY,1,32);}
    dock(s);
}
