#include "platform/video.h"
// #include "audio.h"
#include "platform/input.h"
#include "game/game_state.h"
#include "draw/desenhos.h"


typedef struct { 
    Video video;
    // Audio audio; 
    Input input; 
    Game_logic_State game_lg; 
} App;