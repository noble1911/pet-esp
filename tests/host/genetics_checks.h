#include "eating_food_masks.h"
// Renderer coverage: saved genes, palette ranges, authored poses and clipping.
static void sprite_shot(const char *name,const pixel_pet_art_t *a) {
 char path[128];snprintf(path,sizeof path,"%s.ppm",name);FILE *f=fopen(path,"wb");assert(f);
 fprintf(f,"P6\n72 72\n255\n");
 for(unsigned i=0;i<72*72;i++) {uint32_t c=a->pixels[i]?a->pixels[i]:0xfff6efe1;unsigned char b[]={c>>16,c>>8,c};fwrite(b,1,3,f);}fclose(f);
}
static void genetics_checks(void) {
 struct {uint32_t before;pixel_pet_art_t art;uint32_t after;} guarded={.before=0x12345678,.after=0x76543210};
 pixel_pet_art_t prior[16];Pet p={.stage=PET_STAGE_CHILD};bool capture=getenv("PET_CAPTURE_GENETICS");
 for(unsigned gene=0;gene<7;gene++) {
  memset(p.genes,0,8);const pet_trait_t *t=pet_trait(gene);
  for(unsigned value=0;value<t->count;value++) {
   p.genes[gene]=value;pixel_pet_render(&prior[value],&p,PIXEL_IDLE,12,PIXEL_APPLE);
   for(unsigned old=0;old<value;old++)assert(memcmp(prior[old].pixels,prior[value].pixels,sizeof prior[0].pixels));
   if(capture){char name[64];snprintf(name,sizeof name,"gene-%u-%02u",gene,value);sprite_shot(name,&prior[value]);}
  }
 }
 for(unsigned n=0;n<256;n++)for(unsigned stage=0;stage<6;stage++) {
  p.stage=stage;for(unsigned g=0;g<8;g++)p.genes[g]=(n*(g*2+1)+g*17)%256;
  Pet before=p;
  for(unsigned face=0;face<=PIXEL_PLAY;face++)for(unsigned pose=0;pose<2;pose++)for(unsigned food=0;food<(face==PIXEL_EAT?7:1);food++) {
   unsigned phase=face==PIXEL_SLEEP?pose*8:face==PIXEL_PLAY || face==PIXEL_BATH?pose*3:face==PIXEL_IDLE?pose*12:pose*4;
   pixel_pet_render(&guarded.art,&p,face,phase,food);
   assert(guarded.before==0x12345678 && guarded.after==0x76543210);
   assert(!memcmp(&before,&p,sizeof p));
   unsigned filled=0;for(unsigned i=0;i<72*72;i++){uint32_t c=guarded.art.pixels[i];assert(!c || c>>24==255);filled+=!!c;}
   assert(filled>250 && filled<4200);
  }
 }
 // Every food pixel stays identical across shape, coat, eyes, ears, mouth and pattern.
 for(unsigned food=0;food<7;food++)for(unsigned pose=0;pose<2;pose++) {
  memset(p.genes,0,8);p.stage=PET_STAGE_CHILD;
  pixel_pet_render(&prior[0],&p,PIXEL_EAT,pose*4,food);
  for(unsigned v=0;v<16;v++) {
   for(unsigned g=0;g<7;g++)p.genes[g]=v;
   pixel_pet_render(&guarded.art,&p,PIXEL_EAT,pose*4,food);
   for(int y=0;y<72;y++)for(int x=eating_food_spans[pose][food][y][0];x<eating_food_spans[pose][food][y][1];x++)
    assert(prior[0].pixels[y*72+x]==guarded.art.pixels[y*72+x]);
  }
 }
 // Narrow sleeping silhouettes must not turn empty pillow margins into vertical fur streaks.
 for(unsigned shape=0;shape<8;shape++)for(unsigned ears=0;ears<8;ears++) {
  p.genes[GENE_BODY_SHAPE]=shape;p.genes[GENE_EAR_SHAPE]=ears;
  pixel_pet_render(&guarded.art,&p,PIXEL_SLEEP,0,0);
  for(unsigned i=0;i<72*3;i++)assert(!guarded.art.pixels[i]);
 }
 if(capture)for(unsigned variant=0;variant<8;variant++) {
  p.stage=PET_STAGE_CHILD;for(unsigned g=0;g<8;g++)p.genes[g]=(variant+g*2)%pet_trait(g)->count;
  for(unsigned face=0;face<=PIXEL_PLAY;face++)for(unsigned pose=0;pose<2;pose++)for(unsigned food=0;food<(face==PIXEL_EAT?7:1);food++) {
   unsigned phase=face==PIXEL_SLEEP?pose*8:face==PIXEL_PLAY || face==PIXEL_BATH?pose*3:face==PIXEL_IDLE?pose*12:pose*4;
   pixel_pet_render(&guarded.art,&p,face,phase,food);
   char name[64];snprintf(name,sizeof name,"family-%u-face-%u-pose-%u-food-%u",variant,face,pose,food);sprite_shot(name,&guarded.art);
  }
  for(unsigned stage=0;stage<6;stage++){p.stage=stage;pixel_pet_render(&guarded.art,&p,PIXEL_IDLE,12,0);char name[64];snprintf(name,sizeof name,"growth-%u-%u",variant,stage);sprite_shot(name,&guarded.art);}
 }
 puts("PASS: all 72 visual trait choices distinct, 49,152 mixed-gene/stage/action renders bounded; genes unchanged");
}
