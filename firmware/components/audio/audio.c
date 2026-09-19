// Short, quiet synthesised chirps. Codec writes stay off the LVGL task.
#include "audio.h"
#include <math.h>
#include "bsp/esp-bsp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <stdatomic.h>
static QueueHandle_t s_queue;
static esp_codec_dev_handle_t s_codec;
static atomic_bool s_muted;
static atomic_int s_volume = 35;
static void sound_task(void *arg)
{
    (void)arg; sfx_id_t fx; int16_t pcm[256];
    for (;;) {
        if(xQueueReceive(s_queue,&fx,portMAX_DELAY)!=pdTRUE) continue;
        if(atomic_load(&s_muted)) continue;
        int hz=fx==SFX_HAPPY?880:fx==SFX_EMOTE?660:740;
        int applied_volume = -1;
        for(int block=0;block<8;block++) {
            if (atomic_load(&s_muted)) break;
            int volume = atomic_load(&s_volume);
            if (volume == 0) break;
            // Only the audio worker touches codec volume; UI drags cannot race I2S.
            if (volume != applied_volume) {
                esp_codec_dev_set_out_vol(s_codec, volume);
                applied_volume = volume;
            }
            for(int i=0;i<256;i++) {
                int n=block*256+i;
                float envelope=sinf(3.14159265f*n/2048);
                pcm[i]=(int16_t)(envelope*1800*sinf(6.2831853f*(hz+n/8)*n/16000));
            }
            if(esp_codec_dev_write(s_codec,pcm,sizeof(pcm))!=ESP_OK) break;
        }
    }
}
void audio_init(void)
{
    s_codec=bsp_audio_codec_speaker_init();
    if(!s_codec) return;
    esp_codec_dev_sample_info_t fmt={.sample_rate=16000,.channel=1,.bits_per_sample=16};
    if(esp_codec_dev_open(s_codec,&fmt)!=ESP_OK) { ESP_LOGW("audio","speaker unavailable"); return; }
    esp_codec_dev_set_out_vol(s_codec,35);
    s_queue=xQueueCreate(4,sizeof(sfx_id_t));
    if(s_queue && xTaskCreate(sound_task,"pet_sound",4096,NULL,3,NULL)!=pdPASS) { vQueueDelete(s_queue); s_queue=NULL; }
}
void audio_play(sfx_id_t sfx) { if(s_queue && !atomic_load(&s_muted)) xQueueSend(s_queue,&sfx,0); }
void audio_set_muted(bool muted) { atomic_store(&s_muted,muted); }
void audio_set_volume(int percent)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    atomic_store(&s_volume, percent);
}
int audio_get_volume(void) { return atomic_load(&s_volume); }
