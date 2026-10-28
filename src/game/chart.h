#ifndef ARK_CHART_H
#define ARK_CHART_H

/* As notas de uma música, já em segundos e prontas para o jogo.
 * Quem preenche: por enquanto Chart_demo(); na próxima etapa, o parser de
 * .chart. O resto do jogo não percebe a diferença. */

#include <stdbool.h>
#include <stdint.h>

#define NOTA_STAR 0x01   /* flag: a nota faz parte de uma frase de star power */

typedef enum {
    NOTA_PENDENTE,
    NOTA_ACERTADA,
    NOTA_PERDIDA
} EstadoNota;

typedef struct {
    /* dados da música */
    double  t;          /* instante da nota (segundos) */
    double  duracao;    /* sustain (segundos); 0 = nota simples */
    uint8_t mascara;    /* trastes: bit 0 = verde ... bit 4 = laranja; acorde = vários bits */
    uint8_t flags;      /* NOTA_STAR */

    /* estado do julgamento (zerado a cada partida, Chart_reset) */
    uint8_t estado;     /* EstadoNota */
    bool    segurando;  /* sustain sendo segurado agora */
    double  solto;      /* t em que o sustain foi solto (ou terminou) */
} Nota;

typedef struct {
    double t;
    bool   forte;       /* primeira batida do compasso */
} Batida;

typedef struct {
    Nota   *notas;      /* ordenadas por t */
    int     n;
    Batida *batidas;    /* linhas de batida da pista */
    int     n_batidas;
    double  fim;        /* fim da última nota (segundos) */
    char    titulo[64];
} Chart;

bool Chart_demo(Chart *c);                                  /* a música de demonstração */
bool Chart_from_notes(Chart *c, const Nota *notas, int n);  /* cópia (usado nos testes) */
void Chart_reset(Chart *c);                                 /* todas as notas pendentes de novo */
void Chart_free(Chart *c);

#endif
