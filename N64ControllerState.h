#ifndef MK64LOGIC_N64CONTROLLERSTATE_H
#define MK64LOGIC_N64CONTROLLERSTATE_H


#include <stdint.h>

typedef struct N64ControllerState {
    uint8_t accelerate: 1;
    uint8_t backwards: 1;
    uint8_t start: 1;
    uint8_t useItem: 1;
    uint8_t direction;
} N64ControllerState_t;


#endif //MK64LOGIC_N64CONTROLLERSTATE_H
