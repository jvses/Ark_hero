#include "input.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_scancode.h>
#include <stdbool.h>



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

    // const bool* keys = SDL_GetKeyboardState(NULL); 

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
        
        case SDL_EVENT_KEY_DOWN: // arma a flag quando ver que está presionado e avisa qual botão
            // SDL_Log("Tecla foi apertada %d",ev->key.scancode);
            switch (ev->key.scancode) {
                case SDL_SCANCODE_RIGHT:
                    inp->setas.right = true;
                    break;
                case SDL_SCANCODE_LEFT:
                    inp->setas.left = true;
                    break;
                case SDL_SCANCODE_UP:
                    inp->setas.up = true;
                    break;
                case SDL_SCANCODE_DOWN:
                    inp->setas.down = true;
                    break;
                case SDL_SCANCODE_P://tecla P para teste de troca das imagens de fundo
                    inp->menu_btn.atual = true;
                    // SDL_Log("[input] P foi apertado");
                    break;
                    
            }
            break;
        // case SDL_EVENT_KEY_UP: 
        //     inp->setas=none;
        //     SDL_Log("Zerando a flag da seta");
        //     break;
        case SDL_EVENT_KEY_UP: //desarma a flag quando vê que tecla foi solta 
            switch (ev->key.scancode) {
                case SDL_SCANCODE_RIGHT:
                    inp->setas.right = false;
                    break;
                case SDL_SCANCODE_LEFT:
                    inp->setas.left = false;
                    break;
                case SDL_SCANCODE_UP:
                    inp->setas.up = false;
                    break;
                case SDL_SCANCODE_DOWN:
                    inp->setas.down = false;
                    break;
                case SDL_SCANCODE_P: //tecla P para teste de troca das imagens de fundo
                    inp->menu_btn.atual = false;
                    break;
                // default:
            }
        break;
    
    }

}

void Input_update_gl_arrows(Input *inp,Game_logic_State *gl){
        gl->dir_keys.up=inp->setas.up;
        gl->dir_keys.down=inp->setas.down;
        gl->dir_keys.right=inp->setas.right;
        gl->dir_keys.left=inp->setas.left;
    // SDL_Log("espelhando flags direcionais");
}

bool Input_update_gl_menu(Input *inp){
    //checagem se apertou
    if(inp->menu_btn.atual && !inp->menu_btn.anterior){
        inp->menu_btn.anterior = inp->menu_btn.atual;
        // SDL_Log("[input] Estado do botão P foi de up-down, troca de menu realizada");
        return true;//retorna para troca de menu(botão apertado) 
    }
    else {
        inp->menu_btn.anterior = inp->menu_btn.atual;//atualiza os valores geralemtne aqui vai se manter em false
        // SDL_Log("[input] Estado do botão P constante troca de menu não realizada");
        return false;
    }


}