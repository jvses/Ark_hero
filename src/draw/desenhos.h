#ifndef  DRAW_LIB
#define DRAW_LIB

#include <SDL3/SDL_render.h>
#include <SDL3/SDL.h>
#include "../game/game_state.h"
#include <stdio.h>

#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGTH 480

#define TAB_XU 270 //addr padrão do primeiro X de cima (U de Up) esse é mais curto
#define TAB_XD 170 //addr padrão do primeiro X de baixo (D de Down)
#define TAB_XU_DELTA 100 // diferença do Xu para o próximo
#define TAB_XD_DELTA 300 // diferença do Xd para o próximo

#define TAB_YD 450  //endereços dos Ys com alturas do trapézio perceba que são invertidos para se adequar à visão já que Y crece para baixo e não pra cima.
#define TAB_YD_DELTA 400
#define TAB_YU (TAB_YD - TAB_YD_DELTA)

//Macros para desenhar as linhas que os charts vão seguir
#define CHART_UP_DELTA (TAB_XU_DELTA/5)
#define CHART_DOWN_DELTA (TAB_XD_DELTA/5)

#define CHART_GREEN_COLUMN_X_INIT 



typedef struct{
    SDL_Renderer* render;
    SDL_Texture  *fundo;
} Drawing_state;

typedef enum Colors_draw{
    branco,
    vermelho,
    verde,
    azul,
    amarelo,
    laranja,
    cinza,
    cinza_escuro,
    preto
}Colors_draw;

//acho que não vou precisar de init e/ou quit aqui pois os que manuseio aqui é criado/defetruído na video.h
void Clear_screen(Drawing_state *ds);
void Draw_memory_prepare_stuffs(Drawing_state *ds,Game_logic_State *gl);
void Draw_bg_image(Drawing_state *ds,Game_logic_State *gl);
void Draw_Rectangle(Drawing_state *ds, int color);
void Draw_print_fps(Drawing_state *ds,float val);
void Draw_FillRectangle(Drawing_state *ds,float x, float y, SDL_Color cor);
void Draw_menu_and_bg(Drawing_state *ds,Game_logic_State *gl);
void Draw_menu_idle(Drawing_state *ds,Game_logic_State *gl);
void Draw_menu_pause(Drawing_state *ds,Game_logic_State *gl);
void Draw_music_tab(Drawing_state *ds);
void Draw_Elipse(Drawing_state *ds, float x, float y, float w, float h, SDL_Color cor);


SDL_Color Draw_get_color(int cor);

#endif