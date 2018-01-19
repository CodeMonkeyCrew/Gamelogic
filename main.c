#include "GameLogic.h"
#include "Gamestate.h"
#include <inttypes.h>
#include <stdio.h>

int main() {
    uint8_t player_number = 1;
    uColor_t *color;
    color->color = 0xff0000;
    on_color_change(player_number, *color);
    color->color = 0x0000ff;
    on_color_change(player_number, *color);

    Gamestate_t gs;
    gs.player1.lap = 0;
    int lap = gs.player1.lap;
    printf("%d", lap);

    gs.player1.lap++;
    lap = gs.player1.lap;
    printf("%d", lap);
    getchar();
    return 0;
}