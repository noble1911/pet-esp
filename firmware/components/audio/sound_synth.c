#include "sound_synth.h"
#include <math.h>
#include <string.h>
#define PI 3.14159265358979323846f
static void start(sound_synth_t *s,sound_timbre_t timbre,const sound_note_t *notes,unsigned count)
{
    memset(s,0,sizeof *s);s->timbre=timbre;s->noise=0x12345678;
    if(count>SOUND_MAX_NOTES)count=SOUND_MAX_NOTES;
    memcpy(s->notes,notes,count*sizeof *notes);s->count=count;
}
#define SEQUENCE(t,...) do {const sound_note_t n[]={__VA_ARGS__};start(s,t,n,sizeof n/sizeof n[0]);} while(0)
void sound_start_effect(sound_synth_t *s,sfx_id_t fx)
{
    switch(fx) {
    case SFX_HATCH: SEQUENCE(SOUND_BELL,{60,100},{64,100},{67,120},{72,300});break;
    case SFX_FEED: case SFX_APPLE: SEQUENCE(SOUND_CRUNCH,{76,70},{0,35},{72,110});break;
    case SFX_TOAST: SEQUENCE(SOUND_CRUNCH,{55,100},{0,40},{60,130},{0,30},{57,65});break;
    case SFX_COOKIE: SEQUENCE(SOUND_CRUNCH,{84,45},{0,30},{79,55},{0,30},{81,80});break;
    case SFX_HAPPY: SEQUENCE(SOUND_PLUCK,{60,85},{64,85},{67,85},{72,210});break;
    case SFX_MEET: case SFX_BUTTERFLY: SEQUENCE(SOUND_FLUTE,{79,100},{84,120},{79,110},{76,180});break;
    case SFX_EMOTE: case SFX_CUDDLE: SEQUENCE(SOUND_FLUTE,{67,140},{72,180});break;
    case SFX_STAR: SEQUENCE(SOUND_BELL,{84,80},{88,160});break;
    case SFX_BUBBLE: SEQUENCE(SOUND_POP,{79,105});break;
    case SFX_BOUNCE: SEQUENCE(SOUND_BOUNCE,{57,210});break;
    case SFX_HIDE: SEQUENCE(SOUND_PLUCK,{60,70},{0,30},{67,100});break;
    case SFX_FOUND: SEQUENCE(SOUND_FLUTE,{67,90},{72,90},{79,190});break;
    case SFX_SLEEP: SEQUENCE(SOUND_FLUTE,{72,230},{67,270},{64,350});break;
    case SFX_BATH: SEQUENCE(SOUND_POP,{72,85},{79,85},{84,150});break;
    case SFX_STICKER: SEQUENCE(SOUND_BELL,{72,90},{76,90},{79,90},{84,310});break;
    case SFX_GIFT: SEQUENCE(SOUND_BELL,{60,100},{67,100},{72,100},{76,100},{79,340});break;
    case SFX_SELECT: SEQUENCE(SOUND_PLUCK,{76,65});break;
    default: memset(s,0,sizeof *s);break;
    }
}
bool sound_start_tune(sound_synth_t *s,unsigned tune)
{
    switch(tune) {
    case 0: SEQUENCE(SOUND_BELL,{72,220},{76,220},{79,440},{76,220},{74,220},{72,440},{67,220},{72,220},{76,440},{74,220},{72,660});break;
    case 1: SEQUENCE(SOUND_PLUCK,{60,160},{0,80},{67,160},{72,320},{67,160},{64,160},{67,320},{0,160},{65,160},{69,160},{72,320},{67,160},{64,160},{60,480});break;
    case 2: SEQUENCE(SOUND_FLUTE,{72,400},{67,400},{69,800},{67,400},{64,400},{65,800},{64,400},{60,800});break;
    default: memset(s,0,sizeof *s);return false;
    }
    return true;
}
bool sound_active(const sound_synth_t *s) {return s->index<s->count;}
void sound_render(sound_synth_t *s,int16_t *out,size_t samples)
{
    memset(out,0,samples*sizeof *out);
    for(size_t i=0;i<samples && sound_active(s);i++) {
        sound_note_t note=s->notes[s->index];unsigned length=note.ms*(SOUND_RATE/1000);
        float time=s->sample/(float)SOUND_RATE,u=s->sample/(float)length;
        if(note.midi) {
            float hz=440.0f*powf(2.0f,((int)note.midi-69)/12.0f),v=0;
            if(s->timbre==SOUND_POP)hz*=1.6f-1.3f*u;
            if(s->timbre==SOUND_BOUNCE)hz*=0.7f+2.3f*expf(-u*5.0f)+.14f*sinf(u*24);
            s->phase+=2*PI*hz/SOUND_RATE;if(s->phase>2*PI)s->phase-=2*PI;
            float env=fminf(1,time/.008f)*fminf(1,(length-1-s->sample)/(float)(SOUND_RATE*.018f));
            switch(s->timbre) {
            case SOUND_BELL: v=(sinf(s->phase)+.32f*sinf(2*s->phase)+.12f*sinf(3*s->phase))*expf(-time*7);break;
            case SOUND_PLUCK: v=(sinf(s->phase)+.3f*sinf(2*s->phase))*expf(-time*13);break;
            case SOUND_FLUTE: v=(sinf(s->phase)+.12f*sinf(2*s->phase))*sinf(PI*u);break;
            case SOUND_CRUNCH:
                s->noise^=s->noise<<13;s->noise^=s->noise>>17;s->noise^=s->noise<<5;
                v=((int)(s->noise&65535)-32768)/32768.0f*(.45f+.55f*sinf(s->phase))*expf(-time*22);break;
            case SOUND_POP: v=sinf(s->phase)*expf(-time*25);break;
            case SOUND_BOUNCE: v=sinf(s->phase)*expf(-time*9);break;
            }
            out[i]=(int16_t)(2600*v*env);
        }
        if(++s->sample>=length){s->index++;s->sample=0;s->phase=0;}
    }
}
