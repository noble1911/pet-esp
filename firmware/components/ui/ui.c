// Little Meadow: vector artwork and activities, all owned by the LVGL task.
#include <math.h>
#include <stdio.h>
#include <time.h>
#include "ui.h"
#include "renderer.h"
#include "power.h"
#include "audio.h"
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

typedef enum { HOME, FOOD, CATCH, BATH, SLEEP, PARTY, ALBUM, SETTINGS } View;
static View s_view;
static lv_obj_t *s_root, *s_pet, *s_bars[4], *s_target, *s_counter;
static lv_obj_t *s_dots[5], *s_hint, *s_sleep_bar;
static lv_obj_t *s_volume_label, *s_volume_slider;
static lv_obj_t *s_pet_image, *s_pet_heart;
static pixel_pet_art_t s_pet_art;
static pixel_face_t s_face;
static uint32_t s_started, s_hop_until, s_last_tick;
static unsigned s_hits, s_last_target;
static bool s_eating, s_muted;
static int s_pet_x, s_pet_y;
static const uint32_t colors[4] = {0xf5b164, 0xed99b4, 0x9aafe7, 0x79c8b7};
static void show(View view);
static void refresh(void);

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
    lv_obj_add_event_cb(o,cb,LV_EVENT_CLICKED,(void *)(intptr_t)data);
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
    lv_image_set_scale(o,640); // 20 pixels -> 50-pixel touch-button illustration
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
    (void)e; s_hop_until=lv_tick_get()+600;
    if(s_hint) lv_label_set_text(s_hint,"I love you!");
    audio_play(SFX_FEED);
}
static void make_pet(int x, int y)
{
    s_pet_x=x-8; s_pet_y=y-14;
    s_pet=shape(s_root,s_pet_x,s_pet_y,172,174,0,0);
    lv_obj_set_style_bg_opa(s_pet,LV_OPA_TRANSP,0);
    lv_obj_add_flag(s_pet,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_pet,pet_tap,LV_EVENT_CLICKED,NULL);
    s_face=s_view==SLEEP?PIXEL_SLEEP:PIXEL_IDLE;
    pixel_pet_render(&s_pet_art,pet_state_get(),s_face,0);
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
    if(action==0) pet_state_feed();
    if(action==1) pet_state_play();
    if(action==2) pet_state_rest();
    if(action==3) pet_state_clean();
    audio_play(SFX_HAPPY);
    show(PARTY);
}
static void food_cb(lv_event_t *e)
{
    if(s_eating) return;
    s_eating=true; s_started=lv_tick_get(); s_hop_until=s_started+1200;
    lv_label_set_text(s_hint,"Yum yum!");
    lv_obj_t *o=lv_event_get_target(e); lv_obj_set_style_bg_color(o,lv_color_hex(GOLD),0);
    audio_play(SFX_FEED);
}
static void place_target(void)
{
    // Discrete, widely separated positions keep every star stationary until tapped.
    static const int pos[6][2]={{37,137},{228,150},{131,177},{45,254},{228,261},{133,288}};
    unsigned n=(s_last_target+1+esp_random()%5)%6; s_last_target=n;
    lv_obj_set_pos(s_target,pos[n][0],pos[n][1]);
}
static void catch_cb(lv_event_t *e)
{
    (void)e; s_hits++; update_progress(); audio_play(SFX_FEED);
    if(s_hits==5) { finish(1); return; }
    place_target();
}
static void bubble_cb(lv_event_t *e)
{
    lv_obj_t *o=lv_event_get_target(e);
    if(lv_obj_has_state(o,LV_STATE_DISABLED)) return;
    lv_obj_add_state(o,LV_STATE_DISABLED); lv_obj_add_flag(o,LV_OBJ_FLAG_HIDDEN);
    s_hits++; update_progress(); s_hop_until=lv_tick_get()+400; audio_play(SFX_EMOTE);
    if(s_hits==5) finish(3);
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
    if (lv_event_get_code(e) == LV_EVENT_RELEASED) audio_play(SFX_FEED);
}
static void make_home(void)
{
    meadow();
    for(int i=0;i<4;i++) {
        lv_obj_t *slot=button(s_root,24+i*82,12,74,38,CREAM,nav_cb,(int[]){FOOD,CATCH,SLEEP,BATH}[i]);
        lv_obj_set_style_border_width(slot,0,0);
        lv_obj_t *glyph=lv_image_create(slot);
        lv_image_set_src(glyph,pixel_icon((unsigned)i));
        lv_image_set_pivot(glyph,0,0);lv_image_set_scale(glyph,384);
        lv_image_set_antialias(glyph,false);lv_obj_set_pos(glyph,0,2);
        s_bars[i]=lv_bar_create(slot); lv_obj_set_size(s_bars[i],34,10); lv_obj_set_pos(s_bars[i],36,13);
        lv_obj_set_style_bg_color(s_bars[i],lv_color_hex(0xe9dfcf),LV_PART_MAIN);
        lv_obj_set_style_bg_color(s_bars[i],lv_color_hex(colors[i]),LV_PART_INDICATOR);
        lv_obj_set_style_radius(s_bars[i],0,LV_PART_MAIN); lv_obj_set_style_radius(s_bars[i],0,LV_PART_INDICATOR);
    }
    // Clear feedback lives on a cream strip, away from the detailed room art.
    shape(s_root,24,327,320,23,CREAM,0);
    s_hint=label(s_root,"Tap me for a cuddle",28,331,312,false);
    make_pet(106,167);
    lv_obj_t *album=button(s_root,24,249,54,51,CREAM,nav_cb,ALBUM); icon(album,4,-2,1);
    lv_obj_t *settings=button(s_root,294,250,50,48,CREAM,nav_cb,SETTINGS);
    label(settings,LV_SYMBOL_SETTINGS,0,11,50,true);
    const char *names[]={"Food","Play","Sleep","Bath"};
    int views[]={FOOD,CATCH,SLEEP,BATH};
    for(int i=0;i<4;i++) {
        lv_obj_t *b=button(s_root,24+i*82,352,74,72,(uint32_t[]){0xff95b0,0x93deaf,0xc5a1ed,0x85d6f3}[i],nav_cb,views[i]);
        icon(b,i,8,0); label(b,names[i],0,51,74,false);
        shape(b,21,68,32,3,colors[i],2);
    }
    refresh();
}
static void show(View view)
{
    // Single owner: no screen-specific timers or callbacks survive the root.
    s_volume_label=NULL; s_volume_slider=NULL;
    s_pet=NULL; s_target=NULL; s_hint=NULL; s_counter=NULL; s_sleep_bar=NULL;
    for(int i=0;i<4;i++) s_bars[i]=NULL;
    for(int i=0;i<5;i++) s_dots[i]=NULL;
    if(s_root) lv_obj_delete(s_root);
    s_view=view; s_started=lv_tick_get(); s_hits=0; s_eating=false; s_hop_until=0;
    s_root=shape(lv_screen_active(),0,0,368,448,CREAM,0);
    if(view==HOME) { make_home(); return; }
    if(view==FOOD) {
        room(false); header("Snack time"); make_pet(106,132);
        shape(s_root,32,85,304,29,CREAM,0);
        s_hint=label(s_root,"Pick a yummy snack",32,92,304,false);
        for(int i=0;i<3;i++) {
            lv_obj_t *b=button(s_root,29+i*108,310,94,89,0xffe9d3,food_cb,i);
            if(i==0) icon(b,0,17,8);
            if(i==1) { shape(b,26,15,44,37,0xe9b976,9); shape(b,31,20,34,26,0xffd697,6); }
            if(i==2) { shape(b,25,12,46,46,0xe3ac79,23); for(int j=0;j<4;j++) shape(b,34+j%2*18,23+j/2*17,6,6,0x98745e,3); }
            label(b,(const char*[]){"Apple","Toast","Cookie"}[i],0,65,94,false);
        }
    } else if(view==CATCH) {
        header("Catch the stars"); progress();
        label(s_root,"Tap each golden star",32,405,304,false);
        s_target=button(s_root,40,150,96,96,0xffedb9,catch_cb,0); star(s_target,28,26,0xe8aa35); place_target();
        // Quiet dotted trail gives the playfield depth without distracting targets.
        for(int i=0;i<7;i++) shape(s_root,35+i*46,378,5,5,0xdce8df,3);
    } else if(view==BATH) {
        room(false); header("Bubble bath"); progress(); make_pet(106,159);
        shape(s_root,62,294,244,62,BLUE,30); shape(s_root,53,282,262,21,0xd4eff4,10);
        label(s_root,"Pop all five bubbles",32,397,304,false);
        static const int xy[5][2]={{26,139},{144,125},{264,140},{35,225},{259,229}};
        for(int i=0;i<5;i++) {
            lv_obj_t *b=button(s_root,xy[i][0],xy[i][1],70,70,0xc2e8ef,bubble_cb,i);
            lv_obj_set_style_radius(b,35,0); shape(b,15,13,17,10,0xffffff,6);
        }
    } else if(view==SLEEP) {
        room(true);
        header("Little nap");
        make_pet(106,178);
        s_sleep_bar=lv_bar_create(s_root); lv_obj_set_pos(s_sleep_bar,74,365); lv_obj_set_size(s_sleep_bar,220,12);
        lv_obj_set_style_bg_color(s_sleep_bar,lv_color_hex(GOLD),LV_PART_INDICATOR);
        shape(s_root,94,107,180,32,CREAM,0);
        label(s_root,"Shhh...",94,116,180,true);
    } else if(view==PARTY) {
        unsigned earned=pet_state_get()->evolution_progress;
        label(s_root,earned<=30 && earned%5==0?"New sticker!":"Lovely caring!",24,40,320,true);
        for(int i=0;i<12;i++) shape(s_root,24+(i*71)%315,95+(i*43)%220,7,12,colors[i%4],3);
        make_pet(106,157); star(s_root,163,93,GOLD);
        char text[64]; unsigned n=pet_state_get()->evolution_progress;
        snprintf(text,sizeof(text),"%lu %s  /  %u of 6 stickers",(unsigned long)n,n==1?"star":"stars",(unsigned)(n/5>6?6:n/5));
        label(s_root,text,24,337,320,false);
        lv_obj_t *b=button(s_root,74,376,220,48,MINT,home_cb,0); label(b,"Home " LV_SYMBOL_HOME,0,11,220,true);
        if(n<=30 && n%5==0) {
            lv_obj_t *gift=button(s_root,270,90,70,70,0xffecd1,nav_cb,ALBUM);
            icon(gift,(int)(n/5)-1,7,8);
        }
        s_hop_until=lv_tick_get()+1800;
    } else if(view==ALBUM) {
        header("My stickers"); unsigned n=pet_state_get()->evolution_progress;
        label(s_root,n>=30?"You found them all!":"One sticker every 5 stars",24,94,320,false);
        for(int i=0;i<6;i++) {
            lv_obj_t *tile=shape(s_root,29+(i%3)*108,139+(i/3)*112,94,96,n>=(unsigned)(i+1)*5?0xffecd1:0xe9ede6,22);
            if(n>=(unsigned)(i+1)*5) icon(tile,i,17,12);
            else { shape(tile,33,19,28,31,0xb4beb5,14); shape(tile,38,24,18,22,0xe9ede6,9); shape(tile,28,36,38,27,0xb4beb5,7); }
            char b[16]; snprintf(b,sizeof(b),"%d stars",(i+1)*5); label(tile,b,0,71,94,false);
        }
        label(s_root,"Feed, play, nap and splash!",24,390,320,false);
    } else if(view==SETTINGS) {
        header("Grown-ups");
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
        label(s_root,"No losing. No rushing.\nCare earns stars and stickers.\nPet progress saves automatically.",30,367,308,false);
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
    if(now-s_last_tick>=10000) { pet_state_tick((uint32_t)time(NULL)); s_last_tick=now; refresh(); }
    if(s_pet) {
        bool hopping=(int32_t)(s_hop_until-now)>0;
        int dy=hopping ? -(int)(fabsf(sinf(now/90.0f))*13) : (int)(sinf(now/(s_view == SLEEP ? 1100.0f : 650.0f))*3);
        lv_obj_set_y(s_pet,s_pet_y+dy);
        bool asleep=s_view==SLEEP;
        bool delighted=s_view==PARTY || (hopping && !s_eating && !asleep);
        s_face=asleep?PIXEL_SLEEP:s_eating?PIXEL_EAT:delighted?PIXEL_HAPPY:
               now%4200>4010?PIXEL_BLINK:PIXEL_IDLE;
        pixel_pet_render(&s_pet_art,pet_state_get(),s_face,now/180);
        lv_obj_invalidate(s_pet_image);
        if(hopping && !s_eating && !asleep) lv_obj_remove_flag(s_pet_heart,LV_OBJ_FLAG_HIDDEN);
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
