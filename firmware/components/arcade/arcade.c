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
static const float rocks[3][2]={{88,80},{250,148},{122,210}};
static void star_place(arcade_t *g)
{
    for(unsigned tries=0;tries<64;tries++) {
        float x=28+random_next(g)%285,y=28+random_next(g)%193;
        bool good=hypotf(x-g->tilt.star_x,y-g->tilt.star_y)>65;
        for(unsigned i=0;i<3;i++)if(hypotf(x-rocks[i][0],y-rocks[i][1])<40)good=false;
        if(good){g->tilt.star_x=x;g->tilt.star_y=y;return;}
    }
    // Even the fallback follows the shared course, independent of player movement.
    g->tilt.star_x=g->tilt.star_x<170?302:38;g->tilt.star_y=36;
}
void arcade_start(arcade_t *g,arcade_kind_t kind,uint32_t seed)
{
    memset(g,0,sizeof *g);g->kind=kind;g->rng=seed?seed:1;
    if(kind==ARCADE_CUPS){g->cups.lives=3;for(unsigned i=0;i<3;i++)g->cups.slot[i]=i;cup_round(g);}
    if(kind==ARCADE_PEGS){g->pegs.balls=5;g->pegs.live=(UINT64_C(1)<<33)-1;g->pegs.x=170;g->pegs.y=12;}
    if(kind==ARCADE_TILT){g->tilt.x=170;g->tilt.y=128;g->tilt.star_x=170;g->tilt.star_y=128;star_place(g);}
    if(kind==ARCADE_MEMORY){
        g->memory.first=g->memory.second=-1;
        for(unsigned i=0;i<12;i++)g->memory.deck[i]=i/2;
        for(unsigned i=11;i>0;i--){unsigned j=random_next(g)%(i+1);uint8_t t=g->memory.deck[i];g->memory.deck[i]=g->memory.deck[j];g->memory.deck[j]=t;}
    }
}
void arcade_peg_pos(unsigned p,float *x,float *y)
{
    unsigned row=0;while(p>=(row%2?5u:6u)){p-=row%2?5:6;row++;}
    *x=45+p*50+(row%2?25:0);*y=52+row*31;
}
bool arcade_peg_gold(unsigned p) {return p%3==1;}
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
void arcade_aim(arcade_t *g,float x,float y) {if(g->kind==ARCADE_PEGS && !g->pegs.flying)g->pegs.aim=clamp(atan2f(x-170,fmaxf(30,y-12)),-1.05f,1.05f);}
bool arcade_fire(arcade_t *g)
{
    if(g->kind!=ARCADE_PEGS || g->done || g->pegs.flying || !g->pegs.balls)return false;
    g->pegs.x=170;g->pegs.y=12;g->pegs.vx=sinf(g->pegs.aim)*195;g->pegs.vy=cosf(g->pegs.aim)*195;
    g->pegs.balls--;g->pegs.hits=g->pegs.shot_ms=0;g->pegs.flying=true;return true;
}
static void peg_tick(arcade_t *g,unsigned ms)
{
    float dt=ms/1000.f;g->pegs.bucket=170+sinf(g->elapsed/850.f)*112;
    if(!g->pegs.flying)return;
    g->pegs.shot_ms+=ms;g->pegs.vy+=350*dt;g->pegs.x+=g->pegs.vx*dt;g->pegs.y+=g->pegs.vy*dt;
    if(g->pegs.x<9 || g->pegs.x>331){g->pegs.x=clamp(g->pegs.x,9,331);g->pegs.vx=-g->pegs.vx*.92f;}
    if(g->pegs.y<7){g->pegs.y=7;g->pegs.vy=fabsf(g->pegs.vy);}
    for(unsigned i=0;i<33;i++)if(g->pegs.live&(UINT64_C(1)<<i)){
        float x,y;arcade_peg_pos(i,&x,&y);float dx=g->pegs.x-x,dy=g->pegs.y-y,d=hypotf(dx,dy);
        if(d<14){
            if(d<.01f){dx=0;dy=-1;d=1;}dx/=d;dy/=d;
            g->pegs.x=x+dx*14;g->pegs.y=y+dy*14;
            float dot=g->pegs.vx*dx+g->pegs.vy*dy;
            if(dot<0){g->pegs.vx-=1.75f*dot*dx;g->pegs.vy-=1.75f*dot*dy;}
            g->pegs.live&=~(UINT64_C(1)<<i);g->pegs.hits++;g->cue++;
            g->burst_points=(arcade_peg_gold(i)?50:10)+(g->pegs.hits-1)*5;g->score+=g->burst_points;
            g->burst_x=x;g->burst_y=y;g->burst_ms=450;
        }
    }
    if(!g->pegs.live){g->score+=g->pegs.balls*150;g->done=true;g->pegs.flying=false;return;}
    if(g->pegs.y>242 || g->pegs.shot_ms>=8000){
        if(g->pegs.y>242 && fabsf(g->pegs.x-g->pegs.bucket)<32){g->score+=100;g->cue++;if(g->pegs.catches++<3)g->pegs.balls++;}
        g->pegs.flying=false;if(!g->pegs.balls)g->done=true;
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
    for(unsigned i=0;i<3;i++){
        float dx=g->tilt.x-rocks[i][0],dy=g->tilt.y-rocks[i][1],d=hypotf(dx,dy);
        if(d<29){if(d<.01f){dx=1;dy=0;d=1;}dx/=d;dy/=d;g->tilt.x=rocks[i][0]+dx*29;g->tilt.y=rocks[i][1]+dy*29;
            float dot=g->tilt.vx*dx+g->tilt.vy*dy;if(dot<0){g->tilt.vx-=1.4f*dot*dx;g->tilt.vy-=1.4f*dot*dy;}}
    }
    if(hypotf(g->tilt.x-g->tilt.star_x,g->tilt.y-g->tilt.star_y)<23){g->tilt.stars++;g->score+=100;g->cue++;g->burst_x=g->tilt.star_x;g->burst_y=g->tilt.star_y;g->burst_ms=450;g->burst_points=100;star_place(g);}
    if(g->elapsed>=45000)g->done=true;
}
void arcade_tick(arcade_t *g,unsigned ms,float ax,float ay)
{
    if(g->done)return;
    // Deterministic small steps prevent tunnelling, independent of render cadence.
    while(ms && !g->done){unsigned step=ms>10?10:ms;ms-=step;g->elapsed+=step;g->phase_ms+=step;g->burst_ms=g->burst_ms>step?g->burst_ms-step:0;
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
