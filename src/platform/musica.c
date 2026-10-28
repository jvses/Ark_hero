#include "platform/musica.h"

#define PROFUNDIDADE_MAX 3

/* faixas que o Clone Hero toca juntas; preview.* (a prévia do menu) fica de fora */
static const char *const FAIXAS[] = {
    "song", "guitar", "bass", "rhythm", "keys", "vocals", "vocals_1", "vocals_2",
    "drums", "drums_1", "drums_2", "drums_3", "drums_4", "crowd",
};

static bool e_faixa(const char *base)
{
    for (size_t i = 0; i < SDL_arraysize(FAIXAS); i++)
        if (SDL_strcasecmp(base, FAIXAS[i]) == 0) return true;
    return false;
}

/* separa "Guitar.OGG" em base "Guitar" e extensão "OGG" */
static bool separa(const char *arquivo, char *base, size_t n, const char **ext)
{
    const char *ponto = SDL_strrchr(arquivo, '.');
    if (!ponto || ponto == arquivo) return false;
    const size_t tam = (size_t)(ponto - arquivo);
    if (tam >= n) return false;
    SDL_memcpy(base, arquivo, tam);
    base[tam] = '\0';
    *ext = ponto + 1;
    return true;
}

static SDL_EnumerationResult SDLCALL arquivo_da_pasta(void *dado, const char *pasta, const char *nome)
{
    Musica *m = dado;
    char base[64];
    const char *ext;

    /* comparação sem maiúsculas: song.ini pode dizer Song.ogg e o arquivo ser song.ogg */
    if (SDL_strcasecmp(nome, "notes.chart") == 0) {
        SDL_snprintf(m->chart, sizeof(m->chart), "%s%s", pasta, nome);
    } else if (SDL_strcasecmp(nome, "song.ini") == 0) {
        SDL_snprintf(m->ini, sizeof(m->ini), "%s%s", pasta, nome);
    } else if (separa(nome, base, sizeof(base), &ext) && e_faixa(base)) {
        if (SDL_strcasecmp(ext, "ogg") == 0 || SDL_strcasecmp(ext, "opus") == 0 ||
            SDL_strcasecmp(ext, "wav") == 0) {
            if (m->n_faixas < CARREGADOR_MAX_FAIXAS)
                SDL_snprintf(m->faixas[m->n_faixas++], sizeof(m->faixas[0]), "%s%s", pasta, nome);
        } else if (SDL_strcasecmp(ext, "mp3") == 0) {
            m->ignoradas++;   /* MP3 entra depois */
        }
    }
    return SDL_ENUM_CONTINUE;
}

bool Musica_open(const char *pasta, Musica *m)
{
    *m = (Musica){0};
    const size_t n = SDL_strlen(pasta);
    SDL_snprintf(m->pasta, sizeof(m->pasta), "%s%s", pasta, (n && pasta[n - 1] == '/') ? "" : "/");

    /* nome = último trecho do caminho */
    char tmp[512];
    SDL_strlcpy(tmp, m->pasta, sizeof(tmp));
    tmp[SDL_strlen(tmp) - 1] = '\0';
    const char *barra = SDL_strrchr(tmp, '/');
    SDL_strlcpy(m->nome, barra ? barra + 1 : tmp, sizeof(m->nome));

    if (!SDL_EnumerateDirectory(m->pasta, arquivo_da_pasta, m)) return false;
    return m->chart[0] != '\0' && m->n_faixas > 0;
}

/* ---------------------------------------------------------------------------
 * Varredura (por enquanto simples; a biblioteca com cache vem depois)
 * ------------------------------------------------------------------------- */
typedef struct {
    Musica *saida;
    int     max, n, nivel;
} Busca;

static void busca_em(Busca *b, const char *pasta);

static SDL_EnumerationResult SDLCALL entrada(void *dado, const char *pasta, const char *nome)
{
    Busca *b = dado;
    if (b->n >= b->max) return SDL_ENUM_SUCCESS;

    char caminho[512];
    SDL_snprintf(caminho, sizeof(caminho), "%s%s", pasta, nome);
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(caminho, &info) || info.type != SDL_PATHTYPE_DIRECTORY) return SDL_ENUM_CONTINUE;

    if (Musica_open(caminho, &b->saida[b->n])) b->n++;
    else if (b->nivel < PROFUNDIDADE_MAX) busca_em(b, caminho);   /* pasta de grupo: desce */
    return SDL_ENUM_CONTINUE;
}

static void busca_em(Busca *b, const char *pasta)
{
    char com_barra[512];
    const size_t n = SDL_strlen(pasta);
    SDL_snprintf(com_barra, sizeof(com_barra), "%s%s", pasta, (n && pasta[n - 1] == '/') ? "" : "/");
    b->nivel++;
    SDL_EnumerateDirectory(com_barra, entrada, b);
    b->nivel--;
}

int Musica_list(const char *raiz, Musica *saida, int max)
{
    Busca b = { saida, max, 0, 0 };
    busca_em(&b, raiz);
    return b.n;
}
