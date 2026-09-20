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
    case SFX_CUPCAKE: SEQUENCE(SOUND_BELL,{79,75},{84,100},{88,180});break;
    case SFX_PANCAKES: SEQUENCE(SOUND_PLUCK,{60,110},{64,90},{67,180});break;
    case SFX_JELLY: SEQUENCE(SOUND_BOUNCE,{72,120},{67,120},{76,180});break;
    case SFX_CAKE: SEQUENCE(SOUND_BELL,{72,90},{76,90},{79,140},{84,240});break;
    case SFX_SELECT: SEQUENCE(SOUND_PLUCK,{76,65});break;
    default: memset(s,0,sizeof *s);break;
    }
}
#include "music_data.inc"
static int16_t music_wave[256];
static uint32_t music_steps[128];
static bool music_ready;
unsigned audio_tune_count(void) {return sizeof music_songs/sizeof music_songs[0];}
const char *audio_tune_name(unsigned tune) {return tune<audio_tune_count()?music_songs[tune].name:"";}
bool sound_start_tune(sound_synth_t *s,unsigned tune)
{
    memset(s,0,sizeof *s);
    if(tune>=audio_tune_count())return false;
    if(!music_ready) {
        for(unsigned i=0;i<256;i++)music_wave[i]=(int16_t)(sinf(2*PI*i/256)*32767);
        for(unsigned i=0;i<128;i++)music_steps[i]=(uint32_t)(440.0*pow(2.0,((int)i-69)/12.0)*4294967296.0/SOUND_RATE);
        music_ready=true;
    }
    s->song=&music_songs[tune];s->noise=0x12345678;
    return true;
}
bool sound_active(const sound_synth_t *s)
{
    return s->song?s->music_sample<s->song->duration_ms*(SOUND_RATE/1000):s->index<s->count;
}
static void music_render(sound_synth_t *s,int16_t *out,size_t samples)
{
    for(size_t i=0;i<samples && sound_active(s);i++,s->music_sample++) {
        while(s->next_note<s->song->count && s->song->notes[s->next_note].start_ms*(SOUND_RATE/1000)<=s->music_sample) {
            const music_note_t *n=&s->song->notes[s->next_note++];
            // The importer checks maximum overlap against this fixed pool.
            for(unsigned j=0;j<SOUND_MUSIC_VOICES;j++)if(s->voices[j].age>=s->voices[j].length) {
                music_voice_t *v=&s->voices[j];memset(v,0,sizeof *v);
                v->length=n->duration_ms*(SOUND_RATE/1000);v->velocity=n->velocity;v->midi=n->midi;
                v->step=music_steps[n->midi];
                v->timbre=n->program>=126?4:n->program>=32&&n->program<40?3:
                    n->program>=40&&n->program<112?2:n->program>=8&&n->program<16?1:0;
                if(v->timbre==4) {
                    // Compact kick/snare/hat sounds, without long sustained drum tails.
                    unsigned ms=n->midi>=42?65:150;
                    if(v->length>ms*(SOUND_RATE/1000))v->length=ms*(SOUND_RATE/1000);
                    v->step=music_steps[n->midi==35||n->midi==36?36:60];
                }
                break;
            }
        }
        int32_t mix=0;
        for(unsigned j=0;j<SOUND_MUSIC_VOICES;j++) {
            music_voice_t *v=&s->voices[j];if(v->age>=v->length)continue;
            int32_t wave=music_wave[v->phase>>24];
            if(v->timbre==0)wave=(wave*3+music_wave[(uint32_t)(v->phase*2)>>24])/4;
            else if(v->timbre==1)wave=(wave*4+music_wave[(uint32_t)(v->phase*3)>>24])/5;
            else if(v->timbre==4 && v->midi!=35 && v->midi!=36) {
                s->noise^=s->noise<<13;s->noise^=s->noise>>17;s->noise^=s->noise<<5;
                wave=(int32_t)(s->noise&65535)-32768;
            }
            unsigned attack=v->age<80?v->age*1024/80:1024;
            unsigned left=v->length-1-v->age;
            unsigned release=left<288?left*1024/288:1024;
            unsigned decay=1024;
            if(v->timbre<2)decay=32768000u/(32000+v->age*(v->timbre==1?3:8));
            else if(v->timbre==4)decay=left*1024/v->length;
            int32_t level=(int32_t)(attack*release/1024)*decay/1024;
            mix+=((wave*level/1024)*v->velocity/127)*1000/32768;
            v->phase+=v->step;v->age++;
        }
        // Gentle bounded compression prevents dense MIDI chords from clipping.
        int32_t magnitude=mix<0?-mix:mix;
        out[i]=(int16_t)(mix*8000/(8000+magnitude));
    }
}
void sound_render(sound_synth_t *s,int16_t *out,size_t samples)
{
    memset(out,0,samples*sizeof *out);
    if(s->song){music_render(s,out,samples);return;}
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
