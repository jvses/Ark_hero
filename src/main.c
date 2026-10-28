/* Ark Hero: o ponto de encontro dos módulos.
 *
 * Só este arquivo conhece o App inteiro. Cada módulo recebe só o pedaço que é
 * dele, e é aqui que a plataforma (video, input) e o jogo (game, draw) se ligam.
 *
 * Um frame (SDL_AppIterate), sempre nesta ordem:
 *   1. tempo     2. entrada     3. lógica     4. pedidos do jogo     5. desenho */

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "draw/draw.h"
#include "game/game.h"
#include "platform/input.h"
#include "platform/video.h"

typedef struct {
    Video  video;
    Input  input;
    Draw   draw;
    Game   game;
    Uint64 ultimo_ns;   /* para o dt, enquanto não existe o relógio do áudio */
    bool   sair;
} App;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    SDL_SetAppMetadata("Ark Hero", "0.1", "com.arkhero.game");

    App *app = SDL_calloc(1, sizeof(App));   /* tudo zerado: o Quit aguenta init parcial */
    if (!app) return SDL_APP_FAILURE;
    *appstate = app;

    if (!Video_init(&app->video)) return SDL_APP_FAILURE;
    if (!Input_init(&app->input)) return SDL_APP_FAILURE;

    /* imagens: sempre relativas ao executável. No PC o CMake põe um link
       assets/ ao lado do binário; no console o pacote tem a pasta de verdade. */
    char pasta[512];
    SDL_snprintf(pasta, sizeof(pasta), "%sassets/images/", SDL_GetBasePath());
    if (!Draw_init(&app->draw, app->video.ren, pasta)) return SDL_APP_FAILURE;

    Game_init(&app->game);

    app->ultimo_ns = SDL_GetTicksNS();
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    App *app = appstate;
    if (event->type == SDL_EVENT_QUIT) return SDL_APP_SUCCESS;

    Input_event(&app->input, event);   /* só anota; quem age é o Iterate */
    return app->input.sair ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
}

/* o game pede, a plataforma faz */
static void atende_pedido(App *app)
{
    switch (app->game.pedido) {
    case PEDIDO_MAPEAR:              Input_capture_start(&app->input, app->game.pedido_acao); break;
    case PEDIDO_RESTAURAR_CONTROLES: Input_reset_defaults(&app->input);                       break;
    case PEDIDO_SAIR:                app->sair = true;                                        break;
    case PEDIDO_NENHUM:              break;
    }
}

/* o que a tela mostra e não é do jogo: FPS, nomes dos botões, captura */
static void monta_hud(const App *app, Hud *h)
{
    h->fps           = app->video.fps;
    h->capturando    = app->input.capturando;
    h->captura_resta = Input_capture_left(&app->input);
    h->ultimo        = app->input.ultimo;
    for (int a = 0; a < ACAO_N; a++) {
        h->pad[a]   = Input_pad_label(&app->input, (Acao)a);
        h->tecla[a] = Input_key_label(&app->input, (Acao)a);
    }
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    App *app = appstate;

    /* 1. TEMPO */
    const Uint64 agora_ns = SDL_GetTicksNS();
    double dt = (double)(agora_ns - app->ultimo_ns) / 1e9;
    app->ultimo_ns = agora_ns;
    if (dt > 0.05) dt = 0.05;   /* uma travada não faz a pista pular */

    /* 2. ENTRADA */
    const Botoes botoes = Input_frame(&app->input);

    /* 3. LÓGICA */
    Game_update(&app->game, &botoes, dt);

    /* 4. PEDIDOS do jogo para a plataforma */
    atende_pedido(app);

    /* 5. DESENHO */
    Hud hud;
    monta_hud(app, &hud);
    Video_begin_frame(&app->video);
    Draw_frame(&app->draw, app->video.ren, &app->game, &hud);
    Video_end_frame(&app->video);   /* o vsync espera aqui */

    return (app->sair || app->input.sair) ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    (void)result;
    App *app = appstate;
    if (!app) return;

    /* ordem inversa do Init: as texturas morrem antes do renderer */
    Draw_quit(&app->draw);
    Input_quit(&app->input);
    Video_quit(&app->video);
    SDL_free(app);
    /* sem SDL_Quit(): a SDL chama sozinha depois daqui */
}
