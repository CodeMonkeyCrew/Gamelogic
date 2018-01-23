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
#define OUT_OF_TRACK_CAP 128
#define AVERAGE 128


Gamestate_t gamestate;
Player_t player1;
Player_t player2;

void handle_direction(PlayerStatus_t *player, Command_t *command);
void handle_acceleration(PlayerStatus_t *player, Command_t *command);
void handle_throttle(PlayerStatus_t *player, Command_t *command);
void handle_start(PlayerStatus_t *player, Command_t *command);
void handle_use_item(PlayerStatus_t *player, PlayerStatus_t *enemyPlayer);

void init() {
    player1.status.in_track = 1;
    player1.status.lap = 0;
    player1.status.status = 0;
    player1.status.item = 0;
    player1.status.RFU = 0;
    player1.currentColor = 0x0;

    player2.status.in_track = 1;
    player2.status.lap = 0;
    player2.status.status = 0;
    player2.status.item = 0;
    player2.status.RFU = 0;
    player2.currentColor = 0x0;

    gamestate.player1 = &player1.status;
    gamestate.player2 = &player2.status;
    gamestate.game_status = 1;
}

static void activate_speed_pad_boost(PlayerStatus_t *player) {
    if ((player->status >> BOOSTED_BIT) & 0) {
        player->status |= 1 << BOOSTED_BIT;
    }
    //sendGamestateToPi(gamestate);
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
    if (shouldRGBValue == BLACK) {
        return (isRGBValue >= shouldRGBValue && isRGBValue < shouldRGBValue + COLOR_DEVIATION);
    }

    //needed as 0xffffff + color_deviation may result in unexpected behaviour
    if (shouldRGBValue == WHITE) {
        return (isRGBValue > shouldRGBValue - COLOR_DEVIATION && isRGBValue <= shouldRGBValue);
    }

    //all other colors
    return isRGBValue > shouldRGBValue - COLOR_DEVIATION && isRGBValue < shouldRGBValue + COLOR_DEVIATION;
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
void on_color_change(uint8_t player_number, uColor_t color) {
    uint32_t rgb = color.color;
    Player_t player = player_number == 0 ? player1 : player2;

    if (is_within_deviation(rgb, BLACK)) {
        set_random_item(&player.status);
        player.currentColor = BLACK;
    }
    else if (is_within_deviation(rgb, RED)) {
        activate_speed_pad_boost(&player.status);
        player.currentColor = RED;
    }
    else if (is_within_deviation(rgb, BLUE)){
        if (player.currentColor == YELLOW) {
            subtract_finished_lap(&player.status);
        }
        player.currentColor = BLUE;
    }
    else if (is_within_deviation(rgb, YELLOW)) {
        if (player.currentColor == BLUE) {
            finished_lap(&player.status);
        }
        player.currentColor = YELLOW;
    }
    else if (is_within_deviation(rgb, GREEN)) {
        if (player.currentColor == WHITE) {
            player.status.in_track = 1; //slow down handled in handle_acceleration
        }
        player.currentColor = GREEN;
    }
    else if (is_within_deviation(rgb, WHITE)) {
        if (player.currentColor == GREEN) {
            player.status.in_track = 0;
        }
        player.currentColor = WHITE;
    }
}

/**
 * Apply Gamestat to the commands send to the car
 * @param player player 1 or 2
 * @param command actual command which should be modified
 */
static void handle_new_command(uint8_t player_no, Command_t *command) {
	Player_t player;
    Player_t enemyPlayer;

    if (!player_no) {
		player = player1;
        enemyPlayer = player2;
    } else {
        player = player2;
        enemyPlayer = player1;
    }
	
	switch (command->command) {
		case 1:
            handle_direction(&player.status, command);
			break;
		case 2:
            handle_acceleration(&player.status, command);
			break;
		case 3:
            handle_throttle(&player.status, command);
			break;
		case 4:
            handle_start(&player.status, command);
			break;
		case 5:
            handle_use_item(&player.status, &enemyPlayer.status);
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

//reverse input (left <-> right) left = 0, right = 255, straight forward 127/128
//TODO check limits
void handle_direction(PlayerStatus_t *player, Command_t *command) {
	if (player->status & REVERSED_BIT) {
		int difference;
		if (command->value < AVERAGE) {
			difference = AVERAGE - command->value;
			command->value = AVERAGE + difference;
		}
		else if (command->value > AVERAGE) {
			difference = command->value - AVERAGE;
			command->value = AVERAGE - difference;
		}
	}
}


void handle_acceleration(PlayerStatus_t *player, Command_t *command) {
	if (command->value) {
        if (!player->in_track) {
            command->value = OUT_OF_TRACK_CAP;
        } else if (player->status & BOOSTED_BIT) {
            command->value = BOOSTED_SPEED_CAP;
        } else if (player->status & SLOWED_BIT) {
            command->value = SLOWED_SPEED_CAP;
        } else {
            command->value = SPEED_CAP;
        }
	} else {
        command->value = STOP_CAP;
    }
}

void handle_throttle(PlayerStatus_t *player, Command_t *command) {
	//todo
}

void handle_start(PlayerStatus_t *player, Command_t *command) {
	//todo
}

void handle_use_item(PlayerStatus_t *player, PlayerStatus_t *enemyPlayer) {
    //todo how long are these stati active?
    switch (player->item) {
        case BOOST:
            player->status |= 1 << BOOSTED_BIT;
            player->status |= 0 << SLOWED_BIT;
            break;
        case LIGHTNING:
            enemyPlayer->status |= 0 << BOOSTED_BIT;
            enemyPlayer->status |= 1 << SLOWED_BIT;
            break;
        case REVERSE:
            enemyPlayer->status |= 1 << REVERSED_BIT;
            break;
        default:
            //nothing to do
            break;
    }
    player->item = 0;
}