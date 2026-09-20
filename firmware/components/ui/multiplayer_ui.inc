// Included in ui.c: every LVGL operation stays on the existing UI owner.
static mp_state_t s_mp;
#include <stdlib.h>
#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#endif
static pixel_pet_art_t *s_mp_art;
static lv_obj_t *s_mp_images[2],*s_mp_ball,*s_mp_progress[10],*s_mp_again,*s_mp_medal;
static uint32_t s_mp_poll,s_mp_motion,s_mp_request,s_mp_reward_retry;
static bool s_mp_pending,s_mp_from_self,s_mp_reward_ok;
static uint64_t s_mp_saved_receipt;
static void mp_action(lv_event_t *e)
{
    int action=(int)(intptr_t)lv_event_get_user_data(e);bool ok=false;
    if(action>=0 && (unsigned)action<s_mp.count)ok=multiplayer_invite(s_mp.peers[action].user);
    else if(action==-1)ok=multiplayer_accept(s_mp.invite);
    else if(action==-2)ok=multiplayer_decline(s_mp.invite);
    else if(action==-3 && s_mp.my_turn && !s_mp_pending && lv_tick_get()-s_mp_motion>=850) {
        ok=multiplayer_pass(s_mp.room,s_mp.seq);
        if(ok){s_mp_pending=true;s_mp_request=lv_tick_get();}
    } else if(action==-4)ok=multiplayer_again(s_mp.room);
    if(!ok && s_hint)lv_label_set_text(s_hint,"One little moment. Try again soon.");
}
static Pet mp_other(void)
{
    Pet p={.stage=(uint8_t)s_mp.peer.stage};p.genes[GENE_PATTERN]=(uint8_t)(PET_CHARACTER_MARKER+s_mp.peer.character);return p;
}
static void mp_pair(void)
{
    Pet other=mp_other();pixel_pet_render(&s_mp_art[0],pet_state_get(),PIXEL_IDLE,0,PIXEL_APPLE);
    pixel_pet_render(&s_mp_art[1],&other,PIXEL_IDLE,0,PIXEL_APPLE);
    s_mp_images[0]=art(s_root,&s_mp_art[0].image,30,151,384);
    s_mp_images[1]=art(s_root,&s_mp_art[1].image,230,151,384);
    shape(s_root,16,121,132,26,CREAM,0);shape(s_root,220,121,132,26,CREAM,0);
    label(s_root,pet_state_get()->name,8,125,148,false);label(s_root,s_mp.peer.name,212,125,148,false);
}
static bool mp_game(mp_phase_t phase) {return phase==MP_PLAYING || phase==MP_WAITING || phase==MP_FINISHED;}
static void make_multiplayer(void)
{
    multiplayer_snapshot(&s_mp);s_mp_images[0]=s_mp_images[1]=NULL;s_mp_ball=NULL;s_mp_again=NULL;
    s_mp_pending=false;s_mp_motion=lv_tick_get();memset(s_mp_progress,0,sizeof s_mp_progress);
    if(!s_mp_art) {
#ifdef ESP_PLATFORM
        s_mp_art=heap_caps_calloc(2,sizeof(*s_mp_art),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#else
        s_mp_art=calloc(2,sizeof(*s_mp_art));
#endif
    }
    if(!s_mp_art) {header_to("Multiplayer",PLAY_MENU);s_hint=label(s_root,"A little hiccup. Please restart and try again.",24,180,320,false);return;}
    if(mp_game(s_mp.phase)) {
        room(false);header_to("Pass the ball",PLAY_MENU);mp_pair();
        shape(s_root,24,80,320,36,CREAM,0);
        for(unsigned i=0;i<10;i++) {
            s_mp_progress[i]=shape(s_root,47+(int)i*28,89,20,18,i<s_mp.seq?GOLD:0xe2e7df,0);
            lv_obj_set_style_border_width(s_mp_progress[i],1,0);lv_obj_set_style_border_color(s_mp_progress[i],lv_color_hex(INK),0);
        }
        s_mp_ball=button(s_root,32,273,96,82,CREAM,mp_action,-3);
        lv_obj_set_style_bg_opa(s_mp_ball,LV_OPA_TRANSP,0);lv_obj_set_style_border_width(s_mp_ball,0,0);
        art(s_mp_ball,pixel_icon(1),24,15,512);
        s_mp_medal=art(s_root,pixel_collectible(18),160,282,512);lv_obj_add_flag(s_mp_medal,LV_OBJ_FLAG_HIDDEN);
        s_hint=label(s_root,"Say hello to your friend!",20,361,328,false);
        s_mp_again=button(s_root,69,395,230,44,MINT,mp_action,-4);label(s_mp_again,"Play again",0,14,230,false);
        lv_obj_add_flag(s_mp_again,LV_OBJ_FLAG_HIDDEN);
    } else if(s_mp.phase==MP_INCOMING || s_mp.phase==MP_OUTGOING) {
        header_to("Play together?",PLAY_MENU);mp_pair();
        s_hint=label(s_root,s_mp.phase==MP_INCOMING?"Your friend wants to play!":"Waiting for your friend...",20,271,328,false);
        if(s_mp.phase==MP_INCOMING) {
            lv_obj_t *yes=button(s_root,54,308,260,60,MINT,mp_action,-1);label(yes,"Play together!",0,21,260,true);
        }
        lv_obj_t *no=button(s_root,84,388,200,48,CREAM,mp_action,-2);label(no,s_mp.phase==MP_INCOMING?"Not now":"Cancel invitation",0,16,200,false);
    } else {
        header_to("Multiplayer",PLAY_MENU);
        char title[64];snprintf(title,sizeof title,"%s's playdates",pet_state_get()->name);label(s_root,title,24,87,320,false);
        pixel_pet_render(&s_mp_art[0],pet_state_get(),PIXEL_IDLE,0,PIXEL_APPLE);
        s_mp_images[0]=art(s_root,&s_mp_art[0].image,139,112,320);
        s_hint=label(s_root,s_mp.connected?(s_mp.notice[0]?s_mp.notice:s_mp.count?"Choose a friend!":"Looking for a friend..."):"Connecting to your home...",20,215,328,false);
        if(s_mp.connected && s_mp.count) {
            lv_obj_t *list=shape(s_root,24,254,320,184,CREAM,0);lv_obj_add_flag(list,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(list,LV_DIR_VER);
            for(unsigned i=0;i<s_mp.count;i++) {
                lv_obj_t *b=button(list,0,(int)i*70,316,62,(i%2)?PINK:MINT,mp_action,(int)i);
                friends_icon(b,10,8);label(b,s_mp.peers[i].name,69,12,237,true);label(b,"Tap to invite",69,37,237,false);
            }
        } else {
            label(s_root,s_mp.connected?"Open Multiplayer on your other pet.":"Check Wi-Fi in Options if this takes a while.",30,274,308,false);
            label(s_root,"Both pets need the same home Wi-Fi.",30,337,308,false);
        }
    }
}
static void multiplayer_frame(uint32_t now)
{
    if(now-s_mp_poll>=100) {
        s_mp_poll=now;mp_state_t next;multiplayer_snapshot(&next);
        if(next.reward_id && next.reward_pet==pet_state_get()->pet_id && now-s_mp_reward_retry>=500) {
            s_mp_reward_retry=now;
            s_mp_reward_ok=pet_state_playdate(next.reward_id,next.reward_pet,next.reward_friend,next.friends);
            if(s_mp_reward_ok) {
                multiplayer_ack(next.reward_id,next.reward_pet);
                if(s_mp_saved_receipt!=next.reward_id) {s_mp_saved_receipt=next.reward_id;if(s_view==MULTIPLAYER)audio_play(SFX_STICKER);refresh();}
            }
        }
        if(s_view!=MULTIPLAYER)return;
        bool redraw=mp_game(next.phase)!=mp_game(s_mp.phase) || strcmp(next.room,s_mp.room) ||
            (!mp_game(next.phase) && (next.phase!=s_mp.phase || next.count!=s_mp.count || strcmp(next.notice,s_mp.notice) ||
             strcmp(next.invite,s_mp.invite) || memcmp(next.peers,s_mp.peers,sizeof next.peers)));
        if(redraw) {show(MULTIPLAYER);return;}
        if(next.seq!=s_mp.seq) {
            s_mp_motion=now;s_mp_from_self=!next.my_turn;
            // The last pass belongs to player 2, whose turn was set in the prior snapshot.
            if(next.seq==10)s_mp_from_self=s_mp.my_turn;
            s_mp_pending=false;audio_play(SFX_BOUNCE);
        }
        if(next.phase!=s_mp.phase)s_mp_pending=false;
        s_mp=next;
    }
    if(s_view!=MULTIPLAYER)return;
    bool flying=s_mp.seq>0 && now-s_mp_motion<720 && s_mp.phase!=MP_WAITING;
    if(s_mp_pending && now-s_mp_request>3000)s_mp_pending=false;
    for(unsigned i=0;i<2;i++)if(s_mp_images[i]) {
        Pet other=mp_other();const Pet *pet=i?&other:pet_state_get();
        pixel_face_t face=s_mp.phase==MP_FINISHED && !flying?PIXEL_HAPPY:flying?PIXEL_PLAY:now%4400>4200?PIXEL_BLINK:PIXEL_IDLE;
        pixel_pet_render(&s_mp_art[i],pet,face,flying?(i?0:3):now/180,PIXEL_APPLE);lv_obj_invalidate(s_mp_images[i]);
    }
    if(!s_mp_ball)return;
    for(unsigned i=0;i<10;i++)lv_obj_set_style_bg_color(s_mp_progress[i],lv_color_hex(i<s_mp.seq?GOLD:0xe2e7df),0);
    if(s_mp.phase==MP_FINISHED && !flying) {
        lv_obj_remove_flag(s_mp_medal,LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_mp_ball,LV_OBJ_FLAG_HIDDEN);lv_obj_remove_flag(s_mp_again,LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(lv_obj_get_child(s_mp_again,0),s_mp.again?"Waiting for your friend":"Play again");
        lv_label_set_text(s_hint,s_mp.reward_id && s_mp_saved_receipt!=s_mp.reward_id?"Saving your star...":"Hooray! A star each and happy buddies!");
        return;
    }
    lv_obj_add_flag(s_mp_medal,LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_mp_again,LV_OBJ_FLAG_HIDDEN);lv_obj_remove_flag(s_mp_ball,LV_OBJ_FLAG_HIDDEN);
    if(flying) {
        float t=(now-s_mp_motion)/720.0f;int from=s_mp_from_self?32:240,to=s_mp_from_self?240:32;
        lv_obj_set_pos(s_mp_ball,from+(int)((to-from)*t),273-(int)(sinf(t*3.14159265f)*90));
    } else lv_obj_set_pos(s_mp_ball,s_mp.my_turn?32:240,273);
    bool enabled=s_mp.phase==MP_PLAYING && s_mp.my_turn && !flying && !s_mp_pending && now-s_mp_motion>=850;
    if(enabled)lv_obj_add_flag(s_mp_ball,LV_OBJ_FLAG_CLICKABLE);else lv_obj_remove_flag(s_mp_ball,LV_OBJ_FLAG_CLICKABLE);
    lv_label_set_text(s_hint,s_mp.phase==MP_WAITING?"Your friend is reconnecting...":flying?"Wheee! Catch the ball!":s_mp_pending?"Here comes the ball!":s_mp.my_turn?"Your turn! Tap the ball.":"Your friend's turn. Get ready!");
}
