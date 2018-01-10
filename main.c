#include "GameLogic.h"
#include <inttypes.h>

int main() {
    uint8_t player_number = 1;
    uColor_t *color;
    color->color = 0xff0000;
    on_change(player_number, *color);
    color->color = 0x0000ff;
    on_change(player_number, *color);
    return 0;
}