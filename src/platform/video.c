#include "video.h"
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
// #include <SDL3/SDL_log.h>

static SDL_Renderer *create_renderer(SDL_Window *win);


bool Video_init(Video *vid){
    // SDL_SetAppMetadata("Ark Hero", "0.1", "ArkHero_Game");
    if(!SDL_InitSubSystem(SDL_INIT_VIDEO)){
        SDL_Log("Video Could not initialize error: %s",SDL_GetError());
        return false;
    }

    log_drivers();

    if(
    !SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES) ||
    !SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2) ||
    !SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0)){
        SDL_Log("[video] nao consegui pedir o perfil GLES: %s", SDL_GetError());
    }

    /* No handheld o script de launch exporta ARKHERO_FULLSCREEN=1.
       No desktop fica em janela, que e mais pratico pra desenvolver. */
    SDL_WindowFlags flags = (SDL_WINDOW_OPENGL);//| SDL_WINDOW_RESIZABLE);
    const char *fs = SDL_getenv("ARKHERO_FULLSCREEN");
    if (fs && SDL_strcmp(fs, "1") == 0) flags |= SDL_WINDOW_FULLSCREEN;

    vid->Window_Ark = SDL_CreateWindow("Ark Hero", DEFAULT_WIDTH, DEFAULT_HEIGTH, flags);
    
    if (!vid->Window_Ark) {
        SDL_Log("Could not create window/render error: %s",SDL_GetError());
        return false;
    }

    vid->ds_render.render = create_renderer(vid->Window_Ark);//SDL_CreateRenderer(vid->Window_Ark, NULL);
    if (!vid->ds_render.render) {
        return false;
    }

    if (!SDL_SetRenderVSync(vid->ds_render.render, 1) &&
        !SDL_SetRenderVSync(vid->ds_render.render, SDL_RENDERER_VSYNC_ADAPTIVE)) {
        SDL_Log("[video] sem vsync: %s", SDL_GetError());
    }

    //Comando que ajusta as dimensões para 640x480 nos comandos do render independente da resolução da tela em que tudo é exibido
    SDL_SetRenderLogicalPresentation(vid->ds_render.render, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    // if(!SDL_SetRenderVSync(vid->ds_render.render, 1)){//esse comando faz o Vsinc e faz o jogo acompanhar o fps padrão da tela do hardware que estiver rodando. Se o hardware puder fazer 144Hz ele faz, se for menor ele ajusta para esse valor menor.

    //     SDL_Log("Vsinc Not suported error: %s\nTring adaptative mode",SDL_GetError());
    //     if(!SDL_SetRenderVSync(vid->ds_render.render, -1)){
    //         SDL_Log("Vsinc adaptative mode error: %s",SDL_GetError());
    //         return false;
    //     }
    // }
    return true;
}

void Video_quit(Video *vid){

    if(vid->ds_render.render)
        SDL_DestroyRenderer(vid->ds_render.render);

    if(vid->Window_Ark)
        SDL_DestroyWindow(vid->Window_Ark);
}


void log_drivers(void)
{
    const int v = SDL_GetVersion();
    char buf[256];
    int i;

    SDL_Log("===== Ark Hero / teste de portabilidade =====");
    SDL_Log("SDL compilado : %d.%d.%d",
            SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);
    SDL_Log("SDL runtime   : %d.%d.%d (%s)",
            SDL_VERSIONNUM_MAJOR(v), SDL_VERSIONNUM_MINOR(v), SDL_VERSIONNUM_MICRO(v),
            SDL_GetRevision());

    buf[0] = '\0';
    for (i = 0; i < SDL_GetNumVideoDrivers(); i++) {
        SDL_strlcat(buf, SDL_GetVideoDriver(i), sizeof(buf));
        SDL_strlcat(buf, " ", sizeof(buf));
    }
    SDL_Log("video disp.   : %s", buf);
    SDL_Log("video em uso  : %s", SDL_GetCurrentVideoDriver());

    buf[0] = '\0';
    for (i = 0; i < SDL_GetNumRenderDrivers(); i++) {
        SDL_strlcat(buf, SDL_GetRenderDriver(i), sizeof(buf));
        SDL_strlcat(buf, " ", sizeof(buf));
    }
    SDL_Log("render disp.  : %s", buf);
}

static SDL_Renderer *create_renderer(SDL_Window *win)
{
    const char *tries[4];
    int n = 0;

    const char *forced = SDL_getenv("ARKHERO_RENDER_DRIVER");
    if (forced && *forced) tries[n++] = forced;
    tries[n++] = "opengles2";   /* o backend do handheld */
    tries[n++] = NULL;          /* deixa a SDL escolher */
    tries[n++] = "software";    /* ultimo recurso */

    for (int i = 0; i < n; i++) {
        SDL_Renderer *r = SDL_CreateRenderer(win, tries[i]);
        if (r) {
            SDL_Log("render em uso : %s", SDL_GetRendererName(r));
            return r;
        }
        SDL_Log("renderer '%s' falhou: %s",
                tries[i] ? tries[i] : "(automatico)", SDL_GetError());
    }
    return NULL;
}



void Video_RenderAll(Video *vid){
    SDL_RenderPresent(vid->ds_render.render); // coloca tudo na tela
}