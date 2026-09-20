#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nvs.h"
#include "../../firmware/components/ui/ui.c"
static mp_state_t test_mp;
static unsigned test_mp_chat_invites;
static unsigned test_mp_open,test_mp_close,test_mp_passes,test_mp_accepts,test_mp_declines,test_mp_invites,test_mp_agains,test_mp_acks;
void multiplayer_init(void) {}
void multiplayer_open(const Pet *p) {assert(p);test_mp_open++;}
void multiplayer_close(void) {test_mp_close++;}
void multiplayer_snapshot(mp_state_t *out) {*out=test_mp;}
bool multiplayer_invite(const char *p) {assert(*p);test_mp_invites++;return true;}
bool multiplayer_chat_invite(const char *p) {assert(*p);test_mp_chat_invites++;return true;}
bool multiplayer_accept(const char *p) {assert(*p);test_mp_accepts++;return true;}
bool multiplayer_decline(const char *p) {assert(*p);test_mp_declines++;return true;}
bool multiplayer_pass(const char *p,unsigned seq) {assert(*p && seq==test_mp.seq);test_mp_passes++;return true;}
bool multiplayer_again(const char *p) {assert(*p);test_mp_agains++;return true;}
void multiplayer_ack(uint64_t receipt,uint64_t pet) {assert(receipt && pet==pet_state_get()->pet_id);test_mp_acks++;}
static unsigned test_restarts;
void esp_restart(void) {test_restarts++;}
static Pet saved;
static bool has_save, fail_save;
uint32_t esp_random(void) { static uint32_t seed=15; seed=seed*1664525+1013904223; return seed; }
int nvs_open(const char *n,int mode,int *h) { (void)n;(void)mode;*h=1;return 0; }
int nvs_get_blob(int h,const char *k,void *out,size_t *n) { (void)h;(void)k;if(!has_save)return 1;memcpy(out,&saved,sizeof saved);*n=sizeof saved;return 0; }
int nvs_set_blob(int h,const char *k,const void *p,size_t n) { (void)h;(void)k;if(fail_save)return 1;assert(n==sizeof saved);memcpy(&saved,p,n);has_save=true;return 0; }
int nvs_commit(int h) { (void)h;return 0; }
void nvs_close(int h) { (void)h; }
int nvs_erase_key(int h,const char *k) { (void)h;(void)k;has_save=false;return 0; }
bool renderer_lock(uint32_t ms) { (void)ms;return true; }
void renderer_unlock(void) {}
int power_battery_percent(void) { return 85; }
bool power_is_charging(void) { return true; }
static bool test_power_press, test_power_off_ok=true;
static unsigned test_power_offs;
bool power_take_short_press(void) {bool press=test_power_press;test_power_press=false;return press;}
bool power_request_off(void) {test_power_offs++;return test_power_off_ok;}
static sfx_id_t test_sfx;
void audio_play(sfx_id_t f) {test_sfx=f;}
static bool test_tune;
static unsigned test_tune_choice, test_compositions;
bool audio_play_tune(unsigned n) {test_tune=n<audio_tune_count();test_tune_choice=n;return test_tune;}
void audio_stop_tune(void) {test_tune=false;}
bool audio_tune_playing(void) {return test_tune;}
bool voice_make_tune(const Pet *p,const char *a) {(void)p;(void)a;test_compositions++;return true;}
static bool test_audio_muted;
void audio_set_muted(bool m) {test_audio_muted=m;}
static bool test_boot, test_auto=true;
static unsigned test_remarks, test_reactions;
static pet_event_t test_event;
void voice_note_event(pet_event_t e) {test_event=e;}
bool voice_react(const Pet *p,const char *a) {(void)p;(void)a;test_reactions++;return true;}
bool voice_auto_enabled(void) {return test_auto;}
void voice_set_auto_enabled(bool b) {test_auto=b;}
bool voice_remark(const Pet *p,const char *a) {(void)p;(void)a;test_remarks++;return true;}

static int test_starts, test_ends;
static voice_state_t test_voice=VOICE_READY;
void voice_init(void) {}
void voice_start_talk(const Pet *p,const char *a) {assert(p && a);test_starts++;test_voice=VOICE_LISTENING;}
void voice_end_talk(const Pet *p,const char *a) {assert(p && a);test_ends++;test_voice=VOICE_THINKING;}
static unsigned test_previews,test_cancels,test_preview_choice;
void voice_cancel(void) {test_cancels++;}
bool voice_preview(unsigned choice) {test_previews++;test_preview_choice=choice;return test_voice!=VOICE_OFFLINE;}
void voice_check(void) {}
voice_state_t voice_get_state(void) {return test_voice;}
void voice_status(char *s,size_t n) {snprintf(s,n,"Wi-Fi: Connected\nHome network\nIP: 192.168.1.42\nSignal: -52 dBm\nVoice server: Ready\nCheck passed: server replied");}
static const char *test_caption="Hello! I'm Sprout. Shall we play?";
void voice_caption(char *s,size_t n) {snprintf(s,n,"%s",test_caption);}
bool voice_boot_pressed(void) {return test_boot;}
static bool test_playing;
bool audio_voice_playing(void) {return test_playing;}
bool audio_is_ready(void) {return true;}
int audio_mic_level(void) {return 800;}
static int test_volume=100;
void audio_set_volume(int v) { test_volume=v; }
int audio_get_volume(void) { return test_volume; }
static uint32_t pixels[368*448],partial_pixels[368*40];
static bool partial_render;
static int tx,ty; static bool pressed;
static void read_touch(lv_indev_t *i,lv_indev_data_t *d) { (void)i; d->point.x=tx;d->point.y=ty;d->state=pressed?LV_INDEV_STATE_PRESSED:LV_INDEV_STATE_RELEASED; }
static void flush(lv_display_t *d,const lv_area_t *a,uint8_t *p) {
    if(partial_render)for(int y=a->y1;y<=a->y2;y++)
        memcpy(&pixels[y*368+a->x1],p+(y-a->y1)*(a->x2-a->x1+1)*4,(a->x2-a->x1+1)*4);
    lv_display_flush_ready(d);
}
static void advance(unsigned ms) { for(unsigned n=0;n<ms;n+=20) { lv_tick_inc(20);lv_timer_handler(); } }
static void tap(int x,int y) { lv_obj_update_layout(s_root);advance(40);tx=x;ty=y;pressed=true;advance(60);pressed=false;advance(60); }
static void shot(const char *name) {
    advance(60); lv_refr_now(NULL);
    char path[256];snprintf(path,sizeof path,"%s.ppm",name);FILE *f=fopen(path,"wb");assert(f);
    fprintf(f,"P6\n368 448\n255\n");for(int i=0;i<368*448;i++) { unsigned char b[]={pixels[i]>>16,pixels[i]>>8,pixels[i]};fwrite(b,1,3,f); }fclose(f);
}
#include "character_checks.h"
int main(void)
{
    character_checks();
    if(getenv("PET_CAPTURE_CHARACTERS"))return 0;
    pet_state_init(); assert(pet_state_get()->hunger==100);
    uint32_t tick=pet_state_get()->last_tick;
    pet_state_tick(tick+179);assert(pet_state_get()->hunger==100);
    pet_state_tick(tick+180);assert(pet_state_get()->hunger==99);
    for(int i=1;i<=200;i++)pet_state_tick(tick+180+i*3600);
    assert(pet_state_get()->hunger==20);assert(pet_state_get()->energy==20);
    pet_state_tick(0);assert(pet_state_get()->hunger==20);
    pet_state_feed();assert(pet_state_get()->hunger==50);
    for(int i=0;i<200;i++)pet_state_feed();
    assert(pet_state_get()->hunger==100);assert(pet_state_get()->stage==PET_STAGE_ELDER);
    unsigned stars=pet_state_get()->evolution_progress;uint64_t id=pet_state_get()->pet_id;
    pet_state_init();assert(pet_state_get()->pet_id==id);assert(pet_state_get()->evolution_progress==stars);
    pet_state_reset();
    for(unsigned n=1;n<=100;n++) {
        pet_state_play();
        assert(pet_state_get()->stage==(n>=100?PET_STAGE_ELDER:n>=60?PET_STAGE_ADULT:n>=30?PET_STAGE_TEEN:n>=10?PET_STAGE_CHILD:PET_STAGE_BABY));
    }
    pet_state_reset();
    assert(!strcmp(pet_state_get()->name,"Sprout"));
    assert(!pet_state_set_name(""));assert(!pet_state_set_name("123"));assert(!pet_state_set_name("WayTooLongPetNameHere"));
    assert(pet_state_set_name("Clover"));pet_state_init();assert(!strcmp(pet_state_get()->name,"Clover"));
    assert(pet_state_set_name("Sprout"));
    saved.genes[GENE_BODY_COLOR]=0; pet_state_init();
    lv_init();lv_display_t *d=lv_display_create(368,448);
    lv_display_set_color_format(d,LV_COLOR_FORMAT_XRGB8888);
    partial_render=getenv("PET_PARTIAL_RENDER")!=NULL;
    if(partial_render)lv_display_set_buffers(d,partial_pixels,NULL,sizeof partial_pixels,LV_DISPLAY_RENDER_MODE_PARTIAL);
    else lv_display_set_buffers(d,pixels,NULL,sizeof pixels,LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(d,flush);
    lv_indev_t *input=lv_indev_create();lv_indev_set_type(input,LV_INDEV_TYPE_POINTER);lv_indev_set_read_cb(input,read_touch);
    ui_init();test_power_press=true;advance(160);assert(s_view==HOME && test_power_offs==0);shot("home");
    s_voice_until=0;advance(80);lv_obj_update_layout(s_root);
    assert(lv_obj_has_flag(lv_obj_get_parent(s_caption_label),LV_OBJ_FLAG_HIDDEN));
    shot("home-idle");
    test_playing=true;advance(80);lv_obj_update_layout(s_root);
    assert(s_face==PIXEL_TALK);
    assert(lv_obj_get_height(lv_obj_get_parent(s_caption_label))==94);
    shot("home-speaking");
    // Border/background must survive incremental redraws while the pet animates.
    for(unsigned frame=0;frame<24;frame++) {
        test_caption=frame%2?"Hello! I have a little story about a picnic, some toast, a friendly dragon, and an enormous imaginary cloud castle. What an adventure!":"A tiny picnic sounds lovely. Save a little toast for me!";
        advance(80);lv_refr_now(d);
        assert((pixels[66*368+184]&0xffffff)==INK);
        assert((pixels[72*368+184]&0xffffff)==CREAM);
        uint32_t before[320*94];
        for(unsigned row=0;row<94;row++)memcpy(before+row*320,pixels+(64+row)*368+24,320*4);
        lv_obj_invalidate(s_root);lv_refr_now(d);
        for(unsigned row=0;row<94;row++)assert(!memcmp(before+row*320,pixels+(64+row)*368+24,320*4));
    }
    test_caption="Hello! I'm Sprout. Shall we play?";test_playing=false;

    tap(180,245); advance(60);
    assert((int32_t)(s_hop_until-lv_tick_get()) > 0);
    assert(s_face == PIXEL_HAPPY);
    assert(!lv_obj_has_flag(s_pet_heart, LV_OBJ_FLAG_HIDDEN));
    shot("pet-cuddle");
    advance(700);
    assert(lv_obj_has_flag(s_pet_heart, LV_OBJ_FLAG_HIDDEN));
    tap(60,385);assert(s_view==FOOD);shot("food");
    tap(73,350);tap(73,350);advance(FOOD_DURATION_MS+300);assert(s_view==PARTY);assert(pet_state_get()->evolution_progress==1);shot("party");
    tap(180,395);assert(s_view==HOME);tap(140,385);assert(s_view==PLAY_MENU);shot("play-menu");tap(180,154);assert(s_view==GAMES);shot("games");tap(180,158);assert(s_view==CATCH);shot("play");
    for(int i=0;i<5;i++){lv_obj_update_layout(s_target);int x=lv_obj_get_x(s_target)+48,y=lv_obj_get_y(s_target)+48;tap(x,y);}
    assert(s_view==PARTY);assert(pet_state_get()->evolution_progress==2);
    show(BATH);shot("bath");tap(60,173);tap(178,159);tap(298,174);tap(70,260);tap(294,264);
    assert(s_view==PARTY);assert(pet_state_get()->evolution_progress==3);
    show(SLEEP);shot("sleep");
    assert(s_face == PIXEL_SLEEP);

    advance(2500);tap(48,46);assert(s_view==HOME);advance(4000);assert(pet_state_get()->evolution_progress==3);
    show(SLEEP);advance(6100);assert(s_view==PARTY);assert(pet_state_get()->evolution_progress==4);
    show(FOOD);tap(73,350);tap(48,46);advance(FOOD_DURATION_MS+300);
    assert(s_view==HOME);assert(pet_state_get()->evolution_progress==4);
    show(ALBUM);shot("album-locked");
    for(int i=0;i<26;i++)pet_state_play();show(ALBUM);shot("album");show(HOME);shot("grown-pet");
    show(HOME);shot("talk");
    assert(s_mic && lv_obj_get_width(s_mic)==56 && lv_obj_get_height(s_mic)==48);
    tap(90,342);assert(!s_talking && test_starts==0);
    tx=180;ty=342;pressed=true;advance(100);assert(s_talking);assert(s_view==HOME);
    assert(strstr(lv_label_get_text(s_caption_label),"listening"));
    shot("mic-listening");
    lv_obj_t *room_root=s_root;
    pressed=false;advance(100);assert(!s_talking);assert(test_starts==1 && test_ends==1);
    assert(s_view==HOME && s_root==room_root && s_bars[0]);
    test_voice=VOICE_READY;
    test_boot=true;advance(200);assert(s_talking && s_view==HOME);
    test_boot=false;advance(200);assert(!s_talking);assert(test_starts==2 && test_ends==2);
    test_voice=VOICE_READY;
    test_boot=true;advance(200);advance(20100);assert(!s_talking);assert(test_ends==3);
    test_boot=false;advance(200);assert(test_ends==3);test_voice=VOICE_READY;
    // BOOT also leaves an ongoing care activity in place.
    show(BATH);test_boot=true;advance(200);assert(s_view==BATH && s_talking);
    test_boot=false;advance(200);assert(s_view==BATH && !s_talking);test_voice=VOICE_READY;
    show(HOME);test_voice=VOICE_READY;s_muted=false;audio_set_volume(35);
    s_next_remark=lv_tick_get()-1;unsigned remarks=test_remarks;
    advance(80);assert(test_remarks==remarks+1);advance(80);assert(test_remarks==remarks+1);
    s_next_remark=lv_tick_get()-1;test_auto=false;advance(80);assert(test_remarks==remarks+1);
    test_auto=true;s_muted=true;advance(80);assert(test_remarks==remarks+1);
    s_muted=false;audio_set_volume(0);advance(80);assert(test_remarks==remarks+1);
    audio_set_volume(35);show(BATH);advance(80);assert(test_remarks==remarks+1);
    show(HOME);s_next_remark=lv_tick_get()+90000;
    show(CONNECTION);advance(600);shot("connection");show(NAME);shot("name");
    lv_textarea_set_text(s_name_input,"Clover");tap(180,235);assert(s_view==SETTINGS);assert(!strcmp(pet_state_get()->name,"Clover"));
    show(NAME);lv_textarea_set_text(s_name_input,"");tap(180,235);assert(s_view==NAME);
    pet_state_set_name("Sprout");
    show(SETTINGS);shot("settings");
    tap(245,250); assert(audio_get_volume()>65 && audio_get_volume()<85);
    int chosen_volume=audio_get_volume();
    tap(180,180);assert(s_muted);assert(audio_get_volume()==chosen_volume);
    tap(58,250);assert(audio_get_volume()==0);
    tap(310,250);assert(audio_get_volume()==100);
    show(HOME);show(SETTINGS);assert(lv_slider_get_value(s_volume_slider)==100);
    audio_set_volume(35);show(SETTINGS);
    // Voice audition is independent of saving, and old saves retain the familiar voice.
    s_muted=false;test_voice=VOICE_READY;
    assert(pet_voice_id(pet_state_get())==0);
    assert(!pet_state_set_voice(PET_VOICE_COUNT));
    Pet voice_before=*pet_state_get();
    tap(262,318);assert(s_view==VOICES && s_voice_choice==0);shot("voice-tiny-sprout");
    tap(315,155);assert(s_voice_choice==1 && pet_voice_id(pet_state_get())==0);
    tap(104,307);assert(test_previews==1 && test_preview_choice==1);assert(!memcmp(&voice_before,pet_state_get(),sizeof(Pet)));
    tap(262,307);assert(!s_preview_playing);
    s_muted=true;tap(104,307);assert(test_previews==1);s_muted=false;
    audio_set_volume(0);tap(104,307);assert(test_previews==1);audio_set_volume(35);
    test_voice=VOICE_OFFLINE;tap(104,307);assert(!s_preview_playing && strstr(lv_label_get_text(s_hint),"offline"));test_voice=VOICE_READY;
    fail_save=true;tap(180,413);assert(s_view==VOICES && pet_voice_id(pet_state_get())==0);
    assert(strstr(lv_label_get_text(s_hint),"Could not save"));fail_save=false;
    tap(180,413);assert(s_view==SETTINGS && pet_voice_id(pet_state_get())==1);
    pet_state_init();assert(pet_voice_id(pet_state_get())==1);
    show(VOICES);tap(315,155);tap(315,155);assert(s_voice_choice==3);shot("voice-warm-story");
    unsigned cancels=test_cancels;tap(104,307);tap(48,46);
    assert(s_view==SETTINGS && test_cancels>cancels && pet_voice_id(pet_state_get())==1);
    show(VOICES);tap(315,155);tap(315,155);tap(315,155);assert(s_voice_choice==4);shot("voice-quiet-grown-up");
    tap(180,413);assert(pet_voice_id(pet_state_get())==4);
    Pet voice_after=*pet_state_get();voice_after.inventory[9]=voice_before.inventory[9];
    assert(!memcmp(&voice_before,&voice_after,sizeof(Pet)));
    // Corrupt/unmarked values default safely, and all presets persist without touching progress.
    for(unsigned i=0;i<PET_VOICE_COUNT;i++) {assert(pet_state_set_voice(i));pet_state_init();assert(pet_voice_id(pet_state_get())==i);}
    Pet voice_legacy=*pet_state_get();voice_legacy.inventory[9]=255;assert(pet_voice_id(&voice_legacy)==0);
    assert(pet_state_set_voice(0));show(SETTINGS);
    // Repeated navigation catches dangling object pointers/timers.
    for(int i=0;i<100;i++){show((View)(i%14));advance(80);show(HOME);advance(80);}
    // Review every whole character and all five stages with production geometry.
    Pet snapshot = saved;
    for (int i=0; i<PET_CHARACTER_COUNT; i++) {
        saved = snapshot; saved.genes[GENE_PATTERN] = PET_CHARACTER_MARKER+i;
        saved.evolution_progress = 0; pet_state_init(); show(HOME);
        if (lv_tick_get()%4200 > 3800) advance(500);
        char name[32]; snprintf(name,sizeof name,"pet-character-%d",i); shot(name);
    }
    const unsigned milestones[] = {0,10,30,60,100};
    for (int i=0; i<5; i++) {
        saved = snapshot; saved.evolution_progress = milestones[i];
        pet_state_init(); show(HOME);
        if (lv_tick_get()%4200 > 3800) advance(500);
        char name[32]; snprintf(name,sizeof name,"pet-stage-%d",i); shot(name);
    }
    // Exercise the actual food touch targets: selection survives extra taps,
    // each held snack is different, biting changes the frame, reward is once.
    static uint32_t held[3][PIXEL_PET_SIZE*PIXEL_PET_SIZE];
    for(int food=0;food<3;food++) {
        show(FOOD);unsigned before=pet_state_get()->evolution_progress;
        tap(73+food*108,350);assert(s_eating && s_food==(pixel_food_t)food);
        advance(80);assert(s_face==PIXEL_EAT); // wait for a production animation tick
        memcpy(held[food],s_pet_art.pixels,sizeof held[food]);
        char name[64];snprintf(name,sizeof name,"eat-%s-hold",(const char*[]){"apple","toast","cookie"}[food]);shot(name);
        tap(73+((food+1)%3)*108,350);assert(s_food==(pixel_food_t)food);
        advance(360);snprintf(name,sizeof name,"eat-%s-bite",(const char*[]){"apple","toast","cookie"}[food]);shot(name);
        assert(memcmp(held[food],s_pet_art.pixels,sizeof held[food]));
        advance(FOOD_DURATION_MS+100);assert(s_view==PARTY && pet_state_get()->evolution_progress==before+1);
        advance(1500);assert(pet_state_get()->evolution_progress==before+1);
    }
    assert(memcmp(held[0],held[1],sizeof held[0]));assert(memcmp(held[1],held[2],sizeof held[0]));
    show(FOOD);unsigned meal_stars=pet_state_get()->evolution_progress;
    start_food(0,NULL);advance(1400);assert(s_view==FOOD && s_eating);
    assert(pet_state_get()->evolution_progress==meal_stars);
    advance(800);assert(s_view==FOOD && lv_obj_get_y(s_pet)==s_pet_y);shot("meal-settle");
    advance(420);assert(s_view==FOOD && lv_obj_get_style_opa(s_root,0)<255);shot("meal-fade");
    advance(300);assert(s_view==PARTY && pet_state_get()->evolution_progress==meal_stars+1);
    advance(300);assert(lv_obj_get_style_opa(s_root,0)==255);shot("meal-celebration");
    show(FOOD);meal_stars=pet_state_get()->evolution_progress;start_food(0,NULL);
    advance(2400);tap(48,46);assert(s_view==HOME);
    advance(FOOD_DURATION_MS);assert(pet_state_get()->evolution_progress==meal_stars);
    assert(lv_obj_get_style_opa(s_root,0)==255);
    if(getenv("PET_CAPTURE_MEAL")) {
        show(FOOD);start_food(1,NULL);
        for(unsigned i=0;i<35;i++) {char name[48];snprintf(name,sizeof name,"meal-motion-%02u",i);shot(name);advance(40);}
    }
    // Immediate feedback works offline/muted; spoken reactions are bounded and
    // never queue behind manual speech. Snapshots still learn the latest action.
    show(HOME);test_voice=VOICE_READY;test_auto=true;s_muted=false;audio_set_volume(100);
    s_next_reaction=0;unsigned reactions=test_reactions;
    tap(180,245);assert(test_event==PET_EVENT_CUDDLE && test_reactions==reactions+1);
    tap(180,245);assert(test_reactions==reactions+1);
    assert(strstr(s_reaction_text,"cuddles") || strstr(s_reaction_text,"leaves") || strstr(s_reaction_text,"tickles"));
    show(FOOD);tap(180,350);advance(FOOD_DURATION_MS+100);
    assert(test_event==PET_EVENT_TOAST && strstr(s_reaction_text,"toast"));
    assert(test_reactions==reactions+1); // same 45-second speech cooldown
    show(HOME);s_next_reaction=0;s_muted=true;tap(180,245);assert(test_reactions==reactions+1);
    s_muted=false;audio_set_volume(0);tap(180,245);assert(test_reactions==reactions+1);
    audio_set_volume(100);test_auto=false;tap(180,245);assert(test_reactions==reactions+1);
    test_auto=true;test_voice=VOICE_OFFLINE;tap(180,245);assert(test_reactions==reactions+1);
    test_voice=VOICE_LISTENING;tap(180,245);assert(test_reactions==reactions+1);
    test_voice=VOICE_READY;s_next_reaction=0;tap(180,245);assert(test_reactions==reactions+2);
    assert(!lv_obj_has_flag(lv_obj_get_parent(s_caption_label),LV_OBJ_FLAG_HIDDEN));
    shot("reaction-cuddle");
    show(SLEEP);unsigned nap_before=pet_state_get()->evolution_progress;
    advance(600);tap(48,46);assert(test_event==PET_EVENT_CUDDLE);
    assert(pet_state_get()->evolution_progress==nap_before); // no fake wake-up reaction
    show(SLEEP);advance(6100);assert(test_event==PET_EVENT_NAP);shot("reaction-nap");
    show(FOOD);tap(180,350);advance(FOOD_DURATION_MS+100);shot("reaction-toast");
    // Peekaboo: wrong guesses and rapid taps are free, three actual finds earn
    // exactly one care star. Navigation cancels a pending reveal without reward.
    show(GAMES);tap(180,251);assert(s_view==HIDE);shot("peekaboo");
    unsigned play_before=pet_state_get()->evolution_progress;
    unsigned wrong=(s_hiding+1)%3;tap(76+108*(int)wrong,275);
    assert(s_hits==0 && pet_state_get()->evolution_progress==play_before);
    shot("peekaboo-hint");
    for(unsigned i=0;i<3;i++) {
        unsigned hidden=s_hiding;tap(76+108*(int)hidden,275);
        assert(s_hits==i+1 && s_reveal_until);tap(76+108*(int)hidden,275);
        assert(s_hits==i+1);if(i==0)shot("peekaboo-found");
        advance(1200);
        if(i<2)assert(s_view==HIDE && s_hiding!=hidden);
    }
    assert(s_view==PARTY && test_event==PET_EVENT_HIDE);
    assert(pet_state_get()->evolution_progress==play_before+1);
    advance(3000);assert(pet_state_get()->evolution_progress==play_before+1);
    show(HIDE);tap(76+108*(int)s_hiding,275);tap(48,46);advance(1500);
    assert(s_view==GAMES && pet_state_get()->evolution_progress==play_before+1);

    // Each hit launches a new arc. Landing waits for the child, while flight
    // cannot score again and the fifth bounce finishes before its care reward.
    show(GAMES);tap(180,343);assert(s_view==BALL);shot("bouncy-ball-ready");
    play_before=pet_state_get()->evolution_progress;
    advance(1000);int ball_x=lv_obj_get_x(s_target),ball_y=lv_obj_get_y(s_target);
    advance(1500);assert(lv_obj_get_x(s_target)==ball_x && lv_obj_get_y(s_target)==ball_y);
    tx=ball_x+48;ty=ball_y+48;pressed=true;advance(80);
    advance(1000);assert(s_ball_pressed && s_hits==0);
    assert(lv_obj_get_x(s_target)==ball_x && lv_obj_get_y(s_target)==ball_y);shot("bouncy-ball-squash");
    pressed=false;advance(60);assert(s_hits==1 && s_ball_flying);
    tap(ball_x+48,ball_y+48);assert(s_hits==1);advance(140);shot("bouncy-ball-flight");
    assert(lv_obj_get_y(s_target)<ball_y-50 && s_face==PIXEL_PLAY);
    for(unsigned hit=1;hit<5;hit++) {
        advance(BALL_FLIGHT_MS+BALL_LAND_MS+80);
        assert(s_view==BALL && !s_ball_flying && lv_obj_has_flag(s_target,LV_OBJ_FLAG_CLICKABLE));
        int next_x=lv_obj_get_x(s_target),next_y=lv_obj_get_y(s_target);
        assert(abs(next_x-ball_x)>=100);ball_x=next_x;ball_y=next_y;
        advance(600);assert(lv_obj_get_x(s_target)==ball_x && lv_obj_get_y(s_target)==ball_y);
        if(hit==1)shot("bouncy-ball-landed");
        tap(ball_x+48,ball_y+48);assert(s_hits==hit+1 && s_ball_flying);
        assert(pet_state_get()->evolution_progress==play_before);
        // Extra taps in flight cannot skip a bounce.
        tap(lv_obj_get_x(s_target)+48,lv_obj_get_y(s_target)+48);assert(s_hits==hit+1);
    }
    advance(BALL_FLIGHT_MS);assert(s_view==BALL && s_ball_finish_at);shot("bouncy-ball-five");
    assert(pet_state_get()->evolution_progress==play_before);
    advance(BALL_WIN_MS+80);assert(s_view==PARTY && test_event==PET_EVENT_BALL);
    assert(pet_state_get()->evolution_progress==play_before+1);shot("bouncy-ball-celebration");
    advance(2500);assert(pet_state_get()->evolution_progress==play_before+1);
    show(BALL);tap(lv_obj_get_x(s_target)+48,lv_obj_get_y(s_target)+48);tap(4,4);advance(3000);
    assert(s_view==GAMES && pet_state_get()->evolution_progress==play_before+1);
    // Both mirrored routes stay below navigation and above the status footer.
    for(unsigned mirror=0;mirror<2;mirror++) {
        show(BALL);s_ball_mirror=mirror;
        for(unsigned hit=0;hit<5;hit++) {
            tap(lv_obj_get_x(s_target)+48,lv_obj_get_y(s_target)+48);
            for(unsigned tick=0;tick<22;tick++) {
                advance(40);assert(s_view==BALL);
                assert(lv_obj_get_x(s_target)>=26 && lv_obj_get_x(s_target)<=246);
                assert(lv_obj_get_y(s_target)>=135 && lv_obj_get_y(s_target)<=276);
            }
        }
        // Back out during the final celebration pause: no delayed reward.
        assert(s_ball_finish_at);tap(4,4);advance(2000);
        assert(s_view==GAMES && pet_state_get()->evolution_progress==play_before+1);
    }
    if(getenv("PET_CAPTURE_BALL")) {
        show(BALL);s_ball_mirror=false;
        for(unsigned frame_no=0;frame_no<78;frame_no++) {
            if(s_view==BALL && !s_ball_flying && !s_ball_finish_at && (int32_t)(lv_tick_get()-s_ball_ready)>=0)
                tap(lv_obj_get_x(s_target)+48,lv_obj_get_y(s_target)+48);
            char name[48];snprintf(name,sizeof name,"ball-motion-%02u",frame_no);shot(name);advance(40);
        }
    }

    // Old single-gift saves load unchanged. New placements are additive and
    // transactional; keep the legacy slot readable by rollback firmware.
    Pet legacy=saved;legacy.evolution_progress=29;legacy.inventory[13]=0;legacy.inventory[14]=0;legacy.inventory[15]=101;
    saved=legacy;pet_state_init();assert(pet_sticker_count(pet_state_get())==5);
    assert(pet_room_gifts(pet_state_get())==2);
    assert(pet_decoration_unlocked(pet_state_get(),1));assert(!pet_decoration_unlocked(pet_state_get(),2));
    assert(!pet_toggle_decoration(2));assert(!pet_toggle_decoration(999));assert(!pet_equip_decoration(-2));
    fail_save=true;assert(!pet_toggle_decoration(0));assert(pet_room_gifts(pet_state_get())==2);fail_save=false;
    show(HOME);tap(50,275);assert(s_view==REWARDS);shot("treasures-menu");
    tap(180,325);assert(s_view==DECORATIONS);shot("room-gifts-locked");
    tap(290,170);assert(s_view==DECORATIONS && strstr(lv_label_get_text(s_hint),"1 more"));
    tap(80,170);assert(s_view==DECORATIONS && pet_room_gifts(pet_state_get())==3);
    pet_state_init();assert(pet_room_gifts(pet_state_get())==3);
    assert(pet_state_get()->inventory[15]==100 && pet_state_get()->pet_id==legacy.pet_id && pet_state_get()->evolution_progress==29);
    fail_save=true;tap(80,170);assert(pet_room_gifts(pet_state_get())==3);
    assert(strstr(lv_label_get_text(s_hint),"Couldn't save"));fail_save=false;
    tap(267,372);assert(s_view==HOME);shot("room-two-gifts");
    show(DECORATIONS);tap(80,170);assert(pet_room_gifts(pet_state_get())==2);
    tap(90,372);assert(s_view==DECORATIONS && pet_room_gifts(pet_state_get())==0);
    pet_state_play();show(PARTY);shot("new-room-gift");tap(100,400);assert(s_view==DECORATIONS);
    assert(pet_sticker_count(pet_state_get())==6 && pet_decoration_unlocked(pet_state_get(),2));
    tap(48,46);assert(s_view==REWARDS);tap(180,190);assert(s_view==ALBUM);
    s_album_page=0;show(ALBUM);tap(310,365);assert(s_album_page==1);shot("stickers-next");
    tap(70,175);assert(strstr(lv_label_get_text(s_hint),"5 more") && s_view==ALBUM && pet_wall_sticker(pet_state_get())==-1);
    tap(60,365);assert(s_album_page==0);tap(60,365);assert(s_album_page==3);tap(310,365);assert(s_album_page==0);
    fail_save=true;tap(70,175);assert(s_view==ALBUM && pet_wall_sticker(pet_state_get())==-1);
    assert(strstr(lv_label_get_text(s_hint),"Couldn't save"));fail_save=false;
    tap(70,175);assert(s_view==HOME && pet_wall_sticker(pet_state_get())==0 && s_keepsake_until);
    advance(80);assert(s_face==PIXEL_EAT);shot("apple-sticker-play");
    pet_state_init();assert(pet_wall_sticker(pet_state_get())==0);
    uint32_t sticker_stars=pet_state_get()->evolution_progress;
    advance(4100);assert(!s_keepsake_until);tap(53,107);assert(s_keepsake_until);
    assert(pet_state_get()->evolution_progress==sticker_stars);
    tap(50,275);assert(s_view==REWARDS && !s_keepsake_until);advance(4200);
    tap(180,190);assert(s_view==ALBUM);tap(180,418);assert(s_view==HOME && pet_wall_sticker(pet_state_get())==-1);
    assert(!pet_set_wall_sticker(-2) && !pet_set_wall_sticker(18) && !pet_set_wall_sticker(6));
    saved.evolution_progress=90;pet_state_init();assert(pet_sticker_count(pet_state_get())==18);
    for(unsigned i=0;i<6;i++){assert(pet_decoration_unlocked(pet_state_get(),i));assert(pet_toggle_decoration(i));}
    assert(pet_room_gifts(pet_state_get())==63);pet_state_init();assert(pet_room_gifts(pet_state_get())==63);
    for(unsigned i=0;i<3;i++){s_album_page=i;show(ALBUM);char name[40];snprintf(name,sizeof name,"stickers-page-%u",i+1);shot(name);}
    show(DECORATIONS);shot("room-all-gifts-selected");tap(267,372);assert(s_view==HOME);s_voice_until=0;shot("room-all-gifts");
    // All eighteen stickers activate via real touch. Replays are bounded, do
    // not consume rewards, and leaving/starting voice cancels the animation.
    Pet before_keepsakes=*pet_state_get();
    for(unsigned i=0;i<PET_CARE_STICKER_COUNT;i++) {
        s_album_page=i/6;show(ALBUM);tap(76+(i%3)*108,180+((i%6)/3)*100);
        assert(s_view==HOME && pet_wall_sticker(pet_state_get())==(int)i && s_keepsake_until);
        advance(80);assert(s_face==keepsake_actions[i].pose && test_sfx==keepsake_actions[i].sound);
        char name[40];snprintf(name,sizeof name,"sticker-play-%02u",i);shot(name);
        if(getenv("PET_CAPTURE_ANIMATIONS") && i==6)for(unsigned frame_no=0;frame_no<24;frame_no++) {
            advance(100);snprintf(name,sizeof name,"flutter-%02u",frame_no);shot(name);
        }
        tap(53,107);assert(s_keepsake_until);show(REWARDS);assert(!s_keepsake_until);advance(100);
    }
    assert(pet_state_get()->evolution_progress==before_keepsakes.evolution_progress);
    assert(pet_state_get()->hunger==before_keepsakes.hunger && pet_state_get()->happiness==before_keepsakes.happiness);
    assert(pet_state_get()->energy==before_keepsakes.energy && pet_state_get()->hygiene==before_keepsakes.hygiene);
    assert(pet_state_get()->pet_id==before_keepsakes.pet_id && pet_room_gifts(pet_state_get())==63);
    // Each placed gift is reachable without touching the pet or a care menu.
    const int gift_taps[6][2]={{27,232},{184,95},{102,289},{299,161},{264,293},{344,222}};
    for(unsigned i=0;i<6;i++) {
        show(HOME);tap(gift_taps[i][0],gift_taps[i][1]);assert(s_view==HOME && s_keepsake_until);
    }
    test_voice=VOICE_LISTENING;advance(80);assert(!s_keepsake_until && s_face==PIXEL_LISTEN);test_voice=VOICE_READY;
    show(HOME);s_voice_until=0;shot("room-sticker-and-gifts");
    saved.evolution_progress=UINT32_MAX;pet_state_init();assert(pet_sticker_count(pet_state_get())==18);
    assert(!pet_decoration_unlocked(pet_state_get(),6));
    saved.inventory[14]=255;saved.inventory[15]=255;saved.inventory[13]=255;pet_state_init();
    assert(pet_room_gifts(pet_state_get())==0 && pet_wall_sticker(pet_state_get())==-1);
    saved.inventory[14]=191;saved.evolution_progress=9;saved.inventory[13]=217;pet_state_init();
    assert(pet_room_gifts(pet_state_get())==0 && pet_wall_sticker(pet_state_get())==-1);
    // Exhaustive byte decoding agrees with the backend's bounded mask format.
    Pet encoded=legacy;encoded.evolution_progress=90;encoded.inventory[15]=0;
    for(unsigned value=0;value<256;value++) {
        encoded.inventory[14]=value;encoded.inventory[13]=value;
        assert(pet_room_gifts(&encoded)==(value>=128 && value<=191?(value&63):0));
        assert(pet_wall_sticker(&encoded)==(value>=200 && value<218?(int)value-200:-1));
    }
    saved=legacy;saved.evolution_progress=60;pet_state_init();assert(pet_equip_decoration(1));

    // Visits never grant stars or interrupt speech, and navigation owns all
    // pointers. Weather and equipped gifts can coexist with captions and play.
    show(HOME);s_voice_until=0;s_next_butterfly=lv_tick_get();test_voice=VOICE_READY;
    s_next_remark=lv_tick_get()+90000;advance(80);assert(s_butterfly_until);shot("butterfly-visit");
    play_before=pet_state_get()->evolution_progress;
    tap(lv_obj_get_x(s_butterfly)+30,lv_obj_get_y(s_butterfly)+30);
    assert(!s_butterfly_until && test_event==PET_EVENT_BUTTERFLY && pet_state_get()->evolution_progress==play_before);
    s_next_butterfly=lv_tick_get();test_voice=VOICE_LISTENING;advance(80);assert(!s_butterfly_until);
    test_voice=VOICE_READY;advance(80);assert(s_butterfly_until);show(GAMES);advance(11000);show(HOME);advance(80);
    assert(!s_butterfly_until); // leaving the room dismisses a visit
    for(unsigned mode=0;mode<3;mode++) {
        uint32_t target=((lv_tick_get()/540000)+1)*540000+mode*180000;
        lv_tick_inc(target-lv_tick_get());s_next_remark=target+90000;s_next_butterfly=target+60000;s_voice_until=0;
        advance(80);assert(s_weather_mode==mode);
        assert(lv_obj_has_flag(s_rain[0],LV_OBJ_FLAG_HIDDEN)==(mode!=1));
        char name[40];snprintf(name,sizeof name,"window-weather-%u",mode);shot(name);
    }
    if(getenv("PET_CAPTURE_ANIMATIONS")) {
        for(int scene=0;scene<3;scene++) {
            show((View[]){BALL,HIDE,HOME}[scene]);s_voice_until=0;
            if(scene==2)s_next_butterfly=lv_tick_get();
            for(int i=0;i<32;i++) {
                if(scene==1 && i==12)tap(76+108*(int)s_hiding,275);
                char name[80];snprintf(name,sizeof name,"frames/%s-%02d",(const char*[]){"ball","peekaboo","butterfly"}[scene],i);
                shot(name);advance(100);
            }
        }
    }

    // Music is available from Play; offline choices and compose never earn care
    // stars, Stop works, leaving stops local playback, muted compose is blocked.
    test_voice=VOICE_READY;s_muted=false;audio_set_volume(100);show(PLAY_MENU);tap(180,260);assert(s_view==MUSIC);shot("music");
    unsigned music_before=pet_state_get()->evolution_progress;
    assert(audio_tune_count()==9);
    tx=180;ty=295;pressed=true;advance(60);
    for(int y=295;y>=155;y-=20){ty=y;advance(40);}
    pressed=false;advance(400);
    assert(lv_obj_get_scroll_y(s_music_list)>60 && !test_tune);
    lv_obj_scroll_to_y(s_music_list,0,LV_ANIM_OFF);advance(30);
    for(unsigned i=0;i<audio_tune_count();i++) {
        lv_obj_scroll_to_y(s_music_list,(int)i*56,LV_ANIM_OFF);advance(30);
        int y=lv_obj_get_y(lv_obj_get_child(s_music_list,i))+120-lv_obj_get_scroll_y(s_music_list)+25;
        tap(180,y);assert(test_tune && test_tune_choice==i);
    }
    shot("music-last-songs");
    lv_obj_scroll_to_y(s_music_list,0,LV_ANIM_OFF);advance(30);
    test_boot=true;advance(200);assert(!test_tune && strstr(lv_label_get_text(s_hint),"listening"));
    test_boot=false;advance(200);test_voice=VOICE_READY;
    tap(180,418);assert(!test_tune);unsigned composed=test_compositions;
    tap(180,363);assert(test_compositions==composed+1);
    test_voice=VOICE_THINKING;tap(180,150);assert(!test_tune && strstr(lv_label_get_text(s_hint),"Stop"));test_voice=VOICE_READY;
    s_muted=true;tap(180,363);assert(test_compositions==composed+1);s_muted=false;
    tap(180,150);assert(test_tune);tap(48,46);assert(s_view==PLAY_MENU && !test_tune);
    assert(pet_state_get()->evolution_progress==music_before);
    tap(180,365);assert(s_view==MULTIPLAYER);shot("multiplayer-connecting");
    tap(48,46);assert(s_view==PLAY_MENU);tap(180,154);assert(s_view==GAMES);
    tap(4,4);assert(s_view==PLAY_MENU);tap(4,4);assert(s_view==HOME);
    tap(140,30);assert(s_view==PLAY_MENU);tap(4,4);assert(s_view==HOME);
    // Real UI with deterministic game snapshots: invitations require a tap,
    // rapid taps cannot double-pass, and two whole-character buffers animate.
    {
        Pet before=*pet_state_get();unsigned stars=before.evolution_progress;
        unsigned opens=test_mp_open,closes=test_mp_close;
        test_mp=(mp_state_t){.connected=true,.phase=MP_LOBBY,.count=1};
        test_mp.peers[0]=(mp_peer_t){.user="friend-device",.id="0000000000000042",.name="Rosy",.character=5,.stage=2};
        show(MULTIPLAYER);advance(120);assert(test_mp_open==opens+1);shot("multiplayer-lobby");
        tap(180,283);assert(test_mp_invites==1);
        test_mp.peer=test_mp.peers[0];test_mp.phase=MP_INCOMING;strcpy(test_mp.invite,"invitation");advance(120);
        assert(test_mp_accepts==0);shot("multiplayer-invitation");tap(180,338);assert(test_mp_accepts==1);
        test_mp.phase=MP_PLAYING;test_mp.online=true;test_mp.my_turn=true;strcpy(test_mp.room,"round-one");advance(1000);
        shot("multiplayer-my-turn");int talks=test_starts;test_boot=true;advance(150);test_boot=false;advance(150);assert(test_starts==talks);
        tap(80,312);tap(80,312);assert(test_mp_passes==1);
        test_mp.seq=1;test_mp.my_turn=false;advance(300);shot("multiplayer-pass");
        assert(lv_obj_get_x(s_mp_ball)>32 && lv_obj_get_x(s_mp_ball)<240);
        advance(600);tap(290,312);assert(test_mp_passes==1);
        test_mp.phase=MP_WAITING;advance(120);shot("multiplayer-reconnecting");assert(strstr(lv_label_get_text(s_hint),"reconnecting"));
        test_mp.phase=MP_PLAYING;test_mp.seq=2;test_mp.my_turn=true;advance(1000);tap(80,312);assert(test_mp_passes==2);
        // Each pair uses independent mutable render storage, including same-character play.
        for(unsigned a=0;a<PET_CHARACTER_COUNT;a++)for(unsigned b=0;b<PET_CHARACTER_COUNT;b++) {
            saved.genes[GENE_PATTERN]=(uint8_t)(PET_CHARACTER_MARKER+a);pet_state_init();test_mp.peer.character=b;
            show(MULTIPLAYER);advance(200);assert(s_mp_art[0].image.data!=s_mp_art[1].image.data);
            if(a!=b)assert(memcmp(s_mp_art[0].pixels,s_mp_art[1].pixels,sizeof s_mp_art[0].pixels));
        }
        saved=before;pet_state_init();
        test_mp.peer.character=5;test_mp.seq=10;test_mp.phase=MP_FINISHED;test_mp.my_turn=false;
        test_mp.reward_id=42;test_mp.reward_pet=before.pet_id;test_mp.reward_friend=0x42;test_mp.friends=1;
        fail_save=true;advance(1000);assert(pet_state_get()->evolution_progress==stars && test_mp_acks==0);
        fail_save=false;advance(700);assert(pet_state_get()->evolution_progress==stars+1 && test_mp_acks>0);
        assert(pet_state_get()->friends_met==1 && pet_sticker_unlocked(pet_state_get(),18));
        pet_state_init();advance(700);assert(pet_state_get()->evolution_progress==stars+1);shot("multiplayer-celebration");
        assert(pet_state_playdate(42,before.pet_id,0x42,1));assert(pet_state_playdate(41,before.pet_id,0x42,1));
        assert(pet_state_get()->evolution_progress==stars+1);
        assert(!pet_state_playdate(43,before.pet_id+1,0x42,1));
        tap(180,415);assert(test_mp_agains==1);
        tap(4,4);assert(s_view==PLAY_MENU && test_mp_close==closes+1);
        test_mp=(mp_state_t){0};s_album_page=3;show(ALBUM);shot("buddy-sticker");tap(76,180);
        assert(s_view==HOME && s_keepsake_until && pet_wall_sticker(pet_state_get())==18);shot("buddy-sticker-play");
        saved=before;pet_state_init();show(HOME);
        unsigned chat_stars=pet_state_get()->evolution_progress;
        test_mp=(mp_state_t){.connected=true,.phase=MP_LOBBY,.count=1};
        test_mp.peers[0]=(mp_peer_t){.user="friend-device",.id="0000000000000042",.name="Larry",.character=6,.stage=3,.chat=true};
        show(MULTIPLAYER);tap(270,212);assert(s_mp_chat_mode);shot("pet-chat-lobby");
        tap(180,300);assert(test_mp_chat_invites==1);
        test_mp.peer=test_mp.peers[0];test_mp.chat=true;test_mp.phase=MP_INCOMING;strcpy(test_mp.invite,"chat-invite");advance(140);
        shot("pet-chat-invitation");unsigned accepts=test_mp_accepts;tap(180,338);assert(test_mp_accepts==accepts+1);
        test_mp.phase=MP_PLAYING;strcpy(test_mp.room,"chat-room");advance(140);assert(s_mp_caption && !s_mp_ball);shot("pet-chat-thinking");
        test_mp.seq=1;test_mp.speaking=true;test_mp.my_turn=true;strcpy(test_mp.text,"My human and I could imagine a lovely picnic.");advance(160);
        assert(strstr(lv_label_get_text(s_mp_caption),"picnic"));shot("pet-chat-speaking");
        test_mp.seq=2;test_mp.my_turn=false;strcpy(test_mp.text,"A picnic sounds better than another meeting. Save me toast!");advance(160);shot("pet-chat-listening");
        assert(strstr(lv_label_get_text(s_hint),"Larry"));
        test_mp.seq=8;test_mp.speaking=false;test_mp.phase=MP_FINISHED;advance(160);
        assert(!lv_obj_has_flag(s_mp_again,LV_OBJ_FLAG_HIDDEN));shot("pet-chat-finished");
        unsigned agains=test_mp_agains;tap(260,416);assert(test_mp_agains==agains+1);
        test_mp.again=true;advance(160);assert(strstr(lv_label_get_text(lv_obj_get_child(s_mp_again,0)),"Waiting"));
        assert(pet_state_get()->evolution_progress==chat_stars);
        unsigned close_chat=test_mp_close;tap(80,416);assert(s_view==PLAY_MENU && test_mp_close==close_chat+1);
        test_mp=(mp_state_t){0};s_mp_chat_mode=false;show(HOME);
    }
    show(FOOD);tap(180,350);assert(test_sfx==SFX_TOAST);tap(48,46);
    show(BALL);tap(lv_obj_get_x(s_target)+48,lv_obj_get_y(s_target)+48);assert(test_sfx==SFX_BOUNCE);
    show(HIDE);tap(76+108*(int)s_hiding,275);assert(test_sfx==SFX_FOUND);show(HOME);
    // Record production animation frames for visual review, including all foods.
    if(getenv("PET_CAPTURE_ANIMATIONS")) {
        saved=snapshot;saved.evolution_progress=10;pet_state_init();
        for(int scene=0;scene<9;scene++) {
            View v=(View[]){HOME,FOOD,FOOD,FOOD,BATH,SLEEP,CATCH,PARTY,HOME}[scene];
            show(v);test_voice=VOICE_READY;test_playing=scene==8;s_voice_until=0;
            if(v==FOOD)tap(73+(scene-1)*108,350);
            for(int i=0;i<24;i++) {
                char name[80];snprintf(name,sizeof name,"frames/%s-%02d",(const char*[]){"idle","apple","toast","cookie","bath","sleep","play","party","talk"}[scene],i);
                shot(name);advance(100);
            }
        }
        test_playing=false;
    }
    // Milestone recipes derive from old saves, are never consumed, and are
    // enforced by the model as well as the UI. A meal commits exactly once.
    Pet food_base=saved;
    assert(!pet_special_food(PET_SPECIAL_FOOD_COUNT));
    assert(!pet_food_unlocked(NULL,0));assert(!pet_state_eat(PET_FOOD_COUNT));
    static const unsigned food_milestones[]={10,25,50,100};
    static const pet_event_t food_events[]={PET_EVENT_CUPCAKE,PET_EVENT_PANCAKES,PET_EVENT_JELLY,PET_EVENT_CAKE};
    uint32_t special_frames[4][PIXEL_PET_SIZE*PIXEL_PET_SIZE];
    for(unsigned i=0;i<PET_SPECIAL_FOOD_COUNT;i++) {
        const pet_special_food_t *f=pet_special_food(i);assert(f->stars==food_milestones[i]);
        saved=food_base;saved.evolution_progress=f->stars-1;saved.hunger=40;saved.happiness=40;
        pet_state_init();Pet before=*pet_state_get(),disk=saved;
        assert(pet_food_milestone(f->stars)==(int)i);
        assert(!pet_food_unlocked(pet_state_get(),i+3));assert(!pet_state_eat(i+3));
        assert(!memcmp(&before,pet_state_get(),sizeof before));assert(!memcmp(&disk,&saved,sizeof disk));
        show(FOOD);tap(180,424);assert(s_view==TREATS);tap(180,150+i*80);
        assert(s_view==TREATS && !s_eating);assert(strstr(lv_label_get_text(s_hint),"more star"));
        if(i==0)shot("treats-locked");
        tap(48,46);assert(s_view==FOOD);tap(73,350);advance(FOOD_DURATION_MS+100);
        assert(s_view==PARTY && pet_state_get()->evolution_progress==f->stars);
        char name[64];snprintf(name,sizeof name,"treat-unlock-%u",i);shot(name);
        tap(100,400);assert(s_view==TREATS);assert(pet_food_unlocked(pet_state_get(),i+3));
        snprintf(name,sizeof name,"treats-progress-%u",i);shot(name);
        // Leaving before the bite completes must not feed or produce an event.
        unsigned before_stars=pet_state_get()->evolution_progress;pet_event_t before_event=test_event;
        tap(180,150+i*80);assert(s_view==FOOD && s_eating && s_food==(pixel_food_t)(i+3));
        tap(48,46);advance(FOOD_DURATION_MS+100);assert(s_view==HOME && pet_state_get()->evolution_progress==before_stars && test_event==before_event);
        show(TREATS);tap(180,150+i*80);assert(s_eating);
        assert(test_sfx==(sfx_id_t)(SFX_CUPCAKE+i));advance(80);
        memcpy(special_frames[i],s_pet_art.pixels,sizeof special_frames[i]);
        snprintf(name,sizeof name,"treat-%u-hold",i);shot(name);
        tap(180,350);assert(s_food==(pixel_food_t)(i+3)); // rapid taps cannot switch snacks
        advance(360);snprintf(name,sizeof name,"treat-%u-bite",i);shot(name);
        assert(memcmp(special_frames[i],s_pet_art.pixels,sizeof special_frames[i]));
        advance(FOOD_DURATION_MS+100);assert(s_view==PARTY && test_event==food_events[i]);
        assert(pet_state_get()->evolution_progress==before_stars+1);
        assert(pet_state_get()->hunger==100 && pet_state_get()->happiness==50);
        advance(1400);assert(pet_state_get()->evolution_progress==before_stars+1);
        uint64_t identity=pet_state_get()->pet_id;pet_state_init();
        assert(pet_food_unlocked(pet_state_get(),i+3) && pet_state_get()->pet_id==identity);
        for(unsigned j=0;j<i;j++)assert(memcmp(special_frames[i],special_frames[j],sizeof special_frames[i]));
    }
    // Failed persistence cannot award stats/stars or claim a completed meal.
    saved=food_base;saved.evolution_progress=100;saved.hunger=40;saved.happiness=95;pet_state_init();
    Pet before_meal=*pet_state_get(),before_meal_disk=saved;pet_event_t before_event=test_event;
    show(TREATS);tap(180,390);fail_save=true;advance(FOOD_DURATION_MS+300);
    assert(s_view==FOOD && !s_eating && strstr(lv_label_get_text(s_hint),"Could not save"));
    assert(lv_obj_get_style_opa(s_root,0)==255);
    assert(!memcmp(pet_state_get(),&before_meal,sizeof before_meal));assert(!memcmp(&saved,&before_meal_disk,sizeof saved));
    assert(test_event==before_event);fail_save=false;
    assert(pet_state_eat(6));assert(pet_state_get()->hunger==80 && pet_state_get()->happiness==100);
    saved.evolution_progress=UINT32_MAX;pet_state_init();assert(pet_state_eat(6));
    assert(pet_state_get()->evolution_progress==UINT32_MAX && pet_state_get()->hunger==100);
    // Character selection is explicit and transactional; arrows only preview.
    saved=food_base;pet_state_init();
    Pet before_character=*pet_state_get(), before_disk=saved;
    show(HOME);tap(320,275);assert(s_view==SETTINGS);tap(262,369);assert(s_view==PROFILE);shot("character-profile");
    tap(180,277);assert(s_view==TRAIT && s_trait_gene==0);unsigned start=s_trait_variant;
    tap(50,418);assert(s_trait_variant==(start+PET_CHARACTER_COUNT-1)%PET_CHARACTER_COUNT);tap(310,418);assert(s_trait_variant==start);
    for(unsigned c=0;c<PET_CHARACTER_COUNT;c++) {
        Pet preview=*pet_state_get();pixel_pet_art_t expected;preview.genes[GENE_PATTERN]=PET_CHARACTER_MARKER+s_trait_variant;
        pixel_pet_render(&expected,&preview,PIXEL_IDLE,12,0);assert(!memcmp(expected.pixels,s_pet_art.pixels,sizeof expected.pixels));
        char name[64];snprintf(name,sizeof name,"choose-character-%u",s_trait_variant);shot(name);tap(310,418);
    }
    assert(s_trait_variant==start && !memcmp(&before_disk,&saved,sizeof saved));
    assert(!memcmp(&before_character,pet_state_get(),sizeof before_character));
    while(s_trait_variant!=PET_CHARACTER_COUNT-1)tap(310,418);unsigned choice=s_trait_variant;
    fail_save=true;tap(180,368);assert(s_view==TRAIT && strstr(lv_label_get_text(s_hint),"Couldn't save"));
    assert(!memcmp(&before_disk,&saved,sizeof saved) && !memcmp(&before_character,pet_state_get(),sizeof before_character));
    fail_save=false;tap(180,368);assert(s_view==PROFILE && pet_character_id(pet_state_get())==choice);
    before_character.genes[GENE_PATTERN]=PET_CHARACTER_MARKER+choice;
    assert(!memcmp(&before_character,pet_state_get(),sizeof before_character));
    assert(!memcmp(&before_character,&saved,sizeof before_character));
    pet_state_init();assert(pet_character_id(pet_state_get())==choice);
    assert(!pet_state_set_character(PET_CHARACTER_COUNT) && !pet_state_set_character(255));
    // Personality browsing remains informative and cannot change the pet.
    show(PROFILE);tap(180,357);assert(s_view==TRAIT && s_trait_gene==GENE_PERSONALITY);
    before_disk=saved;unsigned personality=s_trait_variant;
    for(unsigned c=0;c<8;c++)tap(310,418);
    assert(s_trait_variant==personality && !memcmp(&before_disk,&saved,sizeof saved));
    tap(48,46);assert(s_view==PROFILE);tap(48,46);assert(s_view==SETTINGS);tap(48,46);assert(s_view==HOME);
    // Start fresh is a two-step destructive action; opening/cancelling is safe.
    saved=snapshot;saved.evolution_progress=90;saved.inventory[15]=105;
    strcpy(saved.name,"Clover");pet_state_init();
    Pet old_pet=*pet_state_get();Pet old_save=saved;
    show(SETTINGS);tap(180,417);assert(s_view==RESET);shot("start-fresh");
    assert(!memcmp(pet_state_get(),&old_pet,sizeof old_pet));
    tap(180,351);assert(s_view==SETTINGS && test_restarts==0);
    assert(!memcmp(&saved,&old_save,sizeof saved));
    tap(180,417);tap(48,46);assert(s_view==SETTINGS && test_restarts==0);
    tap(180,417);fail_save=true;tap(180,414);
    assert(s_view==RESET && test_restarts==0);
    assert(!memcmp(pet_state_get(),&old_pet,sizeof old_pet));
    assert(!memcmp(&saved,&old_save,sizeof saved));
    assert(strstr(lv_label_get_text(s_hint),"Could not save"));shot("start-fresh-error");
    fail_save=false;tap(180,414);assert(test_restarts==1);
    const Pet *fresh=pet_state_get();
    assert(fresh->pet_id!=old_pet.pet_id && fresh->stage==PET_STAGE_BABY);
    assert(!strcmp(fresh->name,"Sprout") && fresh->evolution_progress==0);
    assert(fresh->hunger==100 && fresh->happiness==100 && fresh->energy==100 && fresh->hygiene==100);
    assert(pet_sticker_count(fresh)==0 && pet_room_gifts(fresh)==0 && pet_wall_sticker(fresh)==-1);
    for(unsigned i=3;i<PET_FOOD_COUNT;i++)assert(!pet_food_unlocked(fresh,i));
    assert(fresh->friends_met==0 && fresh->parent_a==0 && fresh->parent_b==0);
    for(unsigned i=0;i<16;i++)assert(fresh->inventory[i]==0);
    assert(!memcmp(fresh,&saved,sizeof saved));
    uint64_t fresh_id=fresh->pet_id;pet_state_init();assert(pet_state_get()->pet_id==fresh_id);
    pet_state_tick(pet_state_get()->last_tick+179);assert(pet_state_get()->hunger==100);
    show(HOME);shot("fresh-pet");
    saved = snapshot; pet_state_init();
    // Back works across the whole corner, not only the old 48px tile.
    const int back_points[][2]={{2,2},{85,2},{2,73},{85,73},{44,38}};
    const View back_views[]={FOOD,GAMES,SETTINGS,ALBUM,DECORATIONS,TREATS,PROFILE,TRAIT,RESET,PLAY_MENU,MUSIC,MULTIPLAYER,CATCH,HIDE,BALL};
    const View back_dest[]={HOME,PLAY_MENU,HOME,REWARDS,REWARDS,FOOD,SETTINGS,PROFILE,SETTINGS,HOME,PLAY_MENU,PLAY_MENU,GAMES,GAMES,GAMES};
    for(unsigned v=0;v<sizeof back_views/sizeof back_views[0];v++) {
        for(unsigned point=0;point<sizeof back_points/sizeof back_points[0];point++) {
            show(back_views[v]);tap(back_points[point][0],back_points[point][1]);assert(s_view==back_dest[v]);
        }
    }
    show(SETTINGS);shot("larger-back-button");bool auto_before=test_auto;
    tap(80,91);assert(s_view==SETTINGS && test_auto!=auto_before); // adjacent row isn't stolen
    test_auto=auto_before;
    // A PWR short press saves before shutdown, cancels unfinished meals and
    // silences voice/music. Failure at either stage always recovers awake.
    test_voice=VOICE_READY;s_muted=false;audio_set_muted(false);show(FOOD);start_food(0,NULL);
    Pet before_sleep=*pet_state_get();unsigned off_before=test_power_offs;
    test_power_press=true;advance(160);assert(s_view==POWER_OFF && s_power_pending && !s_eating);
    assert(!memcmp(&saved,&before_sleep,sizeof saved));assert(test_audio_muted && !s_talking);
    shot("power-goodnight");advance(300);assert(test_power_offs==off_before+1 && s_power_sent);
    advance(300);assert(test_power_offs==off_before+1); // never repeat the shutdown command
    advance(1700);assert(s_view==HOME && !s_power_pending && !test_audio_muted);
    assert(pet_state_get()->evolution_progress==before_sleep.evolution_progress);
    assert(strstr(s_reaction_text,"Still awake"));shot("power-no-shutdown");
    fail_save=true;test_power_press=true;advance(160);assert(s_view==HOME && !s_power_pending);
    assert(test_power_offs==off_before+1 && strstr(s_reaction_text,"Couldn't save"));fail_save=false;
    advance(900);test_power_off_ok=false;test_power_press=true;advance(600);
    assert(s_view==HOME && !s_power_pending && strstr(s_reaction_text,"Couldn't sleep"));
    assert(!test_audio_muted);test_power_off_ok=true;
    advance(900);s_muted=true;audio_set_muted(true);test_power_press=true;advance(2100);
    assert(s_view==HOME && test_audio_muted);s_muted=false;audio_set_muted(false);
    assert(pet_state_get()->pet_id==before_sleep.pet_id && pet_state_get()->evolution_progress==before_sleep.evolution_progress);
    puts("PASS: decay, floor, restore, persistence, growth; actual pointer taps through food/game/bath, sleep cancellation, rewards, mute, 100 navigation cycles; peekaboo, ball, 18 interactive wall stickers, six simultaneous saved gifts, legacy saves, weather and visits; eight whole characters, preview wraparound and transactional choice; milestone foods with original painted colours, paced meals, fades, compact mic, expanded back targets, PWR save/cancel/error recovery, reactive ball arcs and delayed fifth-bounce reward, cancel, failed saves and capped bonuses");
    lv_deinit();
    return 0;
}
