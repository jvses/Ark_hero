#ifndef ARK_CARREGADOR_H
#define ARK_CARREGADOR_H

/* Decodifica e mistura as faixas de uma música (song, guitar, bass, drums...)
 * num único Pcm, numa thread separada: a tela continua andando enquanto
 * carrega, e o progresso pode ser mostrado.
 *
 * Formatos: Opus (.opus, e .ogg que na verdade é Opus), Ogg Vorbis (.ogg)
 * e WAV. MP3 ainda não. */

#include <SDL3/SDL.h>

#include "platform/audio.h"

#define CARREGADOR_MAX_FAIXAS 12

typedef enum {
    CARGA_PARADO,
    CARGA_ANDANDO,
    CARGA_PRONTO,
    CARGA_ERRO
} EstadoCarga;

typedef struct {
    char          arquivos[CARREGADOR_MAX_FAIXAS][512];
    int           n;

    SDL_Thread   *thread;
    SDL_AtomicInt estado;      /* EstadoCarga */
    SDL_AtomicInt progresso;   /* 0..1000 */
    SDL_AtomicInt cancelar;

    /* preenchidos pela thread; só leia depois de CARGA_PRONTO / CARGA_ERRO */
    Pcm    pcm;
    double segundos;           /* quanto a carga demorou */
    double segundos_audio;     /* soma da duração das faixas (para medir a velocidade) */
    char   erro[160];
} Carregador;

bool        Carregador_start(Carregador *c, const char arquivos[][512], int n);
EstadoCarga Carregador_state(const Carregador *c);
float       Carregador_progress(const Carregador *c);   /* 0..1 */
bool        Carregador_take(Carregador *c, Pcm *saida); /* entrega o Pcm (fica sendo seu) */
void        Carregador_quit(Carregador *c);             /* cancela, espera e libera */

#endif
