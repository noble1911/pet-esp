// Native 56x56 pixel creature: exact integer pixels, no image decoder or scaling blur.
#include "pixel_pet.h"
#include <string.h>
#define W PIXEL_PET_SIZE
#define OUTLINE 0xff3d203dU
#define CREAM   0xffffedb0U
#define BLUSH   0xfff391a8U
static void rect(pixel_pet_art_t *a,int x,int y,int w,int h,uint32_t c)
{
    for(int yy=y;yy<y+h;yy++) for(int xx=x;xx<x+w;xx++)
        if(xx>=0 && xx<W && yy>=0 && yy<W) a->pixels[yy*W+xx]=c;
}
static void oval(pixel_pet_art_t *a,int x,int y,int w,int h,uint32_t c)
{
    for(int yy=0;yy<h;yy++) for(int xx=0;xx<w;xx++) {
        int dx=2*xx-w+1,dy=2*yy-h+1;
        if(dx*dx*h*h+dy*dy*w*w<=w*w*h*h) rect(a,x+xx,y+yy,1,1,c);
    }
}
static void leaf(pixel_pet_art_t *a,int x,int y)
{
    rect(a,x+2,y,5,2,OUTLINE);rect(a,x,y+2,9,5,OUTLINE);
    rect(a,x-2,y+5,8,4,OUTLINE);rect(a,x-3,y+8,4,3,OUTLINE);
    rect(a,x+2,y+2,5,3,0xff7cb957);rect(a,x,y+4,5,3,0xff7cb957);
    rect(a,x-1,y+7,3,2,0xff46965b);
}
void pixel_pet_render(pixel_pet_art_t *a,const Pet *pet,pixel_face_t face,unsigned phase)
{
    static const uint32_t coats[][3]={
        {0xffffdf72,0xffffe991,0xffefb851}, {0xffb7a0e5,0xffd3bdf5,0xff9575c5},
        {0xff8bd6ad,0xffb5e8b9,0xff60b394}, {0xfff6acc7,0xffffcbd6,0xffd982ab},
        {0xffffd498,0xffffe6b3,0xffe4aa70}, {0xff90cce9,0xffbce7f5,0xff68a8d4}
    };
    const uint32_t *c=coats[pet->genes[GENE_BODY_COLOR]%6];
    memset(a->pixels,0,sizeof a->pixels);
    // Sprout silhouette and two little feet are the character's signature.
    leaf(a,29,2);
    if(pet->stage>=PET_STAGE_CHILD) {
        rect(a,21,6,5,5,OUTLINE);rect(a,22,7,3,3,0xff8dce6d);
    }
    rect(a,15,45,8,8,OUTLINE);rect(a,34,45,8,8,OUTLINE);
    rect(a,17,46,4,5,c[2]);rect(a,36,46,4,5,c[2]);
    // A round, soft silhouette with a short sprout; avoid a pointed cone.
    oval(a,8,13,40,37,OUTLINE);
    // Fill only interior pixels: 1-pixel dark contour remains on all sides.
    // Rendering is serialized by the LVGL task; avoid a 3 KB task-stack allocation.
    static uint8_t mask[W*W];
    for(int i=0;i<W*W;i++)mask[i]=a->pixels[i]==OUTLINE;
    for(int y=14;y<49;y++) for(int x=9;x<47;x++) {
        int i=y*W+x;
        if(mask[i]&&mask[i-1]&&mask[i+1]&&mask[i-W]&&mask[i+W])
            a->pixels[i]=y>41?c[2]:y<23?c[1]:c[0];
    }
    rect(a,19,17,5,2,CREAM);rect(a,15,20,3,2,CREAM);
    // Left hand hugs the body; the right hand waves during celebrations.
    oval(a,3,35,8,10,OUTLINE);oval(a,5,36,6,7,c[0]);
    int hand_y=face==PIXEL_HAPPY?23+(phase%2)*2:34;
    oval(a,45,hand_y,8,10,OUTLINE);oval(a,45,hand_y+1,6,7,c[0]);
    // Blush and simple bead eyes stay readable at 3x scale.
    rect(a,13,32,5,4,BLUSH);rect(a,38,32,5,4,BLUSH);
    if(face==PIXEL_SLEEP || face==PIXEL_BLINK) {
        for(int x=18;x<=33;x+=15) {rect(a,x,28,2,1,OUTLINE);rect(a,x+2,29,3,1,OUTLINE);rect(a,x+5,28,1,1,OUTLINE);}
    } else if(face==PIXEL_HAPPY || face==PIXEL_EAT) {
        for(int x=18;x<=33;x+=15) {rect(a,x,28,2,3,OUTLINE);rect(a,x+2,27,3,2,OUTLINE);rect(a,x+5,28,1,3,OUTLINE);}
    } else {
        oval(a,19,26,3,5,OUTLINE);oval(a,34,26,3,5,OUTLINE);
    }
    if(face==PIXEL_SLEEP) {
        rect(a,26,33,4,2,OUTLINE);
    } else if(face==PIXEL_EAT && phase%2==0) {
        rect(a,25,34,6,2,OUTLINE);
    } else {
        rect(a,23,32,10,5,OUTLINE);rect(a,25,37,6,2,OUTLINE);
        rect(a,26,35,5,3,0xffed94b4);
    }
    if(pet->stage>=PET_STAGE_TEEN) {
        rect(a,39,16,3,7,0xfff58dad);rect(a,37,18,7,3,0xfff58dad);rect(a,39,18,3,3,0xffffdf72);
    }
    if(pet->stage>=PET_STAGE_ADULT) {
        rect(a,15,40,26,3,0xffb95886);rect(a,16,40,24,1,0xfff2a0bd);rect(a,37,43,4,5,0xffd3769c);
    }
    if(pet->stage>=PET_STAGE_ELDER) {rect(a,27,41,3,5,CREAM);rect(a,26,42,5,3,CREAM);}
    if(face==PIXEL_EAT) {
        // Apple held by both hands, with a leaf and a cheeky bite notch.
        oval(a,21,38,15,12,OUTLINE);oval(a,23,39,11,9,0xffed5766);
        rect(a,27,35,2,5,OUTLINE);rect(a,29,35,4,2,0xff6eb66c);
        rect(a,25,40,2,3,0xffffd4be);
    }
    if(face==PIXEL_SLEEP) {
        // Patchwork futon: pixel border, pillow, alternating mint/pink patches.
        rect(a,8,37,40,17,OUTLINE);rect(a,10,36,36,17,0xffd6c5f1);
        for(int yy=39;yy<51;yy+=6)for(int xx=11;xx<45;xx+=8)
            rect(a,xx,yy,8,6,((xx/8+yy/6)%2)?0xff83cbb5:0xffe7a4c7);
        rect(a,10,37,36,2,0xffffe8ee);rect(a,11,51,34,1,0xffa18abe);
    }
    a->image=(lv_image_dsc_t){.header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_ARGB8888,.w=W,.h=W,.stride=W*4},.data_size=sizeof a->pixels,.data=(const uint8_t*)a->pixels};
}
const lv_image_dsc_t *pixel_icon(unsigned kind)
{
    static pixel_pet_art_t workspace;
    static uint32_t pixels[7][20*20];
    static lv_image_dsc_t images[7];
    static bool ready;
    if(!ready) {
        for(unsigned k=0;k<7;k++) {
            pixel_pet_art_t *a=&workspace;
            memset(a,0,sizeof *a);
            if(k==0) {
                oval(a,2,6,16,13,OUTLINE);oval(a,4,7,12,10,0xffed5766);
                rect(a,9,2,2,6,OUTLINE);rect(a,11,2,5,3,0xff69ae68);rect(a,5,9,2,4,0xffffc3c0);
            } else if(k==1) {
                oval(a,1,1,18,18,OUTLINE);oval(a,3,3,14,14,0xffffdc75);
                rect(a,10,4,5,6,0xff80d4e1);rect(a,4,10,6,5,0xffed779f);
                rect(a,9,9,3,7,0xfffff2cf);
            } else if(k==2) {
                oval(a,2,1,15,18,OUTLINE);oval(a,4,3,11,14,0xffffdc75);
                // Transparent notch cut from the upper-right quadrant.
                oval(a,9,-1,12,14,0);rect(a,15,11,3,2,OUTLINE);
            } else if(k==3) {
                oval(a,1,8,11,11,OUTLINE);oval(a,3,10,7,7,0xff68c6e8);
                oval(a,10,1,9,9,OUTLINE);oval(a,12,3,5,5,0xffa5e3f7);
                rect(a,4,11,2,2,0xffffffff);rect(a,13,3,2,2,0xffffffff);
            } else if(k==4) {
                static const char *rows[]={
                    ".........##.........", "........####........", "........####........",
                    ".......######.......", ".......######.......", "......########......",
                    ".##################.", "####################", ".##################.",
                    "..################..", "...##############...", "....############....",
                    ".....##########.....", "....############....", "....############....",
                    "...######..######...", "...####......####...", "..###..........###..",
                    "..##............##..", "...................."};
                for(int y=0;y<20;y++)for(int x=0;x<20;x++)if(rows[y][x]=='#') {
                    bool inside=x>0 && x<19 && y>0 && y<19 && rows[y][x-1]=='#' && rows[y][x+1]=='#' && rows[y-1][x]=='#' && rows[y+1][x]=='#';
                    rect(a,x,y,1,1,inside?0xffffdc75:OUTLINE);
                }
            } else if(k==6) {
                oval(a,1,2,10,10,OUTLINE);oval(a,9,2,10,10,OUTLINE);
                rect(a,4,9,12,5,OUTLINE);rect(a,7,14,6,3,OUTLINE);rect(a,9,17,2,2,OUTLINE);
                oval(a,3,4,7,7,0xffed779f);oval(a,10,4,7,7,0xffed779f);
                rect(a,6,9,8,4,0xffed779f);rect(a,8,13,4,2,0xffed779f);
            } else {
                oval(a,7,1,7,18,0xffec85ab);oval(a,1,6,18,8,0xffec85ab);
                oval(a,7,7,7,7,0xffffdc75);
            }
            for(int y=0;y<20;y++) memcpy(&pixels[k][y*20],&a->pixels[y*W],20*sizeof(uint32_t));
            images[k]=(lv_image_dsc_t){.header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_ARGB8888,.w=20,.h=20,.stride=80},.data_size=sizeof pixels[k],.data=(const uint8_t*)pixels[k]};
        }
        ready=true;
    }
    return &images[kind%7];
}
