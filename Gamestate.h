#ifndef MK64LOGIC_GAMESTATE_H
#define MK64LOGIC_GAMESTATE_H

#include <inttypes.h>

/**
 * item -> least -> right
 * 0 0 0 No Item
 * 0 0 1 Boost
 * 0 1 0 Lightning
 * 0 1 1 Reverse
 * 1 0 0
 * 1 0 1
 * 1 1 0
 * 1 1 1
 */

/**
 * status -> least -> right
 * 0 0 0 None
 * 0 0 1 Boosted
 * 0 1 0 Slowed
 * 1 0 0 Reversed
 */

/**
 * game_status -> least -> right
 * 0 1 ready
 * 1 0 in_progress
 * 1 1 finished
 */
typedef struct PlayerStatus {
    uint8_t lap: 3;
    uint8_t item: 3;
    uint8_t status: 3;
    uint8_t in_track: 1;
    uint8_t RFU: 4;
} PlayerStatus_t;

typedef struct Gamestate {
    PlayerStatus_t player1;
    PlayerStatus_t player2;
    uint8_t game_status : 2;
    uint8_t player: 2;
} Gamestate_t;


#endif //MK64LOGIC_GAMESTATE_H
