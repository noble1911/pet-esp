#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "sound_synth.h"
static void u32(FILE *f,unsigned n) {for(int i=0;i<4;i++)fputc((n>>(8*i))&255,f);}
static void u16(FILE *f,unsigned n) {fputc(n&255,f);fputc((n>>8)&255,f);}
static unsigned write_sound(sound_synth_t *s,const char *name)
{
    static int16_t pcm[SOUND_RATE*8];unsigned used=0;int peak=0;
    while(sound_active(s)) {
        assert(used+320<=sizeof pcm/sizeof pcm[0]);
        sound_render(s,pcm+used,320);used+=320;
    }
    for(unsigned i=0;i<used;i++) {int a=pcm[i]<0?-pcm[i]:pcm[i];if(a>peak)peak=a;}
    assert(peak>100 && peak<4000);assert(pcm[0]==0 && pcm[used-1]==0);
    int16_t quiet[640];memset(quiet,1,sizeof quiet);sound_render(s,quiet,640);
    for(unsigned i=0;i<640;i++)assert(!quiet[i]);
    char path[256];snprintf(path,sizeof path,"%s.wav",name);FILE *f=fopen(path,"wb");assert(f);
    fwrite("RIFF",1,4,f);u32(f,36+used*2);fwrite("WAVEfmt ",1,8,f);u32(f,16);u16(f,1);u16(f,1);u32(f,SOUND_RATE);u32(f,SOUND_RATE*2);u16(f,2);u16(f,16);fwrite("data",1,4,f);u32(f,used*2);
    unsigned hash=2166136261u;
    for(unsigned i=0;i<used;i++){u16(f,(unsigned)(uint16_t)pcm[i]);hash=(hash^(uint16_t)pcm[i])*16777619u;}
    fclose(f);return hash;
}
int main(void)
{
    const char *names[]={"hatch","apple-legacy","happy","meet","emote","apple","toast","cookie","cuddle","star","bubble","bounce","hide","found","sleep","bath","sticker","gift","select","butterfly","cupcake","pancakes","jelly","cake"};
    sound_synth_t s;unsigned hashes[SFX_COUNT];assert(sizeof names/sizeof names[0]==SFX_COUNT);
    for(unsigned i=0;i<SFX_COUNT;i++){sound_start_effect(&s,(sfx_id_t)i);hashes[i]=write_sound(&s,names[i]);}
    // Every current interaction has a unique waveform, not a pitch-only alias.
    for(unsigned i=SFX_APPLE;i<SFX_COUNT;i++)for(unsigned j=i+1;j<SFX_COUNT;j++)assert(hashes[i]!=hashes[j]);
    for(unsigned i=0;i<3;i++){assert(sound_start_tune(&s,i));char n[24];snprintf(n,sizeof n,"tune-%u",i);write_sound(&s,n);}
    assert(!sound_start_tune(&s,3) && !sound_active(&s));
    sound_start_effect(&s,SFX_COUNT);assert(!sound_active(&s));
    // Chunk boundaries do not change sample timing or the generated pattern.
    int16_t a[2000],b[2000];sound_start_effect(&s,SFX_BOUNCE);sound_render(&s,a,2000);
    sound_start_effect(&s,SFX_BOUNCE);sound_render(&s,b,137);sound_render(&s,b+137,1863);assert(!memcmp(a,b,sizeof a));
    puts("PASS: distinct effects, bounded peaks/durations, silent tails, deterministic chunk timing; WAV auditions written");
}
