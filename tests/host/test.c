#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nvs.h"
#include "../../firmware/components/ui/ui.c"
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
static sfx_id_t test_sfx;
void audio_play(sfx_id_t f) {test_sfx=f;}
static bool test_tune;
static unsigned test_tune_choice, test_compositions;
bool audio_play_tune(unsigned n) {test_tune=n<3;test_tune_choice=n;return test_tune;}
void audio_stop_tune(void) {test_tune=false;}
bool audio_tune_playing(void) {return test_tune;}
bool voice_make_tune(const Pet *p,const char *a) {(void)p;(void)a;test_compositions++;return true;}
void audio_set_muted(bool m) { (void)m; }
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
void voice_cancel(void) {}
void voice_check(void) {}
voice_state_t voice_get_state(void) {return test_voice;}
void voice_status(char *s,size_t n) {snprintf(s,n,"Wi-Fi: Connected\nHome network\nIP: 192.168.1.42\nSignal: -52 dBm\nVoice server: Ready\nCheck passed: server replied");}
void voice_caption(char *s,size_t n) {snprintf(s,n,"Hello! I'm Sprout. Shall we play?");}
bool voice_boot_pressed(void) {return test_boot;}
static bool test_playing;
bool audio_voice_playing(void) {return test_playing;}
bool audio_is_ready(void) {return true;}
int audio_mic_level(void) {return 800;}
static int test_volume=100;
void audio_set_volume(int v) { test_volume=v; }
int audio_get_volume(void) { return test_volume; }
static uint32_t pixels[368*448];
static int tx,ty; static bool pressed;
static void read_touch(lv_indev_t *i,lv_indev_data_t *d) { (void)i; d->point.x=tx;d->point.y=ty;d->state=pressed?LV_INDEV_STATE_PRESSED:LV_INDEV_STATE_RELEASED; }
static void flush(lv_display_t *d,const lv_area_t *a,uint8_t *p) { (void)a;(void)p;lv_display_flush_ready(d); }
static void advance(unsigned ms) { for(unsigned n=0;n<ms;n+=20) { lv_tick_inc(20);lv_timer_handler(); } }
static void tap(int x,int y) { lv_obj_update_layout(s_root);advance(40);tx=x;ty=y;pressed=true;advance(60);pressed=false;advance(60); }
static void shot(const char *name) {
    advance(60); lv_refr_now(NULL);
    char path[256];snprintf(path,sizeof path,"%s.ppm",name);FILE *f=fopen(path,"wb");assert(f);
    fprintf(f,"P6\n368 448\n255\n");for(int i=0;i<368*448;i++) { unsigned char b[]={pixels[i]>>16,pixels[i]>>8,pixels[i]};fwrite(b,1,3,f); }fclose(f);
}
int main(void)
{
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
    lv_display_set_buffers(d,pixels,NULL,sizeof pixels,LV_DISPLAY_RENDER_MODE_DIRECT);lv_display_set_flush_cb(d,flush);
    lv_indev_t *input=lv_indev_create();lv_indev_set_type(input,LV_INDEV_TYPE_POINTER);lv_indev_set_read_cb(input,read_touch);
    ui_init(); shot("home");
    s_voice_until=0;advance(80);lv_obj_update_layout(s_root);
    assert(lv_obj_has_flag(lv_obj_get_parent(s_caption_label),LV_OBJ_FLAG_HIDDEN));
    shot("home-idle");
    test_playing=true;advance(80);lv_obj_update_layout(s_root);
    assert(s_face==PIXEL_TALK);
    assert(lv_obj_get_height(lv_obj_get_parent(s_caption_label))==70);
    shot("home-speaking");test_playing=false;

    tap(180,245); advance(60);
    assert((int32_t)(s_hop_until-lv_tick_get()) > 0);
    assert(s_face == PIXEL_HAPPY);
    assert(!lv_obj_has_flag(s_pet_heart, LV_OBJ_FLAG_HIDDEN));
    shot("pet-cuddle");
    advance(700);
    assert(lv_obj_has_flag(s_pet_heart, LV_OBJ_FLAG_HIDDEN));
    tap(60,385);assert(s_view==FOOD);shot("food");
    tap(73,350);tap(73,350);advance(1400);assert(s_view==PARTY);assert(pet_state_get()->evolution_progress==1);shot("party");
    tap(180,395);assert(s_view==HOME);tap(140,385);assert(s_view==GAMES);shot("games");tap(180,158);assert(s_view==CATCH);shot("play");
    for(int i=0;i<5;i++){lv_obj_update_layout(s_target);int x=lv_obj_get_x(s_target)+48,y=lv_obj_get_y(s_target)+48;tap(x,y);}
    assert(s_view==PARTY);assert(pet_state_get()->evolution_progress==2);
    show(BATH);shot("bath");tap(60,173);tap(178,159);tap(298,174);tap(70,260);tap(294,264);
    assert(s_view==PARTY);assert(pet_state_get()->evolution_progress==3);
    show(SLEEP);shot("sleep");
    assert(s_face == PIXEL_SLEEP);

    advance(2500);tap(48,46);assert(s_view==HOME);advance(4000);assert(pet_state_get()->evolution_progress==3);
    show(SLEEP);advance(6100);assert(s_view==PARTY);assert(pet_state_get()->evolution_progress==4);
    show(FOOD);tap(73,350);tap(48,46);advance(1400);
    assert(s_view==HOME);assert(pet_state_get()->evolution_progress==4);
    show(ALBUM);shot("album-locked");
    for(int i=0;i<26;i++)pet_state_play();show(ALBUM);shot("album");show(HOME);shot("grown-pet");
    show(HOME);shot("talk");
    tx=180;ty=342;pressed=true;advance(100);assert(s_talking);assert(s_view==HOME);
    assert(strstr(lv_label_get_text(s_caption_label),"listening"));
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
    // Repeated navigation catches dangling object pointers/timers.
    for(int i=0;i<100;i++){show((View)(i%14));advance(80);show(HOME);advance(80);}
    // Review every coat and all five stages with production geometry.
    Pet snapshot = saved;
    for (int i=0; i<6; i++) {
        saved = snapshot; saved.genes[GENE_BODY_COLOR] = i;
        saved.evolution_progress = 0; pet_state_init(); show(HOME);
        if (lv_tick_get()%4200 > 3800) advance(500);
        char name[32]; snprintf(name,sizeof name,"pet-coat-%d",i); shot(name);
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
        advance(240);snprintf(name,sizeof name,"eat-%s-bite",(const char*[]){"apple","toast","cookie"}[food]);shot(name);
        assert(memcmp(held[food],s_pet_art.pixels,sizeof held[food]));
        advance(1300);assert(s_view==PARTY && pet_state_get()->evolution_progress==before+1);
        advance(1500);assert(pet_state_get()->evolution_progress==before+1);
    }
    assert(memcmp(held[0],held[1],sizeof held[0]));assert(memcmp(held[1],held[2],sizeof held[0]));
    // Immediate feedback works offline/muted; spoken reactions are bounded and
    // never queue behind manual speech. Snapshots still learn the latest action.
    show(HOME);test_voice=VOICE_READY;test_auto=true;s_muted=false;audio_set_volume(100);
    s_next_reaction=0;unsigned reactions=test_reactions;
    tap(180,245);assert(test_event==PET_EVENT_CUDDLE && test_reactions==reactions+1);
    tap(180,245);assert(test_reactions==reactions+1);
    assert(strstr(s_reaction_text,"cuddles") || strstr(s_reaction_text,"leaves") || strstr(s_reaction_text,"tickles"));
    show(FOOD);tap(180,350);advance(1300);
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
    show(FOOD);tap(180,350);advance(1300);shot("reaction-toast");
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
    assert(s_view==HOME && pet_state_get()->evolution_progress==play_before+1);

    // The ball moves slowly, freezes under a finger, and rejects double-taps.
    show(GAMES);tap(180,343);assert(s_view==BALL);shot("bouncy-ball");
    play_before=pet_state_get()->evolution_progress;
    advance(400);int ball_x=lv_obj_get_x(s_target),ball_y=lv_obj_get_y(s_target);
    tx=ball_x+48;ty=ball_y+48;pressed=true;advance(80);
    int frozen_x=lv_obj_get_x(s_target),frozen_y=lv_obj_get_y(s_target);
    advance(1000);assert(s_ball_pressed && s_hits==0);
    assert(lv_obj_get_x(s_target)==frozen_x && lv_obj_get_y(s_target)==frozen_y);
    pressed=false;advance(60);assert(s_hits==1);
    tap(frozen_x+48,frozen_y+48);assert(s_hits==1);
    for(unsigned i=1;i<5;i++) {
        advance(440);tap(lv_obj_get_x(s_target)+48,lv_obj_get_y(s_target)+48);
        if(i<4)assert(s_hits==i+1);
    }
    assert(s_view==PARTY && test_event==PET_EVENT_BALL);
    assert(pet_state_get()->evolution_progress==play_before+1);
    advance(2500);assert(pet_state_get()->evolution_progress==play_before+1);
    show(BALL);tap(lv_obj_get_x(s_target)+48,lv_obj_get_y(s_target)+48);tap(48,46);advance(3000);
    assert(s_view==HOME && pet_state_get()->evolution_progress==play_before+1);

    // Schema-1 progress grants all rewards retrospectively. Saving a cosmetic
    // cannot spend stars or change identity, and failures don't alter live state.
    Pet legacy=saved;legacy.evolution_progress=29;legacy.inventory[15]=0;
    saved=legacy;pet_state_init();assert(pet_sticker_count(pet_state_get())==5);
    assert(pet_decoration_unlocked(pet_state_get(),1));assert(!pet_decoration_unlocked(pet_state_get(),2));
    assert(!pet_equip_decoration(2));assert(!pet_equip_decoration(999));assert(!pet_equip_decoration(-2));
    assert(pet_equipped_decoration(pet_state_get())==-1);
    assert(pet_equip_decoration(1));pet_state_init();assert(pet_equipped_decoration(pet_state_get())==1);
    assert(pet_state_get()->pet_id==legacy.pet_id && pet_state_get()->evolution_progress==29);
    fail_save=true;assert(!pet_equip_decoration(0));assert(pet_equipped_decoration(pet_state_get())==1);fail_save=false;
    show(DECORATIONS);shot("room-gifts-locked");tap(290,170);assert(s_view==DECORATIONS);
    assert(strstr(lv_label_get_text(s_hint),"1 more"));
    tap(80,170);assert(s_view==HOME && pet_equipped_decoration(pet_state_get())==0);shot("room-flowers");
    show(DECORATIONS);shot("plain-room-before");tap(90,372);shot("plain-room-after");assert(s_view==HOME && pet_equipped_decoration(pet_state_get())==-1);
    pet_state_play();show(PARTY);shot("new-room-gift");tap(100,400);assert(s_view==DECORATIONS);
    assert(pet_sticker_count(pet_state_get())==6 && pet_decoration_unlocked(pet_state_get(),2));
    s_album_page=0;show(ALBUM);tap(310,365);assert(s_album_page==1);shot("stickers-next");
    tap(70,175);assert(strstr(lv_label_get_text(s_hint),"5 more"));
    tap(60,365);assert(s_album_page==0);tap(60,365);assert(s_album_page==2);tap(310,365);assert(s_album_page==0);
    saved.evolution_progress=90;pet_state_init();assert(pet_sticker_count(pet_state_get())==18);
    for(unsigned i=0;i<6;i++)assert(pet_decoration_unlocked(pet_state_get(),i));
    for(unsigned i=0;i<3;i++){s_album_page=i;show(ALBUM);char name[40];snprintf(name,sizeof name,"stickers-page-%u",i+1);shot(name);}
    show(DECORATIONS);shot("room-gifts");tap(290,280);assert(s_view==HOME && pet_equipped_decoration(pet_state_get())==5);
    s_voice_until=0;shot("room-trophy");
    saved.evolution_progress=UINT32_MAX;pet_state_init();assert(pet_sticker_count(pet_state_get())==18);
    assert(!pet_decoration_unlocked(pet_state_get(),6));
    saved.inventory[15]=255;pet_state_init();assert(pet_equipped_decoration(pet_state_get())==-1);
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
    test_voice=VOICE_READY;s_muted=false;audio_set_volume(100);show(GAMES);tap(180,421);assert(s_view==MUSIC);shot("music");
    unsigned music_before=pet_state_get()->evolution_progress;
    for(unsigned i=0;i<3;i++){tap(180,150+57*(int)i);assert(test_tune && test_tune_choice==i);}
    test_boot=true;advance(200);assert(!test_tune && strstr(lv_label_get_text(s_hint),"listening"));
    test_boot=false;advance(200);test_voice=VOICE_READY;
    tap(180,400);assert(!test_tune);unsigned composed=test_compositions;
    tap(180,336);assert(test_compositions==composed+1);
    test_voice=VOICE_THINKING;tap(180,150);assert(!test_tune && strstr(lv_label_get_text(s_hint),"Stop"));test_voice=VOICE_READY;
    s_muted=true;tap(180,336);assert(test_compositions==composed+1);s_muted=false;
    tap(180,150);assert(test_tune);tap(48,46);assert(s_view==HOME && !test_tune);
    assert(pet_state_get()->evolution_progress==music_before);
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
    // Every saved gene can be explored without applying the preview or saving.
    assert(!pet_trait(8) && !pet_trait(999));
    for(unsigned gene=0;gene<8;gene++) {
        const pet_trait_t *t=pet_trait(gene);assert(t && t->count>0 && t->count<=16);
        for(unsigned value=0;value<256;value++) {
            Pet variant=*pet_state_get();variant.genes[gene]=(uint8_t)value;
            unsigned choice=pet_trait_choice(&variant,gene);
            assert(choice<t->count && t->values[choice] && t->descriptions[choice]);
            if(gene==GENE_BODY_COLOR)assert(choice==value%6);
        }
    }
    Pet before_traits=*pet_state_get(), saved_before_traits=saved;
    s_profile_page=0;show(HOME);shot("traits-home");tap(50,215);assert(s_view==PROFILE);shot("profile-1");
    tap(310,416);assert(s_profile_page==1);shot("profile-2");
    tap(50,416);assert(s_profile_page==0);
    for(unsigned page=0;page<2;page++) {
        for(unsigned card=0;card<4;card++) {
            tap(100+(card%2)*166,272+(card/2)*62);assert(s_view==TRAIT);
            char name[64];snprintf(name,sizeof name,"trait-%u",s_trait_gene);shot(name);
            unsigned start=s_trait_variant;const pet_trait_t *t=pet_trait(s_trait_gene);
            tap(50,418);assert(s_trait_variant==(start+t->count-1)%t->count);
            tap(310,418);assert(s_trait_variant==start);
            for(unsigned choice=0;choice<t->count;choice++) {
                if(s_trait_gene==GENE_BODY_COLOR || s_trait_gene==GENE_PERSONALITY) {
                    snprintf(name,sizeof name,"trait-%u-choice-%u",s_trait_gene,s_trait_variant);shot(name);
                }
                tap(310,418);
            }
            assert(s_trait_variant==start);
            tap(48,46);assert(s_view==PROFILE && s_profile_page==page);
        }
        tap(310,416);
    }
    assert(pet_state_get()->pet_id==before_traits.pet_id);
    assert(!memcmp(pet_state_get()->genes,before_traits.genes,8));
    assert(pet_state_get()->evolution_progress==before_traits.evolution_progress);
    assert(!memcmp(&saved,&saved_before_traits,sizeof saved));
    tap(48,46);assert(s_view==HOME);
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
    assert(pet_sticker_count(fresh)==0 && pet_equipped_decoration(fresh)==-1);
    assert(fresh->friends_met==0 && fresh->parent_a==0 && fresh->parent_b==0);
    for(unsigned i=0;i<16;i++)assert(fresh->inventory[i]==0);
    assert(!memcmp(fresh,&saved,sizeof saved));
    uint64_t fresh_id=fresh->pet_id;pet_state_init();assert(pet_state_get()->pet_id==fresh_id);
    pet_state_tick(pet_state_get()->last_tick+179);assert(pet_state_get()->hunger==100);
    show(HOME);shot("fresh-pet");
    saved = snapshot; pet_state_init();
    puts("PASS: decay, floor, restore, persistence, growth; actual pointer taps through food/game/bath, sleep cancellation, rewards, mute, 100 navigation cycles; peekaboo, ball, 18 stickers, saved room gifts, weather and visits; all eight traits, preview wraparound and unchanged genes/save");
    lv_deinit();
    return 0;
}
