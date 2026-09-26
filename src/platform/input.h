#ifndef INPUT_LIB
#define INPUT_LIB

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_joystick.h>

typedef struct{
    SDL_JoystickID which;
    SDL_Gamepad *gamepad;
} Input;


#endif