/* Ark Hero: o ponto de encontro dos módulos.
 *
 * Só este arquivo conhece o App inteiro. Cada módulo recebe só o pedaço que é
 * dele, e é aqui que a plataforma (video, input, audio) e o jogo (game, draw)
 * se ligam.
 *
 * Um frame (SDL_AppIterate), sempre nesta ordem:
 *   1. tempo   2. entrada   3. som e relógio   4. lógica   5. pedidos   6. desenho */

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "draw/draw.h"
#include "game/game.h"
#include "game/songini.h"
#include "platform/audio.h"
#include "platform/carregador.h"
#include "platform/input.h"
#include "platform/musica.h"
#include "platform/video.h"

typedef enum { SOM_NADA, SOM_PARTIDA, SOM_TESTE } OQueToca;

typedef struct {
    Video  video;
    Input  input;
    Audio  audio;
    Draw   draw;
    Game   game;

    /* som */
    OQueToca   tocando;
    int        partida_tocando;   /* g->partida que está tocando (para saber quando recomeçar) */
    Pcm        cliques;           /* metrônomo da partida, gerado a partir das batidas do chart */
    Carregador carga;             /* decodifica a música de teste numa thread */
    Pcm        musica_teste;
    Musica     teste;             /* a primeira música encontrada */
    bool       tem_teste;
    char       pasta_teste[512];
    char       info_musica[160], info_faixas[256], info_carga[160];

    Uint64 ultimo_ns;
    bool   sair;
} App;

/* ---------------------------------------------------------------------------
 * Música de teste: a primeira pasta de música em songs/ (no console, a pasta
 * do jogo) ou assets/musics/ (no PC, ao lado do repositório).
 * ------------------------------------------------------------------------- */
static void procura_musica_teste(App *app)
{
    const char *raizes[] = { "songs/", "assets/musics/" };
    for (size_t i = 0; i < SDL_arraysize(raizes) && !app->tem_teste; i++) {
        SDL_snprintf(app->pasta_teste, sizeof(app->pasta_teste), "%s%s", SDL_GetBasePath(), raizes[i]);
        app->tem_teste = Musica_list(app->pasta_teste, &app->teste, 1) == 1;
    }
    if (!app->tem_teste) {
        SDL_snprintf(app->pasta_teste, sizeof(app->pasta_teste), "%ssongs/", SDL_GetBasePath());
        SDL_Log("[musica] nenhuma musica de teste (procurei em songs/ e assets/musics/)");
        return;
    }

    /* nome e artista vêm do song.ini; ler o arquivo é da plataforma, entender é do game */
    SongIni ini = {0};
    size_t tam = 0;
    char *texto = app->teste.ini[0] ? SDL_LoadFile(app->teste.ini, &tam) : NULL;
    if (texto) {
        SongIni_parse(texto, tam, &ini);
        SDL_free(texto);
    }
    if (ini.nome[0]) SDL_snprintf(app->info_musica, sizeof(app->info_musica), "%s - %s", ini.nome, ini.artista);
    else             SDL_strlcpy(app->info_musica, app->teste.nome, sizeof(app->info_musica));

    app->info_faixas[0] = '\0';
    for (int i = 0; i < app->teste.n_faixas; i++) {
        const char *barra = SDL_strrchr(app->teste.faixas[i], '/');
        SDL_strlcat(app->info_faixas, barra ? barra + 1 : app->teste.faixas[i], sizeof(app->info_faixas));
        SDL_strlcat(app->info_faixas, "  ", sizeof(app->info_faixas));
    }
    SDL_Log("[musica] teste: %s (%d faixas)", app->info_musica, app->teste.n_faixas);
}

/* ---------------------------------------------------------------------------
 * Som da partida: por enquanto um metrônomo nas batidas do chart. Com ele dá
 * para ouvir se as notas e as linhas de batida estão no tempo do som.
 * ------------------------------------------------------------------------- */
static void comeca_cliques(App *app)
{
    const Chart *c = &app->game.chart;
    Audio_stop(&app->audio);
    Pcm_free(&app->cliques);

    double *tempo = SDL_malloc(sizeof(double) * (size_t)(c->n_batidas > 0 ? c->n_batidas : 1));
    bool   *forte = SDL_malloc(sizeof(bool) * (size_t)(c->n_batidas > 0 ? c->n_batidas : 1));
    if (tempo && forte) {
        for (int i = 0; i < c->n_batidas; i++) {
            tempo[i] = c->batidas[i].t;
            forte[i] = c->batidas[i].forte;
        }
        Audio_make_clicks(&app->cliques, tempo, forte, c->n_batidas, c->fim + 3.0);
    }
    SDL_free(tempo);
    SDL_free(forte);

    Audio_play(&app->audio, &app->cliques, 0.0);
    app->tocando         = SOM_PARTIDA;
    app->partida_tocando = app->game.partida;
}

/* O que deve estar tocando sai do estado do jogo (e não de eventos soltos):
 * na pista, o som da partida; fora dela, nada; na tela de teste, o teste. */
static void sincroniza_som(App *app)
{
    const Game *g = &app->game;
    Audio_set_volume(&app->audio, g->opcoes.volume / 100.0f);

    const bool na_partida = g->tela == TELA_JOGO && !g->terminou;
    if (na_partida) {
        if (app->tocando != SOM_PARTIDA || app->partida_tocando != g->partida) comeca_cliques(app);
        Audio_pause(&app->audio, g->pausado);
    } else if (app->tocando == SOM_PARTIDA) {
        Audio_stop(&app->audio);
        app->tocando = SOM_NADA;
    }
    if (app->tocando == SOM_TESTE && g->tela != TELA_AUDIO) {
        Audio_stop(&app->audio);
        app->tocando = SOM_NADA;
    }

    /* a carga da música de teste terminou? */
    const EstadoCarga e = Carregador_state(&app->carga);
    if (e == CARGA_PRONTO) {
        const double seg = app->carga.segundos, audio = app->carga.segundos_audio;
        Carregador_take(&app->carga, &app->musica_teste);
        SDL_snprintf(app->info_carga, sizeof(app->info_carga),
                     "Carregada: %.1f s de musica em %.2f s (%.0fx o tempo real)",
                     (double)app->musica_teste.frames / AUDIO_TAXA, seg, seg > 0 ? audio / seg : 0.0);
    } else if (e == CARGA_ERRO) {
        SDL_snprintf(app->info_carga, sizeof(app->info_carga), "Erro: %s", app->carga.erro);
    }

    Audio_update(&app->audio);
}

/* ---------------------------------------------------------------------------
 * Pedidos do jogo: o game pede, a plataforma faz
 * ------------------------------------------------------------------------- */
static void atende_pedido(App *app)
{
    switch (app->game.pedido) {
    case PEDIDO_MAPEAR:              Input_capture_start(&app->input, app->game.pedido_acao); break;
    case PEDIDO_RESTAURAR_CONTROLES: Input_reset_defaults(&app->input);                       break;
    case PEDIDO_TROCAR_ANALOGICO:    Input_toggle_stick(&app->input);                         break;
    case PEDIDO_SAIR:                app->sair = true;                                        break;

    case PEDIDO_AUDIO_CARREGAR:
        if (!app->tem_teste || Carregador_state(&app->carga) == CARGA_ANDANDO) break;
        if (app->tocando == SOM_TESTE) { Audio_stop(&app->audio); app->tocando = SOM_NADA; }
        Pcm_free(&app->musica_teste);
        app->info_carga[0] = '\0';
        Carregador_start(&app->carga, (const char (*)[512])app->teste.faixas, app->teste.n_faixas);
        break;

    case PEDIDO_AUDIO_TOCAR:
        if (app->musica_teste.frames == 0) break;   /* ainda não carregou */
        if (app->tocando == SOM_TESTE) {
            Audio_pause(&app->audio, !app->audio.pausado);
        } else {
            Audio_play(&app->audio, &app->musica_teste, 0.0);
            app->tocando = SOM_TESTE;
        }
        break;

    case PEDIDO_NENHUM:
        break;
    }
}

/* ---------------------------------------------------------------------------
 * O que a tela mostra e não é do jogo
 * ------------------------------------------------------------------------- */
static void monta_hud(App *app, Hud *h, Uint64 agora_ns)
{
    h->fps           = app->video.fps;
    h->capturando    = app->input.capturando;
    h->captura_resta = Input_capture_left(&app->input);
    h->ultimo        = app->input.ultimo;
    h->analogico     = Input_stick_label(&app->input);
    for (int a = 0; a < ACAO_N; a++) {
        h->pad[a]   = Input_pad_label(&app->input, (Acao)a);
        h->tecla[a] = Input_key_label(&app->input, (Acao)a);
    }

    HudAudio *ha      = &h->audio;
    ha->ok            = Audio_ok(&app->audio);
    ha->driver        = app->audio.driver;
    ha->buffer_frames = app->audio.frames_buffer;
    ha->musica        = app->tem_teste ? app->info_musica : NULL;
    ha->pasta         = app->pasta_teste;
    ha->faixas        = app->info_faixas;
    ha->ignoradas     = app->teste.ignoradas;
    ha->carga         = (int)Carregador_state(&app->carga);
    ha->progresso     = Carregador_progress(&app->carga);
    ha->carga_info    = app->info_carga[0] ? app->info_carga : NULL;
    ha->tocando       = app->tocando == SOM_TESTE;
    ha->pausado       = app->audio.pausado;
    ha->duracao       = (double)app->musica_teste.frames / AUDIO_TAXA;
    if (ha->tocando) {
        ha->t_cru   = Audio_raw_time(&app->audio);
        ha->t_suave = Audio_song_time(&app->audio, agora_ns);
    }
}

/* ---------------------------------------------------------------------------
 * Callbacks da SDL
 * ------------------------------------------------------------------------- */
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
    Audio_init(&app->audio);   /* sem som o jogo roda do mesmo jeito (relógio pelo dt) */

    /* imagens: sempre relativas ao executável. No PC o CMake põe um link
       assets/ ao lado do binário; no console o pacote tem a pasta de verdade. */
    char pasta[512];
    SDL_snprintf(pasta, sizeof(pasta), "%sassets/images/", SDL_GetBasePath());
    if (!Draw_init(&app->draw, app->video.ren, pasta)) return SDL_APP_FAILURE;

    Game_init(&app->game);
    procura_musica_teste(app);

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

SDL_AppResult SDL_AppIterate(void *appstate)
{
    App *app = appstate;

    /* 1. TEMPO do sistema (para animações e para suavizar o relógio do áudio) */
    const Uint64 agora_ns = SDL_GetTicksNS();
    double dt = (double)(agora_ns - app->ultimo_ns) / 1e9;
    app->ultimo_ns = agora_ns;
    if (dt > 0.05) dt = 0.05;   /* uma travada não faz a pista pular */

    /* 2. ENTRADA */
    const Botoes botoes = Input_frame(&app->input);

    /* 3. SOM E RELÓGIO: na partida, o tempo da música é o do áudio */
    sincroniza_som(app);
    app->game.relogio_audio = app->tocando == SOM_PARTIDA && Audio_ok(&app->audio);
    if (app->game.relogio_audio) app->game.t_audio = Audio_song_time(&app->audio, agora_ns);

    /* 4. LÓGICA: o jogo julga e produz eventos; a pontuação reage a eles */
    Game_update(&app->game, &botoes, dt);
    Score_apply_events(&app->game);

    /* 5. PEDIDOS do jogo para a plataforma */
    atende_pedido(app);

    /* 6. DESENHO */
    Hud hud = {0};
    monta_hud(app, &hud, agora_ns);
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

    /* ordem inversa do Init: a thread de carga para antes de tudo, o som para
       antes de liberar o que ele toca, as texturas morrem antes do renderer */
    Carregador_quit(&app->carga);
    Game_quit(&app->game);
    Draw_quit(&app->draw);
    Audio_quit(&app->audio);
    Pcm_free(&app->cliques);
    Pcm_free(&app->musica_teste);
    Input_quit(&app->input);
    Video_quit(&app->video);
    SDL_free(app);
    /* sem SDL_Quit(): a SDL chama sozinha depois daqui */
}
