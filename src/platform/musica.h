#ifndef ARK_MUSICA_H
#define ARK_MUSICA_H

/* Pastas de música no formato do Clone Hero: uma pasta por música, com
 * notes.chart (ou notes.mid), song.ini e as faixas de áudio separadas
 * (song, guitar, bass, rhythm, drums, keys, vocals, crowd...).
 *
 * A varredura das pastas roda numa thread (uma biblioteca grande num cartão
 * SD lento demora). Aqui só se acha e se lê arquivo; quem interpreta o
 * song.ini e o chart é o game. */

#include <SDL3/SDL.h>

#include "platform/carregador.h"

#define MUSICA_MAX_RAIZES 8

typedef struct {
    char   pasta[512];                          /* termina em '/' */
    char   nome[96];                            /* nome da pasta */
    char   grupo[256];                          /* subpastas entre a raiz e a música: "GH3/Tier 1" */
    char   chave[200];                          /* identifica a música nos recordes (o main.c preenche) */
    char   faixas[CARREGADOR_MAX_FAIXAS][32];   /* nomes dos arquivos de áudio */
    int    n_faixas;
    char   chart[32];                           /* "notes.chart" ou "notes.mid" (como está no disco) */
    int    ignoradas;                           /* faixas num formato que o jogo não lê (flac, m4a...) */
    char  *ini_texto;                           /* o song.ini lido (a thread lê; o main interpreta e libera) */
    size_t ini_tam;
} Musica;

/* caminho completo de um arquivo da música: pasta + nome */
void Musica_path(const Musica *m, const char *arquivo, char *saida, size_t n);

/* as pastas onde procurar: songs/ e assets/musics/ (ao lado do executável)
 * e as que estiverem no <PrefPath>/pastas.txt (criado com um exemplo). */
int  Musica_roots(char raizes[][512], int max);

typedef enum { VARRE_PARADO, VARRE_ANDANDO, VARRE_PRONTO } EstadoVarredura;

typedef struct {
    char          raizes[MUSICA_MAX_RAIZES][512];
    int           n_raizes;
    SDL_Thread   *thread;
    SDL_AtomicInt estado;        /* EstadoVarredura */
    SDL_AtomicInt achadas;       /* para mostrar o progresso */
    SDL_AtomicInt cancelar;

    /* resultado: só leia depois de VARRE_PRONTO */
    Musica *musicas;
    int     n, cap;
    int     pastas;              /* pastas visitadas */
    int     sem_audio;           /* tinham chart mas nenhum áudio suportado */
    double  segundos;
} Varredura;

bool            Varredura_start(Varredura *v);
EstadoVarredura Varredura_state(const Varredura *v);
int             Varredura_found(const Varredura *v);
/* entrega a lista (fica sendo sua: libere com Musica_free_list) */
bool            Varredura_take(Varredura *v, Musica **musicas, int *n);
void            Varredura_quit(Varredura *v);

void Musica_free_list(Musica *musicas, int n);

#endif
