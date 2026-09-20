#include "arcade.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    for(unsigned seed=1;seed<=400;seed++){
        arcade_t g,copy;arcade_start(&g,ARCADE_CUPS,seed);
        assert(!arcade_pick(&g,0));
        for(unsigned round=0;round<15;round++){
            while(g.cups.phase!=ARC_PICK){arcade_tick(&g,10,0,0);assert(!g.done);}
            assert(g.cups.slot[0]!=g.cups.slot[1] && g.cups.slot[1]!=g.cups.slot[2] && g.cups.slot[0]!=g.cups.slot[2]);
            assert(arcade_pick(&g,0));assert(!arcade_pick(&g,0));arcade_tick(&g,1500,0,0);
        }
        assert(g.score==15 && g.cups.lives==3);
        for(unsigned lives=3;lives;lives--){while(g.cups.phase!=ARC_PICK)arcade_tick(&g,10,0,0);assert(arcade_pick(&g,1));arcade_tick(&g,1500,0,0);}
        assert(g.done && g.score==15 && g.cups.lives==0);
        arcade_start(&g,ARCADE_MEMORY,seed);unsigned count[6]={0};
        for(unsigned i=0;i<12;i++){assert(g.memory.deck[i]<6);count[g.memory.deck[i]]++;}
        for(unsigned i=0;i<6;i++)assert(count[i]==2);
        assert(!arcade_pick(&g,12));
        for(unsigned value=0;value<6;value++){
            for(unsigned i=0;i<12;i++)if(g.memory.deck[i]==value){assert(arcade_pick(&g,i));assert(!arcade_pick(&g,i));}
            assert(!arcade_pick(&g,11));arcade_tick(&g,900,0,0);
        }
        assert(g.done && g.score==1200 && g.memory.matched==4095);
        // A child can make many mistakes and still earn the six-pair score.
        arcade_start(&g,ARCADE_MEMORY,seed);
        unsigned other=1;while(g.memory.deck[other]==g.memory.deck[0])other++;
        for(unsigned miss=0;miss<20;miss++){
            assert(arcade_pick(&g,0));assert(arcade_pick(&g,other));
            arcade_tick(&g,900,0,0);assert(g.memory.matched==0 && g.score==0);
        }
        for(unsigned value=0;value<6;value++){
            for(unsigned i=0;i<12;i++)if(g.memory.deck[i]==value)assert(arcade_pick(&g,i));
            arcade_tick(&g,900,0,0);
        }
        assert(g.done && g.score==600 && g.memory.moves==26);
        arcade_start(&g,ARCADE_PEGS,seed);arcade_start(&copy,ARCADE_PEGS,seed);
        assert(!memcmp(&g.pegs,&copy.pegs,sizeof g.pegs));
        arcade_t different;arcade_start(&different,ARCADE_PEGS,seed+1);
        assert(memcmp(g.pegs.px,different.pegs.px,sizeof g.pegs.px) || g.pegs.gold!=different.pegs.gold);
        unsigned gold=0;
        for(unsigned i=0;i<33;i++){
            float x,y;arcade_peg_pos(&g,i,&x,&y);gold+=arcade_peg_gold(&g,i);
            assert(x>=20 && x<=320 && y>=30 && y<=225);
            for(unsigned j=0;j<i;j++){float xx,yy;arcade_peg_pos(&g,j,&xx,&yy);assert(hypotf(x-xx,y-yy)>=23);}
        }
        assert(gold==11);
        arcade_aim(&g,0,12);assert(g.pegs.aim< -1.4f);
        arcade_aim(&g,340,12);assert(g.pegs.aim>1.4f);
        arcade_aim(&g,170,200);assert(g.pegs.aim==0);
        arcade_aim(&g,seed%2?0:340,12);assert(arcade_fire(&g));assert(!arcade_fire(&g));copy=g;
        arcade_tick(&copy,4000,0,0);for(unsigned i=0;i<400;i++)arcade_tick(&g,10,0,0);
        assert(g.score==copy.score && g.pegs.live==copy.pegs.live && fabsf(g.pegs.x-copy.pegs.x)<.001f);
        for(unsigned n=0;n<7000 && !g.done && !g.level_clear;n++){
            if(!g.pegs.flying){arcade_aim(&g,(n%7)*50,200);arcade_fire(&g);}arcade_tick(&g,10,0,0);
            assert(isfinite(g.pegs.x) && isfinite(g.pegs.vy) && g.pegs.balls<=5 && g.pegs.x>=9 && g.pegs.x<=331);
        }
        assert((g.done || g.level_clear) && g.score<=20000 && g.score%5==0);
        arcade_start(&g,ARCADE_TILT,seed);
        for(unsigned i=0;i<4500;i++){
            arcade_tick(&g,10,sinf(i*.017f),cosf(i*.021f));
            assert(isfinite(g.tilt.x) && g.tilt.x>=12 && g.tilt.x<=328 && g.tilt.y>=12 && g.tilt.y<=244);
        }
        assert((g.done || g.level_clear) && g.score%100==0);
        if(g.level_clear)g.done=true;
        arcade_t stop=g;arcade_tick(&g,10000,1,1);assert(!memcmp(&g,&stop,sizeof g));
    }
    // Every bucket catch returns a ball, including the fourth and later catches.
    arcade_t g;arcade_start(&g,ARCADE_PEGS,987);
    for(unsigned i=0;i<9;i++){
        unsigned balls=g.pegs.balls;assert(arcade_fire(&g));
        g.pegs.x=170+sinf((g.elapsed+10)/850.f)*112;g.pegs.y=242;g.pegs.vx=0;g.pegs.vy=100;
        arcade_tick(&g,10,0,0);assert(g.pegs.balls==balls && g.pegs.bonus_ms && !g.done);
    }
    // Clearing orange is sufficient; blue pegs remain. Last-ball win advances.
    g.pegs.live&=~g.pegs.gold;g.pegs.balls=1;assert(arcade_fire(&g));
    g.pegs.x=10;g.pegs.y=242;g.pegs.vx=0;g.pegs.vy=100;
    arcade_tick(&g,10,0,0);assert(g.level_clear && !g.done && g.pegs.live);
    unsigned score=g.score;assert(arcade_next_level(&g));
    assert(g.level==2 && g.score==score && g.pegs.balls==5 && arcade_orange_left(&g)==11);
    arcade_start(&g,ARCADE_PEGS,987);assert(g.score==0 && g.level==1);
    // Upward launch really arcs above the board instead of bouncing off its top.
    arcade_aim(&g,330,0);assert(g.pegs.aim>1.5708f);assert(arcade_fire(&g));
    assert(g.pegs.vy<0);arcade_tick(&g,100,0,0);assert(g.pegs.y<7 && g.pegs.vy<0);
    // Actual physics can now reach an isolated upper peg on either far edge.
    for(unsigned side=0;side<2;side++){
        bool hit=false;
        for(unsigned aim=0;aim<=340 && !hit;aim++){
            arcade_start(&g,ARCADE_PEGS,22);g.pegs.live=g.pegs.gold=1;
            g.pegs.px[0]=side?310:30;g.pegs.py[0]=45;
            arcade_aim(&g,aim,0);assert(arcade_fire(&g));
            for(unsigned t=0;t<200 && g.pegs.flying;t++)arcade_tick(&g,10,0,0);
            hit=!g.pegs.live;
        }
        assert(hit);
    }
    // Seeded gardens progress without dropping score and reset their own timer.
    arcade_start(&g,ARCADE_TILT,44);
    for(unsigned level=1;level<=10;level++){
        assert(g.level==level && g.tilt.goal<=11 && g.tilt.rock_count<=6);
        unsigned goal=g.tilt.goal;
        for(unsigned i=0;i<goal;i++){
            g.tilt.x=g.tilt.star_x;g.tilt.y=g.tilt.star_y;arcade_tick(&g,10,0,0);
        }
        assert(g.level_clear && !g.done);score=g.score;
        arcade_tick(&g,1000,0,0);assert(g.score==score);
        assert(arcade_next_level(&g) && g.score==score && g.level_ms==0);
    }
    arcade_start(&g,ARCADE_TILT,44);arcade_tick(&g,45000,0,0);assert(g.done && g.score==0);
    arcade_start(&g,ARCADE_TILT,44);g.run_limit_ms=180000;g.elapsed=179990;g.level_clear=true;
    arcade_tick(&g,10,0,0);assert(g.done && !g.level_clear && !arcade_next_level(&g));
    printf("PASS: 400 seeds, cup permutations/lives, memory pairs, peg physics/time steps, tilt bounds/time and score invariants\n");
}
