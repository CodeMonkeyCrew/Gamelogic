cmake_minimum_required(VERSION 3.9)
project(MK64Logic C)

set(CMAKE_C_STANDARD 99)

add_executable(MK64Logic main.c Gamestate.h GameLogic.c GameLogic.h SPICommunication.h Zigbee.h)