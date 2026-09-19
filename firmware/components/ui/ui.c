// Little Meadow: illustrated pixel artwork and activities, all owned by the LVGL task.
#include <math.h>
#include <stdio.h>
#include <time.h>
#include "ui.h"
#include "renderer.h"
#include "power.h"
#include "audio.h"
#include "voice.h"
#include <string.h>
#include "esp_random.h"
#include "lvgl.h"
#include "pixel_pet.h"
#include "pixel_rooms.h"

#define INK 0x3d203d
#define CREAM 0xfff9ed
#define MINT 0xdaf3e5
#define GOLD 0xffcf57
#define PINK 0xf3a8b6
#define BLUE 0xa9dbef

typedef enum { HOME, FOOD, CATCH, BATH, SLEEP, PARTY, ALBUM, SETTINGS, CONNECTION, NAME, GAMES, HIDE, BALL, DECORATIONS, MUSIC } View;
static View s_view;
static lv_obj_t *s_root, *s_pet, *s_bars[4], *s_target, *s_counter;
static lv_obj_t *s_dots[5], *s_hint, *s_sleep_bar;
static lv_obj_t *s_volume_label, *s_volume_slider;
static lv_obj_t *s_pet_image, *s_pet_heart;
static lv_obj_t *s_voice_status, *s_caption_label, *s_connection, *s_name_input, *s_name_error;
static bool s_talking, s_boot_down, s_boot_raw;
static uint32_t s_talk_started, s_boot_changed, s_connection_tick, s_voice_until;
static char s_last_caption[512];
static uint32_t s_next_remark, s_last_manual_talk, s_next_reaction, s_reaction_until;
static char s_reaction_text[96];
static pixel_pet_art_t s_pet_art;
static pixel_face_t s_face;
static pixel_food_t s_food;
static uint32_t s_started, s_hop_until, s_last_tick;
static unsigned s_hits, s_last_target;
static bool s_eating, s_muted;
static unsigned s_album_page, s_hiding, s_weather_mode;
static uint32_t s_reveal_until, s_ball_ready, s_ball_time, s_next_butterfly, s_butterfly_until;
static bool s_ball_pressed;
static int s_music_choice=-1;
static lv_obj_t *s_covers[3], *s_cover_art[3], *s_butterfly, *s_butterfly_image;
static lv_obj_t *s_weather, *s_weather_icon, *s_rain[6];
static int s_pet_x, s_pet_y;
static const uint32_t colors[4] = {0xf5b164, 0xed99b4, 0x9aafe7, 0x79c8b7};
static void show(View view);
static void refresh(void);
static const char *activity(void);
static void reaction(pet_event_t event,const char *text,bool speak)
{
    uint32_t now=lv_tick_get();
    voice_note_event(event);
    snprintf(s_reaction_text,sizeof(s_reaction_text),"%s",text);s_reaction_until=now+3500;
    if(s_hint && s_view!=HOME && s_view!=PARTY)lv_label_set_text(s_hint,text);
    if(!speak || (s_view!=HOME && s_view!=PARTY) || s_talking || s_muted ||
       audio_get_volume()==0 || !voice_auto_enabled() || audio_voice_playing() || audio_tune_playing() ||
       voice_get_state()!=VOICE_READY || (s_next_reaction && (int32_t)(s_next_reaction-now)>0))return;
    if(voice_react(pet_state_get(),activity())) {
        s_next_reaction=now+45000;
        s_next_remark=now+240000+esp_random()%180001;
    }
}

static lv_obj_t *shape(lv_obj_t *p, int x, int y, int w, int h, uint32_t c, int r)
{
    lv_obj_t *o = lv_obj_create(p);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o,x,y); lv_obj_set_size(o,w,h);
    lv_obj_set_style_bg_color(o,lv_color_hex(c),0);
    lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);
    lv_obj_set_style_radius(o,0,0);
    (void)r;
    lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return o;
}
static lv_obj_t *label(lv_obj_t *p, const char *txt, int x, int y, int w, bool big)
{
    lv_obj_t *o=lv_label_create(p);
    lv_label_set_text(o,txt); lv_obj_set_pos(o,x,y); lv_obj_set_width(o,w);
    lv_obj_set_style_text_color(o,lv_color_hex(INK),0);
    const lv_font_t *font=big ? &lv_font_unscii_16 : &lv_font_montserrat_14;
    // UNSCII has no LV_SYMBOL glyphs; preserve readable navigation icons.
    for(const unsigned char *c=(const unsigned char *)txt;*c;c++)
        if(*c>=128) { font=big ? &lv_font_montserrat_24 : &lv_font_montserrat_14; break; }
    lv_obj_set_style_text_font(o,font,0);
    lv_obj_set_style_text_align(o,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_remove_flag(o,LV_OBJ_FLAG_CLICKABLE);
    return o;
}
static lv_obj_t *button(lv_obj_t *p,int x,int y,int w,int h,uint32_t c,lv_event_cb_t cb,int data)
{
    lv_obj_t *o=shape(p,x,y,w,h,c,22);
    lv_obj_add_flag(o,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(o,lv_color_hex(0xffffff),LV_STATE_PRESSED);
    lv_obj_set_style_translate_y(o,2,LV_STATE_PRESSED);
    lv_obj_set_style_shadow_color(o,lv_color_hex(0x738e83),0);
    lv_obj_set_style_shadow_opa(o,LV_OPA_20,0);
    lv_obj_set_style_shadow_width(o,0,0);
    lv_obj_set_style_border_color(o,lv_color_hex(INK),0);
    lv_obj_set_style_border_width(o,2,0);
    lv_obj_set_style_shadow_ofs_y(o,3,0);
    if(cb)lv_obj_add_event_cb(o,cb,LV_EVENT_CLICKED,(void *)(intptr_t)data);
    return o;
}
// All icon geometry is native LVGL: crisp at panel resolution, no glyph dependency.
static void star(lv_obj_t *p,int x,int y,uint32_t c)
{
    (void)c;
    lv_obj_t *o=lv_image_create(p);
    lv_image_set_src(o,pixel_icon(4));lv_image_set_pivot(o,0,0);
    lv_image_set_scale(o,512);lv_image_set_antialias(o,false);
    lv_obj_set_pos(o,x,y);
}
static void icon(lv_obj_t *p, int kind, int x, int y)
{
    lv_obj_t *o=lv_image_create(p);
    lv_image_set_src(o,pixel_icon((unsigned)kind));
    lv_image_set_pivot(o,0,0);
    lv_image_set_scale(o,512); // 24 pixels -> 48-pixel illustration, exact 2x grid
    lv_image_set_antialias(o,false);
    lv_obj_set_pos(o,x+3,y);
    lv_obj_remove_flag(o,LV_OBJ_FLAG_CLICKABLE);
}
static void home_cb(lv_event_t *e) { (void)e; show(HOME); }
static void nav_cb(lv_event_t *e) { show((View)(intptr_t)lv_event_get_user_data(e)); }
static void header(const char *title)
{
    lv_obj_t *b=button(s_root,24,22,48,48,0xffffff,home_cb,0);
    label(b,LV_SYMBOL_LEFT,0,11,48,true);
    label(s_root,title,80,32,264,true);
}
static void room(bool night)
{
    lv_obj_t *bg=lv_image_create(s_root);
    lv_image_set_src(bg,night ? &room_night : &room_day);
    lv_image_set_pivot(bg,0,0); lv_image_set_scale(bg,512);
    lv_image_set_antialias(bg,false); lv_obj_set_pos(bg,0,56);
}
static void meadow(void) { room(false); }
static void pet_tap(lv_event_t *e)
{
    (void)e;if(s_view!=HOME)return;
    s_hop_until=lv_tick_get()+600;
    static unsigned cuddle;
    reaction(PET_EVENT_CUDDLE,(const char*[]){"That tickles!","Cozy cuddles!","My leaves are wiggling!"}[cuddle++%3],true);
    audio_play(SFX_CUDDLE);
}
static void make_pet(int x, int y)
{
    s_pet_x=x-8; s_pet_y=y-14;
    s_pet=shape(s_root,s_pet_x,s_pet_y,220,222,0,0);
    lv_obj_set_style_bg_opa(s_pet,LV_OPA_TRANSP,0);
    lv_obj_add_flag(s_pet,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_pet,pet_tap,LV_EVENT_CLICKED,NULL);
    s_face=s_view==SLEEP?PIXEL_SLEEP:PIXEL_IDLE;
    pixel_pet_render(&s_pet_art,pet_state_get(),s_face,0,s_food);
    s_pet_image=lv_image_create(s_pet);
    lv_image_set_src(s_pet_image,&s_pet_art.image);
    lv_image_set_pivot(s_pet_image,0,0);lv_image_set_scale(s_pet_image,768);
    lv_image_set_antialias(s_pet_image,false);lv_obj_set_pos(s_pet_image,2,4);
    s_pet_heart=lv_image_create(s_pet);
    lv_image_set_src(s_pet_heart,pixel_icon(6));
    lv_image_set_pivot(s_pet_heart,0,0);lv_image_set_scale(s_pet_heart,384);
    lv_image_set_antialias(s_pet_heart,false);lv_obj_set_pos(s_pet_heart,135,10);
    lv_obj_add_flag(s_pet_heart,LV_OBJ_FLAG_HIDDEN);
}
static void progress(void)
{
    for(int i=0;i<5;i++) s_dots[i]=shape(s_root,119+i*28,94,18,18,0xe2e7df,9);
}
static void update_progress(void)
{
    for(int i=0;i<5;i++) if(s_dots[i]) lv_obj_set_style_bg_color(s_dots[i],lv_color_hex(i<(int)s_hits?GOLD:0xe2e7df),0);
}
static void finish(int action)
{
    View completed=s_view;
    if(action==0) pet_state_feed();
    if(action==1) pet_state_play();
    if(action==2) pet_state_rest();
    if(action==3) pet_state_clean();
    unsigned earned=pet_state_get()->evolution_progress;bool gift=false;
    for(unsigned i=0;i<PET_DECORATION_COUNT;i++)if(earned==pet_decoration_threshold(i))gift=true;
    audio_play(gift?SFX_GIFT:earned<=90 && earned%5==0?SFX_STICKER:action==3?SFX_BATH:SFX_HAPPY);
    show(PARTY);
    if(action==0)reaction((pet_event_t)(PET_EVENT_APPLE+s_food),
        (const char*[]){"Crunch! Juicy apple!","Mmm, warm buttery toast!","Cookie crumbs on my cheeks!"}[s_food],true);
    if(action==1) {
        if(completed==HIDE)reaction(PET_EVENT_HIDE,"Peekaboo champion! You found me!",true);
        else if(completed==BALL)reaction(PET_EVENT_BALL,"Boing! Five happy bounces!",true);
        else reaction(PET_EVENT_PLAY,"Five twinkly treasures!",true);
    }
    if(action==2)reaction(PET_EVENT_NAP,"Stretch! What a lovely nap!",true);
    if(action==3)reaction(PET_EVENT_BATH,"Sparkly clean, from toes to leaves!",true);
}
static void food_cb(lv_event_t *e)
{
    if(s_eating) return;
    s_food=(pixel_food_t)(intptr_t)lv_event_get_user_data(e);
    s_eating=true; s_started=lv_tick_get(); s_hop_until=s_started+1200;
    lv_label_set_text(s_hint,"Yum yum!");
    lv_obj_t *o=lv_event_get_target(e); lv_obj_set_style_bg_color(o,lv_color_hex(GOLD),0);
    audio_play((sfx_id_t)(SFX_APPLE+s_food));
}
static void place_target(void)
{
    // Discrete, widely separated positions keep every star stationary until tapped.
    static const int pos[6][2]={{24,124},{248,124},{136,112},{20,238},{252,238},{136,302}};
    unsigned n=(s_last_target+1+esp_random()%5)%6; s_last_target=n;
    lv_obj_set_pos(s_target,pos[n][0],pos[n][1]);
}
static void catch_cb(lv_event_t *e)
{
    (void)e; s_hits++; update_progress(); audio_play(SFX_STAR);
    if(s_hits==5) { finish(1); return; }
    s_hop_until=lv_tick_get()+450;
    reaction(PET_EVENT_STAR,(const char*[]){"Got one!","A twinkly treasure!","Catch that sparkle!","One more star!"}[(s_hits-1)%4],false);
    place_target();
}
static void bubble_cb(lv_event_t *e)
{
    lv_obj_t *o=lv_event_get_target(e);
    if(lv_obj_has_state(o,LV_STATE_DISABLED)) return;
    lv_obj_add_state(o,LV_STATE_DISABLED); lv_obj_add_flag(o,LV_OBJ_FLAG_HIDDEN);
    s_hits++; update_progress(); s_hop_until=lv_tick_get()+400; audio_play(SFX_BUBBLE);
    if(s_hits==5) finish(3);
    else reaction(PET_EVENT_BUBBLE,s_hits%2?"Pop! A tiny bubble!":"Splish, splash!",false);
}
static void mute_cb(lv_event_t *e)
{
    (void)e; s_muted=!s_muted; audio_set_muted(s_muted); show(SETTINGS);
}
static void volume_cb(lv_event_t *e)
{
    audio_set_volume(lv_slider_get_value(lv_event_get_target(e)));
    char text[32];
    snprintf(text, sizeof(text), "Volume: %d%%", audio_get_volume());
    lv_label_set_text(s_volume_label, text);
    // One preview on release, rather than a queue of chirps while dragging.
    if (lv_event_get_code(e) == LV_EVENT_RELEASED) audio_play(SFX_SELECT);
}
static const char *activity(void)
{
    return (const char*[]){"home","snack time","catch stars","bubble bath","nap","celebrating","stickers","options","connection check","naming","choosing a game","peekaboo","bouncy ball","room gifts","making music"}[s_view];
}
static void talk_begin(void)
{
    if(s_talking) return;
    audio_stop_tune();
    if(s_view==MUSIC)s_music_choice=-1;
    s_reaction_until=0;s_next_reaction=lv_tick_get()+45000;
    s_last_manual_talk=lv_tick_get();s_next_remark=s_last_manual_talk+240000+esp_random()%180001;
    s_talking=true;s_talk_started=lv_tick_get();s_voice_until=0;
    voice_start_talk(pet_state_get(),activity());
}
static void talk_end(void)
{
    if(!s_talking)return;
    s_talking=false;voice_end_talk(pet_state_get(),activity());
}
static void talk_cb(lv_event_t *e)
{
    if(lv_event_get_code(e)==LV_EVENT_PRESSED)talk_begin();
    if(lv_event_get_code(e)==LV_EVENT_RELEASED || lv_event_get_code(e)==LV_EVENT_PRESS_LOST)talk_end();
}
static void auto_chat_cb(lv_event_t *e)
{
    (void)e;voice_set_auto_enabled(!voice_auto_enabled());
    s_next_remark=lv_tick_get()+90000;show(SETTINGS);
}
static void check_cb(lv_event_t *e) { (void)e;voice_check(); }
static void name_save(lv_event_t *e)
{
    (void)e;
    if(pet_state_set_name(lv_textarea_get_text(s_name_input)))show(SETTINGS);
    else lv_label_set_text(s_name_error,"Use 1-15 letters, starting with a letter.");
}
// Small images share flash-backed descriptors; only Sprout has a mutable buffer.
static lv_obj_t *art(lv_obj_t *parent,const lv_image_dsc_t *src,int x,int y,unsigned scale)
{
    lv_obj_t *o=lv_image_create(parent);lv_image_set_src(o,src);
    lv_image_set_pivot(o,0,0);lv_image_set_scale(o,scale);lv_image_set_antialias(o,false);
    lv_obj_set_pos(o,x,y);lv_obj_remove_flag(o,LV_OBJ_FLAG_CLICKABLE);return o;
}
static const char *sticker_names[PET_STICKER_COUNT]={"Apple","Ball","Moon","Bubbles","Star","Heart",
    "Butterfly","Flower","Bunny","Rainbow","Kite","Water can","Cloud","Sun","Music","Crown","Book","Present"};
static const char *decor_names[PET_DECORATION_COUNT]={"Flowers","Bunting","Teddy","Moon lamp","Cushion","Trophy"};
static void page_cb(lv_event_t *e)
{
    s_album_page=(s_album_page+3+(int)(intptr_t)lv_event_get_user_data(e))%3;show(ALBUM);
}
static void sticker_cb(lv_event_t *e)
{
    unsigned i=(unsigned)(intptr_t)lv_event_get_user_data(e);char text[80];
    unsigned n=pet_state_get()->evolution_progress;
    if(n>=(i+1)*5)snprintf(text,sizeof text,"%s - yours to keep!",sticker_names[i]);
    else snprintf(text,sizeof text,"%s: %lu more stars",sticker_names[i],(unsigned long)((i+1)*5-n));
    lv_label_set_text(s_hint,text);audio_play(SFX_SELECT);
}
static void decorate_cb(lv_event_t *e)
{
    int d=(int)(intptr_t)lv_event_get_user_data(e);char text[80];
    if(d>=0 && !pet_decoration_unlocked(pet_state_get(),(unsigned)d)) {
        snprintf(text,sizeof text,"%s: %lu more stars",decor_names[d],
                 (unsigned long)(pet_decoration_threshold((unsigned)d)-pet_state_get()->evolution_progress));
        lv_label_set_text(s_hint,text);return;
    }
    if(!pet_equip_decoration(d)) {lv_label_set_text(s_hint,"Couldn't save. Please try again.");return;}
    audio_play(SFX_GIFT);show(HOME);
}
static void make_album(void)
{
    header("My stickers");const Pet *p=pet_state_get();unsigned count=pet_sticker_count(p);char text[80];
    snprintf(text,sizeof text,"%u / 18 stickers  |  %lu stars",count,(unsigned long)p->evolution_progress);
    label(s_root,text,24,85,320,false);
    if(count==PET_STICKER_COUNT)snprintf(text,sizeof text,"Your collection is complete!");
    else snprintf(text,sizeof text,"%lu more %s to your next sticker",
                  (unsigned long)((count+1)*5-p->evolution_progress),((count+1)*5-p->evolution_progress)==1?"star":"stars");
    s_hint=label(s_root,text,24,108,320,false);
    for(unsigned j=0;j<6;j++) {
        unsigned i=s_album_page*6+j;bool earned=i<count;
        lv_obj_t *b=button(s_root,29+(j%3)*108,141+(j/3)*100,94,91,earned?0xffecd1:0xe9ede6,sticker_cb,(int)i);
        if(earned)art(b,pixel_collectible(i),23,3,512);
        else {shape(b,36,9,22,22,0xb4beb5,0);shape(b,41,14,12,17,0xe9ede6,0);shape(b,31,24,32,22,0xb4beb5,0);}
        label(b,sticker_names[i],1,52,92,false);
        char threshold[24];snprintf(threshold,sizeof threshold,earned?"%u stars " LV_SYMBOL_OK:"%u stars",(i+1)*5);
        label(b,threshold,0,71,94,false);
    }
    lv_obj_t *prev=button(s_root,29,344,64,44,MINT,page_cb,-1);label(prev,LV_SYMBOL_LEFT,0,9,64,true);
    snprintf(text,sizeof text,"Page %u of 3",s_album_page+1);label(s_root,text,105,357,158,false);
    lv_obj_t *next=button(s_root,275,344,64,44,MINT,page_cb,1);label(next,LV_SYMBOL_RIGHT,0,9,64,true);
    lv_obj_t *gifts=button(s_root,29,399,310,40,BLUE,nav_cb,DECORATIONS);label(gifts,"My room gifts " LV_SYMBOL_HOME,0,11,310,false);
}
static void make_decorations(void)
{
    header("My room gifts");const Pet *p=pet_state_get();int selected=pet_equipped_decoration(p);
    s_hint=label(s_root,"Tap a gift to put it in your room",24,88,320,false);
    for(unsigned i=0;i<PET_DECORATION_COUNT;i++) {
        bool unlocked=pet_decoration_unlocked(p,i);
        lv_obj_t *b=button(s_root,29+(i%3)*108,123+(i/3)*112,94,103,selected==(int)i?MINT:unlocked?0xffecd1:0xe9ede6,decorate_cb,(int)i);
        art(b,pixel_decoration(i),27,5,256);
        if(!unlocked)lv_obj_set_style_opa(b,LV_OPA_60,0);
        label(b,decor_names[i],0,51,94,false);
        char text[24];snprintf(text,sizeof text,selected==(int)i?"In room":unlocked?"Use gift":"%u stars",pet_decoration_threshold(i));
        label(b,text,0,77,94,false);
    }
    lv_obj_t *plain=button(s_root,29,350,145,44,CREAM,decorate_cb,-1);label(plain,"Plain room",0,14,145,false);
    lv_obj_t *album=button(s_root,194,350,145,44,BLUE,nav_cb,ALBUM);label(album,"My stickers",0,14,145,false);
    label(s_root,"Gifts stay yours. Stars aren't spent.",24,413,320,false);
}
static void make_games(void)
{
    header("Let's play!");label(s_root,"A little game, a happy pet",24,85,320,false);
    const char *names[]={"Stars","Peekaboo","Bouncy ball"};
    const char *hints[]={"Catch 5 twinkly stars","Find me 3 times","Give the ball 5 bounces"};
    for(int i=0;i<3;i++) {
        lv_obj_t *b=button(s_root,29,121+i*91,310,79,(uint32_t[]){0xffecd1,MINT,BLUE}[i],nav_cb,(int[]){CATCH,HIDE,BALL}[i]);
        art(b,i==1?pixel_decoration(0):pixel_icon(i==0?4:1),12,13,i==1?320:512);
        label(b,names[i],77,16,222,true);label(b,hints[i],72,47,232,false);
    }
    lv_obj_t *music=button(s_root,29,398,310,44,PINK,nav_cb,MUSIC);
    label(music,"Music " LV_SYMBOL_AUDIO,0,14,310,false);
}
static void music_cb(lv_event_t *e)
{
    int choice=(int)(intptr_t)lv_event_get_user_data(e);
    if(choice==-1) {audio_stop_tune();voice_cancel();s_music_choice=-1;lv_label_set_text(s_hint,"All quiet. Pick a little tune!");return;}
    if(s_muted || audio_get_volume()==0) {lv_label_set_text(s_hint,"Turn sound on in Options first");return;}
    if(choice==3) {
        audio_stop_tune();
        if(voice_make_tune(pet_state_get(),activity())) {s_music_choice=3;lv_label_set_text(s_hint,"Making a tune... Stop cancels");}
        else lv_label_set_text(s_hint,"Voice busy or offline. Try a tune below!");
    } else if(voice_get_state()==VOICE_THINKING || voice_get_state()==VOICE_LISTENING) {
        lv_label_set_text(s_hint,"Making a tune... Stop cancels");
    } else if(audio_play_tune((unsigned)choice)) {
        s_music_choice=choice;lv_label_set_text(s_hint,(const char*[]){"Twinkly meadow","Bouncy dance","Sleepy leaves"}[choice]);
    } else lv_label_set_text(s_hint,"Let me finish talking first");
}
static void make_music(void)
{
    header("Little tunes");s_music_choice=-1;
    s_hint=label(s_root,"Pick a tune. Have a little wiggle!",24,89,320,false);
    const char *titles[]={"Twinkly meadow","Bouncy dance","Sleepy leaves"};
    for(int i=0;i<3;i++) {
        lv_obj_t *b=button(s_root,29,128+i*57,310,46,(uint32_t[]){0xffecd1,MINT,BLUE}[i],music_cb,i);
        art(b,pixel_collectible((unsigned[]){4,14,2}[i]),9,10,256);label(b,titles[i],43,15,254,false);
    }
    char request[64];snprintf(request,sizeof request,"%s, make me a tune!",pet_state_get()->name);
    lv_obj_t *compose=button(s_root,29,313,310,48,PINK,music_cb,3);label(compose,request,0,16,310,false);
    lv_obj_t *stop=button(s_root,89,379,190,44,CREAM,music_cb,-1);label(stop,"Stop music " LV_SYMBOL_STOP,0,14,190,false);
}
static void hide_place(void)
{
    s_hiding=(s_hiding+1+esp_random()%2)%3;s_reveal_until=0;
    for(int i=0;i<3;i++){lv_obj_remove_state(s_covers[i],LV_STATE_DISABLED);lv_obj_set_style_bg_opa(s_covers[i],LV_OPA_TRANSP,0);lv_obj_remove_flag(s_cover_art[i],LV_OBJ_FLAG_HIDDEN);}
    s_pet_x=32+(int)s_hiding*108;s_pet_y=201;lv_obj_set_pos(s_pet,s_pet_x,s_pet_y);
    lv_label_set_text(s_hint,"Can you spot my little leaves?");
}
static void hide_cb(lv_event_t *e)
{
    if(s_view!=HIDE || s_reveal_until)return;
    unsigned i=(unsigned)(intptr_t)lv_event_get_user_data(e);
    if(i!=s_hiding) {
        lv_obj_add_state(s_covers[i],LV_STATE_DISABLED);lv_obj_add_flag(s_cover_art[i],LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(s_hint,"Not here! Try another flowerpot.");audio_play(SFX_HIDE);return;
    }
    s_hits++;update_progress();s_reveal_until=lv_tick_get()+1100;
    lv_obj_add_flag(s_cover_art[i],LV_OBJ_FLAG_HIDDEN);s_pet_y=171;
    lv_label_set_text(s_hint,"Peekaboo! You found me!");audio_play(SFX_FOUND);
}
static void make_hide(void)
{
    room(false);header("Peekaboo");
    for(int i=0;i<3;i++)s_dots[i]=shape(s_root,147+i*28,94,18,18,0xe2e7df,0);
    make_pet(40,215);lv_obj_set_size(s_pet,96,105);lv_image_set_scale(s_pet_image,320);
    lv_obj_remove_flag(s_pet,LV_OBJ_FLAG_CLICKABLE);
    for(int i=0;i<3;i++) {
        s_covers[i]=button(s_root,29+i*108,190,94,132,CREAM,hide_cb,i);
        lv_obj_set_style_bg_opa(s_covers[i],LV_OPA_TRANSP,0);lv_obj_set_style_border_width(s_covers[i],0,0);
        s_cover_art[i]=art(s_covers[i],pixel_decoration(0),7,44,512);
    }
    s_hint=label(s_root,"",24,363,320,false);hide_place();
    label(s_root,"Find me three times. No hurry!",24,413,320,false);
}
static void ball_press(lv_event_t *e)
{
    s_ball_pressed=lv_event_get_code(e)==LV_EVENT_PRESSED;
}
static void ball_cb(lv_event_t *e)
{
    (void)e;uint32_t now=lv_tick_get();
    if(s_view!=BALL || (int32_t)(s_ball_ready-now)>0)return;
    s_hits++;update_progress();audio_play(SFX_BOUNCE);s_ball_ready=now+400;s_hop_until=now+600;
    if(s_hits==5){finish(1);return;}
    lv_label_set_text(s_hint,(const char*[]){"Boing!","Up it goes!","Wheee!","One more bounce!"}[s_hits-1]);
}
static void make_ball(void)
{
    room(false);header("Bouncy ball");progress();make_pet(82,136);
    lv_obj_remove_flag(s_pet,LV_OBJ_FLAG_CLICKABLE);
    s_target=button(s_root,136,265,96,96,CREAM,ball_cb,0);
    lv_obj_set_style_bg_opa(s_target,LV_OPA_TRANSP,0);lv_obj_set_style_border_width(s_target,0,0);
    art(s_target,pixel_icon(1),12,12,768);
    lv_obj_add_event_cb(s_target,ball_press,LV_EVENT_PRESSED,NULL);
    lv_obj_add_event_cb(s_target,ball_press,LV_EVENT_RELEASED,NULL);
    lv_obj_add_event_cb(s_target,ball_press,LV_EVENT_PRESS_LOST,NULL);
    s_hint=label(s_root,"Tap the ball to bounce it!",24,405,320,false);
}
static void butterfly_cb(lv_event_t *e)
{
    (void)e;s_butterfly_until=0;lv_obj_add_flag(s_butterfly,LV_OBJ_FLAG_HIDDEN);
    s_next_butterfly=lv_tick_get()+60000+esp_random()%40001;s_hop_until=lv_tick_get()+700;
    reaction(PET_EVENT_BUTTERFLY,"Hello, little fluttery friend!",true);audio_play(SFX_BUTTERFLY);
}
static void make_room_extras(void)
{
    int d=pet_equipped_decoration(pet_state_get());
    if(d>=0) {
        // Bunting hangs across the window; other gifts sit on the right of the rug.
        art(s_root,pixel_decoration((unsigned)d),d==1?124:266,d==1?38:208,d==1?768:256);
    }
    // Weather stays inside the glass; the authored curtains, frame and garden remain.
    s_weather=shape(s_root,124,78,120,76,0,0);lv_obj_set_style_bg_opa(s_weather,LV_OPA_TRANSP,0);
    s_weather_icon=art(s_weather,pixel_collectible(13),76,d==1?37:0,d==1?256:384);
    for(int i=0;i<6;i++)s_rain[i]=shape(s_weather,6+(i%3)*12+(i/3)*66,24,2,7,0x4675af,0);
    s_weather_mode=3;
    s_butterfly=button(s_root,24,170,60,60,CREAM,butterfly_cb,0);
    lv_obj_set_style_bg_opa(s_butterfly,LV_OPA_TRANSP,0);lv_obj_set_style_border_width(s_butterfly,0,0);
    s_butterfly_image=art(s_butterfly,pixel_collectible(6),6,6,512);
    lv_obj_add_flag(s_butterfly,LV_OBJ_FLAG_HIDDEN);
    if(!s_next_butterfly)s_next_butterfly=lv_tick_get()+20000;
}
static void room_frame(uint32_t now,voice_state_t vs)
{
    unsigned mode=(now/180000)%3;
    if(mode!=s_weather_mode) {
        s_weather_mode=mode;lv_image_set_src(s_weather_icon,pixel_collectible((unsigned[]){13,12,9}[mode]));
        for(int i=0;i<6;i++)if(mode==1)lv_obj_remove_flag(s_rain[i],LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(s_rain[i],LV_OBJ_FLAG_HIDDEN);
    }
    if(mode==1)for(int i=0;i<6;i++)lv_obj_set_y(s_rain[i],22+(now/110+i*5)%19);
    bool quiet=!s_talking && vs!=VOICE_LISTENING && vs!=VOICE_THINKING && vs!=VOICE_SPEAKING && !audio_voice_playing();
    if(!quiet && s_butterfly_until) {s_butterfly_until=0;s_next_butterfly=now+20000;lv_obj_add_flag(s_butterfly,LV_OBJ_FLAG_HIDDEN);}
    if(!s_butterfly_until && quiet && (int32_t)(now-s_next_butterfly)>=0) {
        s_butterfly_until=now+10000;s_next_butterfly=now+60000+esp_random()%40001;
        lv_obj_remove_flag(s_butterfly,LV_OBJ_FLAG_HIDDEN);
    }
    if(s_butterfly_until) {
        if((int32_t)(s_butterfly_until-now)<=0) {s_butterfly_until=0;lv_obj_add_flag(s_butterfly,LV_OBJ_FLAG_HIDDEN);}
        else if(!lv_obj_has_state(s_butterfly,LV_STATE_PRESSED)) {
            float phase=(10000-(s_butterfly_until-now))/1000.0f;
            lv_obj_set_pos(s_butterfly,28+(int)(phase*24),157+(int)(sinf(phase*1.5f)*15));
            lv_image_set_scale_x(s_butterfly_image,(now/180)%2?512:384);
        }
    }
}

static void make_home(void)
{
    meadow();
    for(int i=0;i<4;i++) {
        lv_obj_t *slot=button(s_root,24+i*82,12,74,38,CREAM,nav_cb,(int[]){FOOD,GAMES,SLEEP,BATH}[i]);
        lv_obj_set_style_border_width(slot,0,0);
        lv_obj_t *glyph=lv_image_create(slot);
        lv_image_set_src(glyph,pixel_icon(i==1?6:(unsigned)i));
        lv_image_set_pivot(glyph,0,0);lv_image_set_scale(glyph,384);
        lv_image_set_antialias(glyph,false);lv_obj_set_pos(glyph,-2,0);
        s_bars[i]=lv_bar_create(slot); lv_obj_set_size(s_bars[i],34,10); lv_obj_set_pos(s_bars[i],36,13);
        lv_obj_set_style_bg_color(s_bars[i],lv_color_hex(0xe9dfcf),LV_PART_MAIN);
        lv_obj_set_style_bg_color(s_bars[i],lv_color_hex(colors[i]),LV_PART_INDICATOR);
        lv_obj_set_style_radius(s_bars[i],0,LV_PART_MAIN); lv_obj_set_style_radius(s_bars[i],0,LV_PART_INDICATOR);
        // Three easy-to-read pips, with the continuous value retained underneath.
        shape(slot,46,13,3,10,CREAM,0);shape(slot,59,13,3,10,CREAM,0);
    }

    make_pet(82,120);
    make_room_extras();
    // A fixed, generous hold target stays under the pet. Captions appear only
    // during a conversation, leaving the window visible during ordinary play.
    lv_obj_t *talk=button(s_root,24,320,320,44,CREAM,NULL,0);
    s_hint=label(talk,"Hold to talk to Sprout",4,13,308,false);
    s_voice_status=s_hint;
    lv_obj_add_event_cb(talk,talk_cb,LV_EVENT_PRESSED,NULL);
    lv_obj_add_event_cb(talk,talk_cb,LV_EVENT_RELEASED,NULL);
    lv_obj_add_event_cb(talk,talk_cb,LV_EVENT_PRESS_LOST,NULL);
    lv_obj_t *caption=button(s_root,24,67,320,70,CREAM,NULL,0);
    lv_obj_remove_flag(caption,LV_OBJ_FLAG_CLICKABLE);
    s_caption_label=label(caption,"",8,9,300,false);
    lv_obj_set_height(s_caption_label,52);lv_label_set_long_mode(s_caption_label,LV_LABEL_LONG_SCROLL);
    lv_obj_add_flag(caption,LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *album=button(s_root,24,249,54,51,CREAM,nav_cb,ALBUM); icon(album,4,-2,1);
    lv_obj_t *settings=button(s_root,294,250,50,48,CREAM,nav_cb,SETTINGS);
    label(settings,LV_SYMBOL_SETTINGS,0,11,50,true);
    const char *names[]={"Food","Play","Sleep","Bath"};
    int views[]={FOOD,GAMES,SLEEP,BATH};
    for(int i=0;i<4;i++) {
        lv_obj_t *b=button(s_root,24+i*82,368,74,68,(uint32_t[]){0xff95b0,0x93deaf,0xc5a1ed,0x85d6f3}[i],nav_cb,views[i]);
        // Crisp inset lighting gives the little toy buttons depth.
        shape(b,4,3,66,3,0xffeedf,0);shape(b,3,6,3,57,0xffeedf,0);
        shape(b,4,61,66,3,0x9b688e,0);shape(b,67,6,3,55,0x9b688e,0);
        icon(b,i,8,-2); label(b,names[i],0,45,74,false);
        shape(b,21,64,32,2,colors[i],2);
    }
    refresh();
}
static void show(View view)
{
    if(s_view==MUSIC && view!=MUSIC) {audio_stop_tune();if(s_music_choice==3)voice_cancel();}
    if(s_talking && !s_boot_down)talk_end();
    s_voice_status=NULL;s_caption_label=NULL;s_connection=NULL;s_name_input=NULL;s_name_error=NULL;
    // Single owner: no screen-specific timers or callbacks survive the root.
    s_volume_label=NULL; s_volume_slider=NULL;
    s_weather=NULL;s_weather_icon=NULL;s_butterfly=NULL;s_butterfly_image=NULL;
    memset(s_covers,0,sizeof s_covers);memset(s_cover_art,0,sizeof s_cover_art);memset(s_rain,0,sizeof s_rain);
    s_reveal_until=0;s_ball_ready=0;s_ball_pressed=false;s_ball_time=0;s_butterfly_until=0;
    s_pet=NULL; s_target=NULL; s_hint=NULL; s_counter=NULL; s_sleep_bar=NULL;
    for(int i=0;i<4;i++) s_bars[i]=NULL;
    for(int i=0;i<5;i++) s_dots[i]=NULL;
    if(s_root) lv_obj_delete(s_root);
    s_view=view; s_started=lv_tick_get(); s_hits=0; s_eating=false; s_hop_until=0;s_reaction_until=0;
    s_root=shape(lv_screen_active(),0,0,368,448,CREAM,0);
    if(view==HOME) { make_home(); return; }
    if(view==MUSIC) {make_music();
    } else if(view==GAMES) {make_games();
    } else if(view==HIDE) {make_hide();
    } else if(view==BALL) {make_ball();
    } else if(view==DECORATIONS) {make_decorations();
    } else if(view==FOOD) {
        room(false); header("Snack time"); make_pet(82,114);
        shape(s_root,32,85,304,29,CREAM,0);
        s_hint=label(s_root,"Pick a yummy snack",32,92,304,false);
        for(int i=0;i<3;i++) {
            lv_obj_t *b=button(s_root,29+i*108,310,94,89,0xffe9d3,food_cb,i);
            icon(b,(int[]){0,7,8}[i],19,9);
            label(b,(const char*[]){"Apple","Toast","Cookie"}[i],0,65,94,false);
        }
    } else if(view==CATCH) {
        room(false); header("Catch the stars"); progress(); make_pet(82,134);
        lv_obj_remove_flag(s_pet,LV_OBJ_FLAG_CLICKABLE);
        s_hint=label(s_root,"Tap each golden star",32,405,304,false);
        s_target=button(s_root,40,150,96,96,0xffedb9,catch_cb,0);
        lv_obj_set_style_bg_opa(s_target,LV_OPA_TRANSP,0);lv_obj_set_style_border_width(s_target,0,0);
        star(s_target,28,26,0xe8aa35); place_target();
        // Quiet dotted trail gives the playfield depth without distracting targets.
        for(int i=0;i<7;i++) shape(s_root,35+i*46,378,5,5,0xdce8df,3);
    } else if(view==BATH) {
        room(false); header("Bubble bath"); progress(); make_pet(82,126);
        // The bathtub and foam are part of the illustrated pose.

        s_hint=label(s_root,"Pop all five bubbles",32,397,304,false);
        static const int xy[5][2]={{26,116},{144,100},{264,116},{35,225},{259,229}};
        for(int i=0;i<5;i++) {
            lv_obj_t *b=button(s_root,xy[i][0],xy[i][1],70,70,0xc2e8ef,bubble_cb,i);
            lv_obj_set_style_bg_opa(b,LV_OPA_TRANSP,0);lv_obj_set_style_border_width(b,0,0);
            lv_obj_t *foam=lv_image_create(b);lv_image_set_src(foam,pixel_icon(9));
            lv_image_set_pivot(foam,0,0);lv_image_set_scale(foam,640);lv_image_set_antialias(foam,false);
            lv_obj_set_pos(foam,5,5);lv_obj_remove_flag(foam,LV_OBJ_FLAG_CLICKABLE);
        }
    } else if(view==SLEEP) {
        audio_play(SFX_SLEEP);room(true);
        header("Little nap");
        make_pet(82,128);
        s_sleep_bar=lv_bar_create(s_root); lv_obj_set_pos(s_sleep_bar,74,365); lv_obj_set_size(s_sleep_bar,220,12);
        lv_obj_set_style_bg_color(s_sleep_bar,lv_color_hex(GOLD),LV_PART_INDICATOR);
        lv_obj_t *dream=button(s_root,268,149,62,50,CREAM,NULL,0);
        lv_obj_remove_flag(dream,LV_OBJ_FLAG_CLICKABLE);
        label(dream,"z Z",0,16,62,true);
        s_hint=label(s_root,"My eyes feel sleepy...",24,397,320,false);
    } else if(view==PARTY) {
        unsigned earned=pet_state_get()->evolution_progress;
        bool gift=false;
        for(unsigned i=0;i<PET_DECORATION_COUNT;i++)if(earned==pet_decoration_threshold(i))gift=true;
        bool sticker=earned>0 && earned<=PET_STICKER_COUNT*5 && earned%5==0;
        if(sticker)s_album_page=(earned/5-1)/6;
        label(s_root,gift?"New room gift!":sticker?"New sticker!":"Lovely caring!",24,40,320,true);
        for(int i=0;i<12;i++) shape(s_root,24+(i*71)%315,95+(i*43)%220,7,12,colors[i%4],3);
        make_pet(82,132); star(s_root,163,93,GOLD);
        char text[64]; unsigned n=pet_state_get()->evolution_progress;
        snprintf(text,sizeof(text),"%lu %s  /  %u of 18 stickers",(unsigned long)n,n==1?"star":"stars",pet_sticker_count(pet_state_get()));
        label(s_root,text,24,337,320,false);
        if(sticker) {
            lv_obj_t *gift_button=button(s_root,24,376,152,48,GOLD,nav_cb,gift?DECORATIONS:ALBUM);
            label(gift_button,gift?"My gift":"My sticker",0,16,152,false);
            lv_obj_t *b=button(s_root,192,376,152,48,MINT,home_cb,0);label(b,"Home " LV_SYMBOL_HOME,0,16,152,false);
        } else {
            lv_obj_t *b=button(s_root,74,376,220,48,MINT,home_cb,0);label(b,"Home " LV_SYMBOL_HOME,0,11,220,true);
        }
        lv_obj_t *caption=button(s_root,24,77,320,64,CREAM,NULL,0);
        lv_obj_remove_flag(caption,LV_OBJ_FLAG_CLICKABLE);
        s_caption_label=label(caption,"",8,8,300,false);lv_obj_set_height(s_caption_label,48);
        lv_label_set_long_mode(s_caption_label,LV_LABEL_LONG_SCROLL);lv_obj_add_flag(caption,LV_OBJ_FLAG_HIDDEN);
        s_voice_status=label(s_root,"",24,356,320,false);
        s_hop_until=lv_tick_get()+1800;
    } else if(view==ALBUM) {
        make_album();
    } else if(view==SETTINGS) {
        header("Grown-ups");
        lv_obj_t *chatter=button(s_root,54,77,260,28,CREAM,auto_chat_cb,0);
        label(chatter,voice_auto_enabled()?"Little chats: on":"Little chats: off",0,5,260,false);
        int batt=power_battery_percent(); char b[64];
        if(batt<0) snprintf(b,sizeof(b),"Battery: USB power");
        else snprintf(b,sizeof(b),"Battery: %d%%%s",batt,power_is_charging()?" (charging)":"");
        label(s_root,b,24,111,320,false);
        lv_obj_t *m=button(s_root,54,157,260,58,MINT,mute_cb,0); label(m,s_muted?"Sound off":"Sound on",0,16,260,true);
        snprintf(b, sizeof(b), "Volume: %d%%", audio_get_volume());
        s_volume_label=label(s_root,b,40,237,288,false);
        s_volume_slider=lv_slider_create(s_root);
        lv_obj_set_pos(s_volume_slider,58,284); lv_obj_set_size(s_volume_slider,252,16);
        lv_slider_set_range(s_volume_slider,0,100);
        lv_slider_set_value(s_volume_slider,audio_get_volume(),LV_ANIM_OFF);
        lv_obj_set_ext_click_area(s_volume_slider,18);
        lv_obj_set_style_bg_color(s_volume_slider,lv_color_hex(0xe9dfcf),LV_PART_MAIN);
        lv_obj_set_style_bg_color(s_volume_slider,lv_color_hex(0x93deaf),LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(s_volume_slider,lv_color_hex(INK),LV_PART_KNOB);
        lv_obj_set_style_pad_all(s_volume_slider,10,LV_PART_KNOB);
        lv_obj_set_style_radius(s_volume_slider,0,LV_PART_MAIN);
        lv_obj_set_style_radius(s_volume_slider,0,LV_PART_INDICATOR);
        lv_obj_set_style_radius(s_volume_slider,3,LV_PART_KNOB);
        lv_obj_add_event_cb(s_volume_slider,volume_cb,LV_EVENT_VALUE_CHANGED,NULL);
        lv_obj_add_event_cb(s_volume_slider,volume_cb,LV_EVENT_RELEASED,NULL);
        label(s_root,"Quiet",38,318,72,false);
        label(s_root,"Loud",258,318,72,false);
        lv_obj_t *wifi=button(s_root,34,348,300,36,BLUE,nav_cb,CONNECTION);
        label(wifi,"Wi-Fi & voice connection",0,9,300,false);
        lv_obj_t *name=button(s_root,34,395,300,36,0xffe9d3,nav_cb,NAME);
        snprintf(b,sizeof(b),"Pet name: %s",pet_state_get()->name);label(name,b,0,9,300,false);
    } else if(view==CONNECTION) {
        header("Connection");
        s_connection=label(s_root,"Checking...",24,105,320,false);
        lv_obj_set_style_text_line_space(s_connection,12,0);
        lv_obj_t *b=button(s_root,54,323,260,48,MINT,check_cb,0);label(b,"Check now",0,15,260,true);
        label(s_root,"Wi-Fi connects automatically.\nVoice uses Ron's Mac mini.\nNormal play works offline.",24,385,320,false);
        voice_check();s_connection_tick=0;
    } else if(view==NAME) {
        header("Pet name");
        label(s_root,"What shall we call your pet?",24,88,320,false);
        s_name_input=lv_textarea_create(s_root);lv_obj_set_pos(s_name_input,32,122);lv_obj_set_size(s_name_input,304,48);
        lv_textarea_set_one_line(s_name_input,true);lv_textarea_set_max_length(s_name_input,15);
        lv_textarea_set_accepted_chars(s_name_input,"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz '-");
        lv_textarea_set_text(s_name_input,pet_state_get()->name);
        s_name_error=label(s_root,"A short, friendly name",24,184,320,false);
        lv_obj_t *b=button(s_root,96,215,176,43,MINT,name_save,0);label(b,"Save name",0,13,176,false);
        lv_obj_t *kb=lv_keyboard_create(s_root);lv_obj_set_align(kb,LV_ALIGN_TOP_LEFT);lv_obj_set_pos(kb,0,275);lv_obj_set_size(kb,368,173);
        lv_keyboard_set_textarea(kb,s_name_input);
    }
}
static void refresh(void)
{
    const Pet *p=pet_state_get(); if(!p) return;
    int v[]={p->hunger,p->happiness,p->energy,p->hygiene};
    for(int i=0;i<4;i++) if(s_bars[i]) lv_bar_set_value(s_bars[i],v[i],LV_ANIM_ON);
    if(s_view==HOME && s_hint) {
        int low=0; for(int i=1;i<4;i++) if(v[i]<v[low]) low=i;
        lv_label_set_text(s_hint,v[low]<45?(const char*[]){"A snack would be lovely","Let's catch some stars!","Time for a little nap","Let's pop some bubbles!"}[low]:"Tap me for a cuddle");
    }
}
static void frame(lv_timer_t *t)
{
    (void)t; uint32_t now=lv_tick_get(), elapsed=now-s_started;
    bool boot=voice_boot_pressed();
    if(boot!=s_boot_raw) { s_boot_raw=boot;s_boot_changed=now; }
    if(now-s_boot_changed>=80 && boot!=s_boot_down) {
        s_boot_down=boot;if(boot)talk_begin();else talk_end();
    }
    if(s_talking && now-s_talk_started>=20000)talk_end();
    voice_state_t vs=voice_get_state();
    if(!s_next_remark)s_next_remark=now+90000;
    uint32_t inactivity=lv_display_get_inactive_time(NULL);
    if((int32_t)(now-s_next_remark)>=0 && s_view==HOME && !s_talking &&
       !s_muted && audio_get_volume()>0 && voice_auto_enabled() &&
       (inactivity<600000 || now-s_last_manual_talk<600000) && vs==VOICE_READY && !audio_voice_playing() && !audio_tune_playing()) {
        s_next_remark=now+240000+esp_random()%180001;
        if(voice_remark(pet_state_get(),activity()))s_next_reaction=now+45000;
    }
    bool local_reaction=(int32_t)(s_reaction_until-now)>0;
    if(s_reaction_until && !local_reaction) {
        s_reaction_until=0;
        if(s_hint && s_view==CATCH)lv_label_set_text(s_hint,"Tap each golden star");
        if(s_hint && s_view==BATH)lv_label_set_text(s_hint,"Pop all five bubbles");
    }
    if(s_voice_status) {
        char caption[512];voice_caption(caption,sizeof(caption));
        bool speaking=vs==VOICE_SPEAKING || audio_voice_playing();
        bool busy=vs==VOICE_LISTENING || vs==VOICE_THINKING || speaking;
        if(strcmp(s_last_caption,caption)) { snprintf(s_last_caption,sizeof(s_last_caption),"%s",caption);s_voice_until=now+12000; }
        if(busy && caption[0])s_voice_until=now+12000;
        bool expanded=local_reaction || busy || (caption[0] && (int32_t)(s_voice_until-now)>0);
        if(expanded)lv_obj_remove_flag(lv_obj_get_parent(s_caption_label),LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(lv_obj_get_parent(s_caption_label),LV_OBJ_FLAG_HIDDEN);
        if(vs==VOICE_LISTENING) {
            int level=audio_mic_level();
            lv_label_set_text(s_caption_label,level>300?"I'm listening...\nI can hear your voice!":"I'm listening...\nSpeak close to me");
            lv_label_set_text(s_voice_status,"Let go when you're done");
        } else if(vs==VOICE_THINKING) {
            lv_label_set_text(s_caption_label,local_reaction?s_reaction_text:"One little moment...");
            lv_label_set_text(s_voice_status,"Thinking...");
        } else if(local_reaction && !speaking) {
            lv_label_set_text(s_caption_label,s_reaction_text);
            lv_label_set_text(s_voice_status,s_view==HOME?"Hold here or BOOT to talk":"Hold BOOT to talk");
        } else if(caption[0] && (int32_t)(s_voice_until-now)>0) {
            if(strcmp(lv_label_get_text(s_caption_label),caption))lv_label_set_text(s_caption_label,caption);
            lv_label_set_text(s_voice_status,speaking?"Chatting with you":s_view==HOME?"Hold here or BOOT to reply":"Hold BOOT to reply");
        } else {
            char greeting[80];snprintf(greeting,sizeof(greeting),s_view==HOME?"Hold to talk to %s":"Hold BOOT to talk to %s",pet_state_get()->name);
            if(strcmp(lv_label_get_text(s_voice_status),greeting))lv_label_set_text(s_voice_status,greeting);
        }
    }
    if(s_view==MUSIC) {
        if(vs==VOICE_LISTENING)lv_label_set_text(s_hint,"I'm listening... Let go to reply");
        else if(vs==VOICE_THINKING)lv_label_set_text(s_hint,s_music_choice==3?"Making a tune... Stop cancels":"One little moment... Stop cancels");
        else if(audio_voice_playing())lv_label_set_text(s_hint,s_music_choice==3?"A little song, just for you!":"Chatting with you");
        else if(s_music_choice==3 && (vs==VOICE_ERROR || vs==VOICE_OFFLINE))lv_label_set_text(s_hint,"Couldn't make a tune. Try the ones below!");
        else if(s_music_choice==3 && vs==VOICE_READY)lv_label_set_text(s_hint,"Pick another tune whenever you like");
    }
    if(s_connection && now-s_connection_tick>=500) { char text[256];voice_status(text,sizeof(text));lv_label_set_text(s_connection,text);s_connection_tick=now; }
    if(now-s_last_tick>=10000) { pet_state_tick((uint32_t)time(NULL)); s_last_tick=now; refresh(); }
    if(s_view==HOME)room_frame(now,vs);
    if(s_view==BALL && s_target && !s_ball_pressed && (int32_t)(s_ball_ready-now)<=0) {
        s_ball_time+=40;float phase=s_ball_time/4000.0f*6.2831853f;
        lv_obj_set_pos(s_target,136+(int)(sinf(phase)*108),290-(int)(fabsf(sinf(phase))*144));
    }
    if(s_view==HIDE && s_reveal_until && (int32_t)(now-s_reveal_until)>=0) {
        if(s_hits==3)finish(1);else hide_place();
    }
    if(s_pet) {
        bool hopping=(int32_t)(s_hop_until-now)>0;
        int dy=hopping ? -(int)(fabsf(sinf(now/90.0f))*13) : (int)(sinf(now/(s_view == SLEEP ? 1100.0f : 650.0f))*3);
        if(s_view==SLEEP || s_view==BATH || s_view==HIDE)dy=0;
        else if(s_eating)dy=(elapsed/160)%2?-3:0;
        lv_obj_set_y(s_pet,s_pet_y+dy);
        bool asleep=s_view==SLEEP;
        bool delighted=s_view==PARTY || (s_view==HIDE && s_reveal_until) || (hopping && !s_eating && !asleep && s_view!=BATH);
        s_face=asleep?PIXEL_SLEEP:s_eating?PIXEL_EAT:s_view==BATH?PIXEL_BATH:
               audio_voice_playing()?PIXEL_TALK:vs==VOICE_LISTENING?PIXEL_LISTEN:
               vs==VOICE_THINKING?PIXEL_THINK:delighted?PIXEL_HAPPY:(s_view==CATCH || s_view==BALL)?PIXEL_PLAY:
               now%4200>4010?PIXEL_BLINK:PIXEL_IDLE;
        pixel_pet_render(&s_pet_art,pet_state_get(),s_face,s_eating?elapsed/160:now/180,s_food);
        lv_obj_invalidate(s_pet_image);
        if(hopping && !s_eating && !asleep && s_view!=HIDE) lv_obj_remove_flag(s_pet_heart,LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(s_pet_heart,LV_OBJ_FLAG_HIDDEN);
    }
    if(s_view==FOOD && s_eating && elapsed>=1200) finish(0);
    if(s_view==SLEEP) {
        lv_bar_set_value(s_sleep_bar,(int)(elapsed*100/6000),LV_ANIM_OFF);
        if(elapsed>=6000) finish(2);
    }
}
void ui_init(void)
{
    if(!renderer_lock(0)) return;
    lv_obj_set_style_bg_color(lv_screen_active(),lv_color_hex(CREAM),0);
    lv_obj_remove_flag(lv_screen_active(),LV_OBJ_FLAG_SCROLLABLE);
    show(HOME); lv_timer_create(frame,40,NULL);
    renderer_unlock();
}
void ui_refresh_stats(void) { if(renderer_lock(0)) { refresh(); renderer_unlock(); } }
void ui_show_home(void) { if(renderer_lock(0)) { show(HOME); renderer_unlock(); } }
void ui_show_emote(emote_id_t emote) { (void)emote; }
