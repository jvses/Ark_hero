#include "game/chartfile.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RESOLUCAO_PADRAO 192
#define BPM_PADRAO       120.0
#define PREPARACAO       2.5    /* segundos de pista antes da primeira nota */

static const char *const SECAO_DIF[DIF_N] = {
    [DIF_FACIL]   = "EasySingle",
    [DIF_MEDIO]   = "MediumSingle",
    [DIF_DIFICIL] = "HardSingle",
    [DIF_EXPERT]  = "ExpertSingle",
};

const char *Dificuldade_nome(Dificuldade d)
{
    static const char *const NOMES[DIF_N] = { "Facil", "Medio", "Dificil", "Expert" };
    return (d >= 0 && d < DIF_N) ? NOMES[d] : "?";
}

/* ---------------------------------------------------------------------------
 * Vetor que cresce: dobra a capacidade quando enche
 * ------------------------------------------------------------------------- */
static bool cresce(void **v, int *cap, int n, size_t tam)
{
    if (n < *cap) return true;
    const int nova = *cap ? *cap * 2 : 64;
    void *p = realloc(*v, (size_t)nova * tam);
    if (!p) return false;
    *v   = p;
    *cap = nova;
    return true;
}

/* ---------------------------------------------------------------------------
 * Leitura linha a linha
 * ------------------------------------------------------------------------- */
typedef enum { SEC_NENHUMA, SEC_SONG, SEC_SYNC, SEC_DIF } Secao;

static const char *pula_espacos(const char *p, const char *fim)
{
    while (p < fim && isspace((unsigned char)*p)) p++;
    return p;
}

/* "  768 = N 0 120" -> tick 768, tipo "N", a = 0, b = 120 (b opcional: -1) */
static bool evento(const char *p, const char *fim, uint32_t *tick, char *tipo, long *a, long *b)
{
    char linha[128];
    const size_t n = (size_t)(fim - p) < sizeof(linha) - 1 ? (size_t)(fim - p) : sizeof(linha) - 1;
    memcpy(linha, p, n);
    linha[n] = '\0';

    unsigned long t;
    char tp[4];
    *a = *b = -1;
    const int lidos = sscanf(linha, " %lu = %3s %ld %ld", &t, tp, a, b);
    if (lidos < 2) return false;
    *tick = (uint32_t)t;
    strcpy(tipo, tp);
    return true;
}

/* [Song]: "Resolution = 480", "Offset = 0" */
static void linha_song(ArquivoChart *a, const char *p, const char *fim)
{
    const char *igual = memchr(p, '=', (size_t)(fim - p));
    if (!igual) return;
    const char *v = pula_espacos(igual + 1, fim);
    char valor[32];
    const size_t n = (size_t)(fim - v) < sizeof(valor) - 1 ? (size_t)(fim - v) : sizeof(valor) - 1;
    memcpy(valor, v, n);
    valor[n] = '\0';

    if (strncmp(p, "Resolution", 10) == 0) a->resolucao = atoi(valor);
    else if (strncmp(p, "Offset", 6) == 0) a->offset = atof(valor);
}

static bool linha_sync(ArquivoChart *a, const char *p, const char *fim)
{
    uint32_t tick;
    char tipo[4];
    long x, y;
    if (!evento(p, fim, &tick, tipo, &x, &y)) return true;

    if (strcmp(tipo, "B") == 0 && x > 0) {
        if (!cresce((void **)&a->tempos, &a->cap_tempos, a->n_tempos, sizeof(SegmentoTempo))) return false;
        /* por enquanto guarda o BPM*1000 em seg_por_tick; monta_mapa converte */
        a->tempos[a->n_tempos++] = (SegmentoTempo){ tick, 0.0, (double)x };
    } else if (strcmp(tipo, "TS") == 0 && x > 0) {
        if (!cresce((void **)&a->compassos, &a->cap_compassos, a->n_compassos, sizeof(Compasso))) return false;
        a->compassos[a->n_compassos++] = (Compasso){ tick, (int)x, y >= 0 ? 1 << y : 4 };   /* "TS 6 3" = 6/8 */
    }
    return true;   /* "A" (âncora) e o resto: ignorados */
}

static bool linha_dif(Pista_Dif *d, const char *p, const char *fim)
{
    uint32_t tick;
    char tipo[4];
    long x, y;
    if (!evento(p, fim, &tick, tipo, &x, &y)) return true;

    if (strcmp(tipo, "N") == 0 && x >= 0 && x <= 7) {
        if (!cresce((void **)&d->notas, &d->cap_notas, d->n_notas, sizeof(NotaBruta))) return false;
        d->notas[d->n_notas++] = (NotaBruta){ tick, y > 0 ? (uint32_t)y : 0, (uint8_t)x };
    } else if (strcmp(tipo, "S") == 0 && x == 2) {   /* S 2 = star power (S 64 é da bateria) */
        if (!cresce((void **)&d->frases, &d->cap_frases, d->n_frases, sizeof(FraseBruta))) return false;
        d->frases[d->n_frases++] = (FraseBruta){ tick, y > 0 ? (uint32_t)y : 0 };
    }
    return true;   /* "E" (eventos locais, solos): ignorados por enquanto */
}

/* ---------------------------------------------------------------------------
 * Mapa de tempo
 * ------------------------------------------------------------------------- */
static int compara_tempo(const void *x, const void *y)
{
    const SegmentoTempo *a = x, *b = y;
    return (a->tick > b->tick) - (a->tick < b->tick);
}

static int compara_compasso(const void *x, const void *y)
{
    const Compasso *a = x, *b = y;
    return (a->tick > b->tick) - (a->tick < b->tick);
}

/* converte os BPMs lidos em segmentos (tick, segundos, segundos por tick) */
static bool monta_mapa(ArquivoChart *a)
{
    qsort(a->tempos, (size_t)a->n_tempos, sizeof(SegmentoTempo), compara_tempo);
    if (a->n_tempos == 0 || a->tempos[0].tick != 0) {   /* sem BPM no tick 0: 120 */
        if (!cresce((void **)&a->tempos, &a->cap_tempos, a->n_tempos, sizeof(SegmentoTempo))) return false;
        memmove(a->tempos + 1, a->tempos, sizeof(SegmentoTempo) * (size_t)a->n_tempos);
        a->tempos[0] = (SegmentoTempo){ 0, 0.0, BPM_PADRAO * 1000.0 };
        a->n_tempos++;
    }
    double t = 0.0;
    for (int i = 0; i < a->n_tempos; i++) {
        SegmentoTempo *s = &a->tempos[i];
        if (i > 0) t += (s->tick - a->tempos[i - 1].tick) * a->tempos[i - 1].seg_por_tick;
        const double bpm = s->seg_por_tick / 1000.0;
        s->t            = t;
        s->seg_por_tick = 60.0 / (bpm * a->resolucao);
    }
    qsort(a->compassos, (size_t)a->n_compassos, sizeof(Compasso), compara_compasso);
    return true;
}

double ChartFile_tempo(const ArquivoChart *a, uint32_t tick)
{
    /* busca binária: o último segmento que começa em ou antes do tick */
    int lo = 0, hi = a->n_tempos - 1;
    while (lo < hi) {
        const int meio = (lo + hi + 1) / 2;
        if (a->tempos[meio].tick <= tick) lo = meio;
        else                              hi = meio - 1;
    }
    const SegmentoTempo *s = &a->tempos[lo];
    return s->t + (double)(tick - s->tick) * s->seg_por_tick;
}

static int compara_nota(const void *x, const void *y)
{
    const NotaBruta *a = x, *b = y;
    if (a->tick != b->tick) return (a->tick > b->tick) - (a->tick < b->tick);
    return a->traste - b->traste;
}

static int compara_frase(const void *x, const void *y)
{
    const FraseBruta *a = x, *b = y;
    return (a->tick > b->tick) - (a->tick < b->tick);
}

/* notas e frases em ordem de tempo (o arquivo quase sempre já vem assim) */
static void ordena(Pista_Dif *d)
{
    qsort(d->notas, (size_t)d->n_notas, sizeof(NotaBruta), compara_nota);
    qsort(d->frases, (size_t)d->n_frases, sizeof(FraseBruta), compara_frase);
}

/* ---------------------------------------------------------------------------
 * Interface
 * ------------------------------------------------------------------------- */
bool ChartFile_parse(ArquivoChart *a, const char *texto, size_t tamanho, char *erro, size_t n_erro)
{
    memset(a, 0, sizeof(*a));
    a->resolucao = RESOLUCAO_PADRAO;

    const char *p = texto, *fim = texto + tamanho;
    if (tamanho >= 3 && (unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB && (unsigned char)p[2] == 0xBF)
        p += 3;   /* BOM do UTF-8 */

    Secao secao = SEC_NENHUMA;
    Pista_Dif *dif = NULL;
    bool ok = true;

    while (p < fim && ok) {
        const char *eol = memchr(p, '\n', (size_t)(fim - p));
        if (!eol) eol = fim;
        const char *a0 = pula_espacos(p, eol), *b0 = eol;
        p = eol + 1;
        while (b0 > a0 && isspace((unsigned char)b0[-1])) b0--;   /* tira \r e espaços */
        if (a0 == b0 || *a0 == '{' || *a0 == '}') continue;

        if (*a0 == '[') {
            const char *fecha = memchr(a0, ']', (size_t)(b0 - a0));
            const size_t n = fecha ? (size_t)(fecha - a0 - 1) : 0;
            secao = SEC_NENHUMA;
            if (n == 4 && strncmp(a0 + 1, "Song", 4) == 0)      secao = SEC_SONG;
            else if (n == 9 && strncmp(a0 + 1, "SyncTrack", 9) == 0) secao = SEC_SYNC;
            else {
                for (int d = 0; d < DIF_N; d++) {
                    if (n == strlen(SECAO_DIF[d]) && strncmp(a0 + 1, SECAO_DIF[d], n) == 0) {
                        secao = SEC_DIF;
                        dif   = &a->dif[d];
                    }
                }
            }
            continue;
        }

        switch (secao) {
        case SEC_SONG: linha_song(a, a0, b0);              break;
        case SEC_SYNC: ok = linha_sync(a, a0, b0);         break;
        case SEC_DIF:  ok = linha_dif(dif, a0, b0);        break;
        default:       break;
        }
    }

    if (!ok) {
        snprintf(erro, n_erro, "sem memoria lendo o chart");
        ChartFile_free(a);
        return false;
    }
    if (a->resolucao <= 0) a->resolucao = RESOLUCAO_PADRAO;
    if (!monta_mapa(a)) {
        snprintf(erro, n_erro, "sem memoria no mapa de tempo");
        ChartFile_free(a);
        return false;
    }

    bool alguma = false;
    for (int d = 0; d < DIF_N; d++) {
        ordena(&a->dif[d]);
        alguma |= a->dif[d].n_notas > 0;
    }
    if (!alguma) {
        snprintf(erro, n_erro, "o chart nao tem notas de guitarra");
        ChartFile_free(a);
        return false;
    }
    return true;
}

void ChartFile_free(ArquivoChart *a)
{
    free(a->tempos);
    free(a->compassos);
    for (int d = 0; d < DIF_N; d++) {
        free(a->dif[d].notas);
        free(a->dif[d].frases);
    }
    memset(a, 0, sizeof(*a));
}

int ChartFile_contar(const ArquivoChart *a, Dificuldade d)
{
    const Pista_Dif *p = &a->dif[d];
    int n = 0;
    for (int i = 0; i < p->n_notas; i++)
        if (i == 0 || p->notas[i].tick != p->notas[i - 1].tick) n++;   /* um acorde conta uma */
    return n;
}

bool Chart_from_file(Chart *c, const ArquivoChart *a, Dificuldade d, double atraso_s)
{
    *c = (Chart){0};
    const Pista_Dif *p = &a->dif[d];

    const int n = ChartFile_contar(a, d);
    if (n == 0) return false;
    c->notas = calloc((size_t)n, sizeof(Nota));
    if (!c->notas) return false;

    /* sustains mais curtos que uma semicolcheia viram nota simples */
    const uint32_t corte = (uint32_t)(a->resolucao / 4);
    const double desloca = a->offset + atraso_s;
    int k = -1, fr = 0;

    for (int i = 0; i < p->n_notas; i++) {
        const NotaBruta *b = &p->notas[i];
        if (i == 0 || b->tick != p->notas[i - 1].tick) {   /* tick novo: nota nova */
            k++;
            c->notas[k].t = ChartFile_tempo(a, b->tick) + desloca;

            /* frase de star power que contém este tick */
            while (fr < p->n_frases && p->frases[fr].tick + p->frases[fr].duracao <= b->tick) fr++;
            if (fr < p->n_frases && p->frases[fr].tick <= b->tick) {
                c->notas[k].flags |= NOTA_STAR;
                c->notas[k].frase  = (uint16_t)(fr + 1);
            }
        }
        Nota *nt = &c->notas[k];
        if (b->traste <= 4)      nt->mascara |= (uint8_t)(1u << b->traste);
        else if (b->traste == 5) nt->flags   |= NOTA_FORCADA;
        else if (b->traste == 6) nt->flags   |= NOTA_TAP;
        else if (b->traste == 7) nt->flags   |= NOTA_ABERTA;

        if ((b->traste <= 4 || b->traste == 7) && b->duracao >= corte) {
            const double dur = ChartFile_tempo(a, b->tick + b->duracao) - ChartFile_tempo(a, b->tick);
            if (dur > nt->duracao) nt->duracao = dur;   /* acorde: o sustain mais longo */
        }
    }
    c->n = n;

    /* aberta junto com trastes é um erro do chart: vale o acorde */
    for (int i = 0; i < c->n; i++)
        if (c->notas[i].mascara) c->notas[i].flags &= (uint8_t)~NOTA_ABERTA;

    c->fim = 0.0;
    for (int i = 0; i < c->n; i++)
        if (c->notas[i].t + c->notas[i].duracao > c->fim) c->fim = c->notas[i].t + c->notas[i].duracao;
    c->inicio = c->notas[0].t - PREPARACAO < 0.0 ? c->notas[0].t - PREPARACAO : 0.0;

    /* linhas de batida: uma por semínima; a primeira de cada compasso é forte */
    const int por_batida = a->resolucao;
    int nb = 0;
    while (ChartFile_tempo(a, (uint32_t)(nb * por_batida)) + desloca < c->fim + 3.0) nb++;
    c->batidas = malloc(sizeof(Batida) * (size_t)(nb > 0 ? nb : 1));
    if (!c->batidas) { Chart_free(c); return false; }
    int ts = -1;
    for (int i = 0; i < nb; i++) {
        const uint32_t tick = (uint32_t)(i * por_batida);
        while (ts + 1 < a->n_compassos && a->compassos[ts + 1].tick <= tick) ts++;
        bool forte = (i % 4) == 0;
        if (ts >= 0) {
            const Compasso *cp = &a->compassos[ts];
            const uint32_t tam = (uint32_t)(cp->num * a->resolucao * 4 / (cp->den > 0 ? cp->den : 4));
            forte = tam > 0 && (tick - cp->tick) % tam == 0;
        }
        c->batidas[i] = (Batida){ ChartFile_tempo(a, tick) + desloca, forte };
    }
    c->n_batidas = nb;

    Chart_reset(c);
    return true;
}
