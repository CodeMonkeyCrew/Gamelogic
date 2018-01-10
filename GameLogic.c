#include <stdio.h>
#include "GameLogic.h"
#include "SPICommunication.h"
#include "Zigbee.h"

#define RED     0xff0000
#define GREEN   0x00ff00
#define BLUE    0x0000ff

#define BOOST 20
#define SLOW 20
#define AVERAGE 128

Gamestate_t gamestate;

/**
 * Item Boost
 */
static void on_change_red(uint8_t player) {
    if (player) {
        gamestate.player2.item &= 0x0;
        gamestate.player2.item |= 0x1;
        printf("Boost %i\n", gamestate.player2.item);
    } else {
        gamestate.player1.item &= 0x0;
        gamestate.player1.item |= 0x1;
        printf("Boost %i\n", gamestate.player1.item);
    }
    sendGamestateToPi(gamestate);
}

/**
 * Item Lightning
 */
static void on_change_green(uint8_t player) {
    if (player) {
        gamestate.player2.item &= 0x0;
        gamestate.player2.item |= 0x2;
        printf("Lightning %i\n", gamestate.player2.item);
    } else {
        gamestate.player1.item &= 0x0;
        gamestate.player1.item |= 0x2;
        printf("Lightning %i\n", gamestate.player1.item);
    }
}

/**
 * Item Reverse
 */
static void on_change_blue(uint8_t player) {
    if (player) {
        gamestate.player2.item &= 0x0;
        gamestate.player2.item |= 0x3;
        printf("Reverse %i\n", gamestate.player2.item);
    } else {
        gamestate.player1.item &= 0x0;
        gamestate.player1.item |= 0x3;
        printf("Reverse %i\n", gamestate.player1.item);
    }
}

void on_change( uint8_t player_number, uColor_t color) {
    uint32_t rgb = color.color;
    if (!(rgb ^ RED)) {
        on_change_red(player_number);
    }
    if (!(rgb ^ GREEN)) {
        on_change_green(player_number);
    } else if (!(rgb ^ BLUE)) {
        on_change_blue(player_number);
    }
}

/**
 * Apply Gamestat to the commands send to the car
 * @param player player 1 or 2
 * @param command actual command which should be modified
 */
static void adaptSideEffects(uint8_t player, Command_t *command) {
    uint8_t status;
    if (!player) {
        status = gamestate.player1.status;
    } else {
        status = gamestate.player2.status;
    }
    if (status == 1) {
        command->value += BOOST;
    } else if (status == 2) {
        command->value += SLOW;
    } else if (!(status == 3)) {
        if (command->value > AVERAGE) {
            command->value -= AVERAGE;
        } else if (command->value < AVERAGE) {
            command->value += AVERAGE;
        }
    }

}

void on_command(uint8_t player_number, Command_t command) {
    if (command.command) {
        adaptSideEffects(player_number, &command);
        sendCommand(player_number, command);
    }
    //TODO: Further commands
}