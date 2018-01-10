#ifndef MK64LOGIC_SPICOMMUNICATION_H
#define MK64LOGIC_SPICOMMUNICATION_H

#include "Gamestate.h"

void sendGamestateToPi(Gamestate_t gamestate);

Command_t receiveCommand();
//should call gamelogic.on_command()

#endif //MK64LOGIC_SPICOMMUNICATION_H
