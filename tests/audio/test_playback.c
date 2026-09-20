// Exercise the real audio owner and public controls with deterministic codec/RTOS IO.
#include <assert.h>
#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>
#include "../../firmware/components/audio/audio.c"
static SoundCommand queued;
static bool have_command;
static uint8_t incoming[640];static size_t incoming_n;
static int16_t outputs[12][320];static unsigned writes,limit;
static jmp_buf done;
static void (*after_write)(void);
int xQueueReceive(QueueHandle_t h,void *out,unsigned t) {(void)h;(void)t;if(!have_command)return 0;memcpy(out,&queued,sizeof queued);have_command=false;return pdTRUE;}
int xQueueOverwrite(QueueHandle_t h,const void *in) {(void)h;memcpy(&queued,in,sizeof queued);have_command=true;return pdTRUE;}
QueueHandle_t xQueueCreate(unsigned n,size_t z) {(void)n;(void)z;return (void *)1;}
size_t xStreamBufferReceive(StreamBufferHandle_t h,void *out,size_t n,unsigned t) {(void)h;(void)t;size_t k=incoming_n<n?incoming_n:n;memcpy(out,incoming,k);incoming_n-=k;return k;}
size_t xStreamBufferSend(StreamBufferHandle_t h,const void *in,size_t n,unsigned t) {(void)h;(void)t;assert(n<=640);memcpy(incoming,in,n);incoming_n=n;return n;}
StreamBufferHandle_t xStreamBufferCreateStatic(size_t a,size_t b,uint8_t *c,StaticStreamBuffer_t *d) {(void)a;(void)b;(void)c;(void)d;return (void *)1;}
int esp_codec_dev_write(esp_codec_dev_handle_t h,void *p,size_t n) {(void)h;assert(n==640);memcpy(outputs[writes++],p,n);if(after_write)after_write();if(writes==limit)longjmp(done,1);return 0;}
int esp_codec_dev_read(esp_codec_dev_handle_t h,void *p,size_t n) {(void)h;(void)p;(void)n;return 1;}
int esp_codec_dev_set_out_vol(esp_codec_dev_handle_t h,int v) {(void)h;(void)v;return 0;}
int esp_codec_dev_set_in_gain(esp_codec_dev_handle_t h,float v) {(void)h;(void)v;return 0;}
int esp_codec_dev_open(esp_codec_dev_handle_t h,const esp_codec_dev_sample_info_t *v) {(void)h;(void)v;return 0;}
int bsp_audio_init(void *p) {(void)p;return 0;}
esp_codec_dev_handle_t bsp_audio_codec_speaker_init(void) {return (void *)1;}
esp_codec_dev_handle_t bsp_audio_codec_microphone_init(void) {return (void *)1;}
void *heap_caps_malloc(size_t n,unsigned c) {(void)c;return malloc(n);}
size_t heap_caps_get_largest_free_block(unsigned c) {(void)c;return 65536;}
int xTaskCreatePinnedToCore(void (*f)(void *),const char *n,unsigned a,void *b,unsigned c,TaskHandle_t *d,unsigned e) {(void)f;(void)n;(void)a;(void)b;(void)c;(void)e;if(d)*d=(void *)1;return 1;}
void vTaskDelete(TaskHandle_t t) {(void)t;}
void vTaskDelay(unsigned n) {(void)n;}
static void reset(void) {
    atomic_store(&s_ready,true);atomic_store(&s_muted,false);atomic_store(&s_capture,false);atomic_store(&s_receiving,false);
    atomic_store(&s_accept,false);atomic_store(&s_flush,false);atomic_store(&s_playing,false);atomic_store(&s_tune,false);
    atomic_store(&s_sound_generation,0);atomic_store(&s_volume,100);s_fx=(void *)1;s_pcm=(void *)1;
    have_command=false;incoming_n=0;writes=0;after_write=NULL;memset(&s_synth,0,sizeof s_synth);memset(outputs,0,sizeof outputs);
}
static void run(unsigned n) {limit=n;if(!setjmp(done))play_task(NULL);}
static bool audible(unsigned n) {for(unsigned i=0;i<320;i++)if(outputs[n][i])return true;return false;}
static void stop_after_first(void) {if(writes==1)audio_stop_tune();}
static void capture_after_first(void) {if(writes==1)audio_set_capture(true);if(writes==2)audio_set_capture(false);}
static void mute_after_first(void) {if(writes==1)audio_set_muted(true);if(writes==2)audio_set_muted(false);}
static void speech_after_first(void) {
    if(writes==1){audio_voice_begin();int16_t pcm[320];for(unsigned i=0;i<320;i++)pcm[i]=1234;audio_play_pcm((uint8_t *)pcm,sizeof pcm);}
    if(writes==2)audio_voice_end();
}
int main(void) {
    reset();audio_play(SFX_STAR);run(2);assert(audible(0));
    reset();assert(!audio_play_tune(audio_tune_count()));
    reset();assert(audio_play_tune(audio_tune_count()-1));run(3);assert(audible(0));
    reset();assert(audio_play_tune(0));after_write=stop_after_first;run(3);assert(audible(0)&&!audible(1)&&!audible(2));assert(!audio_tune_playing());
    reset();assert(audio_play_tune(1));after_write=capture_after_first;run(4);assert(audible(0)&&!audible(1)&&!audible(2)&&!audible(3));
    reset();assert(audio_play_tune(2));after_write=mute_after_first;run(4);assert(audible(0)&&!audible(1)&&!audible(2)&&!audible(3));
    reset();assert(audio_play_tune(0));after_write=speech_after_first;run(4);assert(audible(0));
    for(unsigned i=0;i<320;i++)assert(outputs[1][i]==1234);assert(!audible(2)&&!audible(3));
    reset();audio_play(SFX_BOUNCE);audio_set_capture(true);audio_set_capture(false);run(2);assert(!audible(0)&&!audible(1));
    reset();audio_set_volume(0);assert(!audio_play_tune(0));audio_play(SFX_STAR);audio_set_volume(100);run(2);assert(!audible(0)&&!audible(1));
    reset();audio_voice_begin();assert(!audio_play_tune(0));audio_play(SFX_STAR);audio_voice_end();run(2);assert(!audible(0)&&!audible(1));
    reset();audio_play(SFX_BUBBLE);audio_play(SFX_GIFT);assert(have_command && queued.id==SFX_GIFT);run(1);assert(audible(0));
    puts("PASS: actual audio owner: speech priority, capture/mute/stop interruption, no stale/resumed tunes, latest cue wins");
}
