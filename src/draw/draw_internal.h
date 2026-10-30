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
extern const SDL_Color COR_JOGADOR[4];  /* uma cor por jogador: rótulos e bordas da pista */

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

/* ---------------------------------------------------------------------------
 * fonte.c: texto com a fonte de pixel do jogo. Cada caractere ocupa uma célula
 * de 12x16 px vezes a escala (inteira: 1, 2, 3). O texto é UTF-8: acentos e
 * pontuação aparecem; o que a fonte não tiver vira '?'. y é o topo da célula
 * (as maiúsculas começam 3 px abaixo dele; a linha de base fica em y + 14).
 * ------------------------------------------------------------------------- */
#define TEXTO_W 12.0f
#define TEXTO_H 16.0f
bool  fonte_init(SDL_Renderer *ren, const char *arquivo);
void  fonte_quit(void);
float texto_largura(const char *s, float escala);
void  texto(SDL_Renderer *ren, float x, float y, float escala, SDL_Color cor, const char *s);
void  texto_centro(SDL_Renderer *ren, float cx, float y, float escala, SDL_Color cor, const char *s);
void  texto_sombra(SDL_Renderer *ren, float x, float y, float escala, SDL_Color cor, const char *s);
void  texto_corta(char *destino, size_t n, const char *s, float largura, float escala);

/* ---------------------------------------------------------------------------
 * pista.c: a highway em perspectiva. Com vários jogadores, as pistas ficam
 * lado a lado: pos = 0..n-1, da esquerda para a direita.
 * ------------------------------------------------------------------------- */
void  pista_desenha(SDL_Renderer *ren, const Game *g, const Jogador *j, int pos, int n);
float pista_centro(int pos, int n);        /* x do centro da pista */
float pista_largura(int n);                /* largura na linha de acerto */
#define PISTA_Y_ACERTO 422.0f              /* y da linha de acerto */

#endif
