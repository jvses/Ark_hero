#include "video.h"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>


bool Video_init(Video *vid){
    // SDL_SetAppMetadata("Ark Hero", "0.1", "ArkHero_Game");

    if(!SDL_InitSubSystem(SDL_INIT_VIDEO)){
        SDL_Log("Video Could not initialize error: %s",SDL_GetError());
        return false;
    }

    if (!SDL_CreateWindowAndRenderer("Ark Hero", DEFAULT_WIDTH, DEFAULT_HEIGTH, SDL_WINDOW_RESIZABLE , &vid->Window_Ark, &vid->Render_Ark)) {
        SDL_Log("Could not create window/render error: %s",SDL_GetError());
        return false;
    }

    return true;

}

void Video_quit(Video *vid){

    if(vid->Render_Ark)
        SDL_DestroyRenderer(vid->Render_Ark);

    if(vid->Window_Ark)
        SDL_DestroyWindow(vid->Window_Ark);
}


