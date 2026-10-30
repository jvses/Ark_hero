#include "platform/musica.h"

#define PROFUNDIDADE_MAX 6   /* Artista/Álbum/Música/... */

/* faixas que o Clone Hero toca juntas; preview.* (a prévia do menu) fica de fora */
static const char *const FAIXAS[] = {
    "song", "guitar", "bass", "rhythm", "keys", "vocals", "vocals_1", "vocals_2",
    "drums", "drums_1", "drums_2", "drums_3", "drums_4", "crowd",
};

void Musica_path(const Musica *m, const char *arquivo, char *saida, size_t n)
{
    SDL_snprintf(saida, n, "%s%s", m->pasta, arquivo);
}

void Musica_free_list(Musica *musicas, int n)
{
    for (int i = 0; i < n; i++) SDL_free(musicas[i].ini_texto);
    SDL_free(musicas);
}

/* ---------------------------------------------------------------------------
 * As pastas onde procurar
 * ------------------------------------------------------------------------- */
static void com_barra(char *destino, size_t n, const char *pasta)
{
    const size_t t = SDL_strlen(pasta);
    SDL_snprintf(destino, n, "%s%s", pasta, (t && pasta[t - 1] == '/') ? "" : "/");
}

static const char EXEMPLO_PASTAS[] =
    "# Ark Hero - pastas de musicas, uma por linha.\n"
    "# Alem destas, o jogo sempre procura em songs/ (dentro da pasta do jogo)\n"
    "# e em assets/musics/. Linhas com # sao ignoradas. Exemplo:\n"
    "# /roms/ports/clonehero/songs\n";

int Musica_roots(char raizes[][512], int max)
{
    int n = 0;
    const char *base = SDL_GetBasePath();
    if (n < max) SDL_snprintf(raizes[n++], 512, "%ssongs/", base ? base : "");
    if (n < max) SDL_snprintf(raizes[n++], 512, "%sassets/musics/", base ? base : "");

    char *pref = SDL_GetPrefPath("", "arkhero");
    if (!pref) return n;
    char arquivo[512];
    SDL_snprintf(arquivo, sizeof(arquivo), "%spastas.txt", pref);
    SDL_free(pref);

    size_t tam = 0;
    char *texto = SDL_LoadFile(arquivo, &tam);
    if (!texto) {   /* primeira vez: cria o arquivo com o exemplo, para o jogador achar */
        SDL_SaveFile(arquivo, EXEMPLO_PASTAS, sizeof(EXEMPLO_PASTAS) - 1);
        SDL_Log("[musica] criei %s (para acrescentar pastas de musica)", arquivo);
        return n;
    }
    char *fim = NULL;
    for (char *linha = SDL_strtok_r(texto, "\r\n", &fim); linha && n < max;
         linha = SDL_strtok_r(NULL, "\r\n", &fim)) {
        while (*linha == ' ' || *linha == '\t') linha++;
        size_t t = SDL_strlen(linha);
        while (t && (linha[t - 1] == ' ' || linha[t - 1] == '\t')) linha[--t] = '\0';
        if (t == 0 || linha[0] == '#') continue;
        com_barra(raizes[n++], 512, linha);
    }
    SDL_free(texto);
    return n;
}

/* ---------------------------------------------------------------------------
 * Uma pasta: é uma música, ou tem subpastas para olhar
 * ------------------------------------------------------------------------- */
typedef struct {
    Musica atual;          /* o que esta pasta tem */
    bool   tem_mid, tem_ini;
    char   mid[32];
    char   ini[32];
    char (*subpastas)[96]; /* nomes das subpastas */
    int    n_sub, cap_sub;
} Pasta;

static bool e_faixa(const char *base)
{
    for (size_t i = 0; i < SDL_arraysize(FAIXAS); i++)
        if (SDL_strcasecmp(base, FAIXAS[i]) == 0) return true;
    return false;
}

static SDL_EnumerationResult SDLCALL item_da_pasta(void *dado, const char *dir, const char *nome)
{
    Pasta *p = dado;
    if (nome[0] == '.') return SDL_ENUM_CONTINUE;   /* ocultos, "." e ".." */

    char caminho[512];
    SDL_snprintf(caminho, sizeof(caminho), "%s%s", dir, nome);
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(caminho, &info)) return SDL_ENUM_CONTINUE;

    if (info.type == SDL_PATHTYPE_DIRECTORY) {
        if (p->n_sub >= p->cap_sub) {
            const int cap = p->cap_sub ? p->cap_sub * 2 : 16;
            void *novo = SDL_realloc(p->subpastas, sizeof(*p->subpastas) * (size_t)cap);
            if (!novo) return SDL_ENUM_CONTINUE;
            p->subpastas = novo;
            p->cap_sub   = cap;
        }
        SDL_strlcpy(p->subpastas[p->n_sub++], nome, sizeof(p->subpastas[0]));
        return SDL_ENUM_CONTINUE;
    }
    if (info.type != SDL_PATHTYPE_FILE || SDL_strlen(nome) >= 32) return SDL_ENUM_CONTINUE;

    /* sem diferenciar maiúsculas: song.ini pode dizer Song.ogg e o arquivo ser song.ogg */
    Musica *m = &p->atual;
    const char *ponto = SDL_strrchr(nome, '.');
    if (SDL_strcasecmp(nome, "notes.chart") == 0) {
        SDL_strlcpy(m->chart, nome, sizeof(m->chart));
    } else if (SDL_strcasecmp(nome, "notes.mid") == 0) {
        p->tem_mid = true;
        SDL_strlcpy(p->mid, nome, sizeof(p->mid));
    } else if (SDL_strcasecmp(nome, "song.ini") == 0) {
        p->tem_ini = true;
        SDL_strlcpy(p->ini, nome, sizeof(p->ini));
    } else if (ponto) {
        char base[32];
        SDL_strlcpy(base, nome, (size_t)(ponto - nome) + 1 < sizeof(base) ? (size_t)(ponto - nome) + 1 : sizeof(base));
        const char *ext = ponto + 1;
        if (e_faixa(base)) {
            if (SDL_strcasecmp(ext, "ogg") == 0 || SDL_strcasecmp(ext, "opus") == 0 ||
                SDL_strcasecmp(ext, "mp3") == 0 || SDL_strcasecmp(ext, "wav") == 0) {
                if (m->n_faixas < CARREGADOR_MAX_FAIXAS)
                    SDL_strlcpy(m->faixas[m->n_faixas++], nome, sizeof(m->faixas[0]));
            } else {
                m->ignoradas++;   /* faixa num formato que o jogo não lê (flac, m4a...) */
            }
        }
    }
    return SDL_ENUM_CONTINUE;
}

static void acrescenta(Varredura *v, const Musica *m)
{
    if (v->n >= v->cap) {
        const int cap = v->cap ? v->cap * 2 : 64;
        Musica *novo = SDL_realloc(v->musicas, sizeof(Musica) * (size_t)cap);
        if (!novo) return;
        v->musicas = novo;
        v->cap     = cap;
    }
    v->musicas[v->n++] = *m;
    SDL_SetAtomicInt(&v->achadas, v->n);
}

/* rel = o caminho desta pasta a partir da raiz ("" na raiz) */
static void varre(Varredura *v, const char *pasta, const char *rel, int nivel)
{
    if (SDL_GetAtomicInt(&v->cancelar)) return;

    Pasta p;
    SDL_zero(p);
    com_barra(p.atual.pasta, sizeof(p.atual.pasta), pasta);
    if (!SDL_EnumerateDirectory(p.atual.pasta, item_da_pasta, &p)) return;
    v->pastas++;

    /* sem notes.chart, vale o notes.mid (quando os dois existem, o .chart ganha) */
    if (!p.atual.chart[0] && p.tem_mid) SDL_strlcpy(p.atual.chart, p.mid, sizeof(p.atual.chart));

    if (p.atual.chart[0] && p.atual.n_faixas > 0) {   /* é uma música */
        char tmp[512];
        SDL_strlcpy(tmp, p.atual.pasta, sizeof(tmp));
        tmp[SDL_strlen(tmp) - 1] = '\0';
        const char *barra = SDL_strrchr(tmp, '/');
        SDL_strlcpy(p.atual.nome, barra ? barra + 1 : tmp, sizeof(p.atual.nome));
        /* o grupo é o caminho relativo sem o último trecho (a pasta da música) */
        SDL_strlcpy(p.atual.grupo, rel, sizeof(p.atual.grupo));
        char *ultima = SDL_strrchr(p.atual.grupo, '/');
        if (ultima) *ultima = '\0';
        else        p.atual.grupo[0] = '\0';
        if (p.tem_ini) {
            char caminho[600];
            Musica_path(&p.atual, p.ini, caminho, sizeof(caminho));
            p.atual.ini_texto = SDL_LoadFile(caminho, &p.atual.ini_tam);
        }
        acrescenta(v, &p.atual);
    } else {
        if (p.atual.chart[0]) v->sem_audio++;
        if (nivel < PROFUNDIDADE_MAX) {
            for (int i = 0; i < p.n_sub; i++) {
                char sub[600], sub_rel[400];
                SDL_snprintf(sub, sizeof(sub), "%s%s", p.atual.pasta, p.subpastas[i]);
                SDL_snprintf(sub_rel, sizeof(sub_rel), "%s%s%s", rel, rel[0] ? "/" : "", p.subpastas[i]);
                varre(v, sub, sub_rel, nivel + 1);
            }
        }
    }
    SDL_free(p.subpastas);
}

static int SDLCALL trabalha(void *dado)
{
    Varredura *v = dado;
    const Uint64 inicio = SDL_GetTicksNS();
    SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_LOW);   /* procurar pastas não pode travar a tela */
    for (int i = 0; i < v->n_raizes; i++) varre(v, v->raizes[i], "", 0);
    v->segundos = (double)(SDL_GetTicksNS() - inicio) / 1e9;
    SDL_SetAtomicInt(&v->estado, VARRE_PRONTO);   /* por último: daqui em diante o main lê */
    return 0;
}

/* ---------------------------------------------------------------------------
 * Interface
 * ------------------------------------------------------------------------- */
bool Varredura_start(Varredura *v)
{
    if (Varredura_state(v) == VARRE_ANDANDO) return false;
    Varredura_quit(v);

    v->n_raizes = Musica_roots(v->raizes, MUSICA_MAX_RAIZES);
    v->musicas  = NULL;
    v->n = v->cap = v->pastas = v->sem_audio = 0;
    v->segundos = 0.0;
    SDL_SetAtomicInt(&v->achadas, 0);
    SDL_SetAtomicInt(&v->cancelar, 0);
    SDL_SetAtomicInt(&v->estado, VARRE_ANDANDO);
    for (int i = 0; i < v->n_raizes; i++) SDL_Log("[musica] procurando em %s", v->raizes[i]);

    v->thread = SDL_CreateThread(trabalha, "arkhero-varredura", v);
    if (!v->thread) {
        SDL_Log("[musica] nao criei a thread de varredura: %s", SDL_GetError());
        SDL_SetAtomicInt(&v->estado, VARRE_PARADO);
        return false;
    }
    return true;
}

EstadoVarredura Varredura_state(const Varredura *v)
{
    return (EstadoVarredura)SDL_GetAtomicInt((SDL_AtomicInt *)&v->estado);
}

int Varredura_found(const Varredura *v)
{
    return SDL_GetAtomicInt((SDL_AtomicInt *)&v->achadas);
}

bool Varredura_take(Varredura *v, Musica **musicas, int *n)
{
    if (Varredura_state(v) != VARRE_PRONTO) return false;
    if (v->thread) SDL_WaitThread(v->thread, NULL);
    v->thread = NULL;
    *musicas  = v->musicas;
    *n        = v->n;
    v->musicas = NULL;
    v->n = v->cap = 0;
    SDL_SetAtomicInt(&v->estado, VARRE_PARADO);
    return true;
}

void Varredura_quit(Varredura *v)
{
    SDL_SetAtomicInt(&v->cancelar, 1);
    if (v->thread) SDL_WaitThread(v->thread, NULL);
    v->thread = NULL;
    Musica_free_list(v->musicas, v->n);
    v->musicas = NULL;
    v->n = v->cap = 0;
    SDL_SetAtomicInt(&v->estado, VARRE_PARADO);
}
