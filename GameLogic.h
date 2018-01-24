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


enum {
    DIRECTION = 1, ACCELERATE = 2, BACKWARDS = 3, START = 4, USEITEM = 5, NOC = 6
} COMMAND_ENUM;

typedef struct Command {
    uint8_t command;
    uint8_t value;
} Command_t;


enum {
    BOOST = 0, LIGHTNING = 1, REVERSE = 2
} ITEM;

enum {
    BOOSTED_BIT = 0, SLOWED_BIT = 1, REVERSED_BIT = 2
} STATUS;

void init();

void on_color_change(uint8_t player_number, uColor_t color);

void on_command(uint8_t player_number, Command_t command);

#endif //MK64LOGIC_GAMELOGIC_H
