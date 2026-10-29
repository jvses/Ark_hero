#ifndef ARK_SONGINI_H
#define ARK_SONGINI_H

/* O song.ini das músicas do Clone Hero: metadados (nome, artista...) e o
 * atraso do áudio. Recebe o texto já carregado (quem lê o arquivo é a
 * plataforma), então funciona e é testado sem SDL. */

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char nome[128];
    char artista[128];
    char album[128];
    char charter[128];
    int  atraso_ms;     /* "delay": quanto o áudio está adiantado em relação ao chart */
    int  duracao_ms;    /* "song_length", 0 se não tiver */
    int  nota_sp;       /* "multiplier_note": nota MIDI do star power (116; alguns charts antigos usam 103) */
} SongIni;

bool SongIni_parse(const char *texto, size_t tamanho, SongIni *s);

#endif
