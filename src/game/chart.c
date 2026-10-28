#include "game/chart.h"

#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * Música de demonstração: 120 BPM (uma batida = 0,5 s), começando em 2 s.
 * Tem um pouco de tudo para testar: notas simples, sustains, acordes,
 * colcheias e duas frases de star power (o bastante para ativar).
 * ------------------------------------------------------------------------- */
#define G 1    /* verde    */
#define R 2    /* vermelho */
#define Y 4    /* amarelo  */
#define B 8    /* azul     */
#define O 16   /* laranja  */
#define S NOTA_STAR

#define DEMO_BATIDA 0.5   /* segundos por batida (120 BPM) */
#define DEMO_INICIO 2.0   /* segundos até a primeira nota */

typedef struct {
    float   batida;    /* em batidas desde o início */
    float   duracao;   /* em batidas */
    uint8_t mascara;
    uint8_t flags;
} NotaDemo;

static const NotaDemo DEMO[] = {
    /* aquecimento: uma de cada */
    {  0, 0, G, 0 }, {  1, 0, R, 0 }, {  2, 0, Y, 0 }, {  3, 0, B, 0 },
    {  4, 0, O, 0 }, {  5, 0, B, 0 }, {  6, 0, Y, 0 }, {  7, 0, R, 0 },
    /* sustains: segure (e use o whammy) */
    {  8, 1.5f, G, 0 }, { 10, 1.5f, R, 0 }, { 12, 1.5f, Y, 0 },
    /* acordes */
    { 14, 0, G|R, 0 }, { 15, 0, R|Y, 0 }, { 16, 0, Y|B, 0 }, { 17, 0, B|O, 0 },
    { 18, 1.5f, G|Y, 0 },
    /* frase de star power 1 (termina num sustain: whammy enche o medidor) */
    { 20, 0, G, S }, { 20.5f, 0, R, S }, { 21, 0, Y, S }, { 21.5f, 0, B, S },
    { 22, 1.5f, Y, S },
    /* colcheias */
    { 24, 0, G, 0 }, { 24.5f, 0, R, 0 }, { 25, 0, G, 0 }, { 25.5f, 0, R, 0 },
    { 26, 0, Y, 0 }, { 26.5f, 0, B, 0 }, { 27, 0, Y, 0 }, { 27.5f, 0, B, 0 },
    /* frase de star power 2: depois dela o medidor chega a 50% */
    { 28, 0, G|R, S }, { 29, 0, R|Y, S }, { 30, 0, Y|B, S }, { 31, 1.5f, B|O, S },
    /* trecho para usar o star power */
    { 33, 0, O, 0 }, { 33.5f, 0, B, 0 }, { 34, 0, Y, 0 }, { 34.5f, 0, R, 0 },
    { 35, 0, G, 0 }, { 36, 2, G|Y, 0 }, { 38.5f, 0, R|B, 0 }, { 39, 0, Y|O, 0 },
    { 40, 1, R, 0 }, { 41.5f, 0, G, 0 }, { 42, 0, R, 0 }, { 42.5f, 0, Y, 0 },
    { 43, 0, B, 0 }, { 43.5f, 0, O, 0 }, { 44, 2, G|R|Y, 0 },
};

#define N_DEMO ((int)(sizeof(DEMO) / sizeof(DEMO[0])))

/* batidas a cada DEMO_BATIDA, do zero até um pouco depois do fim */
static bool batidas_regulares(Chart *c)
{
    c->n_batidas = (int)((c->fim + 2.0) / DEMO_BATIDA) + 1;
    c->batidas   = malloc(sizeof(Batida) * (size_t)c->n_batidas);
    if (!c->batidas) return false;
    for (int k = 0; k < c->n_batidas; k++) {
        c->batidas[k].t     = k * DEMO_BATIDA;
        c->batidas[k].forte = (k % 4) == 0;
    }
    return true;
}

/* frases de star power numeradas (1, 2, 3...): notas STAR seguidas são da
   mesma frase. Só quando ninguém numerou antes (o parser já numera). */
static void numera_frases(Chart *c)
{
    for (int i = 0; i < c->n; i++)
        if (c->notas[i].frase) return;
    uint16_t frase = 0;
    for (int i = 0; i < c->n; i++) {
        if (!(c->notas[i].flags & NOTA_STAR)) continue;
        if (i == 0 || !(c->notas[i - 1].flags & NOTA_STAR)) frase++;
        c->notas[i].frase = frase;
    }
}

static void calcula_fim(Chart *c)
{
    c->fim = 0.0;
    for (int i = 0; i < c->n; i++) {
        const double fim = c->notas[i].t + c->notas[i].duracao;
        if (fim > c->fim) c->fim = fim;
    }
}

bool Chart_demo(Chart *c)
{
    *c = (Chart){0};
    c->notas = calloc((size_t)N_DEMO, sizeof(Nota));
    if (!c->notas) return false;

    c->n = N_DEMO;
    for (int i = 0; i < N_DEMO; i++) {
        c->notas[i].t       = DEMO_INICIO + DEMO[i].batida * DEMO_BATIDA;
        c->notas[i].duracao = DEMO[i].duracao * DEMO_BATIDA;
        c->notas[i].mascara = DEMO[i].mascara;
        c->notas[i].flags   = DEMO[i].flags;
    }
    calcula_fim(c);
    numera_frases(c);
    strcpy(c->titulo, "Demonstracao");
    if (!batidas_regulares(c)) { Chart_free(c); return false; }
    Chart_reset(c);
    return true;
}

bool Chart_from_notes(Chart *c, const Nota *notas, int n)
{
    *c = (Chart){0};
    c->notas = calloc((size_t)(n > 0 ? n : 1), sizeof(Nota));
    if (!c->notas) return false;
    memcpy(c->notas, notas, sizeof(Nota) * (size_t)n);
    c->n = n;
    calcula_fim(c);
    numera_frases(c);
    strcpy(c->titulo, "Teste");
    if (!batidas_regulares(c)) { Chart_free(c); return false; }
    Chart_reset(c);
    return true;
}

void Chart_reset(Chart *c)
{
    for (int i = 0; i < c->n; i++) {
        c->notas[i].estado    = NOTA_PENDENTE;
        c->notas[i].segurando = false;
        c->notas[i].solto     = -1.0;
    }
}

void Chart_free(Chart *c)
{
    free(c->notas);
    free(c->batidas);
    *c = (Chart){0};
}
