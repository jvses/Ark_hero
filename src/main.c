// #include <SDL3/SDL_init.h>
#include "draw/desenhos.h"
#include "game/game_state.h"
#include "platform/input.h"
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#include <stdint.h>
#include <sys/types.h>
#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL_main.h>
#include "app.h"


// static SDL_Time 


// //funções para despoluir as callback
// void update_video_screen(){

// }






// App jogo;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    App *jogo = SDL_calloc(1, sizeof(App));
    if (!jogo) return SDL_APP_FAILURE;
    *appstate = jogo;            /* a partir daqui a SDL entrega o app(jogo) para os outros callbacks */

    SDL_SetAppMetadata("Ark Hero", "0.1", "ArkHero_Game");
    
    //parte responsável pelo vídeo/tela
    if(!Video_init(&jogo->video)) return SDL_APP_FAILURE;
    else SDL_Log("Vídeo initialized correctly");
    jogo->game_lg.fps_inicio = jogo->game_lg.frame_anterior = SDL_GetTicks();
    jogo->game_lg.fps_frames=0;
    jogo->game_lg.cor_bg=20;
    jogo->game_lg.razao=1;
    jogo->game_lg.game_menu_state=idle;
    jogo->game_lg.rec_addr[0]=200;
    jogo->game_lg.rec_addr[1]=300;
    jogo->game_lg.game_menu_state = idle;
    jogo->game_lg.apertou_P = false;

    
    //parte responsável pelo input/gamepad
    if(!Input_init(&jogo->input)) return SDL_APP_FAILURE;
    else SDL_Log("Input/Gamepad initialized correctly");

    //inicializa valores de botão do menu em false
    jogo->input.menu_btn.atual=false;
    jogo->input.menu_btn.anterior = false;

    Draw_memory_prepare_stuffs(&jogo->video.ds_render,&jogo->game_lg);

    jogo->input.sair = false;

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    App *jogo=appstate;

    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }

    Input_Event_Handler(&jogo->input,event);

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    const Uint64 agora=SDL_GetTicks();//variavel captacao de tempo

    App *jogo=appstate;


    Clear_screen(&jogo->video.ds_render);
    jogo->game_lg.apertou_P = Input_update_gl_menu(&jogo->input);

    // bool need_change_bg=false;
    GS_change_game_menu(&jogo->game_lg);// vai ver se tem ou não que alterar o estado do menu do jogo e mudar de acordo.
    if(jogo->game_lg.apertou_P){
        Draw_memory_prepare_stuffs(&jogo->video.ds_render,&jogo->game_lg);
        SDL_Log("Menu: %d",jogo->game_lg.game_menu_state);
        jogo->game_lg.apertou_P=false;
    }

    Draw_bg_image(&jogo->video.ds_render,&jogo->game_lg);
    Draw_Rectangle(&jogo->video.ds_render, jogo->game_lg.cor_bg);
    Input_update_gl_arrows(&jogo->input,&jogo->game_lg);
    GS_rec_move(&jogo->game_lg);//aqui altera o endereço do retângulo que se move
    Draw_music_tab(&jogo->video.ds_render);
    Draw_Elipse(&jogo->video.ds_render,jogo->game_lg.rec_addr[0],jogo->game_lg.rec_addr[1],100,70,Draw_get_color(azul));
    

    if(jogo->game_lg.cor_bg>=254){
        jogo->game_lg.razao = -1;
    } else if(jogo->game_lg.cor_bg<50){
        jogo->game_lg.razao = 1;
    }
    jogo->game_lg.cor_bg+=jogo->game_lg.razao;
    // SDL_Log("Log depois: i= %d\nrazao=%d",jogo->game_lg.cor,jogo->game_lg.razao);

    //processo para contagem de fps
    const Uint64 intervalo = agora - jogo->game_lg.frame_anterior;
    jogo->game_lg.frame_anterior = agora;
    jogo->game_lg.fps_frames++;
    if (agora - jogo->game_lg.fps_inicio >= 1000) {//se a diferença de tempo entre o primeiro frame e o agora for >= 1000 então passou 1 segundo
        jogo->game_lg.fps =  jogo->game_lg.fps_frames *1000.0f/ (float)(agora - jogo->game_lg.fps_inicio);
        SDL_Log("FPS %.1f",jogo->game_lg.fps);
        jogo->game_lg.fps_inicio = agora;
        jogo->game_lg.fps_frames=0;
    }

    Draw_print_fps(&jogo->video.ds_render, jogo->game_lg.fps);

    Video_RenderAll(&jogo->video);
    if (jogo->input.sair) {
        return SDL_APP_SUCCESS;
    }


    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    App *jogo = appstate;
    /* SDL will clean up the window/renderer for us. */
    Video_quit(&jogo->video);
    Input_quit(&jogo->input);
    SDL_free(jogo);
    // SDL_Quit();
}
