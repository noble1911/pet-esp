// Full illustrated sprite frames, compiled to flash; no runtime image decoder.
#include "pixel_pet.h"
#include <string.h>
#include "pet_sprites.h"
#include "pet_icons.h"
#include "special_food_art.h"
#include "eating_food_masks.h"
#define W PIXEL_PET_SIZE
#define OUTLINE 0xff3d203dU
#define CREAM   0xffffedb0U
#define BLUSH   0xfff391a8U
static void rect(pixel_pet_art_t *a,int x,int y,int w,int h,uint32_t c)
{
    for(int yy=y;yy<y+h;yy++) for(int xx=x;xx<x+w;xx++)
        if(xx>=0 && xx<W && yy>=0 && yy<W) a->pixels[yy*W+xx]=c;
}
#include "genetic_art.h"
// Pose-local rig: separate eyes preserve the tilt in the reaching/listening art.
typedef struct {uint8_t lx,ly,lw,lh,rx,ry,rw,rh,mx,my,mw,mh,top,cx;} gene_rig_t;
static const gene_rig_t rigs[SPRITE_COUNT+8]={
 {23,35,6,6,42,33,6,7,29,39,13,10,25,36}, // wave
 {24,36,6,6,43,36,6,6,31,40,10,6,25,36}, // idle
 {23,36,8,5,41,36,8,5,30,40,12,10,25,36}, // happy
 {29,35,6,7,46,34,6,7,38,41,6,6,25,40}, // listen
 {19,32,11,6,42,32,11,6,29,37,14,8,23,36},
 {19,32,11,6,42,32,11,6,29,37,14,8,23,36},
 {20,32,10,6,42,32,11,6,29,37,14,8,23,36},
 {19,32,11,6,42,32,11,6,30,38,13,6,23,36},
 {19,32,11,6,42,32,11,6,29,38,13,6,23,36},
 {19,32,11,6,42,32,11,7,30,38,13,6,23,36},
 {25,29,7,5,40,29,7,5,32,33,8,2,22,36}, // sleep
 {25,28,7,5,40,28,7,5,32,32,8,2,22,36},
 {25,26,6,6,42,26,6,6,31,30,11,8,18,36}, // bath
 {24,26,8,5,41,26,8,5,31,30,11,8,18,36},
 {22,36,8,5,42,36,8,5,31,40,10,6,25,36}, // blink
 {24,35,6,7,43,35,6,7,30,40,13,10,25,36}, // talk
 {24,28,7,7,40,33,6,7,28,35,12,11,23,34}, // reach left
 {25,33,6,7,40,28,7,7,31,35,12,11,23,37}, // reach right
 {19,32,12,7,42,32,12,7,29,37,14,8,23,36},
 {19,32,11,7,42,32,12,7,29,37,15,8,23,36},
 {19,32,11,6,42,32,11,6,29,37,14,7,23,36},
 {19,32,12,6,42,32,11,6,29,37,14,7,23,36},
 {19,32,11,7,43,32,11,7,30,37,13,7,23,36},
 {19,32,11,7,42,32,11,7,30,38,13,6,23,36},
 {19,32,11,7,42,32,11,7,30,38,12,6,23,36},
 {19,32,11,7,42,32,11,7,30,37,12,7,23,36},
};
static const uint8_t coats[16][3]={
 {255,224,112},{188,156,231},{137,213,171},{245,164,196},{255,202,141},{139,204,232},
 {239,209,100},{171,148,211},{117,196,163},{232,145,184},{239,182,126},{118,186,222},
 {246,230,151},{213,182,237},{164,227,179},{249,185,209}};
static const uint8_t irises[16][3]={{74,145,137},{118,78,54},{55,116,170},{88,140,66},
 {154,102,191},{196,126,46},{194,98,143},{55,136,171},{81,83,148},{132,158,65},
 {186,83,72},{138,94,150},{83,110,106},{176,128,94},{75,164,176},{114,127,167}};
static uint32_t unpack(uint16_t c) {return c?0xff000000U|((uint32_t)((c>>11)*255/31)<<16)|((uint32_t)((c>>5&63)*255/63)<<8)|(c&31)*255/31:0;}
static bool yellow(uint32_t c) {int r=c>>16&255,g=c>>8&255,b=c&255;return c && r>175 && g>140 && b<185 && r>b+35 && g>b+25;}
static unsigned clamp_color(int n) {return n<0?0:n>255?255:(unsigned)n;}
static uint32_t tint(uint32_t c,unsigned coat) {
 if(!coat || !yellow(c))return c;
 int shade=((int)(c>>16&255)+(int)(c>>8&255)+(int)(c&255))/3-197;
 return 0xff000000U|(clamp_color(coats[coat][0]+shade)<<16)|(clamp_color(coats[coat][1]+shade)<<8)|clamp_color(coats[coat][2]+shade);
}
static bool prop_at(pixel_face_t face,unsigned pose,unsigned food,int x,int y,const uint16_t *source) {
 if(face==PIXEL_EAT){const uint8_t *s=eating_food_spans[pose][food][y];return x>=s[0] && x<s[1];}
 // Keep the entire pillow/quilt and tub/foam/droplets in their authored positions.
 if(face==PIXEL_SLEEP)return y>=35 || (source[y*W+x] && (x<19 || x>52));
 if(face==PIXEL_BATH) {
  uint32_t c=unpack(source[y*W+x]);
  return y>=45 || ((y>=39 || x<17 || x>55 || (x<31 && y<24)) && c && !yellow(c));
 }
 return false;
}
static bool inside(int x,int y,int bx,int by,int bw,int bh) {return x>=bx && x<bx+bw && y>=by && y<by+bh;}
static bool face_region(const gene_rig_t *r,int x,int y,bool eyes,bool mouth) {
 return (eyes && (inside(x,y,r->lx,r->ly,r->lw,r->lh)||inside(x,y,r->rx,r->ry,r->rw,r->rh))) ||
        (mouth && inside(x,y,r->mx,r->my,r->mw,r->mh));
}
static uint32_t repair_fur(const uint16_t *p,int x,int y) {
 // Restore only the selected face region, sampling the source's nearby fur.
 for(int d=1;d<15;d++)for(int k=0;k<4;k++) {
  int xx=x+(k==0?d:k==1?-d:0),yy=y+(k==2?-d:k==3?d:0);
  if(xx<0 || xx>=W || yy<0 || yy>=W)continue;
  uint32_t c=unpack(p[yy*W+xx]);if(yellow(c))return c;
 }
 return 0xffffe878U;
}
static void feature(pixel_pet_art_t *a,const uint16_t *p,int w,int h,int ox,int oy,unsigned coat,int iris,
                    pixel_face_t face,unsigned pose,unsigned food,const uint16_t *source) {
 for(int y=0;y<h;y++)for(int x=0;x<w;x++) {
  int dx=ox+x,dy=oy+y;if(dx<0 || dx>=W || dy<0 || dy>=W || prop_at(face,pose,food,dx,dy,source))continue;
  uint32_t c=unpack(p[y*w+x]);if(!c)continue;
  int r=c>>16&255,g=c>>8&255,b=c&255;
  if(iris>=0 && g>r+15 && b>r+8)c=0xff000000U|((uint32_t)irises[iris][0]<<16)|((uint32_t)irises[iris][1]<<8)|irises[iris][2];
  else if(iris==-2)c=tint(c,coat);
  a->pixels[dy*W+dx]=c;
 }
}
static int body_width(unsigned shape,int y) {
 // Continuous profiles avoid horizontal seams where the silhouette changes width.
 int yy=y<25?25:y>65?65:y;
 switch(shape){
 case 1:return yy<42?110+(yy-25)/2:118-(yy-42); // round
 case 2:return 80; // tall/slim
 case 3:return 113; // chubby
 case 4:return 94+(yy-25)/3; // bean, also curves sideways
 case 5:return 80+(yy-25)*3/4; // teardrop
 case 6:return yy<40?100+(yy-25):115-(yy-40); // dumpling
 case 7:return yy<43?110-(yy-25)*5/4:88+(yy-43); // peanut
 default:return 100;
 }
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
        if((unsigned)food>=PET_FOOD_COUNT)food=PIXEL_APPLE;
        if(food<3)id=(pet_sprite_id_t)((phase<3?SPRITE_APPLE:SPRITE_APPLE_BITE)+food);
        break;
    case PIXEL_SLEEP: id=(phase/8)%2?SPRITE_SLEEP_BREATHE:SPRITE_SLEEP;break;
    case PIXEL_BATH: id=(phase/3)%2?SPRITE_BATH_SPLASH:SPRITE_BATH;break;
    case PIXEL_PLAY: id=(phase/3)%2?SPRITE_REACH_RIGHT:SPRITE_REACH_LEFT;break;
    }
    const uint16_t *pixels=face==PIXEL_EAT && food>=3 ? special_food_pose[(phase<3?0:4)+food-3] : pet_sprite_pixels[id];
    unsigned pose=phase<3?0:1;
    unsigned rig_id=face==PIXEL_EAT && food>=3?SPRITE_COUNT+pose*4+food-3:(unsigned)id;
    const gene_rig_t *rig=&rigs[rig_id];
    unsigned coat=pet_trait_choice(pet,GENE_BODY_COLOR), ears=pet_trait_choice(pet,GENE_EAR_SHAPE);
    unsigned eye=pet_trait_choice(pet,GENE_EYE_SHAPE), iris=pet_trait_choice(pet,GENE_EYE_COLOR);
    unsigned mouth=pet_trait_choice(pet,GENE_MOUTH_SHAPE), pattern=pet_trait_choice(pet,GENE_PATTERN);
    bool open_eyes=face!=PIXEL_SLEEP && face!=PIXEL_BLINK && id!=SPRITE_BLINK &&
                   id!=SPRITE_HAPPY && id!=SPRITE_BATH_SPLASH && face!=PIXEL_EAT;
    bool change_mouth=face!=PIXEL_SLEEP;
    memset(a->pixels,0,sizeof a->pixels);
    for(int y=0;y<W;y++)for(int x=0;x<W;x++) {
        uint32_t c=unpack(pixels[y*W+x]);if(!c)continue;
        bool prop=prop_at(face,pose,food,x,y,pixels);
        if(ears && y<rig->top && !prop)continue;
        if(!prop && face_region(rig,x,y,open_eyes,change_mouth))c=repair_fur(pixels,x,y);
        if(!prop)c=tint(c,coat);
        a->pixels[y*W+x]=c;
    }
    // Markings are clipped to original fur. The food silhouette is an occluder.
    if(pattern)for(int y=0;y<20;y++)for(int x=0;x<28;x++) {
        int dx=rig->cx-14+x,dy=44+y;
        uint32_t c=unpack(genetic_markings[pattern][y*28+x]);
        if(c && yellow(unpack(pixels[dy*W+dx])) && !prop_at(face,pose,food,dx,dy,pixels))a->pixels[dy*W+dx]=c;
    }
    if(ears)feature(a,genetic_ears[ears],32,22,rig->cx-16,rig->top<19?0:rig->top-19,coat,-2,face,pose,food,pixels);
    if(open_eyes) {
        // Each eye uses its own anchor, retaining the source pose's head tilt.
        uint16_t half[13*10];
        for(unsigned side=0;side<2;side++) {
            for(int y=0;y<10;y++)for(int x=0;x<13;x++)half[y*13+x]=genetic_eyes[eye][y*26+x+side*13];
            int cx=side?rig->rx+rig->rw/2:rig->lx+rig->lw/2;
            int cy=side?rig->ry+rig->rh/2:rig->ly+rig->lh/2;
            // Pair atlas has outer margin and central gap: align visible eye to anchor.
            feature(a,half,13,10,cx-(side?8:4),cy-6,coat,(int)iris,face,pose,food,pixels);
        }
    }
    if(change_mouth) {
        unsigned row=face==PIXEL_EAT?(pose?2:1):
          (id==SPRITE_TALK || id==SPRITE_HAPPY || id==SPRITE_WAVE || id==SPRITE_REACH_LEFT || id==SPRITE_REACH_RIGHT || face==PIXEL_BATH)?1:0;
        feature(a,genetic_mouths[row*8+mouth],12,9,rig->mx+rig->mw/2-6,rig->my+rig->mh-9,coat,-1,face,pose,food,pixels);
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
    // Reshape the authored character scanlines, keeping foreground props fixed.
    unsigned shape=pet_trait_choice(pet,GENE_BODY_SHAPE);
    if(shape)for(int y=0;y<W;y++) {
        uint32_t row[W];memcpy(row,&a->pixels[y*W],sizeof row);
        int width=body_width(shape,y),curve=shape==4?(y-40)/9:0;
        for(int x=0;x<W;x++) {
            if(prop_at(face,pose,food,x,y,pixels))continue;
            int sx=36+(x-36-curve)*100/width;
            a->pixels[y*W+x]=sx<0 || sx>=W?0:prop_at(face,pose,food,sx,y,pixels)?(face==PIXEL_EAT?tint(repair_fur(pixels,sx,y),coat):0):row[sx];
        }
    }
    if(pet->stage==PET_STAGE_BABY) {
        // Reverse scan prevents the smaller, baseline-aligned copy overwriting its source.
        for(int y=W-1;y>=0;y--) {
            uint32_t row[W]={0};
            if(y>=8){int sy=(y-8)*72/64;for(int x=4;x<68;x++)row[x]=a->pixels[sy*W+(x-4)*72/64];}
            memcpy(&a->pixels[y*W],row,sizeof row);
        }
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
