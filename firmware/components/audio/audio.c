// One speaker owner combines care chirps and buffered speech; mic is duplex.
#include "audio.h"
#include <math.h>
#include <string.h>
#include <stdatomic.h>
#include "bsp/esp-bsp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/stream_buffer.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
static QueueHandle_t s_fx;
static StaticStreamBuffer_t s_pcm_control;
static atomic_bool s_ready;
static atomic_int s_mic_level;
static atomic_uint s_mic_frames;
static StreamBufferHandle_t s_pcm;
static esp_codec_dev_handle_t s_spk, s_mic;
static audio_mic_cb_t s_cb;
static atomic_bool s_muted, s_capture, s_receiving, s_accept, s_flush, s_playing;
static atomic_int s_volume=100;
static void play_task(void *arg)
{
    (void)arg; int16_t buf[320]; int phase=2048, hz=880, volume=-1;
    for (;;) {
        if(atomic_exchange(&s_flush,false)) {
            while(xStreamBufferReceive(s_pcm,buf,sizeof(buf),0)) {}
            phase=2048;
        }
        size_t n=xStreamBufferReceive(s_pcm,buf,sizeof(buf),pdMS_TO_TICKS(10));
        bool speech=n || atomic_load(&s_receiving);
        atomic_store(&s_playing,speech);
        if(speech) phase=2048;
        if(!n) {
            sfx_id_t fx;
            if(xQueueReceive(s_fx,&fx,0)==pdTRUE && !speech && !atomic_load(&s_capture)) {
                phase=0;hz=fx==SFX_HAPPY?880:fx==SFX_EMOTE?660:740;
            }
            memset(buf,0,sizeof(buf));
            if(!speech && !atomic_load(&s_capture) && phase<2048) {
                for(int i=0;i<320 && phase<2048;i++,phase++)
                    buf[i]=(int16_t)(1800*sinf(3.14159265f*phase/2048)*sinf(6.2831853f*(hz+phase/8)*phase/16000));
            }
            n=sizeof(buf);
        }
        int v=atomic_load(&s_muted)?0:atomic_load(&s_volume);
        if(v!=volume) { esp_codec_dev_set_out_vol(s_spk,v);volume=v; }
        // Always feed silence when idle/underrunning: no stale DMA buzz.
        if(v==0) memset(buf,0,n);
        esp_codec_dev_write(s_spk,buf,n);
    }
}
static void mic_task(void *arg)
{
    (void)arg; uint8_t buf[640];
    for(;;) {
        if(esp_codec_dev_read(s_mic,buf,sizeof(buf))==ESP_OK) {
            unsigned peak=0;
            for(size_t i=0;i<sizeof(buf);i+=2) { int v=(int16_t)(buf[i]|(buf[i+1]<<8));unsigned a=v<0?-v:v;if(a>peak)peak=a; }
            atomic_store(&s_mic_level,peak);atomic_fetch_add(&s_mic_frames,1);
            if(atomic_load(&s_capture) && s_cb) s_cb(buf,sizeof(buf));
        } else vTaskDelay(pdMS_TO_TICKS(10));
    }
}
void audio_init(void)
{
    if(bsp_audio_init(NULL)!=ESP_OK) return;
    s_spk=bsp_audio_codec_speaker_init();s_mic=bsp_audio_codec_microphone_init();
    if(!s_spk || !s_mic) { ESP_LOGE("audio","codec unavailable");return; }
    esp_codec_dev_sample_info_t fmt={.sample_rate=16000,.channel=1,.bits_per_sample=16};
    if(esp_codec_dev_open(s_spk,&fmt)!=ESP_OK || esp_codec_dev_open(s_mic,&fmt)!=ESP_OK) return;
    esp_codec_dev_set_in_gain(s_mic,30.0f);
    ESP_LOGI("audio","internal largest block before speech buffer: %u",(unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
    // FreeRTOS dynamic buffers allocate INTERNAL memory even with PSRAM enabled.
    // Audio payload belongs in PSRAM; the stream's control object stays internal.
    uint8_t *storage=heap_caps_malloc(96*1024+1,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(storage)s_pcm=xStreamBufferCreateStatic(96*1024+1,1,storage,&s_pcm_control);
    s_fx=xQueueCreate(4,sizeof(sfx_id_t));
    if(!s_pcm || !s_fx) { ESP_LOGE("audio","audio buffers unavailable");return; }
    TaskHandle_t player=NULL;
    if(xTaskCreatePinnedToCore(play_task,"pet_audio",4096,NULL,6,&player,1)!=pdPASS) { ESP_LOGE("audio","speaker task unavailable");return; }
    if(xTaskCreatePinnedToCore(mic_task,"pet_mic",4096,NULL,5,NULL,1)!=pdPASS) { vTaskDelete(player);ESP_LOGE("audio","mic task unavailable");return; }
    atomic_store(&s_ready,true);
    ESP_LOGI("audio","speaker and mic tasks ready; 96 KB speech buffer in PSRAM");
}
void audio_play(sfx_id_t fx) { if(s_fx && !atomic_load(&s_muted) && !atomic_load(&s_capture) && !atomic_load(&s_playing)) xQueueSend(s_fx,&fx,0); }
void audio_set_muted(bool m) { atomic_store(&s_muted,m); }
void audio_set_volume(int v) { atomic_store(&s_volume,v<0?0:v>100?100:v); }
int audio_get_volume(void) { return atomic_load(&s_volume); }
void audio_set_mic_callback(audio_mic_cb_t cb) { s_cb=cb; }
void audio_set_capture(bool b) { atomic_store(&s_capture,b); }
void audio_voice_begin(void) { atomic_store(&s_accept,true);atomic_store(&s_receiving,true); }
void audio_voice_end(void) { atomic_store(&s_receiving,false); }
void audio_voice_stop(void) { atomic_store(&s_accept,false);atomic_store(&s_receiving,false);atomic_store(&s_flush,true); }
bool audio_voice_playing(void) { return atomic_load(&s_playing); }
void audio_play_pcm(const uint8_t *p,size_t n)
{
    if(s_pcm && atomic_load(&s_accept) && xStreamBufferSend(s_pcm,p,n,0)!=n)
        ESP_LOGW("audio","speech buffer full");
}

bool audio_is_ready(void) { return atomic_load(&s_ready); }
unsigned audio_mic_frames(void) { return atomic_load(&s_mic_frames); }
int audio_mic_level(void) { return atomic_load(&s_mic_level); }
