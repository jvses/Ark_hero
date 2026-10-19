#include "game_state.h"


void GS_rec_move(Game_logic_State *gl){
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
    // switch (key_flag) {
    //     case up:
    //         gl->rec_addr[1] = gl->rec_addr[1] - 5;
    //         break;
    //     case down:
    //         gl->rec_addr[1] = gl->rec_addr[1] + 5;
    //         break;
    //     case right:
    //         gl->rec_addr[0] = gl->rec_addr[0] + 5;
    //         break;
    //     case left:
    //         gl->rec_addr[0] = gl->rec_addr[0] - 5;
    //         break;
    // }
}