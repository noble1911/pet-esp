#include "arcade.h"
#include <math.h>
#include <string.h>
const char *const arcade_names[ARCADE_COUNT]={"Peekaboo Cups","Peg Bounce","Tilt Garden","Memory Pairs"};
static unsigned random_next(arcade_t *g) {g->rng^=g->rng<<13;g->rng^=g->rng>>17;g->rng^=g->rng<<5;return g->rng;}
static float clamp(float v,float lo,float hi) {return v<lo?lo:v>hi?hi:v;}
static void cup_swap(arcade_t *g) {g->cups.a=random_next(g)%3;g->cups.b=(g->cups.a+1+random_next(g)%2)%3;}
static void cup_round(arcade_t *g)
{
    g->cups.phase=ARC_SHOW;g->phase_ms=0;g->cups.picked=-1;g->cups.swaps=0;
    g->cups.goal=3+(g->cups.streak<10?g->cups.streak/2:5);
    g->cups.duration=650-(g->cups.streak<10?g->cups.streak:10)*30;
}
static void tilt_layout(arcade_t *g)
{
    unsigned difficulty=g->level<7?g->level:7;
    g->tilt.goal=4+difficulty;
    g->tilt.rock_count=3+(difficulty-1)/2;
    // One rock per roomy cell; seeded jitter and mirror make fresh gardens.
    static const unsigned cells[6][2]={{75,73},{255,173},{75,205},{255,67},{165,57},{169,216}};
    bool mirror=random_next(g)&1;
    for(unsigned i=0;i<g->tilt.rock_count;i++){
        unsigned x=cells[i][0]+random_next(g)%17-8;
        g->tilt.rx[i]=mirror?340-x:x;
        g->tilt.ry[i]=cells[i][1]+random_next(g)%13-6;
    }
}
static void peg_layout(arcade_t *g)
{
    unsigned layout=random_next(g)%3;
    float rotation=(random_next(g)%6283)/1000.f;
    for(unsigned p=0;p<33;p++) {
        float x,y;
        if(layout==2) {
            // Three concentric rings with room between pegs for the ball.
            unsigned ring=p<8?0:p<19?1:2;
            unsigned index=p-(ring==0?0:ring==1?8:19),count=ring==0?8:ring==1?11:14;
            float angle=rotation+index*6.2831853f/count;
            x=170+(ring==0?45:ring==1?90:140)*cosf(angle);
            y=128+(ring==0?34:ring==1?60:90)*sinf(angle);
        } else {
            unsigned row=0,col=p;
            while(col>=(row%2?5u:6u)){col-=row%2?5:6;row++;}
            x=45+col*50+(row%2?25:0);
            y=layout==0?52+row*31:42+row*29+fabsf(x-170)*.18f;
            x+=(int)(random_next(g)%13)-6;
            y+=(int)(random_next(g)%5)-2;
        }
        g->pegs.px[p]=(uint16_t)lroundf(x);g->pegs.py[p]=(uint16_t)lroundf(y);
    }
    // Exactly eleven gold pegs, chosen without replacement for every board.
    uint8_t order[33];for(unsigned p=0;p<33;p++)order[p]=p;
    for(unsigned p=0;p<11;p++){
        unsigned j=p+random_next(g)%(33-p);uint8_t t=order[p];order[p]=order[j];order[j]=t;
        g->pegs.gold|=UINT64_C(1)<<order[p];
    }
}
static void star_place(arcade_t *g)
{
    for(unsigned tries=0;tries<64;tries++) {
        float x=28+random_next(g)%285,y=28+random_next(g)%193;
        bool good=hypotf(x-g->tilt.star_x,y-g->tilt.star_y)>65;
        for(unsigned i=0;i<g->tilt.rock_count;i++)if(hypotf(x-g->tilt.rx[i],y-g->tilt.ry[i])<45)good=false;
        if(good){g->tilt.star_x=x;g->tilt.star_y=y;return;}
    }
    // Even the fallback follows the shared course, independent of player movement.
    g->tilt.star_x=g->tilt.star_x<170?302:38;g->tilt.star_y=36;
}
void arcade_start(arcade_t *g,arcade_kind_t kind,uint32_t seed)
{
    memset(g,0,sizeof *g);g->kind=kind;g->rng=seed?seed:1;g->level=1;
    if(kind==ARCADE_CUPS){g->cups.lives=3;for(unsigned i=0;i<3;i++)g->cups.slot[i]=i;cup_round(g);}
    if(kind==ARCADE_PEGS){g->pegs.balls=5;g->pegs.live=(UINT64_C(1)<<33)-1;g->pegs.x=g->pegs.bucket=170;g->pegs.y=12;peg_layout(g);}
    if(kind==ARCADE_TILT){g->tilt.x=170;g->tilt.y=128;g->tilt.star_x=170;g->tilt.star_y=128;tilt_layout(g);star_place(g);}
    if(kind==ARCADE_MEMORY){
        g->memory.first=g->memory.second=-1;
        for(unsigned i=0;i<12;i++)g->memory.deck[i]=i/2;
        for(unsigned i=11;i>0;i--){unsigned j=random_next(g)%(i+1);uint8_t t=g->memory.deck[i];g->memory.deck[i]=g->memory.deck[j];g->memory.deck[j]=t;}
    }
}
unsigned arcade_orange_left(const arcade_t *g)
{
    uint64_t remaining=g->pegs.live&g->pegs.gold;unsigned count=0;
    while(remaining){count++;remaining&=remaining-1;}return count;
}
bool arcade_next_level(arcade_t *g)
{
    if(!g->level_clear || g->done)return false;
    g->level++;g->level_ms=0;g->level_clear=false;g->burst_ms=0;
    if(g->kind==ARCADE_PEGS){
        unsigned balls=g->pegs.balls;memset(&g->pegs,0,sizeof g->pegs);
        g->pegs.balls=balls<5?5:balls;g->pegs.live=(UINT64_C(1)<<33)-1;
        g->pegs.x=g->pegs.bucket=170;g->pegs.y=12;peg_layout(g);
    } else if(g->kind==ARCADE_TILT){
        memset(&g->tilt,0,sizeof g->tilt);g->tilt.x=g->tilt.star_x=170;g->tilt.y=g->tilt.star_y=128;
        tilt_layout(g);star_place(g);
    } else return false;
    return true;
}
void arcade_peg_pos(const arcade_t *g,unsigned p,float *x,float *y)
{
    *x=g->pegs.px[p];*y=g->pegs.py[p];
}
bool arcade_peg_gold(const arcade_t *g,unsigned p) {return (g->pegs.gold&(UINT64_C(1)<<p))!=0;}
void arcade_cup_pos(const arcade_t *g,unsigned cup,float *x,float *y)
{
    *x=20+g->cups.slot[cup]*110;*y=128;
    if(g->cups.phase==ARC_SHUFFLE && (cup==g->cups.a || cup==g->cups.b)) {
        unsigned other=cup==g->cups.a?g->cups.b:g->cups.a;
        float t=clamp(g->phase_ms/(float)g->cups.duration,0,1),ease=t*t*(3-2*t);
        *x+=((int)g->cups.slot[other]-(int)g->cups.slot[cup])*110*ease;
        *y+=(cup==g->cups.a?-30:30)*sinf(t*3.14159265f);
    }
}
bool arcade_pick(arcade_t *g,unsigned i)
{
    if(g->done)return false;
    if(g->kind==ARCADE_CUPS && i<3 && g->cups.phase==ARC_PICK){
        g->cups.picked=(int)i;g->cups.phase=ARC_REVEAL;g->phase_ms=0;g->cue++;
        if(i==0){g->cups.streak++;if(g->cups.streak>g->cups.best)g->cups.best=g->cups.streak;g->score=g->cups.best;}
        else {g->cups.lives--;g->cups.streak=0;}
        return true;
    }
    if(g->kind==ARCADE_MEMORY && i<12 && g->memory.second<0 && !(g->memory.matched&(1u<<i)) && g->memory.first!=(int)i){
        if(g->memory.first<0)g->memory.first=(int)i;
        else {g->memory.second=(int)i;g->memory.moves++;g->phase_ms=0;}
        g->cue++;return true;
    }
    return false;
}
void arcade_aim(arcade_t *g,float x,float y) {if(g->kind==ARCADE_PEGS && !g->pegs.flying && !g->level_clear){(void)y;g->pegs.aim=clamp((x-170)/158.f*2.15f,-2.15f,2.15f);}}
bool arcade_fire(arcade_t *g)
{
    if(g->kind!=ARCADE_PEGS || g->done || g->level_clear || g->pegs.flying || !g->pegs.balls)return false;
    g->pegs.x=170;g->pegs.y=12;g->pegs.vx=sinf(g->pegs.aim)*250;g->pegs.vy=cosf(g->pegs.aim)*250;
    g->pegs.balls--;g->pegs.hits=g->pegs.shot_ms=0;g->pegs.flying=true;return true;
}
static void peg_tick(arcade_t *g,unsigned ms)
{
    float dt=ms/1000.f;g->pegs.bucket=170+sinf(g->elapsed/850.f)*112;
    if(!g->pegs.flying)return;
    g->pegs.shot_ms+=ms;g->pegs.vy+=350*dt;g->pegs.x+=g->pegs.vx*dt;g->pegs.y+=g->pegs.vy*dt;
    if(g->pegs.x<9 || g->pegs.x>331){g->pegs.x=clamp(g->pegs.x,9,331);g->pegs.vx=-g->pegs.vx*.92f;}
    // Shallow upward shots may arc above the board and return naturally.
    for(unsigned i=0;i<33;i++)if(g->pegs.live&(UINT64_C(1)<<i)){
        float x,y;arcade_peg_pos(g,i,&x,&y);float dx=g->pegs.x-x,dy=g->pegs.y-y,d=hypotf(dx,dy);
        if(d<14){
            if(d<.01f){dx=0;dy=-1;d=1;}dx/=d;dy/=d;
            g->pegs.x=x+dx*14;g->pegs.y=y+dy*14;
            float dot=g->pegs.vx*dx+g->pegs.vy*dy;
            if(dot<0){g->pegs.vx-=1.75f*dot*dx;g->pegs.vy-=1.75f*dot*dy;}
            g->pegs.live&=~(UINT64_C(1)<<i);g->pegs.hits++;g->cue++;
            g->burst_points=(arcade_peg_gold(g,i)?50:10)+(g->pegs.hits-1)*5;g->score+=g->burst_points;
            g->burst_x=x;g->burst_y=y;g->burst_ms=450;
        }
    }
    if(g->pegs.y>242 || g->pegs.shot_ms>=8000){
        if(g->pegs.y>242 && fabsf(g->pegs.x-g->pegs.bucket)<32){g->score+=100;g->cue++;g->pegs.catches++;g->pegs.balls++;g->pegs.bonus_ms=1600;}
        g->pegs.flying=false;
        if(!arcade_orange_left(g)){g->score+=g->pegs.balls*150;g->level_clear=true;g->cue++;}
        else if(!g->pegs.balls)g->done=true;
    }
}
static void tilt_tick(arcade_t *g,unsigned ms,float ax,float ay)
{
    float dt=ms/1000.f;
    g->tilt.vx=(g->tilt.vx+clamp(ax,-1,1)*420*dt)*expf(-2.8f*dt);
    g->tilt.vy=(g->tilt.vy+clamp(ay,-1,1)*420*dt)*expf(-2.8f*dt);
    g->tilt.x+=g->tilt.vx*dt;g->tilt.y+=g->tilt.vy*dt;
    if(g->tilt.x<12 || g->tilt.x>328){g->tilt.x=clamp(g->tilt.x,12,328);g->tilt.vx*=-.4f;}
    if(g->tilt.y<12 || g->tilt.y>244){g->tilt.y=clamp(g->tilt.y,12,244);g->tilt.vy*=-.4f;}
    for(unsigned i=0;i<g->tilt.rock_count;i++){
        float dx=g->tilt.x-g->tilt.rx[i],dy=g->tilt.y-g->tilt.ry[i],d=hypotf(dx,dy);
        if(d<29){if(d<.01f){dx=1;dy=0;d=1;}dx/=d;dy/=d;g->tilt.x=g->tilt.rx[i]+dx*29;g->tilt.y=g->tilt.ry[i]+dy*29;
            float dot=g->tilt.vx*dx+g->tilt.vy*dy;if(dot<0){g->tilt.vx-=1.4f*dot*dx;g->tilt.vy-=1.4f*dot*dy;}}
    }
    if(hypotf(g->tilt.x-g->tilt.star_x,g->tilt.y-g->tilt.star_y)<23){g->tilt.stars++;g->score+=100;g->cue++;g->burst_x=g->tilt.star_x;g->burst_y=g->tilt.star_y;g->burst_ms=450;g->burst_points=100;if(g->tilt.stars>=g->tilt.goal){g->level_clear=true;g->tilt.vx=g->tilt.vy=0;}else star_place(g);}
    if(g->level_ms>=45000 && !g->level_clear)g->done=true;
}
void arcade_tick(arcade_t *g,unsigned ms,float ax,float ay)
{
    if(g->done)return;
    // Deterministic small steps prevent tunnelling, independent of render cadence.
    while(ms && !g->done){unsigned step=ms>10?10:ms;ms-=step;g->elapsed+=step;
        if(g->run_limit_ms && g->elapsed>=g->run_limit_ms){g->done=true;g->level_clear=false;break;}
        if(g->level_clear)continue;
        g->level_ms+=step;g->phase_ms+=step;g->pegs.bonus_ms=g->pegs.bonus_ms>step?g->pegs.bonus_ms-step:0;g->burst_ms=g->burst_ms>step?g->burst_ms-step:0;
        if(g->kind==ARCADE_PEGS)peg_tick(g,step);
        else if(g->kind==ARCADE_TILT)tilt_tick(g,step,ax,ay);
        else if(g->kind==ARCADE_MEMORY && g->memory.second>=0 && g->phase_ms>=900){
            unsigned a=g->memory.first,b=g->memory.second;
            if(g->memory.deck[a]==g->memory.deck[b]){g->memory.matched|=(1u<<a)|(1u<<b);g->memory.pairs++;g->score=g->memory.pairs*100;g->cue++;}
            g->memory.first=g->memory.second=-1;
            if(g->memory.pairs==6){g->score=600+(g->memory.moves<18?600-(g->memory.moves-6)*50:0);g->done=true;}
        } else if(g->kind==ARCADE_CUPS){
            if(g->cups.phase==ARC_SHOW && g->phase_ms>=1600){g->cups.phase=ARC_SHUFFLE;g->phase_ms=0;cup_swap(g);}
            else if(g->cups.phase==ARC_SHUFFLE && g->phase_ms>=g->cups.duration){
                unsigned t=g->cups.slot[g->cups.a];g->cups.slot[g->cups.a]=g->cups.slot[g->cups.b];g->cups.slot[g->cups.b]=t;
                g->phase_ms=0;if(++g->cups.swaps>=g->cups.goal)g->cups.phase=ARC_PICK;else cup_swap(g);
            } else if(g->cups.phase==ARC_REVEAL && g->phase_ms>=1500){
                if(!g->cups.lives){g->done=true;g->cups.phase=ARC_OVER;}else cup_round(g);
            }
        }
    }
}
