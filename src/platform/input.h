#ifndef INPUT_LIB
#define INPUT_LIB

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_joystick.h>
#include <SDL3/SDL_stdinc.h>
#include "../game/game_state.h"


typedef struct{
    SDL_JoystickID which;
    SDL_Gamepad *gamepad;
    Arrows setas; //melhor implementar a lógica de estado das setas em bool mesmo. Assim controlo também estados com várias pressionadas ao mesmo tempo
    // Uint8 setas;
    bool sair;
} Input;


bool Input_init(Input *inp);
void Input_quit(Input *inp);
void Input_Event_Handler(Input *inp,const SDL_Event *ev);
void Input_update_gl_arrows(Input *inp,Game_logic_State *gl);

#endif
