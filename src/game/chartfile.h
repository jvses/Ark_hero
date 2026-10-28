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
 *                "tick = N traste duracao" (0-4 = verde..laranja, 5 = HOPO
 *                forçado, 6 = tap, 7 = aberta), "tick = S 2 duracao" (star power) */

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

typedef struct {
    uint32_t tick, duracao;
    uint8_t  traste;         /* como está no arquivo: 0..7 */
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
    SegmentoTempo *tempos;
    int            n_tempos, cap_tempos;
    Compasso      *compassos;
    int            n_compassos, cap_compassos;
    Pista_Dif      dif[DIF_N];
} ArquivoChart;

/* devolve false (e a mensagem em erro) se o texto não for um .chart utilizável */
bool   ChartFile_parse(ArquivoChart *a, const char *texto, size_t tamanho, char *erro, size_t n_erro);
void   ChartFile_free(ArquivoChart *a);
double ChartFile_tempo(const ArquivoChart *a, uint32_t tick);   /* segundos, sem o offset */
int    ChartFile_contar(const ArquivoChart *a, Dificuldade d);  /* notas (acorde = 1) */

/* o Chart da dificuldade d; atraso_s desloca todas as notas (song.ini "delay") */
bool   Chart_from_file(Chart *c, const ArquivoChart *a, Dificuldade d, double atraso_s);

const char *Dificuldade_nome(Dificuldade d);   /* "Expert" ... */

#endif
