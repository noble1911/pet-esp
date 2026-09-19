// Full authored frames only: no facial parts, coat tint, silhouette warps or overlays.
#include "pixel_pet.h"
#include <string.h>
#include "character_sprites.h"
#include "pet_icons.h"
#include "special_food_art.h"
#define W PIXEL_PET_SIZE
void pixel_pet_render(pixel_pet_art_t *a,const Pet *pet,pixel_face_t face,unsigned phase,pixel_food_t food)
{
    character_frame_t frame=CHAR_IDLE;
    switch(face) {
    case PIXEL_IDLE:frame=phase%28<7?CHAR_WAVE:CHAR_IDLE;break;
    case PIXEL_BLINK:frame=CHAR_BLINK;break;
    case PIXEL_HAPPY:frame=(character_frame_t[]){CHAR_HAPPY,CHAR_REACH_LEFT,CHAR_HAPPY,CHAR_REACH_RIGHT}[(phase/2)%4];break;
    case PIXEL_LISTEN:frame=CHAR_LISTEN;break;
    case PIXEL_THINK:frame=phase%8<4?CHAR_LISTEN:CHAR_BLINK;break;
    case PIXEL_TALK:frame=phase%3==0?CHAR_IDLE:CHAR_TALK;break;
    case PIXEL_EAT:
        if((unsigned)food>=PET_FOOD_COUNT)food=PIXEL_APPLE;
        frame=(character_frame_t)(CHAR_APPLE_HOLD+(phase<3?0:PET_FOOD_COUNT)+food);break;
    case PIXEL_SLEEP:frame=(phase/8)%2?CHAR_SLEEP_BREATHE:CHAR_SLEEP;break;
    case PIXEL_BATH:frame=(phase/3)%2?CHAR_BATH_SPLASH:CHAR_BATH;break;
    case PIXEL_PLAY:frame=(phase/3)%2?CHAR_REACH_RIGHT:CHAR_REACH_LEFT;break;
    }
    const uint8_t *src=character_rle+character_offsets[pet_character_id(pet)][frame];
    unsigned pos=0;
    while(pos<W*W) {
        unsigned count=*src++;uint16_t rgb=src[0]|((uint16_t)src[1]<<8);src+=2;
        uint32_t c=rgb?0xff000000U|((uint32_t)((rgb>>11)*255/31)<<16)|((uint32_t)((rgb>>5&63)*255/63)<<8)|(rgb&31)*255/31:0;
        for(unsigned n=0;n<count;n++)a->pixels[pos++]=c;
    }
    // Baby growth scales the entire finished frame, keeping all features together.
    if(pet->stage==PET_STAGE_BABY)for(int y=W-1;y>=0;y--) {
        uint32_t row[W]={0};
        if(y>=8){int sy=(y-8)*72/64;for(int x=4;x<68;x++)row[x]=a->pixels[sy*W+(x-4)*72/64];}
        memcpy(&a->pixels[y*W],row,sizeof row);
    }
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

const lv_image_dsc_t *pixel_special_food(unsigned kind)
{
    static lv_image_dsc_t images[PET_SPECIAL_FOOD_COUNT];
    static bool ready;
    if(!ready) {
        for(unsigned i=0;i<PET_SPECIAL_FOOD_COUNT;i++)images[i]=(lv_image_dsc_t){
            .header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_ARGB8888,.w=24,.h=24,.stride=96},
            .data_size=sizeof special_food_icon[i],.data=(const uint8_t*)special_food_icon[i]};
        ready=true;
    }
    return &images[kind%PET_SPECIAL_FOOD_COUNT];
}
