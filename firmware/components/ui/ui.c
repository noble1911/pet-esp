// Little Meadow: illustrated pixel artwork and activities, all owned by the LVGL task.
#include <math.h>
#include <stdio.h>
#include <time.h>
#include "ui.h"
#include "renderer.h"
#include "power.h"
#include "audio.h"
#include "voice.h"
#include "multiplayer.h"
#include "arcade.h"
#include "motion.h"
#include <string.h>
#include "esp_random.h"
#include "esp_system.h"
#include "lvgl.h"
#include "pixel_pet.h"
#include "pixel_rooms.h"

#define INK 0x3d203d
#define CREAM 0xfff9ed
#define MINT 0xdaf3e5
#define GOLD 0xffcf57
#define PINK 0xf3a8b6
#define BLUE 0xa9dbef
#define FOOD_BITE_MS 650
#define FOOD_SETTLE_MS 2100
#define FOOD_DURATION_MS 2800
#define FOOD_FADE_MS 200
#define BALL_FLIGHT_MS 720
#define BALL_LAND_MS 160
#define BALL_WIN_MS 650

typedef enum { HOME, FOOD, CATCH, BATH, SLEEP, PARTY, ALBUM, SETTINGS, CONNECTION, NAME, GAMES, HIDE, BALL, DECORATIONS, MUSIC, RESET, PROFILE, TRAIT, TREATS, REWARDS, POWER_OFF, PLAY_MENU, MULTIPLAYER, VOICES, ARCADE } View;
static View s_view;
static unsigned s_voice_choice;
static bool s_preview_playing;
static uint32_t s_preview_started;
static lv_obj_t *s_root, *s_pet, *s_bars[4], *s_target, *s_counter;
static lv_obj_t *s_dots[5], *s_hint, *s_sleep_bar;
static lv_obj_t *s_volume_label, *s_volume_slider;
static lv_obj_t *s_pet_image, *s_pet_heart, *s_mic;
static lv_obj_t *s_voice_status, *s_caption_label, *s_connection, *s_name_input, *s_name_error;
static bool s_talking, s_boot_down, s_boot_raw;
static uint32_t s_talk_started, s_boot_changed, s_connection_tick, s_voice_until;
static char s_last_caption[512];
static uint32_t s_next_remark, s_last_manual_talk, s_next_reaction, s_reaction_until;
static char s_reaction_text[96];
static pixel_pet_art_t s_pet_art;
static pixel_face_t s_face;
static pixel_food_t s_food;
static uint32_t s_started, s_hop_until, s_last_tick, s_party_fade_until;
static unsigned s_hits, s_last_target;
static bool s_eating, s_muted;
static bool s_power_pending, s_power_sent;
static uint32_t s_power_poll, s_power_at, s_power_ready_at;
static unsigned s_trait_gene, s_trait_variant;
static unsigned s_album_page, s_hiding, s_weather_mode;
static lv_obj_t *s_keepsakes[5], *s_keepsake_label;
static uint32_t s_keepsake_started, s_keepsake_until;
static unsigned s_keepsake_choice;
static void play_keepsake(unsigned sticker);
static uint32_t s_reveal_until, s_ball_ready, s_ball_time, s_next_butterfly, s_butterfly_until;
static bool s_ball_pressed, s_ball_flying, s_ball_mirror;
static int s_ball_from_x, s_ball_from_y, s_ball_to_x, s_ball_to_y;
static uint32_t s_ball_finish_at;
static lv_obj_t *s_ball_image, *s_ball_shadow, *s_ball_ring, *s_ball_trail[3];
enum { MUSIC_COMPOSE=-2 };
static int s_music_choice=-1;
static lv_obj_t *s_music_list;
static uint32_t s_music_started;
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
static void reset_cb(lv_event_t *e)
{
    (void)e;
    if(s_view!=RESET) return;
    if(!pet_state_reset()) {
        lv_label_set_text(s_hint,"Could not save. Please try again.");
        return;
    }
    // Reboot clears queued speech, captions and all activity timers. The
    // new identity selects fresh server memory on its first online chat.
    voice_cancel();audio_stop_tune();
    esp_restart();
}
// Paint the body and tail as one component. Text never outlives its background,
// and the entire panel is invalidated when a caption changes on partial displays.
static void bubble_draw(lv_event_t *e)
{
    lv_obj_t *obj=lv_event_get_target(e);lv_area_t box;lv_obj_get_coords(obj,&box);
    lv_layer_t *layer=lv_event_get_layer(e);lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);
    d.bg_color=lv_color_hex(CREAM);d.bg_opa=LV_OPA_COVER;d.radius=12;
    d.border_color=lv_color_hex(INK);d.border_width=2;d.border_opa=LV_OPA_COVER;
    lv_area_t body={box.x1+2,box.y1+2,box.x2-2,box.y1+79};lv_draw_rect(layer,&d,&body);
    d.radius=0;d.border_width=0;
    const int tail[][5]={{145,78,18,5,0},{149,83,14,5,0},{155,88,8,4,0},
                         {147,77,14,5,1},{151,82,10,5,1},{157,87,4,3,1}};
    for(unsigned i=0;i<6;i++) {
        d.bg_color=lv_color_hex(tail[i][4]?CREAM:INK);
        lv_area_t r={box.x1+tail[i][0],box.y1+tail[i][1],box.x1+tail[i][0]+tail[i][2]-1,box.y1+tail[i][1]+tail[i][3]-1};
        lv_draw_rect(layer,&d,&r);
    }
}
static lv_obj_t *caption_bubble(int y)
{
    lv_obj_t *bubble=shape(s_root,24,y,320,94,CREAM,0);
    lv_obj_set_style_bg_opa(bubble,LV_OPA_TRANSP,0);
    lv_obj_add_event_cb(bubble,bubble_draw,LV_EVENT_DRAW_MAIN,NULL);
    lv_obj_t *caption=label(bubble,"",14,13,292,false);
    lv_obj_set_height(caption,56);lv_label_set_long_mode(caption,LV_LABEL_LONG_DOT);
    lv_obj_remove_flag(caption,LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    return caption;
}
static void caption_text(lv_obj_t *caption,const char *text)
{
    if(strcmp(lv_label_get_text(caption),text)) {
        lv_label_set_text(caption,text);lv_obj_invalidate(lv_obj_get_parent(caption));
    }
}
static void speech_bubble(int y)
{
    s_caption_label=caption_bubble(y);
    lv_obj_add_flag(lv_obj_get_parent(s_caption_label),LV_OBJ_FLAG_HIDDEN);
}
static void microphone(lv_obj_t *parent)
{
    lv_obj_t *capsule=shape(parent,21,6,14,23,INK,0);lv_obj_set_style_radius(capsule,7,0);
    for(int y=11;y<23;y+=5)shape(parent,24,y,8,2,CREAM,0);
    static const lv_point_precise_t points[]={{17,20},{17,25},{20,30},{28,33},{36,30},{39,25},{39,20}};
    lv_obj_t *cup=lv_line_create(parent);lv_line_set_points(cup,points,7);
    lv_obj_set_style_line_color(cup,lv_color_hex(INK),0);lv_obj_set_style_line_width(cup,3,0);
    lv_obj_set_style_line_rounded(cup,true,0);lv_obj_remove_flag(cup,LV_OBJ_FLAG_CLICKABLE);
    shape(parent,26,32,4,7,INK,0);shape(parent,19,39,18,3,INK,0);
}
static void header_to(const char *title,View back)
{
    // The whole top-left corner is a forgiving target; the inset tile is the
    // visual affordance. Ends above the first Options row, without overlap.
    lv_obj_t *hit=button(s_root,0,0,88,76,CREAM,nav_cb,back);
    lv_obj_set_style_bg_opa(hit,LV_OPA_TRANSP,0);lv_obj_set_style_border_width(hit,0,0);
    lv_obj_t *tile=shape(hit,12,10,64,56,0xffffff,0);
    lv_obj_set_style_radius(tile,10,0);lv_obj_set_style_border_width(tile,2,0);
    lv_obj_set_style_border_color(tile,lv_color_hex(INK),0);
    label(tile,LV_SYMBOL_LEFT,0,15,64,true);
    label(s_root,title,94,32,250,true);
}
static void header(const char *title) {header_to(title,HOME);}
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
    if(action==0 && !pet_state_eat((unsigned)s_food)) {
        s_eating=false;s_hop_until=0;lv_obj_set_style_opa(s_root,LV_OPA_COVER,0);
        lv_label_set_text(s_hint,"Could not save. Please try again.");return;
    }
    if(action==1) pet_state_play();
    if(action==2) pet_state_rest();
    if(action==3) pet_state_clean();
    unsigned earned=pet_state_get()->evolution_progress;bool gift=false;
    for(unsigned i=0;i<PET_DECORATION_COUNT;i++)if(earned==pet_decoration_threshold(i))gift=true;
    audio_play(gift || pet_food_milestone(earned)>=0?SFX_GIFT:earned<=90 && earned%5==0?SFX_STICKER:action==3?SFX_BATH:SFX_HAPPY);
    show(PARTY);
    if(completed==FOOD) {
        s_party_fade_until=lv_tick_get()+FOOD_FADE_MS;
        lv_obj_set_style_opa(s_root,LV_OPA_TRANSP,0);
    }
    if(action==0) {
        static const pet_event_t events[]={PET_EVENT_APPLE,PET_EVENT_TOAST,PET_EVENT_COOKIE,
            PET_EVENT_CUPCAKE,PET_EVENT_PANCAKES,PET_EVENT_JELLY,PET_EVENT_CAKE};
        reaction(events[s_food],s_food<3?(const char*[]){"Crunch! Juicy apple!","Mmm, warm buttery toast!","Cookie crumbs on my cheeks!"}[s_food]:pet_special_food(s_food-3)->reaction,true);
    }
    if(action==1) {
        if(completed==HIDE)reaction(PET_EVENT_HIDE,"Peekaboo champion! You found me!",true);
        else if(completed==BALL)reaction(PET_EVENT_BALL,"Boing! Five happy bounces!",true);
        else reaction(PET_EVENT_PLAY,"Five twinkly treasures!",true);
    }
    if(action==2)reaction(PET_EVENT_NAP,"Stretch! What a lovely nap!",true);
    if(action==3)reaction(PET_EVENT_BATH,"Sparkly clean, from toes to leaves!",true);
}
static void start_food(unsigned food,lv_obj_t *button_obj)
{
    if(s_eating || !pet_food_unlocked(pet_state_get(),food)) return;
    s_food=(pixel_food_t)food;
    s_eating=true;s_started=lv_tick_get();s_hop_until=s_started+FOOD_SETTLE_MS;
    lv_label_set_text(s_hint,food<3?"Yum yum!":pet_special_food(food-3)->name);
    if(button_obj)lv_obj_set_style_bg_color(button_obj,lv_color_hex(GOLD),0);
    static const sfx_id_t sounds[]={SFX_APPLE,SFX_TOAST,SFX_COOKIE,SFX_CUPCAKE,SFX_PANCAKES,SFX_JELLY,SFX_CAKE};
    audio_play(sounds[food]);
}
static void food_cb(lv_event_t *e)
{
    start_food((unsigned)(uintptr_t)lv_event_get_user_data(e),lv_event_get_target(e));
}
static void treats_cb(lv_event_t *e)
{
    (void)e;if(!s_eating)show(TREATS);
}
static void treat_cb(lv_event_t *e)
{
    unsigned i=(unsigned)(uintptr_t)lv_event_get_user_data(e);
    if(!pet_food_unlocked(pet_state_get(),i+3)) {
        unsigned remaining=pet_special_food(i)->stars-pet_state_get()->evolution_progress;
        char text[80];snprintf(text,sizeof text,"%u more %s - care, play and grow!",remaining,remaining==1?"star":"stars");
        lv_label_set_text(s_hint,text);return;
    }
    show(FOOD);start_food(i+3,NULL);
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
    return (const char*[]){"home","snack time","catch stars","bubble bath","nap","celebrating","stickers","options","connection check","naming","choosing a game","peekaboo","bouncy ball","room gifts","making music","grown-ups","my pet","my traits","special treats","choosing stickers or gifts","going to sleep","choosing games, music or multiplayer","playing a cooperative playdate","choosing a voice","playing an arcade game"}[s_view];
}
static void talk_begin(void)
{
    if(s_talking || s_view==MULTIPLAYER || s_view==ARCADE) return;
    if(s_view==VOICES) {voice_cancel();s_preview_playing=false;}
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
static void voice_stop_cb(lv_event_t *e)
{
    (void)e;voice_cancel();s_preview_playing=false;
    lv_label_set_text(s_hint,"Preview stopped. Nothing changed.");
}
static void voice_step_cb(lv_event_t *e)
{
    voice_cancel();s_preview_playing=false;
    int direction=(int)(intptr_t)lv_event_get_user_data(e);
    s_voice_choice=(s_voice_choice+PET_VOICE_COUNT+direction)%PET_VOICE_COUNT;
    show(VOICES);
}
static void voice_preview_cb(lv_event_t *e)
{
    (void)e;
    if(s_muted || audio_get_volume()==0) {lv_label_set_text(s_hint,"Turn sound on in Options first.");return;}
    s_talking=false;
    s_preview_playing=voice_preview(s_voice_choice);
    s_preview_started=lv_tick_get();
    lv_label_set_text(s_hint,s_preview_playing?"Getting the voice ready...":"Voice offline. Check Wi-Fi in Options.");
}
static void voice_save_cb(lv_event_t *e)
{
    (void)e;voice_cancel();s_preview_playing=false;
    if(pet_state_set_voice(s_voice_choice))show(SETTINGS);
    else lv_label_set_text(s_hint,"Could not save. Please try again.");
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
    "Butterfly","Flower","Bunny","Rainbow","Kite","Water can","Cloud","Sun","Music","Crown","Book","Present","Best buddies"};
static const char *decor_names[PET_DECORATION_COUNT]={"Flowers","Bunting","Teddy","Moon lamp","Cushion","Trophy"};
static void page_cb(lv_event_t *e)
{
    s_album_page=(s_album_page+4+(int)(intptr_t)lv_event_get_user_data(e))%4;show(ALBUM);
}
static void sticker_cb(lv_event_t *e)
{
    int i=(int)(intptr_t)lv_event_get_user_data(e);char text[80];
    if(i>=0 && !pet_sticker_unlocked(pet_state_get(),(unsigned)i)) {
        if(i==PET_CARE_STICKER_COUNT) {lv_label_set_text(s_hint,"Finish a playdate to earn Best buddies!");audio_play(SFX_SELECT);return;}
        snprintf(text,sizeof text,"%s: %lu more stars",sticker_names[i],
                 (unsigned long)((i+1)*5-pet_state_get()->evolution_progress));
        lv_label_set_text(s_hint,text);audio_play(SFX_SELECT);return;
    }
    if(!pet_set_wall_sticker(i)) {lv_label_set_text(s_hint,"Couldn't save. Please try again.");return;}
    show(HOME);if(i>=0)play_keepsake((unsigned)i);
}
static void decorate_cb(lv_event_t *e)
{
    int d=(int)(intptr_t)lv_event_get_user_data(e);char text[80];
    if(d>=0 && !pet_decoration_unlocked(pet_state_get(),(unsigned)d)) {
        snprintf(text,sizeof text,"%s: %lu more stars",decor_names[d],
                 (unsigned long)(pet_decoration_threshold((unsigned)d)-pet_state_get()->evolution_progress));
        lv_label_set_text(s_hint,text);return;
    }
    bool ok=d<0?pet_equip_decoration(-1):pet_toggle_decoration((unsigned)d);
    if(!ok) {lv_label_set_text(s_hint,"Couldn't save. Please try again.");return;}
    audio_play(SFX_GIFT);show(DECORATIONS);
}
static void choose_character_cb(lv_event_t *e)
{
    (void)e;
    if(!pet_state_set_character(s_trait_variant)) {
        lv_label_set_text(s_hint,"Couldn't save. Please try again.");return;
    }
    voice_cancel();audio_play(SFX_GIFT);show(PROFILE);
}
static void trait_open_cb(lv_event_t *e)
{
    s_trait_gene=(unsigned)(uintptr_t)lv_event_get_user_data(e);
    s_trait_variant=s_trait_gene==0?pet_character_id(pet_state_get()):pet_trait_choice(pet_state_get(),s_trait_gene);show(TRAIT);
}
static void trait_browse_cb(lv_event_t *e)
{
    unsigned count=s_trait_gene==0?PET_CHARACTER_COUNT:pet_trait(s_trait_gene)->count;
    int delta=(int)(intptr_t)lv_event_get_user_data(e);
    s_trait_variant=(s_trait_variant+count+delta)%count;show(TRAIT);
}
static void trait_portrait(int x,int y,const Pet *p)
{
    pixel_pet_render(&s_pet_art,p,PIXEL_IDLE,12,PIXEL_APPLE);
    art(s_root,&s_pet_art.image,x,y,512);
}
static void make_treats(void)
{
    header_to("Special treats",FOOD);
    s_hint=label(s_root,"Earn once, enjoy again and again!",16,83,336,false);
    for(unsigned i=0;i<PET_SPECIAL_FOOD_COUNT;i++) {
        const pet_special_food_t *f=pet_special_food(i);bool unlocked=pet_food_unlocked(pet_state_get(),i+3);
        lv_obj_t *b=button(s_root,24,116+i*80,320,72,unlocked?0xffe9d3:0xe9ede6,treat_cb,(int)i);
        art(b,pixel_special_food(i),8,10,512);
        label(b,f->name,68,10,242,false);
        char text[72];
        if(unlocked)snprintf(text,sizeof text,f->stars==pet_state_get()->evolution_progress?"New! Tap to enjoy":"Yours! Tap to enjoy");
        else snprintf(text,sizeof text,"%u stars  |  %lu more to go",f->stars,(unsigned long)(f->stars-pet_state_get()->evolution_progress));
        label(b,text,62,39,252,false);
    }
}
static void make_profile(void)
{
    const Pet *p=pet_state_get();char text[96];
    header_to("My pet",SETTINGS);
    trait_portrait(18,79,p);
    label(s_root,p->name,172,100,174,true);
    const char *stages[]={"Egg","Baby","Child","Teen","Grown-up","Elder"};
    snprintf(text,sizeof text,"%s  |  %lu stars",stages[p->stage<=PET_STAGE_ELDER?p->stage:0],(unsigned long)p->evolution_progress);
    label(s_root,text,166,148,182,false);
    const pet_trait_t *personality=pet_trait(GENE_PERSONALITY);
    snprintf(text,sizeof text,"%s little friend",personality->values[pet_trait_choice(p,GENE_PERSONALITY)]);
    label(s_root,text,166,180,182,false);
    label(s_root,"Your little friend",24,221,320,false);
    lv_obj_t *card=button(s_root,24,247,320,64,MINT,trait_open_cb,0);
    label(card,"Choose character",0,8,320,false);
    label(card,pet_character(pet_character_id(p))->name,0,34,320,false);
    card=button(s_root,24,327,320,64,BLUE,trait_open_cb,GENE_PERSONALITY);
    label(card,"Personality",0,8,320,false);
    label(card,personality->values[pet_trait_choice(p,GENE_PERSONALITY)],0,34,320,false);
    label(s_root,"Seven little friends to choose from",16,412,336,false);
}
static void make_trait(void)
{
    const Pet *p=pet_state_get();bool character=s_trait_gene==0;char text[80];
    const pet_trait_t *t=pet_trait(GENE_PERSONALITY);
    const pet_character_t *c=pet_character(character?s_trait_variant:pet_character_id(p));
    unsigned count=character?PET_CHARACTER_COUNT:t->count;
    header_to(character?"Character":"Personality",PROFILE);
    label(s_root,character?c->name:t->values[s_trait_variant],24,86,320,true);
    bool mine=s_trait_variant==(character?pet_character_id(p):pet_trait_choice(p,GENE_PERSONALITY));
    label(s_root,mine?"This one is mine!":"Just looking - your pet stays the same",16,116,336,false);
    Pet preview=*p;
    if(character)preview.genes[GENE_PATTERN]=(uint8_t)(PET_CHARACTER_MARKER+s_trait_variant);
    trait_portrait(112,138,&preview);
    label(s_root,character?c->description:t->descriptions[s_trait_variant],24,285,320,false);
    s_hint=label(s_root,character?"Your name and stars stay the same.":"My personality helps shape my chats",16,327,336,false);
    if(character) {
        lv_obj_t *choose=button(s_root,66,350,236,38,MINT,choose_character_cb,0);
        label(choose,mine?"My character":"Choose this character",0,10,236,false);
        if(mine)lv_obj_add_state(choose,LV_STATE_DISABLED);
    }
    lv_obj_t *prev=button(s_root,24,397,62,44,BLUE,trait_browse_cb,-1);label(prev,LV_SYMBOL_LEFT,0,10,62,true);
    snprintf(text,sizeof text,"%u / %u",s_trait_variant+1,count);label(s_root,text,94,411,180,false);
    lv_obj_t *next=button(s_root,282,397,62,44,BLUE,trait_browse_cb,1);label(next,LV_SYMBOL_RIGHT,0,10,62,true);
}
static void make_album(void)
{
    header_to("My stickers",REWARDS);const Pet *p=pet_state_get();unsigned count=pet_sticker_count(p);char text[80];
    snprintf(text,sizeof text,"%u / 19 stickers  |  %lu stars",count,(unsigned long)p->evolution_progress);
    label(s_root,text,24,85,320,false);
    if(count==PET_STICKER_COUNT)snprintf(text,sizeof text,"Your collection is complete!");
    else if(p->evolution_progress>=90)snprintf(text,sizeof text,"Play together for your buddy sticker!");
    else {unsigned more=5-p->evolution_progress%5;snprintf(text,sizeof text,"%u more %s to your next sticker",more,more==1?"star":"stars");}
    s_hint=label(s_root,text,24,108,320,false);
    for(unsigned j=0;j<6;j++) {
        unsigned i=s_album_page*6+j;if(i>=PET_STICKER_COUNT)break;bool earned=pet_sticker_unlocked(p,i);
        lv_obj_t *b=button(s_root,29+(j%3)*108,141+(j/3)*100,94,91,earned?0xffecd1:0xe9ede6,sticker_cb,(int)i);
        if(earned)art(b,pixel_collectible(i),23,3,512);
        else {shape(b,36,9,22,22,0xb4beb5,0);shape(b,41,14,12,17,0xe9ede6,0);shape(b,31,24,32,22,0xb4beb5,0);}
        label(b,i==18?"Buddies":sticker_names[i],1,52,92,false);
        char threshold[24];
        if(pet_wall_sticker(p)==(int)i)snprintf(threshold,sizeof threshold,"On wall");
        else if(earned)snprintf(threshold,sizeof threshold,"Tap to play");
        else if(i==PET_CARE_STICKER_COUNT)snprintf(threshold,sizeof threshold,"Playdate");
        else snprintf(threshold,sizeof threshold,"%u stars",(i+1)*5);
        label(b,threshold,0,71,94,false);
    }
    lv_obj_t *prev=button(s_root,29,344,64,44,MINT,page_cb,-1);label(prev,LV_SYMBOL_LEFT,0,9,64,true);
    snprintf(text,sizeof text,"Page %u of 4",s_album_page+1);label(s_root,text,105,357,158,false);
    lv_obj_t *next=button(s_root,275,344,64,44,MINT,page_cb,1);label(next,LV_SYMBOL_RIGHT,0,9,64,true);
    if(pet_wall_sticker(p)>=0) {
        lv_obj_t *clear=button(s_root,29,399,310,40,BLUE,sticker_cb,-1);label(clear,"Take sticker off wall",0,11,310,false);
    } else label(s_root,"Pick one for your wall. Tap it to play!",16,410,336,false);
}
static void make_decorations(void)
{
    header_to("My room gifts",REWARDS);const Pet *p=pet_state_get();unsigned placed=pet_room_gifts(p);
    s_hint=label(s_root,"Tap to add or remove. Pick several!",24,88,320,false);
    for(unsigned i=0;i<PET_DECORATION_COUNT;i++) {
        bool unlocked=pet_decoration_unlocked(p,i), selected=(placed&(1u<<i))!=0;
        lv_obj_t *b=button(s_root,29+(i%3)*108,123+(i/3)*112,94,103,selected?MINT:unlocked?0xffecd1:0xe9ede6,decorate_cb,(int)i);
        art(b,pixel_decoration(i),27,5,256);
        if(!unlocked)lv_obj_set_style_opa(b,LV_OPA_60,0);
        label(b,decor_names[i],0,51,94,false);
        char text[24];snprintf(text,sizeof text,selected?"Remove " LV_SYMBOL_OK:unlocked?"Add gift":"%u stars",pet_decoration_threshold(i));
        label(b,text,0,77,94,false);
    }
    lv_obj_t *plain=button(s_root,29,350,145,44,CREAM,decorate_cb,-1);label(plain,"Put all away",0,14,145,false);
    lv_obj_t *album=button(s_root,194,350,145,44,MINT,nav_cb,HOME);label(album,"See my room",0,14,145,false);
    label(s_root,"Tap your gifts in the room to play!",24,413,320,false);
}
static void make_rewards(void)
{
    header("My treasures");label(s_root,"What shall we play with?",24,91,320,false);
    lv_obj_t *stickers=button(s_root,24,137,320,112,PINK,nav_cb,ALBUM);
    art(stickers,pixel_collectible(5),15,28,640);
    label(stickers,"Stickers",88,24,218,true);
    label(stickers,"Pick one for your wall",82,60,228,false);
    lv_obj_t *gifts=button(s_root,24,270,320,112,MINT,nav_cb,DECORATIONS);
    art(gifts,pixel_collectible(17),15,28,640);
    label(gifts,"Gifts",88,24,218,true);
    label(gifts,"Make your room yours",82,60,228,false);
    label(s_root,"Caring earns stars and little surprises",16,410,336,false);
}
// Every sticker has a little pretend action. These never change care stats or
// grant stars. One bounded effect is reused, so repeated taps cannot stack work.
typedef enum { FLOAT_UP, BOUNCE, ORBIT, FLUTTER, SHOWER, SWAY } keepsake_motion_t;
static const struct {pixel_face_t pose; sfx_id_t sound; keepsake_motion_t motion; const char *text;} keepsake_actions[PET_STICKER_COUNT]={
    {PIXEL_EAT,SFX_APPLE,SWAY,"Crunchy apple wiggles!"},
    {PIXEL_PLAY,SFX_BOUNCE,BOUNCE,"Boing, boing, boing!"},
    {PIXEL_SLEEP,SFX_SLEEP,FLOAT_UP,"A tiny moonbeam dream..."},
    {PIXEL_BATH,SFX_BUBBLE,FLOAT_UP,"Bubbly little toes!"},
    {PIXEL_HAPPY,SFX_STAR,SHOWER,"Twinkle shower!"},
    {PIXEL_HAPPY,SFX_CUDDLE,FLOAT_UP,"A pocketful of cuddles!"},
    {PIXEL_PLAY,SFX_BUTTERFLY,FLUTTER,"Flutter with me!"},
    {PIXEL_HAPPY,SFX_BUTTERFLY,SWAY,"My flowers do a little dance!"},
    {PIXEL_PLAY,SFX_BOUNCE,BOUNCE,"Bunny hops! Hop, hop!"},
    {PIXEL_PLAY,SFX_HAPPY,ORBIT,"Rainbow wiggle dance!"},
    {PIXEL_PLAY,SFX_BUTTERFLY,FLUTTER,"Whoosh! My kite can fly!"},
    {PIXEL_HAPPY,SFX_BUBBLE,SHOWER,"Sprinkle, sprinkle, little leaves!"},
    {PIXEL_SLEEP,SFX_SLEEP,SWAY,"Soft cloud daydreams..."},
    {PIXEL_HAPPY,SFX_HAPPY,ORBIT,"Warm sunshine on my cheeks!"},
    {PIXEL_PLAY,SFX_EMOTE,BOUNCE,"A little dum-de-dum dance!"},
    {PIXEL_HAPPY,SFX_GIFT,SHOWER,"Royal wiggles! Ta-da!"},
    {PIXEL_SLEEP,SFX_SLEEP,SWAY,"Once upon a tiny leaf..."},
    {PIXEL_HAPPY,SFX_GIFT,SHOWER,"Surprise! A happy little wiggle!"},
    {PIXEL_HAPPY,SFX_MEET,ORBIT,"Best buddies! A little dance together!"}
};
static void play_keepsake(unsigned sticker)
{
    if(s_view!=HOME || sticker>=PET_STICKER_COUNT)return;
    s_keepsake_choice=sticker;s_keepsake_started=lv_tick_get();s_keepsake_until=s_keepsake_started+4000;
    for(unsigned i=0;i<5;i++) {
        lv_image_set_src(s_keepsakes[i],pixel_collectible(sticker));
        lv_obj_remove_flag(s_keepsakes[i],LV_OBJ_FLAG_HIDDEN);
    }
    lv_label_set_text(s_keepsake_label,keepsake_actions[sticker].text);
    lv_obj_remove_flag(s_keepsake_label,LV_OBJ_FLAG_HIDDEN);
    audio_play(keepsake_actions[sticker].sound);
}
static void room_gift_cb(lv_event_t *e)
{
    unsigned gift=(unsigned)(uintptr_t)lv_event_get_user_data(e);
    static const unsigned actions[PET_DECORATION_COUNT]={7,9,5,2,1,15};
    if(gift<PET_DECORATION_COUNT)play_keepsake(actions[gift]);
}
static void wall_sticker_cb(lv_event_t *e)
{
    (void)e;int sticker=pet_wall_sticker(pet_state_get());if(sticker>=0)play_keepsake((unsigned)sticker);
}
static void keepsake_frame(uint32_t now,voice_state_t vs)
{
    if(!s_keepsake_until)return;
    if((int32_t)(now-s_keepsake_until)>=0 || s_talking || vs==VOICE_LISTENING || vs==VOICE_THINKING || vs==VOICE_SPEAKING || audio_voice_playing()) {
        s_keepsake_until=0;
        for(unsigned i=0;i<5;i++)lv_obj_add_flag(s_keepsakes[i],LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_keepsake_label,LV_OBJ_FLAG_HIDDEN);return;
    }
    float t=(now-s_keepsake_started)/1000.0f;
    for(unsigned i=0;i<5;i++) {
        float phase=t*2.5f+i*1.256637f;int x=32+i*61,y=175;
        switch(keepsake_actions[s_keepsake_choice].motion) {
            case FLOAT_UP: y=276-(int)(t*42+i*28)%142;x+=(int)(sinf(phase)*10);break;
            case BOUNCE: y=267-(int)(fabsf(sinf(phase))*90);break;
            case ORBIT: x=164+(int)(cosf(phase)*114);y=214+(int)(sinf(phase)*61);break;
            case FLUTTER: x=20+(int)(t*43+i*62)%298;y=180+(int)(sinf(phase*1.5f)*34);break;
            case SHOWER: y=140+(int)(t*63+i*31)%137;x+=(int)(sinf(phase)*7);break;
            case SWAY: y=173+(int)(sinf(phase)*23);x+=(int)(cosf(phase)*10);break;
        }
        lv_obj_set_pos(s_keepsakes[i],x,y);
        lv_image_set_scale_x(s_keepsakes[i],keepsake_actions[s_keepsake_choice].motion==FLUTTER && (now/180+i)%2?256:384);
    }
}
static void friends_icon(lv_obj_t *parent,int x,int y)
{
    for(int i=0;i<2;i++) {
        int dx=x+i*27,dy=y+i*6;uint32_t color=i?PINK:GOLD;
        lv_obj_t *head=shape(parent,dx+5,dy,14,14,INK,0);
        lv_obj_set_style_radius(head,LV_RADIUS_CIRCLE,0);
        head=shape(parent,dx+7,dy+2,10,10,color,0);
        lv_obj_set_style_radius(head,LV_RADIUS_CIRCLE,0);
        shape(parent,dx+3,dy+17,18,17,INK,0);
        shape(parent,dx,dy+20,24,8,INK,0);
        shape(parent,dx+5,dy+19,14,12,color,0);
        shape(parent,dx+4,dy+31,6,9,INK,0);
        shape(parent,dx+14,dy+31,6,9,INK,0);
    }
}
#include "arcade_ui.inc"
#include "multiplayer_ui.inc"
static void make_play_menu(void)
{
    header("Let's play!");label(s_root,"What shall we do?",24,85,320,false);
    const char *names[]={"Games","Music","Multiplayer"};
    const char *hints[]={"Pick a little game","Tunes and little dances","Play with a friend"};
    for(unsigned i=0;i<3;i++) {
        lv_obj_t *b=button(s_root,24,111+i*106,320,94,(uint32_t[]){MINT,PINK,BLUE}[i],nav_cb,(int[]){GAMES,MUSIC,MULTIPLAYER}[i]);
        if(i==2)friends_icon(b,19,23);
        else art(b,i==0?pixel_icon(1):pixel_collectible(14),20,23,512);
        label(b,names[i],88,20,218,true);label(b,hints[i],82,55,228,false);
    }
}
static void make_games(void)
{
    header_to("Games",PLAY_MENU);label(s_root,"Little challenges, big smiles",24,80,320,false);
    const char *hints[]={"Follow the cups. Best streak!","Aim, bounce, catch a bonus","Roll gently and collect stars","Turn two cards. Find a pair"};
    for(unsigned i=0;i<ARCADE_COUNT;i++) {
        lv_obj_t *b=button(s_root,24,106+i*82,320,74,(uint32_t[]){PINK,0xffecd1,MINT,BLUE}[i],arcade_menu_cb,i);
        art(b,i==0?pixel_decoration(0):i==1?pixel_icon(1):i==2?pixel_icon(4):pixel_collectible(4),12,14,i==0?256:384);
        label(b,arcade_names[i],62,14,250,true);label(b,hints[i],59,45,254,false);
    }
}
static void music_cb(lv_event_t *e)
{
    int choice=(int)(intptr_t)lv_event_get_user_data(e);
    if(choice==-1) {audio_stop_tune();voice_cancel();s_music_choice=-1;lv_label_set_text(s_hint,"All quiet. Pick a little tune!");return;}
    if(s_muted || audio_get_volume()==0) {lv_label_set_text(s_hint,"Turn sound on in Options first");return;}
    if(choice==MUSIC_COMPOSE) {
        audio_stop_tune();
        if(voice_make_tune(pet_state_get(),activity())) {s_music_choice=MUSIC_COMPOSE;lv_label_set_text(s_hint,"Making a tune... Stop cancels");}
        else lv_label_set_text(s_hint,"Voice busy or offline. Pick a song!");
    } else if(voice_get_state()==VOICE_THINKING || voice_get_state()==VOICE_LISTENING) {
        lv_label_set_text(s_hint,"Making a tune... Stop cancels");
    } else if(audio_play_tune((unsigned)choice)) {
        s_music_choice=choice;s_music_started=lv_tick_get();lv_label_set_text_fmt(s_hint,"Playing: %s",audio_tune_name((unsigned)choice));
    } else lv_label_set_text(s_hint,"Let me finish talking first");
}
static void make_music(void)
{
    header_to("Little tunes",PLAY_MENU);s_music_choice=-1;
    s_hint=label(s_root,"Swipe up to find more songs",24,89,320,false);
    s_music_list=shape(s_root,24,120,320,204,CREAM,0);
    lv_obj_add_flag(s_music_list,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(s_music_list,LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_music_list,LV_SCROLLBAR_MODE_ON);
    lv_obj_set_style_bg_color(s_music_list,lv_color_hex(INK),LV_PART_SCROLLBAR);
    lv_obj_set_style_width(s_music_list,4,LV_PART_SCROLLBAR);
    for(unsigned i=0;i<audio_tune_count();i++) {
        lv_obj_t *b=button(s_music_list,2,2+(int)i*56,307,50,(uint32_t[]){0xffecd1,MINT,BLUE}[i%3],music_cb,(int)i);
        art(b,pixel_collectible(14),8,12,256);label(b,audio_tune_name(i),38,17,262,false);
    }
    char request[64];snprintf(request,sizeof request,"%s, make me a tune!",pet_state_get()->name);
    lv_obj_t *compose=button(s_root,29,339,310,48,PINK,music_cb,MUSIC_COMPOSE);label(compose,request,0,16,310,false);
    lv_obj_t *stop=button(s_root,89,397,190,44,CREAM,music_cb,-1);label(stop,"Stop music " LV_SYMBOL_STOP,0,14,190,false);
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
    room(false);header_to("Peekaboo",GAMES);
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
static void ball_pose_scale(unsigned x,unsigned y)
{
    lv_image_set_scale_x(s_ball_image,x);lv_image_set_scale_y(s_ball_image,y);
    // Keep the bottom of a squashed ball on the same baseline.
    lv_obj_set_y(s_ball_image,36+(768-(int)y)*12/256);
}
static void ball_press(lv_event_t *e)
{
    s_ball_pressed=lv_event_get_code(e)==LV_EVENT_PRESSED;
    if(!s_ball_flying)ball_pose_scale(s_ball_pressed?850:768,s_ball_pressed?620:768);
}
static void ball_cb(lv_event_t *e)
{
    (void)e;uint32_t now=lv_tick_get();
    if(s_view!=BALL || s_ball_flying || s_hits>=5 || (int32_t)(s_ball_ready-now)>0)return;
    static const int landings[5][2]={{246,258},{26,252},{246,276},{26,276},{136,246}};
    s_ball_from_x=lv_obj_get_x(s_target);s_ball_from_y=lv_obj_get_y(s_target);
    s_ball_to_x=s_ball_mirror?272-landings[s_hits][0]:landings[s_hits][0];
    s_ball_to_y=landings[s_hits][1];
    s_hits++;update_progress();audio_play(SFX_BOUNCE);
    s_ball_time=now;s_ball_flying=true;s_ball_ready=now+BALL_FLIGHT_MS+BALL_LAND_MS;
    s_hop_until=now+BALL_FLIGHT_MS;
    lv_obj_remove_flag(s_target,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_ball_ring,LV_OBJ_FLAG_HIDDEN);
    char text[32];snprintf(text,sizeof text,"%u / 5 bounces",s_hits);lv_label_set_text(s_counter,text);
    lv_label_set_text(s_hint,(const char*[]){"Boing! Over there!","Whoosh! Across the room!","Wheee! Look at it fly!","One more! You can do it!","Five! A big happy bounce!"}[s_hits-1]);
}
static void ball_arc(float t,int *x,int *y)
{
    if(t<0)t=0;
    if(t>1)t=1;
    float eased=t*t*(3.0f-2.0f*t);
    *x=s_ball_from_x+(int)((s_ball_to_x-s_ball_from_x)*eased);
    *y=s_ball_from_y+(int)((s_ball_to_y-s_ball_from_y)*t)-(int)(sinf(t*3.14159265f)*105);
}
static void ball_frame(uint32_t now)
{
    if(s_ball_finish_at && (int32_t)(now-s_ball_finish_at)>=0) {finish(1);return;}
    if(s_ball_flying) {
        uint32_t age=now-s_ball_time;
        if(age>=BALL_FLIGHT_MS) {
            s_ball_flying=false;lv_obj_set_pos(s_target,s_ball_to_x,s_ball_to_y);
            lv_image_set_rotation(s_ball_image,0);ball_pose_scale(880,590);
            for(unsigned i=0;i<3;i++)lv_obj_add_flag(s_ball_trail[i],LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(s_ball_ring,LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(s_ball_ring,s_ball_to_x+12,s_ball_to_y+58);
            lv_obj_set_size(s_ball_ring,72,22);lv_obj_set_style_opa(s_ball_ring,LV_OPA_COVER,0);
            lv_obj_set_pos(s_ball_shadow,s_ball_to_x+17,s_ball_to_y+77);
            lv_obj_set_width(s_ball_shadow,62);lv_obj_set_style_bg_opa(s_ball_shadow,LV_OPA_20,0);
            s_hop_until=now+450;audio_play(SFX_STAR);
            if(s_hits==5)s_ball_finish_at=now+BALL_WIN_MS;
        } else {
            float t=age<80?0:(age-80)/(float)(BALL_FLIGHT_MS-80);int x,y;ball_arc(t,&x,&y);
            lv_obj_set_pos(s_target,x,y);
            ball_pose_scale(age<80?880:720,age<80?590:820);
            lv_image_set_rotation(s_ball_image,(int)(t*3600));
            int ground=s_ball_from_y+(int)((s_ball_to_y-s_ball_from_y)*t);
            lv_obj_set_pos(s_ball_shadow,x+25,ground+78);lv_obj_set_width(s_ball_shadow,46);
            lv_obj_set_style_bg_opa(s_ball_shadow,LV_OPA_10,0);
            for(unsigned i=0;i<3;i++) {
                float behind=t-(i+1)*0.12f;
                if(behind<=0)lv_obj_add_flag(s_ball_trail[i],LV_OBJ_FLAG_HIDDEN);
                else {ball_arc(behind,&x,&y);lv_obj_set_pos(s_ball_trail[i],x+36,y+36);lv_obj_remove_flag(s_ball_trail[i],LV_OBJ_FLAG_HIDDEN);}
            }
        }
    } else {
        if(!s_ball_pressed)ball_pose_scale(768,768);
        if(!lv_obj_has_flag(s_ball_ring,LV_OBJ_FLAG_HIDDEN)) {
            uint32_t age=now-s_ball_time-BALL_FLIGHT_MS;
            if(age>=320)lv_obj_add_flag(s_ball_ring,LV_OBJ_FLAG_HIDDEN);
            else {int spread=(int)(age/12);lv_obj_set_pos(s_ball_ring,s_ball_to_x+12-spread,s_ball_to_y+58-spread/3);
                lv_obj_set_size(s_ball_ring,72+spread*2,22+spread*2/3);
                lv_obj_set_style_opa(s_ball_ring,(lv_opa_t)(255-age*255/320),0);}
        }
        if(s_hits<5 && (int32_t)(now-s_ball_ready)>=0) {
            lv_obj_add_flag(s_target,LV_OBJ_FLAG_CLICKABLE);
            if(s_hits)lv_label_set_text(s_hint,"It landed! Tap for another bounce.");
        }
    }
}
static void make_ball(void)
{
    room(false);header_to("Bouncy ball",GAMES);progress();
    for(unsigned i=0;i<5;i++) {
        lv_obj_set_style_radius(s_dots[i],LV_RADIUS_CIRCLE,0);
        lv_obj_set_style_border_width(s_dots[i],2,0);lv_obj_set_style_border_color(s_dots[i],lv_color_hex(INK),0);
    }
    make_pet(112,130);
    // Give the ball room to fly, with a smaller pet cheering behind it.
    lv_obj_set_size(s_pet,160,160);lv_image_set_scale(s_pet_image,512);
    lv_obj_set_pos(s_pet_heart,106,8);lv_obj_remove_flag(s_pet,LV_OBJ_FLAG_CLICKABLE);
    s_ball_mirror=(esp_random()&1)!=0;
    s_ball_shadow=shape(s_root,153,343,62,12,INK,0);
    lv_obj_set_style_radius(s_ball_shadow,LV_RADIUS_CIRCLE,0);lv_obj_set_style_bg_opa(s_ball_shadow,LV_OPA_20,0);
    s_ball_ring=shape(s_root,148,324,72,22,GOLD,0);
    lv_obj_set_style_radius(s_ball_ring,LV_RADIUS_CIRCLE,0);lv_obj_set_style_bg_opa(s_ball_ring,LV_OPA_TRANSP,0);
    lv_obj_set_style_border_width(s_ball_ring,3,0);lv_obj_set_style_border_color(s_ball_ring,lv_color_hex(GOLD),0);
    lv_obj_add_flag(s_ball_ring,LV_OBJ_FLAG_HIDDEN);
    for(unsigned i=0;i<3;i++) {
        s_ball_trail[i]=art(s_root,pixel_icon(4),0,0,256);
        lv_obj_set_style_opa(s_ball_trail[i],(lv_opa_t)(210-i*45),0);lv_obj_add_flag(s_ball_trail[i],LV_OBJ_FLAG_HIDDEN);
    }
    s_target=button(s_root,136,266,96,96,CREAM,ball_cb,0);
    lv_obj_set_style_bg_opa(s_target,LV_OPA_TRANSP,0);lv_obj_set_style_border_width(s_target,0,0);
    lv_obj_set_style_translate_y(s_target,0,LV_STATE_PRESSED);
    lv_obj_add_flag(s_target,LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    s_ball_image=art(s_target,pixel_icon(1),36,36,768);lv_image_set_pivot(s_ball_image,12,12);
    lv_obj_add_event_cb(s_target,ball_press,LV_EVENT_PRESSED,NULL);
    lv_obj_add_event_cb(s_target,ball_press,LV_EVENT_RELEASED,NULL);
    lv_obj_add_event_cb(s_target,ball_press,LV_EVENT_PRESS_LOST,NULL);
    s_counter=label(s_root,"0 / 5 bounces",24,375,320,true);
    s_hint=label(s_root,"Tap the ball. Make it fly!",16,410,336,false);
}
static void butterfly_cb(lv_event_t *e)
{
    (void)e;s_butterfly_until=0;lv_obj_add_flag(s_butterfly,LV_OBJ_FLAG_HIDDEN);
    s_next_butterfly=lv_tick_get()+60000+esp_random()%40001;s_hop_until=lv_tick_get()+700;
    reaction(PET_EVENT_BUTTERFLY,"Hello, little fluttery friend!",true);audio_play(SFX_BUTTERFLY);
}
static void make_room_extras(void)
{
    unsigned gifts=pet_room_gifts(pet_state_get());
    // Fixed toy-sized spots keep the pet, care buttons and talk target clear.
    static const int positions[PET_DECORATION_COUNT][2]={{3,208},{124,56},{78,265},{275,137},{240,269},{320,198}};
    for(unsigned d=0;d<PET_DECORATION_COUNT;d++)if(gifts&(1u<<d)) {
        lv_obj_t *gift=button(s_root,positions[d][0],positions[d][1],d==1?120:48,d==1?82:48,CREAM,room_gift_cb,(int)d);
        lv_obj_set_style_bg_opa(gift,LV_OPA_TRANSP,0);lv_obj_set_style_border_width(gift,0,0);
        art(gift,pixel_decoration(d),d==1?0:4,d==1?-18:4,d==1?768:256);
    }
    int sticker=pet_wall_sticker(pet_state_get());
    if(sticker>=0) {
        lv_obj_t *wall=button(s_root,25,77,56,60,0xffecd1,wall_sticker_cb,0);
        art(wall,pixel_collectible((unsigned)sticker),8,11,384);
    }
    // Weather stays inside the glass; the authored curtains, frame and garden remain.
    s_weather=shape(s_root,124,78,120,76,0,0);lv_obj_set_style_bg_opa(s_weather,LV_OPA_TRANSP,0);
    s_weather_icon=art(s_weather,pixel_collectible(13),76,(gifts&2)?37:0,(gifts&2)?256:384);
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
        lv_obj_t *slot=button(s_root,24+i*82,12,74,38,CREAM,nav_cb,(int[]){FOOD,PLAY_MENU,SLEEP,BATH}[i]);
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
    for(unsigned i=0;i<5;i++) {
        s_keepsakes[i]=art(s_root,pixel_collectible(4),32+i*61,175,384);
        lv_obj_add_flag(s_keepsakes[i],LV_OBJ_FLAG_HIDDEN);
    }
    s_keepsake_label=label(s_root,"",12,299,344,false);
    lv_obj_set_style_bg_color(s_keepsake_label,lv_color_hex(CREAM),0);
    lv_obj_set_style_bg_opa(s_keepsake_label,LV_OPA_COVER,0);
    lv_obj_add_flag(s_keepsake_label,LV_OBJ_FLAG_HIDDEN);
    // Compact icon, with a finger-sized target and the same hold/release input.
    s_mic=button(s_root,156,316,56,48,CREAM,NULL,0);
    lv_obj_set_style_radius(s_mic,18,0);
    lv_obj_set_style_bg_color(s_mic,lv_color_hex(MINT),LV_STATE_PRESSED);microphone(s_mic);
    lv_obj_add_event_cb(s_mic,talk_cb,LV_EVENT_PRESSED,NULL);
    lv_obj_add_event_cb(s_mic,talk_cb,LV_EVENT_RELEASED,NULL);
    lv_obj_add_event_cb(s_mic,talk_cb,LV_EVENT_PRESS_LOST,NULL);
    speech_bubble(64);
    lv_obj_t *album=button(s_root,24,249,54,51,CREAM,nav_cb,REWARDS); icon(album,4,-2,1);
    lv_obj_t *settings=button(s_root,294,250,50,48,CREAM,nav_cb,SETTINGS);
    label(settings,LV_SYMBOL_SETTINGS,0,11,50,true);
    const char *names[]={"Food","Play","Sleep","Bath"};
    int views[]={FOOD,PLAY_MENU,SLEEP,BATH};
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
    if(s_view==ARCADE && view!=ARCADE && s_arc_kind==ARCADE_CUPS)
        arcade_save_best(pet_state_get()->pet_id,ARCADE_CUPS,s_arc.cups.best);
    if(view==ARCADE && s_view!=ARCADE) {voice_cancel();audio_stop_tune();s_talking=false;}
    if(s_view==VOICES && view!=VOICES) {voice_cancel();s_preview_playing=false;}
    if(view==VOICES && s_view!=VOICES) {
        voice_cancel();audio_stop_tune();s_talking=false;s_preview_playing=false;
        s_voice_choice=pet_voice_id(pet_state_get());
    }
    if(s_view==MULTIPLAYER && view!=MULTIPLAYER)multiplayer_close();
    if(view==MULTIPLAYER && s_view!=MULTIPLAYER) {
        voice_cancel();audio_stop_tune();s_talking=false;multiplayer_open(pet_state_get());
    }
    if(s_view==MUSIC && view!=MUSIC) {audio_stop_tune();if(s_music_choice==MUSIC_COMPOSE)voice_cancel();}
    if(s_talking && !s_boot_down)talk_end();
    s_mic=NULL;s_voice_status=NULL;s_caption_label=NULL;s_connection=NULL;s_name_input=NULL;s_name_error=NULL;
    // Single owner: no screen-specific timers or callbacks survive the root.
    s_volume_label=NULL; s_volume_slider=NULL;
    s_keepsake_until=0;s_keepsake_label=NULL;memset(s_keepsakes,0,sizeof s_keepsakes);
    s_weather=NULL;s_weather_icon=NULL;s_butterfly=NULL;s_butterfly_image=NULL;
    memset(s_covers,0,sizeof s_covers);memset(s_cover_art,0,sizeof s_cover_art);memset(s_rain,0,sizeof s_rain);
    s_reveal_until=0;s_ball_ready=0;s_ball_pressed=false;s_ball_time=0;s_butterfly_until=0;
    s_ball_flying=false;s_ball_finish_at=0;s_ball_image=NULL;s_ball_shadow=NULL;s_ball_ring=NULL;
    memset(s_ball_trail,0,sizeof s_ball_trail);
    s_pet=NULL; s_target=NULL; s_hint=NULL; s_counter=NULL; s_sleep_bar=NULL;
    for(int i=0;i<4;i++) s_bars[i]=NULL;
    for(int i=0;i<5;i++) s_dots[i]=NULL;
    if(s_root) lv_obj_delete(s_root);
    if(view!=POWER_OFF)s_power_pending=false;
    s_view=view; s_started=lv_tick_get(); s_hits=0; s_eating=false; s_hop_until=0;s_reaction_until=0;s_party_fade_until=0;
    s_root=shape(lv_screen_active(),0,0,368,448,CREAM,0);
    if(view==HOME) { make_home(); return; }
    if(view==POWER_OFF) {
        room(true);make_pet(82,128);
        label(s_root,"See you soon!",24,22,320,true);
        label(s_root,"Press PWR to wake me",24,370,320,false);
    } else if(view==REWARDS) {make_rewards();
    } else if(view==TREATS) {make_treats();
    } else if(view==PROFILE) {make_profile();
    } else if(view==TRAIT) {make_trait();
    } else if(view==MUSIC) {make_music();
    } else if(view==PLAY_MENU) {make_play_menu();
    } else if(view==MULTIPLAYER) {make_multiplayer();
    } else if(view==ARCADE) {make_arcade(NULL);
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
        unsigned unlocked=0;for(unsigned i=0;i<PET_SPECIAL_FOOD_COUNT;i++)unlocked+=pet_food_unlocked(pet_state_get(),i+3);
        char text[64];snprintf(text,sizeof text,"Special treats  %u / %u",unlocked,PET_SPECIAL_FOOD_COUNT);
        lv_obj_t *treats=button(s_root,29,405,310,38,GOLD,treats_cb,0);label(treats,text,0,10,310,false);
    } else if(view==CATCH) {
        room(false); header_to("Catch the stars",GAMES); progress(); make_pet(82,134);
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
        int new_food=pet_food_milestone(earned);
        label(s_root,new_food>=0?"New special treat!":gift?"New room gift!":sticker?"New sticker!":"Lovely caring!",24,40,320,true);
        for(int i=0;i<12;i++) shape(s_root,24+(i*71)%315,95+(i*43)%220,7,12,colors[i%4],3);
        make_pet(82,132); star(s_root,163,93,GOLD);
        char text[64]; unsigned n=pet_state_get()->evolution_progress;
        snprintf(text,sizeof(text),"%lu %s  /  %u of 18 stickers",(unsigned long)n,n==1?"star":"stars",pet_sticker_count(pet_state_get()));
        if(new_food>=0)snprintf(text,sizeof text,"%lu stars: treat%s%s",(unsigned long)n,gift?" + gift":"",sticker?" + sticker":"");
        label(s_root,text,24,337,320,false);
        if(sticker || new_food>=0) {
            lv_obj_t *gift_button=button(s_root,24,376,152,48,GOLD,nav_cb,new_food>=0?TREATS:gift?DECORATIONS:ALBUM);
            label(gift_button,new_food>=0?pet_special_food((unsigned)new_food)->name:gift?"My gift":"My sticker",0,16,152,false);
            lv_obj_t *b=button(s_root,192,376,152,48,MINT,home_cb,0);label(b,"Home " LV_SYMBOL_HOME,0,16,152,false);
        } else {
            lv_obj_t *b=button(s_root,74,376,220,48,MINT,home_cb,0);label(b,"Home " LV_SYMBOL_HOME,0,11,220,true);
        }
        speech_bubble(77);
        s_voice_status=label(s_root,"",24,356,320,false);
        s_hop_until=lv_tick_get()+1800;
    } else if(view==ALBUM) {
        make_album();
    } else if(view==SETTINGS) {
        header("Grown-ups");
        lv_obj_t *chatter=button(s_root,54,77,260,36,CREAM,auto_chat_cb,0);
        label(chatter,voice_auto_enabled()?"Little chats: on":"Little chats: off",0,9,260,false);
        int batt=power_battery_percent(); char b[64];
        if(batt<0) snprintf(b,sizeof(b),"Battery: USB power");
        else snprintf(b,sizeof(b),"Battery: %d%%%s",batt,power_is_charging()?" (charging)":"");
        label(s_root,b,24,120,320,false);
        lv_obj_t *m=button(s_root,54,148,260,44,MINT,mute_cb,0); label(m,s_muted?"Sound off":"Sound on",0,12,260,true);
        snprintf(b, sizeof(b), "Volume: %d%%", audio_get_volume());
        s_volume_label=label(s_root,b,40,207,288,false);
        s_volume_slider=lv_slider_create(s_root);
        lv_obj_set_pos(s_volume_slider,58,242); lv_obj_set_size(s_volume_slider,252,16);
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
        label(s_root,"Quiet",38,270,72,false);
        label(s_root,"Loud",258,270,72,false);
        lv_obj_t *wifi=button(s_root,34,297,144,44,BLUE,nav_cb,CONNECTION);
        label(wifi,LV_SYMBOL_WIFI " Wi-Fi",0,13,144,false);
        lv_obj_t *voices=button(s_root,190,297,144,44,PINK,nav_cb,VOICES);
        label(voices,LV_SYMBOL_AUDIO " Voice",0,13,144,false);
        lv_obj_t *name=button(s_root,34,347,144,44,0xffe9d3,nav_cb,NAME);
        label(name,"Pet name",0,13,144,false);
        lv_obj_t *profile=button(s_root,190,347,144,44,MINT,nav_cb,PROFILE);
        label(profile,"My pet",0,13,144,false);
        lv_obj_t *reset=button(s_root,34,397,300,44,PINK,nav_cb,RESET);
        label(reset,"Start fresh...",0,13,300,false);
    } else if(view==VOICES) {
        header_to("Pet voice",SETTINGS);
        char count[32];snprintf(count,sizeof count,"Voice %u of %u",s_voice_choice+1,PET_VOICE_COUNT);
        label(s_root,count,24,83,320,false);
        lv_obj_t *card=shape(s_root,24,115,320,123,BLUE,12);
        label(card,pet_voice(s_voice_choice)->name,50,19,220,true);
        label(card,pet_voice(s_voice_choice)->description,48,62,224,false);
        lv_obj_t *prev=button(s_root,28,127,48,56,CREAM,voice_step_cb,-1);
        label(prev,LV_SYMBOL_LEFT,0,18,48,true);
        lv_obj_t *next=button(s_root,292,127,48,56,CREAM,voice_step_cb,1);
        label(next,LV_SYMBOL_RIGHT,0,18,48,true);
        char current[64];snprintf(current,sizeof current,"Saved: %s",pet_voice(pet_voice_id(pet_state_get()))->name);
        label(s_root,current,24,248,320,false);
        lv_obj_t *preview=button(s_root,34,282,144,48,MINT,voice_preview_cb,0);
        label(preview,LV_SYMBOL_PLAY " Listen",0,14,144,true);
        lv_obj_t *stop=button(s_root,190,282,144,48,CREAM,voice_stop_cb,0);
        label(stop,LV_SYMBOL_STOP " Stop",0,14,144,true);
        s_hint=label(s_root,"Listen first, then choose your favourite.",24,343,320,false);
        lv_obj_t *save=button(s_root,34,390,300,48,PINK,voice_save_cb,0);
        label(save,"Use this voice",0,14,300,true);
    } else if(view==RESET) {
        header_to("Start fresh?",SETTINGS);
        label(s_root,"A new little beginning",24,96,320,true);
        label(s_root,"This replaces your pet with a new\nbaby named Sprout. Stars, stickers,\nroom gifts and chats start fresh.",24,148,320,false);
        label(s_root,"You cannot undo this on the toy.\nWi-Fi setup stays saved.",24,225,320,false);
        s_hint=label(s_root,"Ask a grown-up before restarting.",16,283,336,false);
        lv_obj_t *keep=button(s_root,34,328,300,48,MINT,nav_cb,SETTINGS);
        label(keep,"Keep my pet",0,15,300,true);
        lv_obj_t *reset=button(s_root,34,390,300,48,PINK,reset_cb,0);
        label(reset,"Yes, start fresh",0,15,300,true);
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
static void power_error(const char *text)
{
    show(HOME);audio_set_muted(s_muted);
    snprintf(s_reaction_text,sizeof s_reaction_text,"%s",text);s_reaction_until=lv_tick_get()+5000;
}
static void start_power_off(void)
{
    // Persist only completed care. An unfinished activity is cancelled, never
    // fast-forwarded into a reward. Failed saves leave the device awake.
    if(!pet_state_save(pet_state_get())) {power_error("Couldn't save. Still awake! Try PWR again.");return;}
    voice_cancel();audio_set_muted(true);s_talking=false;
    show(POWER_OFF);
    pixel_pet_render(&s_pet_art,pet_state_get(),PIXEL_SLEEP,0,PIXEL_APPLE);lv_obj_invalidate(s_pet_image);
    s_power_pending=true;s_power_sent=false;s_power_at=lv_tick_get()+250;
}
static void frame(lv_timer_t *t)
{
    (void)t; uint32_t now=lv_tick_get(), elapsed=now-s_started;
    if(s_power_pending) {
        if(!s_power_sent && (int32_t)(now-s_power_at)>=0) {
            if(!power_request_off()) {power_error("Couldn't sleep. Please try PWR again.");return;}
            s_power_sent=true;
        }
        // A responsive I2C bus does not guarantee the hardware powered down.
        // Recover visibly if USB/board behaviour keeps the ESP running.
        if(s_power_sent && (int32_t)(now-s_power_at)>=1500)
            power_error("Still awake. Please try PWR again.");
        return;
    }
    if(now-s_power_poll>=120) {
        s_power_poll=now;
        bool pressed=power_take_short_press();
        if(pressed && (int32_t)(now-s_power_ready_at)>=0) {
            s_power_ready_at=now+800;start_power_off();return;
        }
    }
    bool boot=voice_boot_pressed();
    if(boot!=s_boot_raw) { s_boot_raw=boot;s_boot_changed=now; }
    if(now-s_boot_changed>=80 && boot!=s_boot_down) {
        s_boot_down=boot;if(boot)talk_begin();else talk_end();
    }
    if(s_talking && now-s_talk_started>=20000)talk_end();
    voice_state_t vs=voice_get_state();
    if(s_view==VOICES && s_preview_playing) {
        if(vs==VOICE_ERROR || vs==VOICE_OFFLINE) {
            s_preview_playing=false;lv_label_set_text(s_hint,"Voice unavailable. Check Wi-Fi or retry.");
        } else if(vs==VOICE_SPEAKING || audio_voice_playing())lv_label_set_text(s_hint,"How does this one sound?");
        else if(vs==VOICE_READY && now-s_preview_started>500) {
            s_preview_playing=false;lv_label_set_text(s_hint,"Like it? Tap Use this voice to save.");
        }
    }
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
    if(s_mic) {
        bool listening=vs==VOICE_LISTENING || s_talking;
        lv_obj_set_style_bg_color(s_mic,lv_color_hex(listening?MINT:vs==VOICE_THINKING?BLUE:CREAM),0);
        lv_obj_set_style_border_color(s_mic,lv_color_hex(listening?0x348768:INK),0);
        lv_obj_set_style_border_width(s_mic,listening?3:2,0);
    }
    if(s_caption_label) {
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
            caption_text(s_caption_label,level>300?"I'm listening...\nI can hear your voice!":"I'm listening...\nSpeak close to me");
            if(s_voice_status)lv_label_set_text(s_voice_status,"Let go when you're done");
        } else if(vs==VOICE_THINKING) {
            caption_text(s_caption_label,local_reaction?s_reaction_text:"One little moment...");
            if(s_voice_status)lv_label_set_text(s_voice_status,"Thinking...");
        } else if(local_reaction && !speaking) {
            caption_text(s_caption_label,s_reaction_text);
            if(s_voice_status)lv_label_set_text(s_voice_status,s_view==HOME?"Hold here or BOOT to talk":"Hold BOOT to talk");
        } else if(caption[0] && (int32_t)(s_voice_until-now)>0) {
            if(strcmp(lv_label_get_text(s_caption_label),caption))caption_text(s_caption_label,caption);
            if(s_voice_status)lv_label_set_text(s_voice_status,speaking?"Chatting with you":s_view==HOME?"Hold here or BOOT to reply":"Hold BOOT to reply");
        } else {
            char greeting[80];snprintf(greeting,sizeof(greeting),s_view==HOME?"Hold to talk to %s":"Hold BOOT to talk to %s",pet_state_get()->name);
            if(s_voice_status && strcmp(lv_label_get_text(s_voice_status),greeting))lv_label_set_text(s_voice_status,greeting);
        }
    }
    if(s_view==MUSIC) {
        if(vs==VOICE_LISTENING)lv_label_set_text(s_hint,"I'm listening... Let go to reply");
        else if(vs==VOICE_THINKING)lv_label_set_text(s_hint,s_music_choice==MUSIC_COMPOSE?"Making a tune... Stop cancels":"One little moment... Stop cancels");
        else if(audio_voice_playing())lv_label_set_text(s_hint,s_music_choice==MUSIC_COMPOSE?"A little song, just for you!":"Chatting with you");
        else if(s_music_choice==MUSIC_COMPOSE && (vs==VOICE_ERROR || vs==VOICE_OFFLINE))lv_label_set_text(s_hint,"Couldn't make a tune. Pick a song!");
        else if(s_music_choice==MUSIC_COMPOSE && vs==VOICE_READY)lv_label_set_text(s_hint,"Pick another tune whenever you like");
        else if(s_music_choice>=0 && !audio_tune_playing() && lv_tick_get()-s_music_started>500) {
            s_music_choice=-1;lv_label_set_text(s_hint,"Pick another song whenever you like");
        }
    }
    if(s_connection && now-s_connection_tick>=500) { char text[256];voice_status(text,sizeof(text));lv_label_set_text(s_connection,text);s_connection_tick=now; }
    if(now-s_last_tick>=10000) { pet_state_tick((uint32_t)time(NULL)); s_last_tick=now; refresh(); }
    multiplayer_frame(now);
    if(s_view==ARCADE)arcade_frame(now,NULL);
    if(s_view==HOME) {room_frame(now,vs);keepsake_frame(now,vs);}
    if(s_view==BALL && s_target)ball_frame(now);
    if(s_view==HIDE && s_reveal_until && (int32_t)(now-s_reveal_until)>=0) {
        if(s_hits==3)finish(1);else hide_place();
    }
    if(s_pet) {
        bool hopping=(int32_t)(s_hop_until-now)>0;
        int dy=hopping ? -(int)(fabsf(sinf(now/90.0f))*13) : (int)(sinf(now/(s_view == SLEEP ? 1100.0f : 650.0f))*3);
        if(s_view==SLEEP || s_view==BATH || s_view==HIDE)dy=0;
        else if(s_eating)dy=elapsed<FOOD_SETTLE_MS && elapsed>=FOOD_BITE_MS ? -(int)(fabsf(sinf(elapsed/160.0f))*3) : 0;
        lv_obj_set_y(s_pet,s_pet_y+dy);
        bool asleep=s_view==SLEEP;
        bool delighted=s_view==PARTY || (s_view==HIDE && s_reveal_until) || (hopping && !s_eating && !asleep && s_view!=BATH);
        s_face=asleep?PIXEL_SLEEP:s_eating?PIXEL_EAT:s_view==BATH?PIXEL_BATH:
               audio_voice_playing()?PIXEL_TALK:vs==VOICE_LISTENING?PIXEL_LISTEN:
               vs==VOICE_THINKING?PIXEL_THINK:delighted?PIXEL_HAPPY:(s_view==CATCH || s_view==BALL)?PIXEL_PLAY:
               now%4200>4010?PIXEL_BLINK:PIXEL_IDLE;
        bool keepsake=s_view==HOME && s_keepsake_until;
        if(keepsake) {
            s_face=keepsake_actions[s_keepsake_choice].pose;
            lv_obj_set_y(s_pet,s_pet_y+(s_face==PIXEL_PLAY?-(int)(fabsf(sinf(now/120.0f))*10):0));
        }
        bool reaching=s_view==BALL && s_ball_flying && !audio_voice_playing() &&
            vs!=VOICE_LISTENING && vs!=VOICE_THINKING && vs!=VOICE_SPEAKING;
        if(reaching)s_face=PIXEL_PLAY;
        pixel_pet_render(&s_pet_art,pet_state_get(),s_face,reaching?(s_ball_to_x>s_ball_from_x?3:0):s_eating?(elapsed<FOOD_BITE_MS?0:3):now/180,
                         keepsake && s_keepsake_choice==0?PIXEL_APPLE:s_food);
        lv_obj_invalidate(s_pet_image);
        if(hopping && !s_eating && !asleep && s_view!=HIDE) lv_obj_remove_flag(s_pet_heart,LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(s_pet_heart,LV_OBJ_FLAG_HIDDEN);
    }
    if(s_view==FOOD && s_eating) {
        if(elapsed>=FOOD_DURATION_MS)finish(0);
        else if(elapsed>FOOD_DURATION_MS-FOOD_FADE_MS)
            lv_obj_set_style_opa(s_root,(lv_opa_t)((FOOD_DURATION_MS-elapsed)*255/FOOD_FADE_MS),0);
    }
    if(s_view==PARTY && s_party_fade_until) {
        int32_t remaining=(int32_t)(s_party_fade_until-now);
        lv_obj_set_style_opa(s_root,remaining>0?(lv_opa_t)((FOOD_FADE_MS-remaining)*255/FOOD_FADE_MS):LV_OPA_COVER,0);
        if(remaining<=0)s_party_fade_until=0;
    }
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
    s_power_ready_at=lv_tick_get()+1000;
    show(HOME); lv_timer_create(frame,40,NULL);
    renderer_unlock();
}
void ui_refresh_stats(void) { if(renderer_lock(0)) { refresh(); renderer_unlock(); } }
void ui_show_home(void) { if(renderer_lock(0)) { show(HOME); renderer_unlock(); } }
void ui_show_emote(emote_id_t emote) { (void)emote; }
