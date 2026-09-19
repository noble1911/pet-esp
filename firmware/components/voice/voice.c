#include "voice.h"
#include "audio.h"
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
#include "cJSON.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_websocket_client.h"
#include "esp_log.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#if __has_include("secrets.h")
#include "secrets.h"
#endif
#ifndef CFG_WIFI_SSID
#define CFG_WIFI_SSID ""
#define CFG_WIFI_PASS ""
#endif
#ifndef PET_DEVICE_TOKEN
#define PET_DEVICE_TOKEN ""
#endif
#define PET_USER_ID "pet-meadow-3cdc756e3104"
#define PET_GATEWAY "ws://192.168.1.117:8770/ws"

typedef struct { int kind; Pet pet; char activity[32]; pet_event_t event; unsigned event_age; size_t len; uint8_t pcm[640]; } Command;
enum { START=1, END, CANCEL, PCM, CHECK, HELLO, REMARK, REACT, MUSIC };
static QueueHandle_t s_commands;
static esp_websocket_client_handle_t s_ws;
static atomic_bool s_auto=true;
static atomic_int s_recent_event=PET_EVENT_NONE;
static atomic_uint s_event_at;
static const char *event_names[]={"", "cuddle", "ate_apple", "ate_toast", "ate_cookie",
    "caught_star", "finished_star_game", "popped_bubble", "finished_bath", "finished_nap",
    "finished_hide_game", "finished_ball_game", "butterfly_visit", "ate_star_cupcake", "ate_berry_pancakes", "ate_rainbow_jelly", "ate_party_cake"};
void voice_note_event(pet_event_t event)
{
    if((unsigned)event>=sizeof(event_names)/sizeof(event_names[0]))return;
    atomic_store(&s_event_at,xTaskGetTickCount());atomic_store(&s_recent_event,event);
}
static atomic_int s_state=VOICE_OFFLINE;
static atomic_bool s_wifi, s_ready, s_capturing, s_overflow, s_requested;
static portMUX_TYPE s_lock=portMUX_INITIALIZER_UNLOCKED;
static char s_ip[20]="--", s_caption[512]="", s_check[48]="";
static char s_rx[8192];
static size_t s_rxlen;
static int s_rxop;
static uint32_t s_last_pong, s_check_started;
static void caption_set(const char *s) { portENTER_CRITICAL(&s_lock);strlcpy(s_caption,s,sizeof(s_caption));portEXIT_CRITICAL(&s_lock); }
void voice_caption(char *s,size_t n) { portENTER_CRITICAL(&s_lock);strlcpy(s,s_caption,n);portEXIT_CRITICAL(&s_lock); }
voice_state_t voice_get_state(void) { return (voice_state_t)atomic_load(&s_state); }
bool voice_boot_pressed(void) { return gpio_get_level(GPIO_NUM_0)==0; }
static bool send_json(cJSON *obj)
{
    char *s=cJSON_PrintUnformatted(obj);bool ok=false;
    if(s && s_ws && esp_websocket_client_is_connected(s_ws))
        ok=esp_websocket_client_send_text(s_ws,s,strlen(s),pdMS_TO_TICKS(1500))==(int)strlen(s);
    cJSON_free(s);cJSON_Delete(obj);return ok;
}
static cJSON *message(const char *type) { cJSON *o=cJSON_CreateObject();cJSON_AddStringToObject(o,"type",type);return o; }
static void add_pet(cJSON *o,const Command *c)
{
    const Pet *p=&c->pet;cJSON *j=cJSON_AddObjectToObject(o,"pet");char id[17];
    snprintf(id,sizeof(id),"%016llx",(unsigned long long)p->pet_id);
    cJSON_AddStringToObject(j,"pet_id",id);cJSON_AddStringToObject(j,"name",p->name);
    cJSON_AddNumberToObject(j,"stage",p->stage);
    cJSON_AddNumberToObject(j,"artwork_version",2);
    cJSON_AddNumberToObject(j,"fullness",p->hunger);cJSON_AddNumberToObject(j,"happiness",p->happiness);
    cJSON_AddNumberToObject(j,"energy",p->energy);cJSON_AddNumberToObject(j,"cleanliness",p->hygiene);
    cJSON_AddNumberToObject(j,"stars",p->evolution_progress);cJSON_AddNumberToObject(j,"generation",p->generation);
    cJSON_AddNumberToObject(j,"friends_met",p->friends_met);cJSON_AddStringToObject(j,"activity",c->activity);
    if(c->event!=PET_EVENT_NONE && c->event_age<=120) {
        cJSON_AddStringToObject(j,"recent_event",event_names[c->event]);
        cJSON_AddNumberToObject(j,"recent_event_age_seconds",c->event_age);
    }
    cJSON *a=cJSON_AddArrayToObject(j,"genes");for(int i=0;i<8;i++) cJSON_AddItemToArray(a,cJSON_CreateNumber(p->genes[i]));
    a=cJSON_AddArrayToObject(j,"inventory");for(int i=0;i<16;i++) cJSON_AddItemToArray(a,cJSON_CreateNumber(p->inventory[i]));
}
static bool enqueue(int kind,const Pet *pet,const char *activity)
{
    if(!s_commands) return false;
    Command c={.kind=kind};if(pet)c.pet=*pet;if(activity)strlcpy(c.activity,activity,sizeof(c.activity));
    c.event=(pet_event_t)atomic_load(&s_recent_event);
    c.event_age=(unsigned)((xTaskGetTickCount()-atomic_load(&s_event_at))/configTICK_RATE_HZ);
    if(xQueueSend(s_commands,&c,0)!=pdTRUE) {atomic_store(&s_overflow,true);return false;}
    return true;
}
void voice_start_talk(const Pet *pet,const char *activity)
{
    if(!atomic_load(&s_ready)) { caption_set("Voice is offline. Check Wi-Fi in Options.");return; }
    if(!audio_is_ready()) { caption_set("My microphone needs a restart.");atomic_store(&s_state,VOICE_ERROR);return; }
    if(atomic_exchange(&s_requested,true)) return;
    audio_voice_stop();caption_set("");atomic_store(&s_state,VOICE_LISTENING);enqueue(START,pet,activity);
}
void voice_end_talk(const Pet *pet,const char *activity)
{
    audio_set_capture(false);atomic_store(&s_capturing,false);
    if(atomic_exchange(&s_requested,false)) { atomic_store(&s_state,VOICE_THINKING);enqueue(END,pet,activity); }
}
void voice_cancel(void) { atomic_store(&s_requested,false);audio_set_capture(false);atomic_store(&s_capturing,false);audio_voice_stop();enqueue(CANCEL,NULL,NULL); }
void voice_check(void) { enqueue(CHECK,NULL,NULL); }
bool voice_auto_enabled(void) { return atomic_load(&s_auto); }
void voice_set_auto_enabled(bool enabled)
{
    atomic_store(&s_auto,enabled);
    nvs_handle_t h;
    if(nvs_open("pet_voice",NVS_READWRITE,&h)==ESP_OK) {
        nvs_set_u8(h,"auto",enabled);nvs_commit(h);nvs_close(h);
    }
}
static bool auto_turn(int kind,const Pet *pet,const char *activity)
{
    if(!voice_auto_enabled() || !audio_is_ready() || !atomic_load(&s_ready) ||
       atomic_load(&s_requested) || voice_get_state()!=VOICE_READY || audio_voice_playing())return false;
    caption_set("");atomic_store(&s_state,VOICE_THINKING);
    if(!enqueue(kind,pet,activity)) {atomic_store(&s_state,VOICE_READY);return false;}
    return true;
}
bool voice_make_tune(const Pet *pet,const char *activity)
{
    if(!audio_is_ready() || !atomic_load(&s_ready) || atomic_load(&s_requested) ||
       voice_get_state()!=VOICE_READY || audio_voice_playing())return false;
    audio_stop_tune();caption_set("");atomic_store(&s_state,VOICE_THINKING);
    if(!enqueue(MUSIC,pet,activity)){atomic_store(&s_state,VOICE_READY);return false;}
    return true;
}
bool voice_remark(const Pet *pet,const char *activity) {return auto_turn(REMARK,pet,activity);}
bool voice_react(const Pet *pet,const char *activity) {return auto_turn(REACT,pet,activity);}
static void mic(const uint8_t *p,size_t n)
{
    if(!atomic_load(&s_capturing) || n>640) return;
    Command c={.kind=PCM,.len=n};memcpy(c.pcm,p,n);
    if(xQueueSend(s_commands,&c,0)!=pdTRUE) atomic_store(&s_overflow,true);
}
void voice_status(char *out,size_t n)
{
    wifi_ap_record_t ap;bool connected=atomic_load(&s_wifi);int rssi=0;
    if(connected && esp_wifi_sta_get_ap_info(&ap)==ESP_OK) rssi=ap.rssi;
    char ip[20],check[48];portENTER_CRITICAL(&s_lock);strlcpy(ip,s_ip,sizeof(ip));strlcpy(check,s_check,sizeof(check));portEXIT_CRITICAL(&s_lock);
    snprintf(out,n,"Wi-Fi: %s\n%s\nIP: %s\nSignal: %d dBm\nVoice server: %s\n%s",
        connected?"Connected":"Connecting...",CFG_WIFI_SSID,ip,rssi,atomic_load(&s_ready)?"Ready":"Offline",check);
}
static void handle_json(void)
{
    cJSON *o=cJSON_Parse(s_rx);if(!o)return;
    const cJSON *t=cJSON_GetObjectItem(o,"type"),*v=cJSON_GetObjectItem(o,"value");
    if(cJSON_IsString(t)) {
        const char *type=t->valuestring;
        if(!strcmp(type,"ready")) { atomic_store(&s_ready,true);atomic_store(&s_state,VOICE_READY);ESP_LOGI("voice","pet gateway ready"); }
        else if(!strcmp(type,"state") && cJSON_IsString(v)) {
            // Never let a delayed idle/listening event overwrite a new local capture.
            if(!atomic_load(&s_requested) && strcmp(v->valuestring,"listening")) atomic_store(&s_state,!strcmp(v->valuestring,"thinking")?VOICE_THINKING:!strcmp(v->valuestring,"speaking")?VOICE_SPEAKING:VOICE_READY);
        } else if(!strcmp(type,"say")) {
            const cJSON *text=cJSON_GetObjectItem(o,"text");
            if(cJSON_IsString(text) && voice_get_state()!=VOICE_LISTENING) { portENTER_CRITICAL(&s_lock);strlcat(s_caption,text->valuestring,sizeof(s_caption));portEXIT_CRITICAL(&s_lock); }
        } else if(!strcmp(type,"tts_start") && voice_get_state()!=VOICE_LISTENING) audio_voice_begin();
        else if(!strcmp(type,"tts_end")) audio_voice_end();
        else if(!strcmp(type,"error")) { atomic_store(&s_state,VOICE_ERROR);audio_set_capture(false);atomic_store(&s_capturing,false);audio_voice_stop();const cJSON *code=cJSON_GetObjectItem(o,"code");
            const char *reason=cJSON_IsString(code)?code->valuestring:"unknown";
            caption_set(!strcmp(reason,"no_speech")?"I didn't catch that. Hold and speak close to me.":!strcmp(reason,"no_audio")?"My microphone didn't send sound. Please restart me.":"I couldn't hear back. Please try again.");
            ESP_LOGW("voice","gateway error: %s",reason); }
        else if(!strcmp(type,"pong")) { portENTER_CRITICAL(&s_lock);s_last_pong=xTaskGetTickCount();snprintf(s_check,sizeof(s_check),"Check passed: server replied");portEXIT_CRITICAL(&s_lock); }
    }
    cJSON_Delete(o);
}
static void ws_event(void *arg,esp_event_base_t base,int32_t id,void *event)
{
    (void)arg;(void)base;esp_websocket_event_data_t *d=event;
    if(id==WEBSOCKET_EVENT_CONNECTED) { s_rxlen=0;s_rxop=0;enqueue(HELLO,NULL,NULL); }
    else if(id==WEBSOCKET_EVENT_DISCONNECTED) {
        atomic_store(&s_ready,false);atomic_store(&s_state,VOICE_OFFLINE);atomic_store(&s_capturing,false);atomic_store(&s_requested,false);
        audio_set_capture(false);audio_voice_stop();s_rxlen=0;s_rxop=0;
    } else if(id==WEBSOCKET_EVENT_DATA) {
        int op=d->op_code;if(op==1 || op==2)s_rxop=op;
        if(s_rxop==2) { if(d->data_len>0)audio_play_pcm((const uint8_t*)d->data_ptr,d->data_len); }
        else if(s_rxop==1) {
            if(s_rxlen+d->data_len>=sizeof(s_rx)) { s_rxlen=0;s_rxop=0;return; }
            memcpy(s_rx+s_rxlen,d->data_ptr,d->data_len);s_rxlen+=d->data_len;
        }
        if(d->payload_offset+d->data_len>=d->payload_len && d->fin) {
            if(s_rxop==1) { s_rx[s_rxlen]=0;handle_json(); }
            s_rxlen=0;s_rxop=0;
        }
    }
}
static void wifi_event(void *arg,esp_event_base_t base,int32_t id,void *data)
{
    (void)arg;
    if(base==WIFI_EVENT && id==WIFI_EVENT_STA_START) esp_wifi_connect();
    else if(base==WIFI_EVENT && id==WIFI_EVENT_STA_DISCONNECTED) { atomic_store(&s_wifi,false);esp_wifi_connect(); }
    else if(base==IP_EVENT && id==IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *e=data;char ip[20];snprintf(ip,sizeof(ip),IPSTR,IP2STR(&e->ip_info.ip));
        portENTER_CRITICAL(&s_lock);strlcpy(s_ip,ip,sizeof(s_ip));portEXIT_CRITICAL(&s_lock);
        atomic_store(&s_wifi,true);ESP_LOGI("voice","Wi-Fi connected: %s",ip);
    }
}
static void worker(void *arg)
{
    (void)arg;Command c;uint32_t capture_started=0, turn_started=0;
    while(!atomic_load(&s_wifi)) vTaskDelay(pdMS_TO_TICKS(200));
    esp_websocket_client_start(s_ws);
    for(;;) {
        if(atomic_exchange(&s_overflow,false)) { voice_cancel();caption_set("Connection was slow. Please try again."); }
        uint32_t now=xTaskGetTickCount();
        if(atomic_load(&s_capturing) && now-capture_started>pdMS_TO_TICKS(25000)) {
            // UI releases normally at 20 seconds. Worker fallback never leaves mic on.
            voice_cancel();caption_set("Hold Talk to try again.");
        }
        if(voice_get_state()==VOICE_THINKING && turn_started && now-turn_started>pdMS_TO_TICKS(90000)) {
            voice_cancel();caption_set("Voice took too long. Please try again.");turn_started=0;
        }
        portENTER_CRITICAL(&s_lock);
        if(s_check_started && now-s_check_started>pdMS_TO_TICKS(5000)) {
            if(s_last_pong<s_check_started)strlcpy(s_check,"No reply. Try checking again.",sizeof(s_check));
            s_check_started=0;
        }
        portEXIT_CRITICAL(&s_lock);
        if(xQueueReceive(s_commands,&c,pdMS_TO_TICKS(100))!=pdTRUE) continue;
        cJSON *o=NULL;
        if(c.kind==HELLO) {
            o=message("hello");cJSON_AddNumberToObject(o,"proto",1);cJSON_AddStringToObject(o,"device_token",PET_DEVICE_TOKEN);
            cJSON_AddStringToObject(o,"user_id",PET_USER_ID);cJSON_AddStringToObject(o,"surface","pet");
            cJSON *a=cJSON_AddObjectToObject(o,"capture");cJSON_AddNumberToObject(a,"rate",16000);
            a=cJSON_AddObjectToObject(o,"playback");cJSON_AddNumberToObject(a,"rate",16000);send_json(o);
        } else if(c.kind==START && atomic_load(&s_ready)) {
            o=message("audio_start");add_pet(o,&c);
            if(send_json(o)) { capture_started=now;atomic_store(&s_capturing,true);audio_set_capture(true);ESP_LOGI("voice","capture started; mic frames=%u",audio_mic_frames()); }
            else { atomic_store(&s_requested,false);atomic_store(&s_state,VOICE_ERROR);caption_set("Connection lost. Please try again."); }
        } else if(c.kind==END) {
            audio_set_capture(false);atomic_store(&s_capturing,false);
            o=message("audio_end");add_pet(o,&c);send_json(o);turn_started=now;ESP_LOGI("voice","capture ended; mic frames=%u peak=%d",audio_mic_frames(),audio_mic_level());
        } else if(c.kind==REMARK || c.kind==REACT || c.kind==MUSIC) {
            // A manual press takes priority, even if it races this queued remark.
            if(atomic_load(&s_requested))continue;
            if(c.kind!=MUSIC && !voice_auto_enabled()) {atomic_store(&s_state,VOICE_READY);continue;}
            o=message("text");add_pet(o,&c);cJSON_AddBoolToObject(o,"proactive",c.kind!=MUSIC);
            cJSON_AddStringToObject(o,"text",c.kind==MUSIC?"I tapped Make me a tune on the toy. Please use compose_tune to create and play an original little melody for me, with just a short introduction.":c.kind==REACT?"A device care event just happened. React briefly to the recent_event in the snapshot; nobody has spoken.":"A quiet moment in the pet's room.");
            if(!send_json(o)) {atomic_store(&s_state,VOICE_READY);continue;}
            turn_started=now;ESP_LOGI("voice","automatic pet %s requested",c.kind==REACT?"reaction":"remark");
        } else if(c.kind==PCM && atomic_load(&s_ready)) {
            if(esp_websocket_client_send_bin(s_ws,(char*)c.pcm,c.len,pdMS_TO_TICKS(300))!=(int)c.len) atomic_store(&s_overflow,true);
        } else if(c.kind==CANCEL) { send_json(message("cancel"));atomic_store(&s_state,atomic_load(&s_ready)?VOICE_READY:VOICE_OFFLINE); }
        else if(c.kind==CHECK) {
            portENTER_CRITICAL(&s_lock);s_check_started=now;strlcpy(s_check,"Checking server...",sizeof(s_check));portEXIT_CRITICAL(&s_lock);
            if(!atomic_load(&s_wifi))esp_wifi_connect();else send_json(message("ping"));
        }
    }
}
void voice_init(void)
{
    nvs_handle_t h;uint8_t enabled=1;
    if(nvs_open("pet_voice",NVS_READONLY,&h)==ESP_OK) {nvs_get_u8(h,"auto",&enabled);nvs_close(h);}
    atomic_store(&s_auto,enabled!=0);
    ESP_LOGI("voice","pet identity: %016llx (%s)",(unsigned long long)pet_state_get()->pet_id,pet_state_get()->name);
    gpio_config_t btn={.pin_bit_mask=1ULL<<GPIO_NUM_0,.mode=GPIO_MODE_INPUT,.pull_up_en=GPIO_PULLUP_ENABLE};gpio_config(&btn);
    s_commands=xQueueCreate(24,sizeof(Command));if(!s_commands)return;
    audio_set_mic_callback(mic);
    if(!strlen(CFG_WIFI_SSID) || !strlen(PET_DEVICE_TOKEN)) { caption_set("Voice needs Wi-Fi setup");return; }
    if(esp_netif_init()!=ESP_OK || esp_event_loop_create_default()!=ESP_OK) return;
    esp_netif_create_default_wifi_sta();wifi_init_config_t init=WIFI_INIT_CONFIG_DEFAULT();
    if(esp_wifi_init(&init)!=ESP_OK)return;
    esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,wifi_event,NULL);
    esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP,wifi_event,NULL);
    wifi_config_t cfg={0};strlcpy((char*)cfg.sta.ssid,CFG_WIFI_SSID,sizeof(cfg.sta.ssid));strlcpy((char*)cfg.sta.password,CFG_WIFI_PASS,sizeof(cfg.sta.password));
    esp_wifi_set_mode(WIFI_MODE_STA);esp_wifi_set_config(WIFI_IF_STA,&cfg);esp_wifi_start();esp_wifi_set_ps(WIFI_PS_NONE);
    esp_websocket_client_config_t ws={.uri=PET_GATEWAY,.buffer_size=2048,.reconnect_timeout_ms=3000,.network_timeout_ms=10000,.keep_alive_enable=true,.keep_alive_idle=5,.keep_alive_interval=5,.keep_alive_count=3,.task_stack=6144};
    s_ws=esp_websocket_client_init(&ws);if(!s_ws)return;
    esp_websocket_register_events(s_ws,WEBSOCKET_EVENT_ANY,ws_event,NULL);
    xTaskCreate(worker,"pet_voice",6144,NULL,4,NULL);
}
