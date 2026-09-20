#pragma once
#include <stdbool.h>
#include <stdint.h>
// No allocation, rendering or hardware dependencies. Coordinates are a 340x256 playfield.
typedef enum { ARCADE_CUPS, ARCADE_PEGS, ARCADE_TILT, ARCADE_MEMORY, ARCADE_COUNT } arcade_kind_t;
typedef enum { ARC_SHOW, ARC_SHUFFLE, ARC_PICK, ARC_REVEAL, ARC_OVER } cups_phase_t;
typedef struct {
    arcade_kind_t kind; uint32_t rng, elapsed, phase_ms; unsigned score; bool done;
    float burst_x,burst_y;unsigned burst_ms,burst_points;
    unsigned cue; // Monotonic feedback counter, polled by the UI.
    struct {cups_phase_t phase;unsigned slot[3],a,b,swaps,goal,duration,lives,streak,best;int picked;} cups;
    struct {float x,y,vx,vy,aim,bucket;unsigned balls,catches,hits,shot_ms;uint64_t live,gold;uint16_t px[33],py[33];bool flying;} pegs;
    struct {float x,y,vx,vy,star_x,star_y;unsigned stars;} tilt;
    struct {uint8_t deck[12];uint16_t matched;int first,second;unsigned moves,pairs;} memory;
} arcade_t;
extern const char *const arcade_names[ARCADE_COUNT];
void arcade_start(arcade_t *g,arcade_kind_t kind,uint32_t seed);
void arcade_tick(arcade_t *g,unsigned ms,float tilt_x,float tilt_y);
bool arcade_pick(arcade_t *g,unsigned index);
bool arcade_fire(arcade_t *g);
void arcade_aim(arcade_t *g,float x,float y);
void arcade_peg_pos(const arcade_t *g,unsigned peg,float *x,float *y);
bool arcade_peg_gold(const arcade_t *g,unsigned peg);
void arcade_cup_pos(const arcade_t *g,unsigned cup,float *x,float *y);
unsigned arcade_best(uint64_t pet,arcade_kind_t kind);
bool arcade_save_best(uint64_t pet,arcade_kind_t kind,unsigned score);
