#ifndef ARK_DRAW_H
#define ARK_DRAW_H

/* A aparência do jogo: transforma o estado (const Game *) em pixels.
 * Não altera o jogo e não cria nem destrói textura durante o frame. */

#include <SDL3/SDL.h>

#include "game/game.h"

enum { IMAGEM_INICIO, IMAGEM_JOGO, IMAGEM_CONFIG, IMAGEM_N };

typedef struct {
    SDL_Texture *imagem[IMAGEM_N];   /* fundos, carregados uma vez no Draw_init */
} Draw;

/* Estado do áudio, para a tela de teste de áudio (vem da plataforma). */
typedef struct {
    bool        ok;              /* existe dispositivo de áudio */
    const char *driver;
    int         buffer_frames;
    const char *musica;          /* "Titulo - Artista", ou NULL se não achou nenhuma */
    const char *pasta;           /* onde a música de teste foi procurada */
    const char *faixas;          /* "guitar.opus  bass.opus ..." */
    int         ignoradas;       /* faixas em formato ainda não suportado (mp3) */
    int         carga;           /* 0 parado, 1 carregando, 2 pronta, 3 erro */
    float       progresso;       /* 0..1 */
    const char *carga_info;      /* resumo da carga, ou o erro */
    bool        tocando, pausado;
    double      t_cru, t_suave;  /* os dois relógios, para comparar */
    double      duracao;         /* segundos */
} HudAudio;

/* O que aparece na tela mas não é estado do jogo: vem da plataforma.
 * O main.c preenche a cada frame (o draw não conhece o input nem o video). */
typedef struct {
    float       fps;
    int         capturando;             /* ação esperando botão novo, ou -1 */
    double      captura_resta;          /* segundos */
    const char *pad[ACAO_N];            /* rótulo do botão de cada ação: "L1" */
    const char *tecla[ACAO_N];          /* rótulo da tecla de cada ação: "A" */
    const char *ultimo;                 /* último botão apertado (diagnóstico) */
    const char *analogico;              /* analógico do whammy: "Esquerdo" / "Direito" */
    HudAudio    audio;
} Hud;

bool Draw_init(Draw *d, SDL_Renderer *ren, const char *pasta_imagens);
void Draw_quit(Draw *d);
void Draw_frame(const Draw *d, SDL_Renderer *ren, const Game *g, const Hud *h);

#endif
