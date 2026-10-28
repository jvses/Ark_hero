#include "game/songini.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* copia o valor tirando as tags de texto rico do Clone Hero: "<color=orange>X</color>" -> "X" */
static void copia_sem_tags(char *destino, size_t max, const char *v, size_t n)
{
    size_t k = 0;
    bool dentro = false;
    for (size_t i = 0; i < n && k + 1 < max; i++) {
        if (v[i] == '<') { dentro = true; continue; }
        if (v[i] == '>') { dentro = false; continue; }
        if (!dentro) destino[k++] = v[i];
    }
    destino[k] = '\0';
}

static bool chave_igual(const char *chave, size_t n, const char *nome)
{
    if (strlen(nome) != n) return false;
    for (size_t i = 0; i < n; i++)
        if (tolower((unsigned char)chave[i]) != nome[i]) return false;
    return true;
}

bool SongIni_parse(const char *texto, size_t tamanho, SongIni *s)
{
    memset(s, 0, sizeof(*s));
    bool na_secao = false, achou = false;

    const char *p = texto, *fim = texto + tamanho;
    if (tamanho >= 3 && (unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB && (unsigned char)p[2] == 0xBF)
        p += 3;   /* BOM do UTF-8 */

    while (p < fim) {
        const char *eol = memchr(p, '\n', (size_t)(fim - p));
        if (!eol) eol = fim;
        const char *a = p, *b = eol;
        p = eol + 1;

        while (a < b && isspace((unsigned char)*a)) a++;
        while (b > a && isspace((unsigned char)b[-1])) b--;
        if (a == b || *a == ';' || *a == '#') continue;

        if (*a == '[') {   /* [song] ou [Song] */
            na_secao = (b - a == 6) && chave_igual(a + 1, 4, "song");
            continue;
        }
        if (!na_secao) continue;

        const char *igual = memchr(a, '=', (size_t)(b - a));
        if (!igual) continue;
        const char *ck = a, *cf = igual, *va = igual + 1;
        while (cf > ck && isspace((unsigned char)cf[-1])) cf--;
        while (va < b && isspace((unsigned char)*va)) va++;
        const size_t nk = (size_t)(cf - ck), nv = (size_t)(b - va);

        char numero[24];
        const size_t nn = nv < sizeof(numero) - 1 ? nv : sizeof(numero) - 1;
        memcpy(numero, va, nn);
        numero[nn] = '\0';

        if      (chave_igual(ck, nk, "name"))        copia_sem_tags(s->nome, sizeof(s->nome), va, nv);
        else if (chave_igual(ck, nk, "artist"))      copia_sem_tags(s->artista, sizeof(s->artista), va, nv);
        else if (chave_igual(ck, nk, "album"))       copia_sem_tags(s->album, sizeof(s->album), va, nv);
        else if (chave_igual(ck, nk, "charter") ||
                 chave_igual(ck, nk, "frets"))       copia_sem_tags(s->charter, sizeof(s->charter), va, nv);
        else if (chave_igual(ck, nk, "delay"))       s->atraso_ms  = atoi(numero);
        else if (chave_igual(ck, nk, "song_length")) s->duracao_ms = atoi(numero);
        else continue;
        achou = true;
    }
    return achou;
}
