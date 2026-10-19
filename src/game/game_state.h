#ifndef GAME_LOGIC_LIB
#define GAME_LOGIC_LIB

#include <stdint.h>
#include <stdbool.h>

// typedef struct{
//     bool up,down,left,right;
// } Directions_Flag_keys;

typedef struct{
    bool up,down,left,right;
} Arrows;//struct das setas que guarda a lógica do teclado


//structs de um chart(notas, geralmente 5 cores)
typedef struct{
    int cor;
    float addrx,addry;
} Chart;




typedef enum{
    CHART_green,
    CHART_red,
    CHART_yellow,
    CHART_blue,
    CHART_orange
} ChartColumn_color;

typedef enum Menu_table{
    idle,//tela padrão de início
    gameplay,//tela de jogo rodando
    config
}Menu_table;

typedef enum Arrows_Direc{
    none,
    up,
    down,
    left,
    right
} Arrows_Direc;

typedef struct {
    uint8_t cor_bg;//bg só tem escala de cinza no momento
    int8_t razao; 
    bool sair;
    int64_t fps_inicio;//instante em ms começo
    int64_t frame_anterior;//instante do frame anterior(ms)
    int fps_frames;//frames condados no segundo atual
    float fps;//valor mostrado na tela
    float rec_addr[2];
    Arrows dir_keys;//flags para guargar estado de botões de direção
    uint8_t game_menu_state; //deve armazenar valores de Menu_table e julgar estado do game
} Game_logic_State;

void GS_rec_move(Game_logic_State *gl);




#endif