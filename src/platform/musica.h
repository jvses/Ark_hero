#ifndef ARK_MUSICA_H
#define ARK_MUSICA_H

/* Pastas de música no formato do Clone Hero: uma pasta por música, com
 * notes.chart (ou notes.mid), song.ini e as faixas de áudio separadas
 * (song, guitar, bass, rhythm, drums, keys, vocals, crowd...).
 * Aqui só se acha arquivo; quem interpreta o song.ini e o chart é o game. */

#include <SDL3/SDL.h>

#include "platform/carregador.h"

typedef struct {
    char pasta[512];                               /* termina em '/' */
    char nome[128];                                /* nome da pasta */
    char faixas[CARREGADOR_MAX_FAIXAS][512];       /* caminhos dos áudios */
    int  n_faixas;
    char chart[512];                               /* notes.chart, ou "" */
    char ini[512];                                 /* song.ini, ou "" */
    int  ignoradas;                                /* áudios em formato ainda não suportado (mp3) */
} Musica;

/* Procura músicas dentro de "raiz" (até 3 níveis de subpastas). Devolve quantas. */
int  Musica_list(const char *raiz, Musica *saida, int max);
bool Musica_open(const char *pasta, Musica *m);   /* preenche a partir de uma pasta */

#endif
