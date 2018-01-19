#include <stdio.h>
#include <stdlib.h>
#include "GameLogic.h"
#include "SPICommunication.h"
#include "Zigbee.h"

#define RED     0xff0000
#define GREEN   0x00ff00
#define BLUE    0x0000ff
#define BLACK   0x000000
#define COLOR_DEVIATION 0x000030

#define MAX_NO_OF_LAPS 3
#define NO_OF_ITEMS 3

#define SPEED_CAP 128
#define SPEED_CAP_INCREASE 20
#define SPEED_CAP_DECREASE 20
#define AVERAGE 128


Gamestate_t gamestate;


void handle_direction(PlayerStatus_t *player, Command_t *command);
void handle_acceleration(PlayerStatus_t *player, Command_t *command);
void handle_throttle(PlayerStatus_t *player, Command_t *command);
void handle_start(PlayerStatus_t *player, Command_t *command);
void handle_use_item(PlayerStatus_t *player, PlayerStatus_t *enemyPlayer);


static void activate_speed_pad_boost(PlayerStatus_t *player) {
    if ((player->status >> BOOSTED_BIT) & 0) {
        player->status |= 1 << BOOSTED_BIT;
    }
    printf("Boost!!!!!");
    //sendGamestateToPi(gamestate);
}

//player drives over track line
static void cross_track_line(PlayerStatus_t *player) {
    //toggle in_track bit
    //if player leaves track it should be 0 and slow down dramatically (see function handle_acceleration)
    //if player reenters track it should be 1 and return to normal speed
    //TODO needs testing if it really only toggles once upon crossing the track line
    //TODO or add two colored track lines
    player->in_track ^= 1;
}

//player drove over finishing line
static void finished_lap(PlayerStatus_t *player) {
    if (player->lap < MAX_NO_OF_LAPS) {
        player->lap++;
    } else {
        //set game status to finished if one player finishes the race
        gamestate.game_status |= 1 << 0;
        gamestate.game_status |= 1 << 1;
    }
}

//whenever the player drives over item box
static void set_random_item(PlayerStatus_t *player) {
    if (player->item == 0) {
        int randomItemNumber = rand() % NO_OF_ITEMS;
        player->item = randomItemNumber;
    }
}

void on_color_change(uint8_t player_number, uColor_t color) {
    uint32_t rgb = color.color;
    PlayerStatus_t player = player_number == 0 ? gamestate.player1 : gamestate.player2;

    if (rgb > BLACK && rgb < BLACK + COLOR_DEVIATION) {
        set_random_item(&player);
    }
    else if (rgb > BLUE - COLOR_DEVIATION && rgb < BLUE + COLOR_DEVIATION){
        finished_lap(&player);
    }
    else if (rgb > GREEN - COLOR_DEVIATION && rgb < GREEN + COLOR_DEVIATION) {
        cross_track_line(&player);
    }
    else if (rgb > RED - COLOR_DEVIATION && rgb < RED + COLOR_DEVIATION) {
        activate_speed_pad_boost(&player);
    }
}

/**
 * Apply Gamestat to the commands send to the car
 * @param player player 1 or 2
 * @param command actual command which should be modified
 */
static void handle_new_command(uint8_t player_no, Command_t *command) {
	PlayerStatus_t player;
    PlayerStatus_t enemyPlayer;

    if (!player_no) {
		player = gamestate.player1;
        enemyPlayer = gamestate.player2;
    } else {
        player = gamestate.player2;
        enemyPlayer = gamestate.player1;
    }
	
	switch (command->command) {
		case 1:
            handle_direction(&player, command);
			break;
		case 2:
            handle_acceleration(&player, command);
			break;
		case 3:
            handle_throttle(&player, command);
			break;
		case 4:
            handle_start(&player, command);
			break;
		case 5:
            handle_use_item(&player, &enemyPlayer);
			break;
		default:
		    //nothing to do here yet
            break;
	}
}

void on_command(uint8_t player_number, Command_t command) {
	if (command.command) { //TODO catch NOC or make NOC=0 instead 6?
        handle_new_command(player_number, &command);
		//sendCommand(player_number, command);
	}
}

//reverse input (left -> right, right -> left) left = 0, right = 255, straight forward 127/128
void handle_direction(PlayerStatus_t *player, Command_t *command) {
	if ((player->status >> REVERSED_BIT) & 1) {
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
	//cap the speed if necessary
	if (command->value > SPEED_CAP) {
		command->value = SPEED_CAP;
	}

	if ((player->status >> BOOSTED_BIT) & 1) {
		command->value += SPEED_CAP_INCREASE;
	}
	else if ((player->status >> SLOWED_BIT) & 1) {
		if (command->value - SPEED_CAP_DECREASE < 0) {
			command->value = 0;
		}
		else {
			command->value -= SPEED_CAP_DECREASE;
		}
	}

    //half the speed if player is outside of track
    if (!player->in_track) {
        command->value %= 2;
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
            break;
        case LIGHTNING:
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