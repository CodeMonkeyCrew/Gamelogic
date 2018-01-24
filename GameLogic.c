#include <stdio.h>
#include <stdlib.h>
#include "GameLogic.h"
#include "SPICommunication.h"
#include "Zigbee.h"

#define BLACK   0x000000
#define BLUE    0x0000ff
#define GREEN   0x00ff00
#define RED     0xff0000
#define YELLOW  0xffff00
#define WHITE   0xffffff
#define COLOR_DEVIATION 0x000030

#define MAX_NO_OF_LAPS 3
#define NO_OF_ITEMS 3

#define SPEED_CAP 158
#define BOOSTED_SPEED_CAP 168
#define SLOWED_SPEED_CAP 148
#define STOP_CAP 127
#define OUT_OF_TRACK_CAP 135
#define OUT_OF_TRACK_BACKWARDS_CAP 129
#define BACKWARDS_CAP 100
#define MAX_DIRECTION 255


static Gamestate_t gamestate;
static Player_t player1;
static Player_t player2;

int get_steering(Player_t *player);
int get_engine(Player_t *player);
int handle_acceleration(Player_t *player);
int handle_backwards(Player_t *player);
void handle_start(Player_t *player);
void handle_use_item(Player_t *player, Player_t *enemyPlayer);

void init() {
    player1.status = &gamestate.player1;

    player1.status->in_track = 1;
    player1.status->lap = 0;
    player1.status->status = 0;
    player1.status->item = 0;
    player1.status->RFU = 0;
    player1.currentColor = 0x0;
    player1.consecutiveColorReadings = 0;

    player1.controllerState.accelerate = 0;
    player1.controllerState.backwards = 0;
    player1.controllerState.start = 0;
    player1.controllerState.useItem = 0;
    player1.controllerState.direction = 127;

    player2.status = &gamestate.player2;
    player2.status->in_track = 1;
    player2.status->lap = 0;
    player2.status->status = 0;
    player2.status->item = 0;
    player2.status->RFU = 0;
    player2.currentColor = 0x0;
    player2.consecutiveColorReadings = 0;

    player2.controllerState.accelerate = 0;
    player2.controllerState.backwards = 0;
    player2.controllerState.start = 0;
    player2.controllerState.useItem = 0;
    player2.controllerState.direction = 127;

    gamestate.game_status = 1;
}

static void activate_speed_pad_boost(PlayerStatus_t *player) {
    if ((player->status >> BOOSTED_BIT) & 0) {
        player->status |= 1 << BOOSTED_BIT;
    }
    sendGamestateToPi(gamestate);
}

//player drives over finishing line
static void finished_lap(PlayerStatus_t *player) {
    if (player->lap < MAX_NO_OF_LAPS) {
        player->lap++;
    } else {
        //set game status to finished if one player finishes the race
        gamestate.game_status |= 1 << 0;
        gamestate.game_status |= 1 << 1;
    }
}

//player reverses over finishing line
static void subtract_finished_lap(PlayerStatus_t *player) {
    if (player->lap > 0) {
        player->lap--;
    }
}

//whenever the player drives over item box
static void set_random_item(PlayerStatus_t *player) {
    if (player->item == 0) {
        int randomItemNumber = rand() % NO_OF_ITEMS;
        player->item = randomItemNumber;
    }
}

static int is_within_deviation(uint32_t isRGBValue, int shouldRGBValue) {
    //needed as 0x000000 - color_deviation may result in unexpected behaviour
    if (isRGBValue < COLOR_DEVIATION) {
        return (isRGBValue >= shouldRGBValue && isRGBValue < shouldRGBValue + COLOR_DEVIATION);
    }

    //needed as 0xffffff + color_deviation may result in unexpected behaviour
    if (isRGBValue > WHITE - COLOR_DEVIATION) {
        return (isRGBValue > shouldRGBValue - COLOR_DEVIATION && isRGBValue <= shouldRGBValue);
    }

    //all other colors
    return isRGBValue > shouldRGBValue - COLOR_DEVIATION && isRGBValue < shouldRGBValue + COLOR_DEVIATION;
}

static void compare_to_previous_color(Player_t *player, uint32_t color) {
    if (player->currentColor == color) {
        player->consecutiveColorReadings++;
    } else {
        player->currentColor = color;
        player->consecutiveColorReadings = 0;
    }
}

//read this
/* quick summary of what which color means. read the README for further explanation
 * BLACK: set new item
 * RED: player drove over speed boost pad
 * BLUE->Yellow: player drove over finishing line in correct direction. increase finished laps by 1 and finish race if
 *               needed
 * YELLOW->BLUE: player drove over finishing line in false direction. decrease finished laps by 1
 * GREEN->WHITE: player left track, decrease speed cap
 * WHITE->GREEN: player reentered track, increase speed cap
*/
static void interpret_color(Player_t *player) {
    switch (player->currentColor) {
        case BLACK:
            set_random_item(player->status);
            break;
        case RED:
            activate_speed_pad_boost(player->status);
            break;
        case BLUE:
            if (player->previousColor == YELLOW) {
                subtract_finished_lap(player->status);
            }
            break;
        case YELLOW:
            if (player->previousColor == BLUE) {
                finished_lap(player->status);
            }
            break;
        case GREEN:
            if (player->previousColor == WHITE) {
                player->status->in_track = 1;
            }
            break;
        case WHITE:
            if (player->previousColor == GREEN) {
                player->status->in_track = 0;
            }
            break;
        default:
            //nothing to do
            break;

    }
    player->previousColor = player->currentColor;
}


void on_color_change(uint8_t player_number, uColor_t color) {
    uint32_t rgb = color.color;
    Player_t *player = player_number == 0 ? &player1 : &player2;

    if (is_within_deviation(rgb, BLACK)) {
        compare_to_previous_color(player, BLACK);
    }
    else if (is_within_deviation(rgb, RED)) {
        compare_to_previous_color(player, RED);
    }
    else if (is_within_deviation(rgb, BLUE)){
        compare_to_previous_color(player, BLUE);
    }
    else if (is_within_deviation(rgb, YELLOW)) {
        compare_to_previous_color(player, YELLOW);
    }
    else if (is_within_deviation(rgb, GREEN)) {
        compare_to_previous_color(player, GREEN);
    }
    else if (is_within_deviation(rgb, WHITE)) {
        compare_to_previous_color(player, WHITE);
    }

    if (player->consecutiveColorReadings == 3) {
        interpret_color(player);
    }
}




//vvvvvv handling commands vvvvvvvv




/**
 * Apply Gamestate to the commands send to the car
 * @param player player 1 or 2
 * @param command actual command which should be modified
 */
static void handle_new_command(uint8_t player_no, Command_t *command) {
    Player_t *player;
    Player_t *enemyPlayer;

    if (!player_no) {
        player = &player1;
        enemyPlayer = &player2;
    } else {
        player = &player2;
        enemyPlayer = &player1;
    }
	
	switch (command->command) {
		case DIRECTION:
            player->controllerState.direction = command->value;
            break;
		case ACCELERATE:
            if (!player->controllerState.backwards) {
                player->controllerState.accelerate = command->value;
            }
            break;
		case BACKWARDS:
            if (!player->controllerState.accelerate) {
                player->controllerState.backwards = command->value;
            }
            break;
		case START:
            player->controllerState.start = command->value;

			break;
		case USEITEM:
            if (command->value) {
                handle_use_item(player, enemyPlayer);
            }
			break;
		default:
		    //nothing to do here yet
            break;
	}
}

void on_command(uint8_t player_number, Command_t command) {
	if (command.command) { //TODO catch NOC or make NOC=0 instead 6?
        handle_new_command(player_number, &command);
		sendCommand(player_number, command);
	}
}

//returns direction input of player. left = 0, right = 255, straight forward 127/128
// if reversed bit active, reverse direction (left <-> right)
int get_steering(Player_t *player) {
	if (player->status->status & REVERSED_BIT) {
		return MAX_DIRECTION - player->controllerState.direction;
	}
    return player->controllerState.direction;
}

int get_engine(Player_t *player) {
    if (player->controllerState.accelerate) {
        return handle_acceleration(player);
    }
    else if (player->controllerState.backwards) {
        return handle_backwards(player);
    }
    else {
        return STOP_CAP;
    }
}

int handle_acceleration(Player_t *player) {
    if (!player->status->in_track) {
        return OUT_OF_TRACK_CAP;
    } else if (player->status->status & BOOSTED_BIT) {
        return BOOSTED_SPEED_CAP;
    } else if (player->status->status & SLOWED_BIT) {
        return SLOWED_SPEED_CAP;
    } else {
        return SPEED_CAP;
    }
}

int handle_backwards(Player_t *player) {
	if (!player->status->in_track) {
        return OUT_OF_TRACK_BACKWARDS_CAP;
    } else {
        return BACKWARDS_CAP;
    }
}

void handle_start(Player_t *player) {
	//todo
}

void handle_use_item(Player_t *player, Player_t *enemyPlayer) {
    //todo how long are these stati active?
    switch (player->status->item) {
        case BOOST:
            player->status->status |= 1 << BOOSTED_BIT;
            player->status->status |= 0 << SLOWED_BIT;
            break;
        case LIGHTNING:
            enemyPlayer->status->status |= 0 << BOOSTED_BIT;
            enemyPlayer->status->status |= 1 << SLOWED_BIT;
            break;
        case REVERSE:
            enemyPlayer->->status->status |= 1 << REVERSED_BIT;
            break;
        default:
            //nothing to do
            break;
    }
    player->status->item = 0;
}