#ifndef ARK_CHARTFILE_H
#define ARK_CHARTFILE_H

/* Leitura do formato .chart (Feedback / Moonscraper / Clone Hero).
 *
 * O arquivo guarda tudo em TICKS; os segundos dependem do andamento, que
 * muda ao longo da música ([SyncTrack]). Por isso o parser monta um MAPA DE
 * TEMPO uma vez, e a conversão tick -> segundos é uma busca binária nele
 * (ARQUITETURA 6.6). Depois, Chart_from_file gera o Chart de uma dificuldade,
 * já em segundos: nenhum tick circula durante a partida.
 *
 * Seções lidas:
 *   [Song]       Resolution (ticks por semínima), Offset (s)
 *   [SyncTrack]  "tick = B bpm*1000", "tick = TS num [expoente do denominador]"
 *   [EasySingle] [MediumSingle] [HardSingle] [ExpertSingle]
 *                "tick = N traste duracao" (0-4 = verde..laranja, 5 = inverte
 *                o HOPO, 6 = tap, 7 = aberta), "tick = S 2 duracao" (star power)
 *
 * O .mid (midifile.c) preenche a mesma estrutura, pelas funções ChartFile_add_*. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game/chart.h"

typedef enum { DIF_FACIL, DIF_MEDIO, DIF_DIFICIL, DIF_EXPERT, DIF_N } Dificuldade;

typedef struct {
    uint32_t tick;
    double   t;              /* esse tick em segundos */
    double   seg_por_tick;   /* 60 / (bpm * resolucao) */
} SegmentoTempo;

typedef struct {
    uint32_t tick;
    int      num, den;       /* 3/4, 6/8... */
} Compasso;

/* NotaBruta.traste: 0..4 = verde..laranja, e os marcadores abaixo */
enum {
    BRUTA_INVERTE_HOPO = 5,   /* .chart "N 5": troca HOPO <-> palhetada */
    BRUTA_TAP          = 6,
    BRUTA_ABERTA       = 7,
    BRUTA_FORCA_HOPO   = 8,   /* .mid: força HOPO */
    BRUTA_FORCA_PALHETA = 9   /* .mid: força palhetada */
};

typedef struct {
    uint32_t tick, duracao;
    uint8_t  traste;
} NotaBruta;

typedef struct {
    uint32_t tick, duracao;
} FraseBruta;

typedef struct {
    NotaBruta  *notas;
    int         n_notas, cap_notas;
    FraseBruta *frases;
    int         n_frases, cap_frases;
} Pista_Dif;

typedef struct {
    int            resolucao;
    double         offset;   /* [Song] Offset, em segundos */
    uint32_t       corte_sustain;   /* sustain mais curto que isso (ticks) vira nota simples */
    SegmentoTempo *tempos;
    int            n_tempos, cap_tempos;
    Compasso      *compassos;
    int            n_compassos, cap_compassos;
    Pista_Dif      dif[DIF_N];
} ArquivoChart;

/* devolve false (e a mensagem em erro) se o texto não for um .chart utilizável */
bool   ChartFile_parse(ArquivoChart *a, const char *texto, size_t tamanho, char *erro, size_t n_erro);

/* .mid (midifile.c): nota_sp = nota do star power (song.ini "multiplier_note", em geral 116) */
bool   ChartFile_parse_mid(ArquivoChart *a, const unsigned char *dados, size_t tamanho, int nota_sp,
                           char *erro, size_t n_erro);

/* para os leitores de formato: acrescentar e, no fim, fechar (mapa de tempo, ordem, conferência) */
bool   ChartFile_add_tempo(ArquivoChart *a, uint32_t tick, double bpm);
bool   ChartFile_add_compasso(ArquivoChart *a, uint32_t tick, int num, int den);
bool   ChartFile_add_nota(Pista_Dif *d, uint32_t tick, uint32_t duracao, int traste);
bool   ChartFile_add_frase(Pista_Dif *d, uint32_t tick, uint32_t duracao);
bool   ChartFile_finish(ArquivoChart *a, char *erro, size_t n_erro);
void   ChartFile_free(ArquivoChart *a);
double ChartFile_tempo(const ArquivoChart *a, uint32_t tick);   /* segundos, sem o offset */
int    ChartFile_contar(const ArquivoChart *a, Dificuldade d);  /* notas (acorde = 1) */

/* o Chart da dificuldade d; atraso_s desloca todas as notas (song.ini "delay") */
bool   Chart_from_file(Chart *c, const ArquivoChart *a, Dificuldade d, double atraso_s);

const char *Dificuldade_nome(Dificuldade d);   /* "Expert" ... */

#endif
