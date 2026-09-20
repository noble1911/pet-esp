// Included in ui.c. One compact simulation and one LVGL tree, reused across games.
static arcade_kind_t s_arc_kind;
static arcade_t s_arc;
static bool s_arc_vs,s_arc_started,s_arc_completed,s_arc_saved,s_arc_touch,s_arc_have_sensor;
static unsigned s_arc_best,s_arc_cue;
static bool s_arc_recorded,s_arc_rewarded;
static lv_obj_t *s_arc_burst,*s_arc_bonus,*s_arc_mascot;
static uint32_t s_arc_pose_at;
static unsigned s_arc_level_seen;
static uint32_t s_arc_tick,s_arc_poll,s_arc_send;
static float s_arc_zero_x,s_arc_zero_y,s_arc_ax,s_arc_ay,s_arc_touch_x,s_arc_touch_y;
static lv_obj_t *s_arc_field,*s_arc_status,*s_arc_note,*s_arc_go,*s_arc_go_text,*s_arc_mode_text;
static lv_obj_t *s_arc_pieces[33],*s_arc_icons[12],*s_arc_ball,*s_arc_bucket,*s_arc_star,*s_arc_dots[15],*s_arc_pet,*s_arc_result;
static mp_state_t s_arc_net;
static void make_arcade(const mp_state_t *mp);
static void arcade_frame(uint32_t now,const mp_state_t *mp);
static void arcade_menu_cb(lv_event_t *e) {s_arc_kind=(arcade_kind_t)(intptr_t)lv_event_get_user_data(e);show(ARCADE);}
static void arc_round(lv_obj_t *o,unsigned radius) {lv_obj_set_style_radius(o,radius,0);}
static void arc_hidden(lv_obj_t *o,bool hidden) {if(hidden)lv_obj_add_flag(o,LV_OBJ_FLAG_HIDDEN);else lv_obj_remove_flag(o,LV_OBJ_FLAG_HIDDEN);}
static void arc_center(void)
{
    float x,y;s_arc_have_sensor=motion_read(&x,&y);
    if(s_arc_have_sensor){s_arc_zero_x=x;s_arc_zero_y=y;s_arc_ax=s_arc_ay=0;}
}
static void arc_mode_cb(lv_event_t *e)
{
    (void)e;s_arc_touch=!s_arc_touch;arc_center();
    if(!s_arc_have_sensor)s_arc_touch=true;
    lv_label_set_text(s_arc_mode_text,s_arc_touch?"Use tilt":"Use touch");
}
static bool arc_persist(void)
{
    if(!s_arc_recorded)s_arc_recorded=arcade_save_best(pet_state_get()->pet_id,s_arc_kind,s_arc.score);
    if(!s_arc_vs && !s_arc_rewarded)s_arc_rewarded=pet_state_game_reward();
    return s_arc_recorded && (s_arc_vs || s_arc_rewarded);
}
static void arc_go_cb(lv_event_t *e)
{
    (void)e;
    if(s_arc.done || (s_arc_vs && s_arc_net.phase==MP_FINISHED)) {
        if(s_arc_vs && s_arc_net.phase!=MP_FINISHED)return;
        if(!s_arc_saved){s_arc_saved=arc_persist();if(!s_arc_saved)return;}
        if(s_arc_vs)multiplayer_again(s_arc_net.room);else show(ARCADE);
        return;
    }
    if(s_arc.level_clear) {
        if(!arcade_save_best(pet_state_get()->pet_id,s_arc_kind,s_arc.score)) {
            lv_label_set_text(s_arc_note,"Could not save. Tap Next to retry.");return;
        }
        if(s_arc.score>s_arc_best)s_arc_best=s_arc.score;
        arcade_next_level(&s_arc);s_arc_tick=lv_tick_get();s_arc_touch_x=s_arc_touch_y=0;
        return;
    }
    if(!s_arc_started) {
        arc_center();
        if(s_arc_vs)multiplayer_ready(s_arc_net.room);
        else {s_arc_started=true;s_arc_tick=lv_tick_get();}
        return;
    }
    if(s_arc_kind==ARCADE_PEGS && arcade_fire(&s_arc))audio_play(SFX_BOUNCE);
    if(s_arc_kind==ARCADE_TILT)arc_center();
}
static void arc_pick_cb(lv_event_t *e)
{
    if(!s_arc_started || s_arc.done)return;
    unsigned i=(unsigned)(intptr_t)lv_event_get_user_data(e);
    if(s_arc_vs && s_arc_kind==ARCADE_MEMORY) {
        if(s_arc_net.my_turn && lv_tick_get()-s_arc_send>250 && multiplayer_flip(s_arc_net.room,s_arc_net.seq,i)){s_arc_send=lv_tick_get();audio_play(SFX_SELECT);}
    } else arcade_pick(&s_arc,i);
}
static void arc_touch_cb(lv_event_t *e)
{
    lv_indev_t *input=lv_indev_active();if(!input || !s_arc_started || s_arc.done)return;
    lv_point_t p;lv_indev_get_point(input,&p);lv_area_t area;lv_obj_get_coords(s_arc_field,&area);
    if(s_arc_kind==ARCADE_PEGS)arcade_aim(&s_arc,p.x-area.x1,p.y-area.y1);
    if(s_arc_kind==ARCADE_TILT && s_arc_touch){
        s_arc_touch_x=(p.x-area.x1-s_arc.tilt.x)/65.f;s_arc_touch_y=(p.y-area.y1-s_arc.tilt.y)/65.f;
        if(lv_event_get_code(e)==LV_EVENT_RELEASED || lv_event_get_code(e)==LV_EVENT_PRESS_LOST)s_arc_touch_x=s_arc_touch_y=0;
    }
}
static const lv_image_dsc_t *arc_card_image(unsigned i) {return pixel_collectible((unsigned[]){0,4,7,10,14,17}[i%6]);}
// One scalable painted cup, shared by the menu and the moving cups.
static lv_obj_t *arc_cup_part(lv_obj_t *p,int scale,int x,int y,int w,int h,uint32_t color,int radius)
{
    lv_obj_t *o=shape(p,x*scale/80,y*scale/80,(w*scale+79)/80,(h*scale+79)/80,color,0);
    arc_round(o,radius*scale/80);return o;
}
static void arc_cup_paint(lv_obj_t *p,int scale)
{
    arc_cup_part(p,scale,7,78,66,10,0xd8b7a5,5);
    for(int y=14;y<76;y+=4){
        int left=18-(y-14)/6,width=80-2*left;
        arc_cup_part(p,scale,left,y,width,5,INK,0);
        arc_cup_part(p,scale,left+3,y,width-6,5,0xd97479,0);
        arc_cup_part(p,scale,left+5,y,7,5,0xffb9a4,0);
        arc_cup_part(p,scale,left+width-12,y,7,5,0xb9566e,0);
    }
    arc_cup_part(p,scale,17,8,46,15,INK,7);
    arc_cup_part(p,scale,20,11,40,8,0xffcbb0,4);
    arc_cup_part(p,scale,3,72,74,14,INK,6);
    arc_cup_part(p,scale,6,74,68,8,0xf3958d,4);
    arc_cup_part(p,scale,9,74,62,3,0xffdab6,1);
    // Tiny cream heart painted onto each identical cup.
    arc_cup_part(p,scale,31,38,8,9,0xffedcb,4);
    arc_cup_part(p,scale,40,38,8,9,0xffedcb,4);
    arc_cup_part(p,scale,34,44,11,7,0xffedcb,0);
    arc_cup_part(p,scale,37,51,5,3,0xffedcb,0);
}
static void arc_cup_icon(lv_obj_t *parent)
{
    lv_obj_t *icon=shape(parent,12,10,46,54,CREAM,0);
    lv_obj_set_style_bg_opa(icon,LV_OPA_TRANSP,0);arc_cup_paint(icon,46);
}
static void make_arcade(const mp_state_t *mp)
{
    s_arc_vs=mp!=NULL;if(mp){s_arc_net=*mp;s_arc_kind=mp->mode==MP_PEGS?ARCADE_PEGS:mp->mode==MP_TILT?ARCADE_TILT:ARCADE_MEMORY;}
    arcade_start(&s_arc,s_arc_kind,mp?mp->seed:esp_random());
    if(mp && s_arc_kind!=ARCADE_MEMORY)s_arc.run_limit_ms=180000;
    s_arc_level_seen=0;
    s_arc_started=s_arc_completed=false;s_arc_saved=true;s_arc_recorded=s_arc_rewarded=false;s_arc_cue=0;s_arc_tick=lv_tick_get();s_arc_send=0;
    s_arc_best=arcade_best(pet_state_get()->pet_id,s_arc_kind);s_arc_touch=false;s_arc_touch_x=s_arc_touch_y=0;
    memset(s_arc_pieces,0,sizeof s_arc_pieces);memset(s_arc_icons,0,sizeof s_arc_icons);
    s_arc_ball=s_arc_bucket=s_arc_star=s_arc_pet=s_arc_mode_text=NULL;
    header_to(arcade_names[s_arc_kind],s_arc_vs?PLAY_MENU:GAMES);
    pixel_pet_render(&s_pet_art,pet_state_get(),PIXEL_PLAY,0,PIXEL_APPLE);
    s_arc_mascot=art(s_root,&s_pet_art.image,20,68,128);
    s_arc_status=label(s_root,"",60,81,290,false);
    s_arc_field=shape(s_root,14,108,340,256,s_arc_kind==ARCADE_TILT?0xe0efc5:s_arc_kind==ARCADE_PEGS?0xe2f0f5:0xffecd5,8);
    arc_round(s_arc_field,8);lv_obj_set_style_border_width(s_arc_field,2,0);lv_obj_set_style_border_color(s_arc_field,lv_color_hex(INK),0);
    if(s_arc_kind==ARCADE_PEGS || s_arc_kind==ARCADE_TILT){
        lv_obj_add_flag(s_arc_field,LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(s_arc_field,arc_touch_cb,LV_EVENT_PRESSED,NULL);
        lv_obj_add_event_cb(s_arc_field,arc_touch_cb,LV_EVENT_PRESSING,NULL);
        lv_obj_add_event_cb(s_arc_field,arc_touch_cb,LV_EVENT_RELEASED,NULL);
        lv_obj_add_event_cb(s_arc_field,arc_touch_cb,LV_EVENT_PRESS_LOST,NULL);
    }
    if(s_arc_kind==ARCADE_CUPS){
        pixel_pet_render(&s_pet_art,pet_state_get(),PIXEL_HAPPY,0,PIXEL_APPLE);
        s_arc_pet=art(s_arc_field,&s_pet_art.image,22,80,256);
        for(unsigned i=0;i<3;i++){
            lv_obj_t *cup=button(s_arc_field,20+i*110,128,80,90,CREAM,arc_pick_cb,i);s_arc_pieces[i]=cup;
            lv_obj_set_style_bg_opa(cup,LV_OPA_TRANSP,0);lv_obj_set_style_border_width(cup,0,0);arc_cup_paint(cup,80);
        }
    } else if(s_arc_kind==ARCADE_PEGS){
        for(unsigned i=0;i<33;i++){float x,y;arcade_peg_pos(&s_arc,i,&x,&y);s_arc_pieces[i]=shape(s_arc_field,x-8,y-8,16,16,arcade_peg_gold(&s_arc,i)?0xf49a59:0x709bbb,0);arc_round(s_arc_pieces[i],8);shape(s_arc_pieces[i],4,3,5,4,CREAM,0);}
        for(unsigned i=0;i<15;i++){s_arc_dots[i]=shape(s_arc_field,0,0,3,3,INK,0);arc_round(s_arc_dots[i],2);}
        s_arc_bucket=shape(s_arc_field,138,242,64,10,0xe494a5,0);shape(s_arc_bucket,4,0,56,3,CREAM,0);
        s_arc_ball=shape(s_arc_field,164,6,12,12,CREAM,0);arc_round(s_arc_ball,6);lv_obj_set_style_border_width(s_arc_ball,2,0);lv_obj_set_style_border_color(s_arc_ball,lv_color_hex(INK),0);
    } else if(s_arc_kind==ARCADE_TILT){
        for(unsigned i=0;i<6;i++) {lv_obj_t *rock=shape(s_arc_field,0,0,36,36,0x9ca39a,0);s_arc_pieces[i]=rock;arc_round(rock,13);shape(rock,9,6,17,6,0xc8cec0,0);}
        s_arc_star=art(s_arc_field,pixel_icon(4),0,0,320);
        s_arc_ball=shape(s_arc_field,160,118,20,20,0xe795af,0);arc_round(s_arc_ball,10);shape(s_arc_ball,5,3,7,5,0xffeef3,0);
        arc_center();s_arc_touch=!s_arc_have_sensor;
        lv_obj_t *mode=button(s_root,192,397,152,44,CREAM,arc_mode_cb,0);
        s_arc_mode_text=label(mode,s_arc_touch?"Use tilt":"Use touch",0,14,152,false);
    } else {
        for(unsigned i=0;i<12;i++){
            lv_obj_t *card=button(s_arc_field,10+(i%4)*82,12+(i/4)*81,74,71,0xa9cad8,arc_pick_cb,i);s_arc_pieces[i]=card;arc_round(card,8);
            label(card,"?",0,22,74,true);s_arc_icons[i]=art(card,arc_card_image(s_arc.memory.deck[i]),13,11,512);
            arc_hidden(s_arc_icons[i],true);
        }
    }
    s_arc_burst=shape(s_arc_field,0,0,20,20,GOLD,0);arc_round(s_arc_burst,LV_RADIUS_CIRCLE);
    lv_obj_set_style_bg_opa(s_arc_burst,LV_OPA_TRANSP,0);lv_obj_set_style_border_width(s_arc_burst,2,0);lv_obj_set_style_border_color(s_arc_burst,lv_color_hex(0xe3a23c),0);
    s_arc_bonus=label(s_arc_field,"",0,0,66,false);arc_hidden(s_arc_burst,true);arc_hidden(s_arc_bonus,true);
    s_arc_result=shape(s_arc_field,15,45,310,160,CREAM,0);arc_round(s_arc_result,10);lv_obj_set_style_border_width(s_arc_result,2,0);
    label(s_arc_result,s_arc_vs?"Ready, friends?":"Let's play!",8,20,294,true);
    const char *tips[]={"Watch where I hide.\nFollow the moving cups.\nThree chances. No hurry!","Drag left or right to aim.\nClear ALL orange pegs!\nBucket catches give a ball.","Hold the toy comfortably.\nTilt gently to roll the marble.\nCatch every star in 45 seconds.","Tap two pictures. Find a pair!\nRemember where they hide.\nNo rush in solo play."};
    label(s_arc_result,tips[s_arc_kind],10,62,290,false);
    s_arc_note=label(s_root,"",12,373,344,false);
    s_arc_go=button(s_root,s_arc_kind==ARCADE_TILT?24:54,397,s_arc_kind==ARCADE_TILT?152:260,44,MINT,arc_go_cb,0);s_arc_go_text=label(s_arc_go,s_arc_vs?"Ready!":"Let's play!",0,13,s_arc_kind==ARCADE_TILT?152:260,s_arc_kind!=ARCADE_TILT);
}
static void arcade_frame(uint32_t now,const mp_state_t *mp)
{
    if(mp){if(mp->matched!=s_arc_net.matched)audio_play(SFX_FOUND);s_arc_net=*mp;}
    if(s_arc_vs && !s_arc_started && s_arc_net.started && !s_arc_net.countdown_ms){s_arc_started=true;s_arc_tick=now;arc_center();}
    unsigned dt=now-s_arc_tick;s_arc_tick=now;if(dt>100)dt=100;
    if(s_arc_started && !s_arc.done && (!s_arc_vs || s_arc_net.phase==MP_PLAYING)){
        if(s_arc_kind==ARCADE_TILT && !s_arc_touch && now-s_arc_poll>=30){
            s_arc_poll=now;float x,y;
            if(motion_read(&x,&y)){float dx=x-s_arc_zero_x,dy=y-s_arc_zero_y;if(fabsf(dx)<.025f)dx=0;if(fabsf(dy)<.025f)dy=0;s_arc_ax=.75f*s_arc_ax+.25f*dx*3.2f;s_arc_ay=.75f*s_arc_ay+.25f*dy*3.2f;}
            else {s_arc_touch=true;s_arc_touch_x=s_arc_touch_y=0;lv_label_set_text(s_arc_mode_text,"Use tilt");}
        }
        if(!(s_arc_vs && s_arc_kind==ARCADE_MEMORY))arcade_tick(&s_arc,dt,s_arc_touch?s_arc_touch_x:s_arc_ax,s_arc_touch?s_arc_touch_y:s_arc_ay);
    }
    if(s_arc.cue!=s_arc_cue){audio_play(s_arc_kind==ARCADE_PEGS?SFX_BOUNCE:s_arc_kind==ARCADE_TILT?SFX_STAR:s_arc_kind==ARCADE_CUPS && s_arc.cups.picked!=0?SFX_HIDE:SFX_FOUND);s_arc_cue=s_arc.cue;}
    if(s_arc.done && !s_arc_completed){
        s_arc_completed=true;
        s_arc_saved=arc_persist();
        if(s_arc_saved && s_arc.score>s_arc_best)s_arc_best=s_arc.score;
        if(!s_arc_vs && s_arc_rewarded)audio_play(SFX_HAPPY);
    }
    if(s_arc_vs && s_arc.done && !s_arc_net.submitted && s_arc_net.phase==MP_PLAYING && now-s_arc_send>1000){
        if(multiplayer_score(s_arc_net.room,s_arc.score))s_arc_send=now;
    }
    if(s_arc_level_seen!=s_arc.level){
        s_arc_level_seen=s_arc.level;
        if(s_arc_kind==ARCADE_PEGS)for(unsigned i=0;i<33;i++){
            float x,y;arcade_peg_pos(&s_arc,i,&x,&y);lv_obj_set_pos(s_arc_pieces[i],x-8,y-8);
            lv_obj_set_style_bg_color(s_arc_pieces[i],lv_color_hex(arcade_peg_gold(&s_arc,i)?0xf49a59:0x709bbb),0);
        }
        if(s_arc_kind==ARCADE_TILT)for(unsigned i=0;i<6;i++){
            arc_hidden(s_arc_pieces[i],i>=s_arc.tilt.rock_count);
            if(i<s_arc.tilt.rock_count)lv_obj_set_pos(s_arc_pieces[i],s_arc.tilt.rx[i]-18,s_arc.tilt.ry[i]-18);
        }
    }
    bool end=s_arc_vs?s_arc_net.phase==MP_FINISHED:s_arc.done;
    bool waiting=s_arc_vs && s_arc.done && !end;
    arc_hidden(s_arc_result,s_arc_started && !end && !waiting && !s_arc.level_clear);
    if(!s_arc_started){
        lv_label_set_text(s_arc_note,s_arc_vs?(s_arc_net.countdown_ms?"Ready, steady...":s_arc_net.ready?"Waiting for your friend...":"Both players tap Ready to start") : "Press Let's play when you're ready");
        if(s_arc_vs)lv_label_set_text(s_arc_go_text,s_arc_net.ready?"Waiting...":"Ready!");
    } else if(end || waiting){
        char title[64],detail[128];
        if(s_arc_vs){
            snprintf(title,sizeof title,"%s",waiting?"All done!":s_arc_net.score==s_arc_net.peer_score?"A happy tie!":s_arc_net.score>s_arc_net.peer_score?"You win!":"Well played!");
            snprintf(detail,sizeof detail,"You: %u   %s: %u\n%s",s_arc_net.submitted?s_arc_net.score:s_arc.score,s_arc_net.peer.name,s_arc_net.peer_score,waiting?"Waiting for your friend...":s_arc_net.notice[0]?s_arc_net.notice:"A care star for each of you!");
        } else {snprintf(title,sizeof title,"%s",s_arc.score>=s_arc_best && s_arc.score?"Lovely score!":"Nice playing!");snprintf(detail,sizeof detail,"%s: %u\nBest: %u\n%s",s_arc_kind==ARCADE_CUPS?"Best streak":"Score",s_arc.score,s_arc_best,s_arc_saved?"Another little adventure?":"Could not save. Tap to retry.");}
        lv_label_set_text(lv_obj_get_child(s_arc_result,0),title);lv_label_set_text(lv_obj_get_child(s_arc_result,1),detail);
        lv_label_set_text(s_arc_go_text,!s_arc_saved?"Retry save":waiting?"Waiting...":s_arc_vs && s_arc_net.again?"Waiting...":"Play again");
        lv_label_set_text(s_arc_note,end?"That was fun!":"Your friend's round is still going");
    }
    if(s_arc.level_clear && !end && !waiting){
        char title[48],detail[96];snprintf(title,sizeof title,"%s %u clear!",s_arc_kind==ARCADE_TILT?"Garden":"Level",s_arc.level);
        snprintf(detail,sizeof detail,"Score: %u\nYour score comes with you!\n%s",s_arc.score,s_arc_vs?"Three-minute challenge":"Ready for the next one?");
        lv_label_set_text(lv_obj_get_child(s_arc_result,0),title);lv_label_set_text(lv_obj_get_child(s_arc_result,1),detail);
        lv_label_set_text(s_arc_go_text,s_arc_kind==ARCADE_TILT?"Next garden":"Next level");
        lv_label_set_text(s_arc_note,s_arc_kind==ARCADE_TILT?"All stars caught! A new garden awaits":"All orange pegs cleared! Well done!");
    }
    if(now-s_arc_pose_at>=200){
        s_arc_pose_at=now;pixel_pet_render(&s_pet_art,pet_state_get(),(end || s_arc.level_clear)?PIXEL_HAPPY:s_arc.burst_ms?PIXEL_PLAY:now%4200>4000?PIXEL_BLINK:PIXEL_IDLE,now/180,PIXEL_APPLE);
        lv_obj_invalidate(s_arc_mascot);if(s_arc_pet)lv_obj_invalidate(s_arc_pet);
    }
    bool burst=s_arc.burst_ms>0 && !end && !s_arc.level_clear;
    arc_hidden(s_arc_burst,!burst);arc_hidden(s_arc_bonus,!burst);
    if(burst){
        int spread=(450-s_arc.burst_ms)/15,diameter=20+spread;
        lv_obj_set_size(s_arc_burst,diameter,diameter);lv_obj_set_pos(s_arc_burst,s_arc.burst_x-diameter/2,s_arc.burst_y-diameter/2);
        lv_obj_set_style_opa(s_arc_burst,(lv_opa_t)(s_arc.burst_ms*255/450),0);
        char points[16];snprintf(points,sizeof points,"+%u",s_arc.burst_points);lv_label_set_text(s_arc_bonus,points);
        lv_obj_set_pos(s_arc_bonus,s_arc.burst_x-33,s_arc.burst_y-25-spread/2);
    }
    char status[96];
    if(s_arc_vs)snprintf(status,sizeof status,"You %u   %s %u",s_arc_kind==ARCADE_MEMORY?s_arc_net.score:s_arc.score,s_arc_net.peer.name,s_arc_net.peer_score);
    else snprintf(status,sizeof status,"%s %u     Best %u",s_arc_kind==ARCADE_CUPS?"Streak":"Score",s_arc_kind==ARCADE_CUPS?s_arc.cups.streak:s_arc.score,s_arc_best);
    lv_label_set_text(s_arc_status,status);
    if(s_arc_kind==ARCADE_CUPS){
        for(unsigned i=0;i<3;i++){float x,y;arcade_cup_pos(&s_arc,i,&x,&y);if(i==0 && (s_arc.cups.phase==ARC_SHOW || s_arc.cups.phase==ARC_REVEAL))y+=25;lv_obj_set_pos(s_arc_pieces[i],x,y);}
        float x,y;arcade_cup_pos(&s_arc,0,&x,&y);lv_obj_set_pos(s_arc_pet,x+4,82);arc_hidden(s_arc_pet,s_arc.cups.phase!=ARC_SHOW && s_arc.cups.phase!=ARC_REVEAL);
        if(s_arc_started && !end){
            char hint[96];snprintf(hint,sizeof hint,"%s   %u hearts",s_arc.cups.phase==ARC_SHOW?"Here I am!":s_arc.cups.phase==ARC_SHUFFLE?"Watch the cups...":s_arc.cups.phase==ARC_PICK?"Which cup am I under?":s_arc.cups.picked==0?"Peekaboo! Well spotted!":"Here I was! Try again!",s_arc.cups.lives);
            lv_label_set_text(s_arc_note,hint);lv_label_set_text(s_arc_go_text,s_arc.cups.phase==ARC_PICK?"Tap a cup above":"Keep watching!");
        }
    } else if(s_arc_kind==ARCADE_PEGS){
        for(unsigned i=0;i<33;i++)arc_hidden(s_arc_pieces[i],!(s_arc.pegs.live&(UINT64_C(1)<<i)));
        lv_obj_set_pos(s_arc_ball,s_arc.pegs.flying?s_arc.pegs.x-6:164,s_arc.pegs.flying?s_arc.pegs.y-6:6);
        lv_obj_set_x(s_arc_bucket,s_arc.pegs.bucket-32);
        float px=170,py=12,vx=sinf(s_arc.pegs.aim)*250,vy=cosf(s_arc.pegs.aim)*250;
        for(unsigned i=0;i<15;i++){
            for(unsigned step=0;step<6;step++){
                vy+=3.5f;px+=vx*.01f;py+=vy*.01f;
                if(px<9 || px>331){px=px<9?9:331;vx=-vx*.92f;}
            }
            lv_obj_set_pos(s_arc_dots[i],px-1,py-1);
            arc_hidden(s_arc_dots[i],s_arc.pegs.flying || s_arc.done || s_arc.level_clear);
        }
        if(s_arc_started && !end && !waiting && !s_arc.level_clear){char b[80];snprintf(b,sizeof b,"Level %u | %u balls | %u orange",s_arc.level,s_arc.pegs.balls,arcade_orange_left(&s_arc));if(s_arc.pegs.bonus_ms)snprintf(b,sizeof b,"Bucket catch! +1 ball!  (%u balls)",s_arc.pegs.balls);lv_label_set_text(s_arc_note,b);lv_label_set_text(s_arc_go_text,s_arc.pegs.flying?"Boing, boing!":"Launch!");}
    } else if(s_arc_kind==ARCADE_TILT){
        lv_obj_set_pos(s_arc_ball,s_arc.tilt.x-10,s_arc.tilt.y-10);lv_obj_set_pos(s_arc_star,s_arc.tilt.star_x-15,s_arc.tilt.star_y-15);
        if(s_arc_started && !end && !waiting && !s_arc.level_clear){char b[80];snprintf(b,sizeof b,"Garden %u | %u/%u stars | %us",s_arc.level,s_arc.tilt.stars,s_arc.tilt.goal,(unsigned)((45000-s_arc.level_ms+999)/1000));lv_label_set_text(s_arc_note,b);lv_label_set_text(s_arc_go_text,"Centre tilt");}
    } else {
        for(unsigned i=0;i<12;i++){
            bool matched=s_arc_vs?(s_arc_net.matched&(1u<<i)):(s_arc.memory.matched&(1u<<i));
            int value=s_arc_vs?s_arc_net.cards[i]:matched || s_arc.memory.first==(int)i || s_arc.memory.second==(int)i?s_arc.memory.deck[i]:-1;
            arc_hidden(s_arc_icons[i],value<0);arc_hidden(lv_obj_get_child(s_arc_pieces[i],0),value>=0);
            if(value>=0)lv_image_set_src(s_arc_icons[i],arc_card_image((unsigned)value));
            lv_obj_set_style_bg_color(s_arc_pieces[i],lv_color_hex(matched?MINT:value>=0?CREAM:0xa9cad8),0);
        }
        if(s_arc_started && !end){char b[96];snprintf(b,sizeof b,"%u pairs found  |  %u turns",s_arc.memory.pairs,s_arc.memory.moves);
            if(s_arc_vs)snprintf(b,sizeof b,"%s",s_arc_net.my_turn?"Your turn! Turn two cards":"Watch your friend's turn");
            lv_label_set_text(s_arc_note,b);lv_label_set_text(s_arc_go_text,"Tap two cards above");}
    }
}
