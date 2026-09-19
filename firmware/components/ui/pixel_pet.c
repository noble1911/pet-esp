// Full illustrated sprite frames, compiled to flash; no runtime image decoder.
#include "pixel_pet.h"
#include <string.h>
#include "pet_sprites.h"
#include "pet_icons.h"
#define W PIXEL_PET_SIZE
#define OUTLINE 0xff3d203dU
#define CREAM   0xffffedb0U
#define BLUSH   0xfff391a8U
static void rect(pixel_pet_art_t *a,int x,int y,int w,int h,uint32_t c)
{
    for(int yy=y;yy<y+h;yy++) for(int xx=x;xx<x+w;xx++)
        if(xx>=0 && xx<W && yy>=0 && yy<W) a->pixels[yy*W+xx]=c;
}
void pixel_pet_render(pixel_pet_art_t *a,const Pet *pet,pixel_face_t face,unsigned phase,pixel_food_t food)
{
    pet_sprite_id_t id=SPRITE_IDLE;
    switch(face) {
    case PIXEL_IDLE: id=phase%28<7?SPRITE_WAVE:SPRITE_IDLE;break;
    case PIXEL_BLINK: id=SPRITE_BLINK;break;
    case PIXEL_HAPPY: id=(pet_sprite_id_t[]){SPRITE_HAPPY,SPRITE_REACH_LEFT,SPRITE_HAPPY,SPRITE_REACH_RIGHT}[(phase/2)%4];break;
    case PIXEL_LISTEN: id=SPRITE_LISTEN;break;
    case PIXEL_THINK: id=phase%8<4?SPRITE_LISTEN:SPRITE_BLINK;break;
    case PIXEL_TALK: id=phase%3==0?SPRITE_IDLE:SPRITE_TALK;break;
    case PIXEL_EAT:
        if(food>PIXEL_COOKIE)food=PIXEL_APPLE;
        id=(pet_sprite_id_t)((phase<3?SPRITE_APPLE:SPRITE_APPLE_BITE)+food);break;
    case PIXEL_SLEEP: id=(phase/8)%2?SPRITE_SLEEP_BREATHE:SPRITE_SLEEP;break;
    case PIXEL_BATH: id=(phase/3)%2?SPRITE_BATH_SPLASH:SPRITE_BATH;break;
    case PIXEL_PLAY: id=(phase/3)%2?SPRITE_REACH_RIGHT:SPRITE_REACH_LEFT;break;
    }
    static const uint8_t coats[6][3]={{255,224,112},{188,156,231},{137,213,171},
                                    {245,164,196},{255,202,141},{139,204,232}};
    unsigned coat=pet->genes[GENE_BODY_COLOR]%6;
    memset(a->pixels,0,sizeof a->pixels);
    for(int y=0;y<W;y++)for(int x=0;x<W;x++) {
        uint16_t rgb=pet_sprite_pixels[id][y*W+x];
        if(!rgb)continue;
        int r=((rgb>>11)&31)*255/31,g=((rgb>>5)&63)*255/63,b=(rgb&31)*255/31;
        // Tint only yellow fur; preserve food, quilt, tub, face and leaves.
        bool fur=r>175 && g>140 && b<185 && r>b+35 && g>b+25;
        if(face==PIXEL_EAT && x>22 && x<50 && y>39)fur=false;
        if(face==PIXEL_SLEEP && y>35)fur=false;
        if(face==PIXEL_BATH && y>39)fur=false;
        if(coat && fur) {
            int shade=(r+g+b)/3-197;
            r=coats[coat][0]+shade;g=coats[coat][1]+shade;b=coats[coat][2]+shade;
            r=r<0?0:r>255?255:r;g=g<0?0:g>255?255:g;b=b<0?0:b>255?255:b;
        }
        // Baby remains a little smaller, with feet on the same baseline.
        int dx=x,dy=y;
        if(pet->stage==PET_STAGE_BABY){dx=4+x*64/72;dy=8+y*64/72;}
        a->pixels[dy*W+dx]=0xff000000U|((uint32_t)r<<16)|((uint32_t)g<<8)|(uint32_t)b;
    }
    // Small growth keepsakes sit on the finished artwork rather than rebuilding it.
    if(pet->stage>=PET_STAGE_TEEN && face!=PIXEL_SLEEP && face!=PIXEL_BATH) {
        rect(a,47,27,3,7,0xfff58dad);rect(a,45,29,7,3,0xfff58dad);rect(a,47,29,3,3,CREAM);
    }
    if(pet->stage>=PET_STAGE_ADULT && face!=PIXEL_SLEEP && face!=PIXEL_BATH && face!=PIXEL_EAT) {
        rect(a,25,54,23,2,0xffbd5e88);rect(a,43,56,4,5,0xffe994b5);
    }
    if(pet->stage>=PET_STAGE_ELDER && face!=PIXEL_SLEEP && face!=PIXEL_BATH && face!=PIXEL_EAT)
        {rect(a,34,54,3,4,CREAM);rect(a,33,55,5,2,CREAM);}
    a->image=(lv_image_dsc_t){.header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_ARGB8888,.w=W,.h=W,.stride=W*4},.data_size=sizeof a->pixels,.data=(const uint8_t*)a->pixels};
}
const lv_image_dsc_t *pixel_icon(unsigned kind)
{
    static lv_image_dsc_t images[9];
    static bool ready;
    if(!ready) {
        for(unsigned i=0;i<9;i++)images[i]=(lv_image_dsc_t){
            .header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_ARGB8888,.w=24,.h=24,.stride=96},
            .data_size=sizeof pet_icon_pixels[i],.data=(const uint8_t*)pet_icon_pixels[i]};
        ready=true;
    }
    const unsigned map[]={0,1,2,3,4,5,5,6,7,8};
    return &images[map[kind%10]];
}

#include "playtime_art.h"
const lv_image_dsc_t *pixel_collectible(unsigned kind)
{
    if(kind<6)return pixel_icon(kind);
    static lv_image_dsc_t images[12];
    static bool ready;
    if(!ready) {
        for(unsigned i=0;i<12;i++)images[i]=(lv_image_dsc_t){
            .header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_ARGB8888,.w=24,.h=24,.stride=96},
            .data_size=sizeof collectible_pixels[i],.data=(const uint8_t*)collectible_pixels[i]};
        ready=true;
    }
    return &images[(kind-6)%12];
}
const lv_image_dsc_t *pixel_decoration(unsigned kind)
{
    static lv_image_dsc_t images[6];
    static bool ready;
    if(!ready) {
        for(unsigned i=0;i<6;i++)images[i]=(lv_image_dsc_t){
            .header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_ARGB8888,.w=40,.h=40,.stride=160},
            .data_size=sizeof decoration_pixels[i],.data=(const uint8_t*)decoration_pixels[i]};
        ready=true;
    }
    return &images[kind%6];
}
