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
        arcade_start(&g,ARCADE_PEGS,seed);arcade_aim(&g,(seed%7)*50,180);assert(arcade_fire(&g));assert(!arcade_fire(&g));copy=g;
        arcade_tick(&copy,4000,0,0);for(unsigned i=0;i<400;i++)arcade_tick(&g,10,0,0);
        assert(g.score==copy.score && g.pegs.live==copy.pegs.live && fabsf(g.pegs.x-copy.pegs.x)<.001f);
        for(unsigned n=0;n<7000 && !g.done;n++){
            if(!g.pegs.flying){arcade_aim(&g,(n%7)*50,200);arcade_fire(&g);}arcade_tick(&g,10,0,0);
            assert(isfinite(g.pegs.x) && isfinite(g.pegs.vy) && g.pegs.balls<=5 && g.pegs.x>=9 && g.pegs.x<=331);
        }
        assert(g.done && g.score<=20000 && g.score%5==0);
        arcade_start(&g,ARCADE_TILT,seed);
        for(unsigned i=0;i<4500;i++){
            arcade_tick(&g,10,sinf(i*.017f),cosf(i*.021f));
            assert(isfinite(g.tilt.x) && g.tilt.x>=12 && g.tilt.x<=328 && g.tilt.y>=12 && g.tilt.y<=244);
        }
        assert(g.done && g.elapsed==45000 && g.score%100==0);
        arcade_t stop=g;arcade_tick(&g,10000,1,1);assert(!memcmp(&g,&stop,sizeof g));
    }
    printf("PASS: 400 seeds, cup permutations/lives, memory pairs, peg physics/time steps, tilt bounds/time and score invariants\n");
}
