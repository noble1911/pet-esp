#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
typedef void *QueueHandle_t;
typedef void *StreamBufferHandle_t;
typedef void *TaskHandle_t;
typedef struct {int unused;} StaticStreamBuffer_t;
typedef void *esp_codec_dev_handle_t;
typedef struct {int sample_rate,channel,bits_per_sample;} esp_codec_dev_sample_info_t;
#define ESP_OK 0
#define pdTRUE 1
#define pdPASS 1
#define pdMS_TO_TICKS(x) (x)
#define MALLOC_CAP_INTERNAL 1
#define MALLOC_CAP_8BIT 2
#define MALLOC_CAP_SPIRAM 4
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
#define ESP_LOGW(...) ((void)0)
int xQueueReceive(QueueHandle_t,void *,unsigned);
int xQueueOverwrite(QueueHandle_t,const void *);
QueueHandle_t xQueueCreate(unsigned,size_t);
size_t xStreamBufferReceive(StreamBufferHandle_t,void *,size_t,unsigned);
size_t xStreamBufferSend(StreamBufferHandle_t,const void *,size_t,unsigned);
StreamBufferHandle_t xStreamBufferCreateStatic(size_t,size_t,uint8_t *,StaticStreamBuffer_t *);
int esp_codec_dev_write(esp_codec_dev_handle_t,void *,size_t);
int esp_codec_dev_read(esp_codec_dev_handle_t,void *,size_t);
int esp_codec_dev_set_out_vol(esp_codec_dev_handle_t,int);
int esp_codec_dev_set_in_gain(esp_codec_dev_handle_t,float);
int esp_codec_dev_open(esp_codec_dev_handle_t,const esp_codec_dev_sample_info_t *);
int bsp_audio_init(void *);
esp_codec_dev_handle_t bsp_audio_codec_speaker_init(void);
esp_codec_dev_handle_t bsp_audio_codec_microphone_init(void);
void *heap_caps_malloc(size_t,unsigned);
size_t heap_caps_get_largest_free_block(unsigned);
int xTaskCreatePinnedToCore(void (*)(void *),const char *,unsigned,void *,unsigned,TaskHandle_t *,unsigned);
void vTaskDelete(TaskHandle_t);
void vTaskDelay(unsigned);
