#ifndef ARK_DRAW_INTERNAL_H
#define ARK_DRAW_INTERNAL_H

/* Uso interno do draw/: o que um arquivo do draw expõe para os outros.
 * O resto do projeto só conhece o draw.h. */

#include <SDL3/SDL.h>

#include "draw/draw.h"

/* ---------------------------------------------------------------------------
 * Paleta
 * ------------------------------------------------------------------------- */
extern const SDL_Color COR_TRASTE[5];   /* verde, vermelho, amarelo, azul, laranja */
extern const SDL_Color COR_ACENTO;      /* rosa vaporwave: seleção, bordas da pista */
extern const SDL_Color COR_CIANO;
extern const SDL_Color COR_PAINEL;      /* fundo translúcido dos menus */
extern const SDL_Color COR_TEXTO;
extern const SDL_Color COR_TEXTO_FRACO;
extern const SDL_Color COR_TEXTO_ESCURO;

SDL_Color cor_alfa(SDL_Color c, float alfa);      /* mesma cor, alfa 0..1 multiplicando */
SDL_Color cor_mistura(SDL_Color a, SDL_Color b, float t);   /* t = 0 -> a, 1 -> b */
SDL_FColor cor_f(SDL_Color c);                   /* 0..255 -> 0.0..1.0 (SDL_Vertex) */

/* ---------------------------------------------------------------------------
 * formas.c: primitivas que a SDL não tem
 * ------------------------------------------------------------------------- */
void elipse(SDL_Renderer *ren, float cx, float cy, float w, float h, SDL_Color cor);
void ret(SDL_Renderer *ren, SDL_FRect r, SDL_Color cor);
void ret_arredondado(SDL_Renderer *ren, SDL_FRect r, float raio, SDL_Color cor);
void contorno_arredondado(SDL_Renderer *ren, SDL_FRect r, float raio, SDL_Color cor);
void ret_degrade(SDL_Renderer *ren, SDL_FRect r, SDL_Color topo, SDL_Color base);

/* Texto com a fonte de debug da SDL: 8x8 px vezes a escala, só ASCII. */
#define TEXTO_PX 8.0f
float texto_largura(const char *s, float escala);
void  texto(SDL_Renderer *ren, float x, float y, float escala, SDL_Color cor, const char *s);
void  texto_centro(SDL_Renderer *ren, float cx, float y, float escala, SDL_Color cor, const char *s);
void  texto_sombra(SDL_Renderer *ren, float x, float y, float escala, SDL_Color cor, const char *s);

/* ---------------------------------------------------------------------------
 * pista.c: a highway em perspectiva
 * ------------------------------------------------------------------------- */
void pista_desenha(SDL_Renderer *ren, const Game *g);

#endif
