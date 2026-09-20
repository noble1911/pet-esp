#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "pet_state.h"
#define MP_MAX_PEERS 4
typedef enum { MP_OFFLINE, MP_CLOSED, MP_LOBBY, MP_OUTGOING, MP_INCOMING, MP_PLAYING, MP_WAITING, MP_FINISHED } mp_phase_t;
typedef struct {char user[40],id[17],name[16];unsigned character,stage;} mp_peer_t;
typedef struct {
    mp_phase_t phase;bool connected,my_turn,online,again;
    unsigned seq,count;mp_peer_t peer,peers[MP_MAX_PEERS];
    char room[33],invite[33],notice[96];
    uint64_t reward_id,reward_pet,reward_friend;unsigned friends;
} mp_state_t;
void multiplayer_init(void);
void multiplayer_open(const Pet *pet);
void multiplayer_close(void);
void multiplayer_snapshot(mp_state_t *out);
bool multiplayer_invite(const char *user);
bool multiplayer_accept(const char *invite);
bool multiplayer_decline(const char *invite);
bool multiplayer_pass(const char *room,unsigned seq);
bool multiplayer_again(const char *room);
void multiplayer_ack(uint64_t receipt,uint64_t pet);
