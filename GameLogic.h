#ifndef MK64LOGIC_GAMELOGIC_H
#define MK64LOGIC_GAMELOGIC_H

#include "Gamestate.h"

typedef union u_Color {
    struct Color {
        uint8_t clear;
        uint8_t blue;
        uint8_t green;
        uint8_t red;
    } Color_t;
    uint32_t color;
} uColor_t;
/**
 *  DIRECTION = 1
 *  ACCELERATE = 2
 *  THROTTLE = 3
 *  START = 4
 *  USE = 5
 *  NOC = 6
 */

typedef struct Command {
    uint8_t command;
    uint8_t value;
} Command_t;

void on_change(uint8_t player_number, uColor_t color);

void on_command(uint8_t player_number, Command_t command);

#endif //MK64LOGIC_GAMELOGIC_H
