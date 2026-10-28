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

/* O que aparece na tela mas não é estado do jogo: vem da plataforma.
 * O main.c preenche a cada frame (o draw não conhece o input nem o video). */
typedef struct {
    float       fps;
    int         capturando;             /* ação esperando botão novo, ou -1 */
    double      captura_resta;          /* segundos */
    const char *pad[ACAO_N];            /* rótulo do botão de cada ação: "L1" */
    const char *tecla[ACAO_N];          /* rótulo da tecla de cada ação: "A" */
    const char *ultimo;                 /* último botão apertado (diagnóstico) */
} Hud;

bool Draw_init(Draw *d, SDL_Renderer *ren, const char *pasta_imagens);
void Draw_quit(Draw *d);
void Draw_frame(const Draw *d, SDL_Renderer *ren, const Game *g, const Hud *h);

#endif
