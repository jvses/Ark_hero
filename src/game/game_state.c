#include "game_state.h"


void GS_change_game_menu(Game_logic_State *gl){
    if(gl->apertou_P){
        //troca de menu caso houve troca;
        if(gl->game_menu_state == idle){
            gl->game_menu_state = gameplay;
        } else if(gl->game_menu_state == gameplay){
            gl->game_menu_state = config;
        } else if(gl->game_menu_state == config){
            gl->game_menu_state = idle;
        }
        //vou deixar para trocar estado da variável no main.c reconhece a troca lá e ajeita a flag lá
        // gl->apertou_P = false;
    }
    // gl->game_menu_state = new_menu;
}

void GS_rec_move(Game_logic_State *gl){

    if (gl->game_menu_state == idle) {
        if(gl->dir_keys.up){
            gl->game_menu_state_selected-=1;
            if (gl->game_menu_state_selected<idle) {
                gl->game_menu_state_selected = config;
            }
        }
        if(gl->dir_keys.down){
            gl->game_menu_state_selected+=1;
            if (gl->game_menu_state_selected>config) {
                gl->game_menu_state_selected = idle;
            }
        }

        // if(gl->dir_keys.right){
        //     gl->rec_addr[0] = gl->rec_addr[0] + 5;
        // }
        // if(gl->dir_keys.left){
        //     gl->rec_addr[0] = gl->rec_addr[0] - 5;
        // }
    
    } else if (gl->game_menu_state == gameplay) {
        float xspeed=5,yspeed=5;
        if(gl->dir_keys.up){
            gl->rec_addr[1] = gl->rec_addr[1] - 5;
        }
        if(gl->dir_keys.down){
            gl->rec_addr[1] = gl->rec_addr[1] + 5;
        }
        if(gl->dir_keys.right){
            gl->rec_addr[0] = gl->rec_addr[0] + 5;
        }
        if(gl->dir_keys.left){
            gl->rec_addr[0] = gl->rec_addr[0] - 5;
        }
    }
}