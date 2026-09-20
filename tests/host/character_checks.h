#include "character_sprites.h"
static void character_shot(const char *name,const pixel_pet_art_t *a) {
 char path[128];snprintf(path,sizeof path,"%s.ppm",name);FILE *f=fopen(path,"wb");assert(f);
 fprintf(f,"P6\n72 72\n255\n");for(unsigned i=0;i<72*72;i++){uint32_t c=a->pixels[i]?a->pixels[i]:0xfff6efe1;unsigned char b[]={c>>16,c>>8,c};fwrite(b,1,3,f);}fclose(f);
}
static void character_checks(void) {
 bool capture=getenv("PET_CAPTURE_CHARACTERS");Pet p={.stage=PET_STAGE_CHILD};
 struct {uint32_t before;pixel_pet_art_t a;uint32_t after;} out={.before=0x12345678,.after=0x87654321};
 pixel_pet_art_t previous[PET_CHARACTER_COUNT];
 assert(!pet_character(PET_CHARACTER_COUNT));assert(pet_character_id(NULL)==0);
 for(unsigned marker=0;marker<256;marker++) {p.genes[GENE_PATTERN]=marker;assert(pet_character_id(&p)==(marker>=240 && marker<240+PET_CHARACTER_COUNT?marker-240:0));}
 // Decode every compiled complete sprite, checking the run boundaries explicitly.
 for(unsigned c=0;c<PET_CHARACTER_COUNT;c++)for(unsigned f=0;f<26;f++) {
  unsigned at=character_offsets[c][f],end=f==25?(c==PET_CHARACTER_COUNT-1?sizeof character_rle:character_offsets[c+1][0]):character_offsets[c][f+1],pixels=0;
  while(at<end){assert(at+2<end && character_rle[at]);pixels+=character_rle[at];at+=3;assert(pixels<=72*72);}
  assert(at==end && pixels==72*72);
 }
 for(unsigned c=0;c<PET_CHARACTER_COUNT;c++) {
  memset(p.genes,0,8);p.genes[GENE_PATTERN]=240+c;p.stage=PET_STAGE_CHILD;
  pixel_pet_render(&previous[c],&p,PIXEL_IDLE,12,0);
  for(unsigned old=0;old<c;old++)assert(memcmp(previous[c].pixels,previous[old].pixels,sizeof previous[c].pixels));
  // Legacy component genes and personality cannot move/change any character pixels.
  for(unsigned value=0;value<256;value++) {
   for(unsigned g=0;g<8;g++)if(g!=GENE_PATTERN)p.genes[g]=value;
   pixel_pet_render(&out.a,&p,PIXEL_IDLE,12,0);assert(!memcmp(out.a.pixels,previous[c].pixels,sizeof out.a.pixels));
  }
  for(unsigned stage=0;stage<6;stage++)for(unsigned face=0;face<=PIXEL_PLAY;face++)for(unsigned pose=0;pose<2;pose++)for(unsigned food=0;food<(face==PIXEL_EAT?7:1);food++) {
   unsigned phase=face==PIXEL_SLEEP?pose*8:face==PIXEL_PLAY || face==PIXEL_BATH?pose*3:face==PIXEL_IDLE?pose*12:pose*4;
   p.stage=stage;Pet before=p;pixel_pet_render(&out.a,&p,face,phase,food);
   assert(out.before==0x12345678 && out.after==0x87654321 && !memcmp(&before,&p,sizeof p));
   unsigned filled=0;for(unsigned i=0;i<72*72;i++){uint32_t v=out.a.pixels[i];assert(!v || v>>24==255);filled+=!!v;}assert(filled>250 && filled<4300);
   if(capture && stage==PET_STAGE_CHILD){char name[96];snprintf(name,sizeof name,"character-%u-face-%u-pose-%u-food-%u",c,face,pose,food);character_shot(name,&out.a);}
  }
  for(unsigned food=0;food<7;food++){
   pixel_pet_art_t hold,bite;pixel_pet_render(&hold,&p,PIXEL_EAT,0,food);pixel_pet_render(&bite,&p,PIXEL_EAT,4,food);assert(memcmp(hold.pixels,bite.pixels,sizeof hold.pixels));
  }
 }
 puts("PASS: 208 whole frames decode safely, all eight characters distinct, legacy genes inert, all actions/stages bounded");
}
