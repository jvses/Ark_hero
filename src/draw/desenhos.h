#ifndef  DRAW_LIB
#define DRAW_LIB

#include <SDL3/SDL_render.h>
#include <SDL3/SDL.h>
#include "../game/game_state.h"


typedef struct{
    SDL_Renderer render;
    Game_logic_State gl;//game logic(aqui vão ficar o estado lógico de tudo o que acontece no jogo)
} Drawing_state;



#endif