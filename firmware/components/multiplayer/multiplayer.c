#include "multiplayer.h"
#include "voice.h"
#include "audio.h"
#include "cJSON.h"
#include "esp_websocket_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <inttypes.h>

typedef struct {char type[12],key[40];unsigned seq,epoch,value;uint64_t receipt,pet;} Command;
static QueueHandle_t s_commands;
static esp_websocket_client_handle_t s_ws;
static portMUX_TYPE s_lock=portMUX_INITIALIZER_UNLOCKED;
static mp_state_t s_state;
static Pet s_pet;
static atomic_bool s_connected,s_hello,s_active;
static atomic_uint s_epoch;
static atomic_bool s_audio,s_audio_ended;
static unsigned s_audio_seq;
static char s_audio_room[33];
static TickType_t s_audio_end_at;
static int s_rxop;
static char s_rx[4096];static size_t s_rxlen;static bool s_rxvalid;
static const char *string(cJSON *o,const char *key)
{
    cJSON *v=cJSON_GetObjectItemCaseSensitive(o,key);return cJSON_IsString(v)?v->valuestring:"";
}
static unsigned number(cJSON *o,const char *key,unsigned maximum)
{
    cJSON *v=cJSON_GetObjectItemCaseSensitive(o,key);
    return cJSON_IsNumber(v)&&v->valuedouble>=0&&v->valuedouble<=maximum?(unsigned)v->valuedouble:0;
}
static void peer(cJSON *o,mp_peer_t *p)
{
    strlcpy(p->user,string(o,"user"),sizeof p->user);strlcpy(p->id,string(o,"id"),sizeof p->id);
    strlcpy(p->name,string(o,"name"),sizeof p->name);p->character=number(o,"character",PET_CHARACTER_COUNT-1);p->stage=number(o,"stage",5);
    p->chat=cJSON_IsTrue(cJSON_GetObjectItem(o,"chat"));p->games=cJSON_IsTrue(cJSON_GetObjectItem(o,"games"));
}
static void receive(void)
{
    cJSON *o=cJSON_Parse(s_rx);if(!o)return;
    if(!strcmp(string(o,"type"),"chat_audio_start") && atomic_load(&s_active)) {
        // A room may have been left while audio was queued on the socket.
        mp_state_t state;multiplayer_snapshot(&state);
        if(state.chat && state.phase==MP_PLAYING && state.my_turn &&
           !strcmp(state.room,string(o,"room")) && state.seq==number(o,"seq",8)) {
            strlcpy(s_audio_room,state.room,sizeof s_audio_room);s_audio_seq=state.seq;
            atomic_store(&s_audio_ended,false);atomic_store(&s_audio,true);audio_voice_begin();
        }
    } else if(!strcmp(string(o,"type"),"chat_audio_end") && atomic_load(&s_audio) &&
              !strcmp(s_audio_room,string(o,"room")) && s_audio_seq==number(o,"seq",8)) {
        audio_voice_end();s_audio_end_at=xTaskGetTickCount();atomic_store(&s_audio_ended,true);
    }
    if(!strcmp(string(o,"type"),"play_state")) {
        mp_state_t next={.connected=true};const char *phases[]={"offline","closed","lobby","outgoing","incoming","playing","waiting","finished"};
        for(unsigned i=0;i<8;i++)if(!strcmp(string(o,"phase"),phases[i]))next.phase=(mp_phase_t)i;
        next.seq=number(o,"seq",100000);next.my_turn=cJSON_IsTrue(cJSON_GetObjectItem(o,"my_turn"));
        next.online=cJSON_IsTrue(cJSON_GetObjectItem(o,"online"));next.again=cJSON_IsTrue(cJSON_GetObjectItem(o,"again"));
        const char *modes[]={"ball","chat","pegs","tilt","memory"};
        for(unsigned i=0;i<5;i++)if(!strcmp(string(o,"mode"),modes[i]))next.mode=(mp_mode_t)i;
        next.seed=number(o,"seed",UINT32_MAX);next.score=number(o,"score",200000);next.peer_score=number(o,"peer_score",200000);
        next.countdown_ms=number(o,"countdown_ms",3000);next.remaining_ms=number(o,"remaining_ms",240000);
        next.ready=cJSON_IsTrue(cJSON_GetObjectItem(o,"ready"));next.started=cJSON_IsTrue(cJSON_GetObjectItem(o,"started"));
        next.submitted=cJSON_IsTrue(cJSON_GetObjectItem(o,"submitted"));next.peer_submitted=cJSON_IsTrue(cJSON_GetObjectItem(o,"peer_submitted"));
        next.matched=number(o,"matched",4095);memset(next.cards,-1,sizeof next.cards);
        cJSON *cards=cJSON_GetObjectItem(o,"cards");
        for(unsigned i=0;i<12;i++){cJSON *v=cJSON_GetArrayItem(cards,i);if(cJSON_IsNumber(v) && v->valueint>=0 && v->valueint<6)next.cards[i]=v->valueint;}
        next.chat=!strcmp(string(o,"mode"),"chat");next.speaking=cJSON_IsTrue(cJSON_GetObjectItem(o,"speaking"));
        strlcpy(next.text,string(o,"text"),sizeof next.text);
        strlcpy(next.room,string(o,"room"),sizeof next.room);strlcpy(next.invite,string(o,"invite"),sizeof next.invite);
        strlcpy(next.notice,string(o,"notice"),sizeof next.notice);peer(cJSON_GetObjectItem(o,"peer"),&next.peer);
        cJSON *peers=cJSON_GetObjectItem(o,"peers"),*item;
        cJSON_ArrayForEach(item,peers) {if(next.count>=MP_MAX_PEERS)break;peer(item,&next.peers[next.count++]);}
        cJSON *reward=cJSON_GetObjectItem(o,"reward");
        next.reward_id=strtoull(string(reward,"id"),NULL,10);next.reward_pet=strtoull(string(reward,"pet"),NULL,16);
        next.reward_friend=strtoull(string(reward,"friend"),NULL,16);next.friends=number(reward,"friends",65535);
        if(atomic_load(&s_audio) && (!next.chat || !next.speaking || strcmp(next.room,s_audio_room))) {
            atomic_store(&s_audio,false);atomic_store(&s_audio_ended,false);audio_voice_stop();
        }
        portENTER_CRITICAL(&s_lock);s_state=next;portEXIT_CRITICAL(&s_lock);
        atomic_store(&s_connected,true);
    }
    cJSON_Delete(o);
}
static void event(void *arg,esp_event_base_t base,int32_t id,void *data)
{
    (void)arg;(void)base;
    if(id==WEBSOCKET_EVENT_CONNECTED) {s_rxlen=0;s_rxvalid=false;atomic_store(&s_hello,true);}
    else if(id==WEBSOCKET_EVENT_DISCONNECTED || id==WEBSOCKET_EVENT_ERROR || id==WEBSOCKET_EVENT_CLOSED) {
        if(atomic_exchange(&s_audio,false))audio_voice_stop();
        atomic_store(&s_audio_ended,false);
        atomic_store(&s_connected,false);
        portENTER_CRITICAL(&s_lock);s_state.connected=false;s_state.phase=MP_OFFLINE;portEXIT_CRITICAL(&s_lock);
    } else if(id==WEBSOCKET_EVENT_DATA) {
        esp_websocket_event_data_t *d=data;
        if(d->op_code==1 || d->op_code==2)s_rxop=d->op_code;
        if(s_rxop==2) {
            if(atomic_load(&s_audio) && atomic_load(&s_active) && d->data_len>0)audio_play_pcm((const uint8_t*)d->data_ptr,d->data_len);
            if(d->fin && d->payload_offset+d->data_len>=d->payload_len)s_rxop=0;
            return;
        }
        if(d->op_code==1 && d->payload_offset==0) {s_rxlen=0;s_rxvalid=true;}
        if(d->op_code!=1 && d->op_code!=0)return;
        if(d->data_len<0 || s_rxlen+(size_t)d->data_len>=sizeof s_rx) {s_rxvalid=false;return;}
        if(s_rxvalid && d->data_len) {memcpy(s_rx+s_rxlen,d->data_ptr,d->data_len);s_rxlen+=d->data_len;}
        if(d->fin && d->payload_offset+d->data_len>=d->payload_len) {
            if(s_rxvalid) {s_rx[s_rxlen]=0;receive();}s_rxlen=0;s_rxvalid=false;
        }
    }
}
static cJSON *message(const char *type)
{
    cJSON *o=cJSON_CreateObject();cJSON_AddStringToObject(o,"type",type);return o;
}
static bool send_json(cJSON *o)
{
    char *json=cJSON_PrintUnformatted(o);bool ok=false;
    if(json && s_ws && esp_websocket_client_is_connected(s_ws))
        ok=esp_websocket_client_send_text(s_ws,json,strlen(json),pdMS_TO_TICKS(1000))==(int)strlen(json);
    cJSON_free(json);cJSON_Delete(o);return ok;
}
static void add_pet(cJSON *o)
{
    Pet p;portENTER_CRITICAL(&s_lock);p=s_pet;portEXIT_CRITICAL(&s_lock);
    char id[17];snprintf(id,sizeof id,"%016" PRIx64,p.pet_id);
    cJSON *profile=cJSON_AddObjectToObject(o,"pet");cJSON_AddStringToObject(profile,"id",id);
    cJSON_AddStringToObject(profile,"name",p.name);cJSON_AddNumberToObject(profile,"character",pet_character_id(&p));
    cJSON_AddNumberToObject(profile,"stage",p.stage);
    cJSON_AddNumberToObject(profile,"fullness",p.hunger);cJSON_AddNumberToObject(profile,"happiness",p.happiness);
    cJSON_AddNumberToObject(profile,"energy",p.energy);cJSON_AddNumberToObject(profile,"cleanliness",p.hygiene);
    cJSON_AddStringToObject(profile,"voice_preset",pet_voice(pet_voice_id(&p))->id);
    cJSON_AddNumberToObject(profile,"stars",p.evolution_progress);cJSON_AddNumberToObject(profile,"personality",p.genes[7]%8);
}
static void worker(void *arg)
{
    (void)arg;unsigned sent_epoch=UINT32_MAX;TickType_t ping=0;
    while(!voice_wifi_connected())vTaskDelay(pdMS_TO_TICKS(200));
    esp_websocket_client_start(s_ws);
    for(;;) {
        if(atomic_exchange(&s_hello,false)) {
            cJSON *o=message("hello");cJSON_AddNumberToObject(o,"proto",1);cJSON_AddNumberToObject(o,"artwork",3);
            cJSON_AddNumberToObject(o,"chat",1);cJSON_AddNumberToObject(o,"games",2);
            cJSON_AddStringToObject(o,"user_id",voice_device_id());cJSON_AddStringToObject(o,"device_token",voice_device_token());
            add_pet(o);send_json(o);sent_epoch=UINT32_MAX;ping=xTaskGetTickCount();
        }
        unsigned epoch=atomic_load(&s_epoch);
        if(atomic_load(&s_connected) && epoch!=sent_epoch) {
            bool active=atomic_load(&s_active);cJSON *o=message(active?"join":"leave");if(active)add_pet(o);
            if(send_json(o))sent_epoch=epoch;
        }
        Command c;
        if(xQueueReceive(s_commands,&c,pdMS_TO_TICKS(100))==pdTRUE && atomic_load(&s_connected) && (c.epoch==atomic_load(&s_epoch) || !strcmp(c.type,"ack"))) {
            cJSON *o=message(c.type);
            if(!strcmp(c.type,"invite")) {cJSON_AddStringToObject(o,"user",c.key);cJSON_AddStringToObject(o,"mode",((const char*[]){"ball","chat","pegs","tilt","memory"})[c.seq<=MP_MEMORY?c.seq:0]);}
            if(!strcmp(c.type,"accept") || !strcmp(c.type,"decline"))cJSON_AddStringToObject(o,"invite",c.key);
            if(!strcmp(c.type,"pass") || !strcmp(c.type,"again") || !strcmp(c.type,"ready") || !strcmp(c.type,"score") || !strcmp(c.type,"flip")) {cJSON_AddStringToObject(o,"room",c.key);cJSON_AddNumberToObject(o,"seq",c.seq);}
            if(!strcmp(c.type,"score"))cJSON_AddNumberToObject(o,"score",c.seq);
            if(!strcmp(c.type,"flip"))cJSON_AddNumberToObject(o,"card",c.value);
            if(!strcmp(c.type,"ack")) {
                char id[24],pet[17];snprintf(id,sizeof id,"%" PRIu64,c.receipt);snprintf(pet,sizeof pet,"%016" PRIx64,c.pet);
                cJSON_AddStringToObject(o,"id",id);cJSON_AddStringToObject(o,"pet",pet);
            }
            send_json(o);
        }
        if(atomic_load(&s_audio_ended) && !audio_voice_playing() && xTaskGetTickCount()-s_audio_end_at>pdMS_TO_TICKS(250)) {
            cJSON *o=message("heard");cJSON_AddStringToObject(o,"room",s_audio_room);cJSON_AddNumberToObject(o,"seq",s_audio_seq);
            if(send_json(o)) {atomic_store(&s_audio_ended,false);atomic_store(&s_audio,false);}
        }
        if(atomic_load(&s_connected) && xTaskGetTickCount()-ping>pdMS_TO_TICKS(5000)) {send_json(message("ping"));ping=xTaskGetTickCount();}
    }
}
void multiplayer_init(void)
{
    s_pet=*pet_state_get();s_commands=xQueueCreate(12,sizeof(Command));if(!s_commands)return;
    char uri[128];strlcpy(uri,voice_gateway(),sizeof uri);char *path=strrchr(uri,'/');if(path)strlcpy(path,"/play",sizeof uri-(size_t)(path-uri));
    esp_websocket_client_config_t cfg={.uri=uri,.buffer_size=2048,.task_stack=6144,.enable_close_reconnect=true,.reconnect_timeout_ms=5000,.network_timeout_ms=5000,.ping_interval_sec=15};
    s_ws=esp_websocket_client_init(&cfg);if(!s_ws)return;
    esp_websocket_register_events(s_ws,WEBSOCKET_EVENT_ANY,event,NULL);
    xTaskCreate(worker,"playdates",6144,NULL,3,NULL);
}
void multiplayer_open(const Pet *p)
{
    portENTER_CRITICAL(&s_lock);s_pet=*p;portEXIT_CRITICAL(&s_lock);
    atomic_store(&s_active,true);atomic_fetch_add(&s_epoch,1);
}
void multiplayer_close(void) {atomic_store(&s_active,false);atomic_fetch_add(&s_epoch,1);if(atomic_exchange(&s_audio,false))audio_voice_stop();atomic_store(&s_audio_ended,false);}
void multiplayer_snapshot(mp_state_t *out) {portENTER_CRITICAL(&s_lock);*out=s_state;portEXIT_CRITICAL(&s_lock);}
static bool command(const char *type,const char *key,unsigned seq)
{
    if(!s_commands || !atomic_load(&s_connected) || !atomic_load(&s_active))return false;
    Command c={.seq=seq,.epoch=atomic_load(&s_epoch)};strlcpy(c.type,type,sizeof c.type);strlcpy(c.key,key,sizeof c.key);
    return xQueueSend(s_commands,&c,0)==pdTRUE;
}
bool multiplayer_invite(const char *user) {return command("invite",user,0);}
bool multiplayer_chat_invite(const char *user) {return command("invite",user,1);}
bool multiplayer_accept(const char *invite) {return command("accept",invite,0);}
bool multiplayer_decline(const char *invite) {return command("decline",invite,0);}
bool multiplayer_pass(const char *room,unsigned seq) {return command("pass",room,seq);}
bool multiplayer_again(const char *room) {return command("again",room,0);}
void multiplayer_ack(uint64_t receipt,uint64_t pet)
{
    if(s_commands) {Command c={.receipt=receipt,.pet=pet};strlcpy(c.type,"ack",sizeof c.type);xQueueSend(s_commands,&c,0);}
}

bool multiplayer_game_invite(const char *user,mp_mode_t mode) {return mode>=MP_PEGS && mode<=MP_MEMORY && command("invite",user,mode);}
bool multiplayer_ready(const char *room) {return command("ready",room,0);}
bool multiplayer_score(const char *room,unsigned score) {return command("score",room,score);}
bool multiplayer_flip(const char *room,unsigned seq,unsigned card)
{
    if(card>=12 || !s_commands || !atomic_load(&s_connected) || !atomic_load(&s_active))return false;
    Command c={.seq=seq,.epoch=atomic_load(&s_epoch),.value=card};strlcpy(c.type,"flip",sizeof c.type);strlcpy(c.key,room,sizeof c.key);
    return xQueueSend(s_commands,&c,0)==pdTRUE;
}
