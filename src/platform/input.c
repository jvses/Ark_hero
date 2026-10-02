#include "input.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>



bool Input_init(Input *inp){
    if(!SDL_InitSubSystem(SDL_INIT_GAMEPAD)){
        SDL_Log("Input Could not initialize error: %s",SDL_GetError());
        return false;
    }
    return true; 
}

void Input_quit(Input *inp){
    SDL_CloseGamepad(inp->gamepad);
    inp->gamepad = NULL;
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);

}
void Input_Event_Handler(Input *inp,const SDL_Event *ev){
    switch (ev->type) {
        case SDL_EVENT_GAMEPAD_ADDED:
            if(!inp->gamepad){
                inp->gamepad = SDL_OpenGamepad(ev->gdevice.which);
                if(inp->gamepad){
                    inp->which = ev->gdevice.which;
                    SDL_Log("[input] controle: %s", SDL_GetGamepadName(inp->gamepad));
                }
            }
            break;
        
        case SDL_EVENT_GAMEPAD_REMOVED:
            if(inp->gamepad && ev->gdevice.which == inp->which){
                SDL_CloseGamepad(inp->gamepad);
                inp->gamepad = NULL;
            }
            break;
        
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            const bool start = ev->gbutton.button == SDL_GAMEPAD_BUTTON_START;
            const bool select = ev->gbutton.button == SDL_GAMEPAD_BUTTON_BACK;
            if ((start && SDL_GetGamepadButton(inp->gamepad, SDL_GAMEPAD_BUTTON_BACK)) || (select && SDL_GetGamepadButton(inp->gamepad, SDL_GAMEPAD_BUTTON_START))) {
                inp->sair = true;
            }
            break;
    
    }

}



