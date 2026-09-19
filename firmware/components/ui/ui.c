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

#define INK 0x39465c
#define CREAM 0xfff9ed
#define MINT 0xdaf3e5
#define GOLD 0xffcf57
#define PINK 0xf3a8b6
#define BLUE 0xa9dbef

typedef enum { HOME, FOOD, CATCH, BATH, SLEEP, PARTY, ALBUM, SETTINGS } View;
static View s_view;
static lv_obj_t *s_root, *s_pet, *s_eyes[2], *s_bars[4], *s_target, *s_counter;
static lv_obj_t *s_dots[5], *s_hint, *s_sleep_bar;
static lv_obj_t *s_ears[2], *s_closed_eyes[2], *s_pupils[2], *s_paws[2];
static lv_obj_t *s_smile, *s_open_mouth, *s_pet_heart;
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
    lv_obj_set_style_radius(o,r,0);
    lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return o;
}
static lv_obj_t *label(lv_obj_t *p, const char *txt, int x, int y, int w, bool big)
{
    lv_obj_t *o=lv_label_create(p);
    lv_label_set_text(o,txt); lv_obj_set_pos(o,x,y); lv_obj_set_width(o,w);
    lv_obj_set_style_text_color(o,lv_color_hex(INK),0);
    lv_obj_set_style_text_font(o,big ? &lv_font_montserrat_24 : &lv_font_montserrat_14,0);
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
    lv_obj_set_style_shadow_width(o,8,0);
    lv_obj_set_style_shadow_ofs_y(o,3,0);
    lv_obj_add_event_cb(o,cb,LV_EVENT_CLICKED,(void *)(intptr_t)data);
    return o;
}
// All icon geometry is native LVGL: crisp at panel resolution, no glyph dependency.
static void star(lv_obj_t *p,int x,int y,uint32_t c)
{
    static const lv_point_precise_t pts[]={{20,0},{25,13},{40,15},{29,25},{32,40},{20,32},{8,40},{11,25},{0,15},{15,13},{20,0}};
    lv_obj_t *o=lv_line_create(p); lv_line_set_points(o,pts,11);
    lv_obj_set_pos(o,x,y); lv_obj_set_style_line_color(o,lv_color_hex(c),0);
    lv_obj_set_style_line_width(o,5,0); lv_obj_set_style_line_rounded(o,true,0);
    shape(p,x+14,y+14,13,15,c,8);
}
static void icon(lv_obj_t *p,int kind,int x,int y)
{
    if(kind==0) { // apple
        shape(p,x+8,y+13,29,31,0xef817d,15); shape(p,x+25,y+13,27,31,0xef817d,15);
        shape(p,x+28,y+2,5,15,INK,2); shape(p,x+33,y+3,17,9,0x66b998,7);
        shape(p,x+15,y+20,5,11,0xffc1ad,3);
    } else if(kind==1) { // beach ball
        shape(p,x+8,y+4,42,42,0xf0ac78,21);
        shape(p,x+22,y+5,14,40,0xffe4a1,7);
        shape(p,x+9,y+21,40,9,0xffffff,5);
    } else if(kind==2) {
        shape(p,x+9,y+3,39,39,0x8c9fda,20);
        shape(p,x+24,y-1,30,30,CREAM,16);
        shape(p,x+43,y+32,5,5,GOLD,3);
    } else if(kind==3) {
        shape(p,x+9,y+22,26,26,0x76c5d6,13); shape(p,x+30,y+9,23,23,0x9ad8e5,12);
        shape(p,x+10,y+3,14,14,0xbee9ef,7); shape(p,x+15,y+27,7,6,0xffffff,4);
        shape(p,x+35,y+13,6,5,0xffffff,3);
    } else if(kind==4) star(p,x+9,y+3,GOLD);
    else { // flower
        for(int i=0;i<5;i++) { float a=i*6.283185f/5; shape(p,x+21+(int)(14*cosf(a)),y+18+(int)(14*sinf(a)),20,20,PINK,10); }
        shape(p,x+23,y+20,16,16,GOLD,8);
    }
}
static void home_cb(lv_event_t *e) { (void)e; show(HOME); }
static void nav_cb(lv_event_t *e) { show((View)(intptr_t)lv_event_get_user_data(e)); }
static void header(const char *title)
{
    lv_obj_t *b=button(s_root,24,22,48,48,0xffffff,home_cb,0);
    label(b,LV_SYMBOL_LEFT,0,11,48,true);
    label(s_root,title,80,32,208,true);
}
static void cloud(int x,int y)
{
    shape(s_root,x,y+12,74,22,0xffffff,12); shape(s_root,x+12,y,32,35,0xffffff,16);
    shape(s_root,x+36,y+7,27,26,0xffffff,14);
}
static void meadow(void)
{
    shape(s_root,0,0,368,448,0xe7f5ef,0);
    shape(s_root,277,90,46,46,0xffe5a2,23);
    cloud(24,100); cloud(212,157);
    shape(s_root,-90,262,370,180,0xc2e4ce,180);
    shape(s_root,160,252,320,190,0xb2d9c0,160);
    shape(s_root,0,314,368,134,0xb2d9c0,0);
    shape(s_root,87,298,196,33,0x95c5af,17);
    for(int i=0;i<6;i++) {
        int x=26+i*61, y=306+(i%2)*25;
        shape(s_root,x,y,4,13,0x7cac8b,2);
        shape(s_root,x-4,y-3,12,7,i%2 ? CREAM : 0xffdea0,5);
    }
}
static void pet_tap(lv_event_t *e)
{
    (void)e; s_hop_until=lv_tick_get()+600;
    if(s_hint) lv_label_set_text(s_hint,"I love you!");
    audio_play(SFX_FEED);
}
// The pet is drawn at native panel resolution. Shared geometry keeps every
// expression and growth accessory inside the same touch target.
static lv_obj_t *pet_part(lv_obj_t *parent, int x, int y, int w, int h,
                          uint32_t light, uint32_t base, uint32_t edge, int radius)
{
    lv_obj_t *o = shape(parent, x, y, w, h, light, radius);
    lv_obj_set_style_bg_grad_color(o, lv_color_hex(base), 0);
    lv_obj_set_style_bg_grad_dir(o, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_color(o, lv_color_hex(edge), 0);
    lv_obj_set_style_border_width(o, 2, 0);
    return o;
}

static lv_obj_t *pet_curve(lv_obj_t *parent, int x, int y, int w, int h,
                           uint32_t color, int start, int end, int thickness)
{
    lv_obj_t *o = lv_arc_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_arc_set_bg_angles(o, start, end);
    lv_obj_set_style_arc_color(o, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(o, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_arc_width(o, thickness, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(o, true, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(o, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

static void make_pet(int x, int y)
{
    const Pet *pet = pet_state_get();
    // Each coat has a light, midtone and outline, so it reads on both the
    // pale meadow and dark bedtime screen without a heavy black border.
    static const uint32_t coats[][3] = {
        {0xffdfbc, 0xedb98e, 0xbb886f}, {0xdcd4ff, 0xafa3e2, 0x8175b2},
        {0xc9f0df, 0x8dcdbb, 0x629e91}, {0xffd5e0, 0xeaa5bf, 0xba7b99},
        {0xffeab2, 0xecc979, 0xb39a5d}, {0xd0eafa, 0x9fc7e3, 0x709cb9}
    };
    const uint32_t *c = coats[pet->genes[GENE_BODY_COLOR] % 6];
    s_pet_x = x - 8;
    s_pet_y = y - 14;
    s_pet = shape(s_root, s_pet_x, s_pet_y, 172, 174, 0, 0);
    lv_obj_set_style_bg_opa(s_pet, LV_OPA_TRANSP, 0);
    lv_obj_add_flag(s_pet, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_pet, pet_tap, LV_EVENT_CLICKED, NULL);

    // One slightly shorter ear gives the silhouette a less mechanical feel.
    for (int i = 0; i < 2; i++) {
        s_ears[i] = pet_part(s_pet, i ? 107 : 31, i ? 12 : 3,
                             35, i ? 68 : 76, c[0], c[1], c[2], 18);
        lv_obj_set_style_transform_pivot_x(s_ears[i], 17, 0);
        lv_obj_set_style_transform_pivot_y(s_ears[i], 63, 0);
        lv_obj_set_style_transform_rotation(s_ears[i], i ? 110 : -90, 0);
        pet_part(s_ears[i], 9, 10, 17, i ? 44 : 52,
                 0xffdfe3, 0xf0b7c6, 0xe4a8b8, 10);
        shape(s_ears[i], 10, 10, 5, 24, 0xffedf0, 3);
    }
    pet_part(s_pet, 39, 111, 94, 57, c[0], c[1], c[2], 29);
    pet_part(s_pet, 51, 121, 70, 39, 0xfff7e5, 0xf4dec5, 0xe3cbb3, 21);
    // Tail, feet and arms are drawn behind the oversized head.
    pet_part(s_pet, 128, 122, 23, 25, 0xfff4e6, c[0], c[2], 13);
    for (int i = 0; i < 2; i++) {
        lv_obj_t *foot = pet_part(s_pet, i ? 103 : 30, 150, 39, 23,
                                  c[0], c[1], c[2], 13);
        shape(foot, 11, 10, 17, 7, 0xf1c5cd, 4);
        s_paws[i] = pet_part(s_pet, i ? 139 : 9, 111, 24, 33,
                              c[0], c[1], c[2], 13);
    }
    lv_obj_t *head = pet_part(s_pet, 14, 44, 144, 92, c[0], c[1], c[2], 43);
    // A subtle forehead shine and broad cream muzzle give the face volume.
    lv_obj_t *shine = shape(head, 26, 7, 53, 12, 0xffffff, 8);
    lv_obj_set_style_bg_opa(shine, LV_OPA_30, 0);
    shape(s_pet, 57, 96, 58, 31, 0xfff3df, 17);
    for (int i = 0; i < 2; i++) {
        shape(s_pet, i ? 119 : 28, 101, 24, 13, 0xeb9db2, 7);
        shape(s_pet, i ? 123 : 32, 103, 9, 3, 0xffced8, 2);
        // Eyes are containers, allowing the pupils to glance independently.
        s_eyes[i] = shape(s_pet, i ? 105 : 44, 77, 23, 29, 0xfff9ef, 12);
        s_pupils[i] = shape(s_eyes[i], 2, 1, 20, 27, INK, 10);
        shape(s_pupils[i], 4, 4, 7, 8, 0xffffff, 4);
        shape(s_pupils[i], 12, 17, 4, 4, 0xb1c4dc, 2);
        s_closed_eyes[i] = pet_curve(s_pet, i ? 105 : 44, 84, 23, 17,
                                     INK, 15, 165, 3);
        lv_obj_add_flag(s_closed_eyes[i], LV_OBJ_FLAG_HIDDEN);
    }
    pet_part(s_pet, 80, 100, 12, 8, 0xbd8190, 0x936273, 0x936273, 5);
    s_smile = shape(s_pet, 72, 106, 29, 18, 0, 0);
    lv_obj_set_style_bg_opa(s_smile, LV_OPA_TRANSP, 0);
    pet_curve(s_smile, 1, 0, 14, 12, INK, 0, 165, 2);
    pet_curve(s_smile, 13, 0, 14, 12, INK, 15, 180, 2);
    s_open_mouth = shape(s_pet, 77, 109, 19, 16, INK, 9);
    shape(s_open_mouth, 4, 9, 12, 6, 0xefabb8, 6);
    lv_obj_add_flag(s_open_mouth, LV_OBJ_FLAG_HIDDEN);

    if (pet->stage >= PET_STAGE_CHILD) {
        shape(s_pet, 71, 43, 16, 10, c[0], 7);
        shape(s_pet, 84, 40, 13, 14, c[0], 7);
    }
    // Small accessories sit off the face; the old central flower hid the tuft.
    if (pet->stage >= PET_STAGE_TEEN) {
        shape(s_pet, 120, 44, 19, 9, 0x76b99b, 6);
        for (int i = 0; i < 5; i++) {
            float a = i * 6.283185f / 5;
            shape(s_pet, 118 + (int)(9*cosf(a)), 43 + (int)(9*sinf(a)),
                  13, 13, 0xffc3d3, 7);
        }
        pet_part(s_pet, 120, 46, 10, 10, 0xffe6a1, GOLD, 0xe4b24f, 5);
    }
    if (pet->stage >= PET_STAGE_ADULT) {
        pet_part(s_pet, 49, 131, 75, 10, 0xf9c1ce, 0xe39bb1, 0xc7839a, 5);
        pet_part(s_pet, 111, 137, 13, 22, 0xf9c1ce, 0xe39bb1, 0xc7839a, 4);
    }
    if (pet->stage >= PET_STAGE_ELDER) {
        pet_part(s_pet, 76, 137, 20, 20, 0xffe7a5, GOLD, 0xd1a04d, 10);
        shape(s_pet, 84, 141, 4, 12, 0xfff8de, 2);
        shape(s_pet, 80, 145, 12, 4, 0xfff8de, 2);
    }
    // A little heart appears during cuddles, anchored inside the pet bounds.
    s_pet_heart = shape(s_pet, 146, 51, 24, 25, 0, 0);
    lv_obj_set_style_bg_opa(s_pet_heart, LV_OPA_TRANSP, 0);
    shape(s_pet_heart, 2, 2, 12, 14, 0xe99ab0, 7);
    shape(s_pet_heart, 11, 2, 12, 14, 0xe99ab0, 7);
    shape(s_pet_heart, 7, 8, 12, 14, 0xe99ab0, 5);
    lv_obj_add_flag(s_pet_heart, LV_OBJ_FLAG_HIDDEN);
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
static void make_home(void)
{
    meadow();
    label(s_root,"Little Meadow",54,22,260,true);
    for(int i=0;i<4;i++) {
        lv_obj_t *slot=button(s_root,24+i*82,61,74,38,CREAM,nav_cb,(int[]){FOOD,CATCH,SLEEP,BATH}[i]);
        // Small matching coloured indicators; full-size pictograms sit below.
        s_bars[i]=lv_bar_create(slot); lv_obj_set_size(s_bars[i],50,10); lv_obj_set_pos(s_bars[i],12,14);
        lv_obj_set_style_bg_color(s_bars[i],lv_color_hex(0xe9e8de),LV_PART_MAIN);
        lv_obj_set_style_bg_color(s_bars[i],lv_color_hex(colors[i]),LV_PART_INDICATOR);
        lv_obj_set_style_radius(s_bars[i],6,LV_PART_MAIN); lv_obj_set_style_radius(s_bars[i],6,LV_PART_INDICATOR);
    }
    s_hint=label(s_root,"Tap me for a cuddle",40,126,288,false);
    make_pet(106,161);
    lv_obj_t *album=button(s_root,24,249,54,51,CREAM,nav_cb,ALBUM); icon(album,4,-2,1);
    lv_obj_t *settings=button(s_root,294,250,50,48,CREAM,nav_cb,SETTINGS);
    label(settings,LV_SYMBOL_SETTINGS,0,11,50,true);
    const char *names[]={"Food","Play","Sleep","Bath"};
    int views[]={FOOD,CATCH,SLEEP,BATH};
    for(int i=0;i<4;i++) {
        lv_obj_t *b=button(s_root,24+i*82,352,74,72,CREAM,nav_cb,views[i]);
        icon(b,i,8,0); label(b,names[i],0,51,74,false);
        shape(b,21,68,32,3,colors[i],2);
    }
    refresh();
}
static void show(View view)
{
    // Single owner: no screen-specific timers or callbacks survive the root.
    s_pet=NULL; s_target=NULL; s_hint=NULL; s_counter=NULL; s_sleep_bar=NULL;
    for(int i=0;i<4;i++) s_bars[i]=NULL;
    for(int i=0;i<5;i++) s_dots[i]=NULL;
    if(s_root) lv_obj_delete(s_root);
    s_view=view; s_started=lv_tick_get(); s_hits=0; s_eating=false; s_hop_until=0;
    s_root=shape(lv_screen_active(),0,0,368,448,CREAM,0);
    if(view==HOME) { make_home(); return; }
    if(view==FOOD) {
        header("Snack time"); make_pet(106,124);
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
        header("Bubble bath"); progress(); make_pet(106,159);
        shape(s_root,62,294,244,62,BLUE,30); shape(s_root,53,282,262,21,0xd4eff4,10);
        label(s_root,"Pop all five bubbles",32,397,304,false);
        static const int xy[5][2]={{26,139},{144,125},{264,140},{35,225},{259,229}};
        for(int i=0;i<5;i++) {
            lv_obj_t *b=button(s_root,xy[i][0],xy[i][1],70,70,0xc2e8ef,bubble_cb,i);
            lv_obj_set_style_radius(b,35,0); shape(b,15,13,17,10,0xffffff,6);
        }
    } else if(view==SLEEP) {
        lv_obj_set_style_bg_color(s_root,lv_color_hex(0x343e62),0);
        header("Little nap");
        // Override header title for the night palette.
        lv_obj_set_style_text_color(lv_obj_get_child(s_root,1),lv_color_hex(CREAM),0);
        for(int i=0;i<8;i++) shape(s_root,30+(i*47)%310,106+(i*31)%190,4,4,GOLD,2);
        shape(s_root,271,105,39,39,0xb4bce9,20);
        shape(s_root,285,100,30,30,0x343e62,16);
        make_pet(106,178);
        s_sleep_bar=lv_bar_create(s_root); lv_obj_set_pos(s_sleep_bar,74,365); lv_obj_set_size(s_sleep_bar,220,12);
        lv_obj_set_style_bg_color(s_sleep_bar,lv_color_hex(GOLD),LV_PART_INDICATOR);
        lv_obj_t *l=label(s_root,"Shhh...",70,125,228,true); lv_obj_set_style_text_color(l,lv_color_hex(CREAM),0);
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
        label(s_root,"No losing. No rushing.\nYour pet is safe while you are away.\n\nCare earns stars and stickers.\nGrowth: 10, 30, 60, 100 stars.\n\nProgress saves automatically.",30,248,308,false);
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
        bool asleep = s_view == SLEEP;
        bool delighted = s_view == PARTY || (hopping && !s_eating && !asleep);
        bool blink = now % 4200 > 4010;
        bool closed = asleep || delighted || blink;
        for (int i = 0; i < 2; i++) {
            if (closed) {
                lv_obj_add_flag(s_eyes[i], LV_OBJ_FLAG_HIDDEN);
                lv_obj_remove_flag(s_closed_eyes[i], LV_OBJ_FLAG_HIDDEN);
                lv_arc_set_bg_angles(s_closed_eyes[i], delighted ? 195 : 15,
                                      delighted ? 345 : 165);
            } else {
                lv_obj_remove_flag(s_eyes[i], LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(s_closed_eyes[i], LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_x(s_pupils[i], 2 + (int)(sinf(now / 1700.0f) * 1.5f));
            }
            int flutter = asleep ? 0 : (int)(sinf(now / (hopping ? 95.0f : 800.0f)) * (hopping ? 55 : 12));
            lv_obj_set_style_transform_rotation(s_ears[i], (i ? 110 : -90) + flutter, 0);
            lv_obj_set_y(s_paws[i], 111 + (delighted ? (int)(sinf(now / 100.0f + i) * 5) : 0));
        }
        bool open = !asleep && (delighted || (s_eating && now % 300 < 180));
        if (open) {
            lv_obj_add_flag(s_smile, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(s_open_mouth, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_remove_flag(s_smile, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(s_open_mouth, LV_OBJ_FLAG_HIDDEN);
        }
        if (hopping && !s_eating && !asleep) {
            lv_obj_remove_flag(s_pet_heart, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_y(s_pet_heart, 48 + (int)(sinf(now / 200.0f) * 4));
        } else lv_obj_add_flag(s_pet_heart, LV_OBJ_FLAG_HIDDEN);
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
