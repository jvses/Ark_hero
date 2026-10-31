#include "game/chartfile.h"
#include "game/texto.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RESOLUCAO_PADRAO 192
#define BPM_PADRAO       120.0
#define PREPARACAO       2.5    /* segundos de pista antes da primeira nota */
#define HOPO_LIMIAR      (65.0 / 192.0)   /* até essa fração de batida da anterior, vira HOPO */

/* o nome de uma seção do .chart é <dificuldade><instrumento>: "ExpertDoubleBass" */
static const char *const SECAO_DIF[DIF_N]    = { "Easy", "Medium", "Hard", "Expert" };
static const char *const SECAO_INSTR[INSTR_N] = { "Single", "DoubleBass", "DoubleRhythm", "DoubleGuitar", "Keyboard" };

const char *Dificuldade_nome(Dificuldade d)
{
    static const char *const NOMES[DIF_N] = { N_("Fácil"), N_("Médio"), N_("Difícil"), N_("Expert") };
    return (d >= 0 && d < DIF_N) ? NOMES[d] : "?";
}

const char *Instrumento_nome(Instrumento i)
{
    static const char *const NOMES[INSTR_N] = { N_("Guitarra"), N_("Baixo"), N_("Rítmica"), N_("Guitarra coop"), N_("Teclado") };
    return (i >= 0 && i < INSTR_N) ? NOMES[i] : "?";
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
typedef enum { SEC_NENHUMA, SEC_SONG, SEC_SYNC, SEC_EVENTS, SEC_DIF } Secao;

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
        return ChartFile_add_tempo(a, tick, x / 1000.0);              /* "B 120000" = 120 BPM */
    } else if (strcmp(tipo, "TS") == 0 && x > 0) {
        return ChartFile_add_compasso(a, tick, (int)x, y >= 0 ? 1 << y : 4);   /* "TS 6 3" = 6/8 */
    }
    return true;   /* "A" (âncora) e o resto: ignorados */
}

/* [Events]: "768 = E \"section Intro\"" (os outros eventos, como letras, ficam de fora) */
static bool linha_events(ArquivoChart *a, const char *p, const char *fim)
{
    char *depois;
    const unsigned long tick = strtoul(p, &depois, 10);
    if (depois == p) return true;
    const char *q = pula_espacos(depois, fim);
    if (q >= fim || *q != '=') return true;
    q = pula_espacos(q + 1, fim);
    if (q >= fim || *q != 'E') return true;
    q = pula_espacos(q + 1, fim);
    return ChartFile_add_secao(a, (uint32_t)tick, q, (size_t)(fim - q));
}

static bool linha_dif(Pista_Dif *d, const char *p, const char *fim)
{
    uint32_t tick;
    char tipo[4];
    long x, y;
    if (!evento(p, fim, &tick, tipo, &x, &y)) return true;

    if (strcmp(tipo, "N") == 0 && x >= 0 && x <= 7) {
        return ChartFile_add_nota(d, tick, y > 0 ? (uint32_t)y : 0, (int)x);
    } else if (strcmp(tipo, "S") == 0 && x == 2) {   /* S 2 = star power (S 64 é da bateria) */
        return ChartFile_add_frase(d, tick, y > 0 ? (uint32_t)y : 0);
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
    /* qsort/memmove com ponteiro nulo é comportamento indefinido, mesmo com 0 itens */
    if (a->n_tempos > 0) qsort(a->tempos, (size_t)a->n_tempos, sizeof(SegmentoTempo), compara_tempo);
    if (a->n_tempos == 0 || a->tempos[0].tick != 0) {   /* sem BPM no tick 0: 120 */
        if (!cresce((void **)&a->tempos, &a->cap_tempos, a->n_tempos, sizeof(SegmentoTempo))) return false;
        if (a->n_tempos > 0) memmove(a->tempos + 1, a->tempos, sizeof(SegmentoTempo) * (size_t)a->n_tempos);
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
    if (a->n_compassos > 0) qsort(a->compassos, (size_t)a->n_compassos, sizeof(Compasso), compara_compasso);
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
    if (d->n_notas > 0)  qsort(d->notas, (size_t)d->n_notas, sizeof(NotaBruta), compara_nota);
    if (d->n_frases > 0) qsort(d->frases, (size_t)d->n_frases, sizeof(FraseBruta), compara_frase);
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
            else if (n == 6 && strncmp(a0 + 1, "Events", 6) == 0)    secao = SEC_EVENTS;
            else {
                for (int d = 0; d < DIF_N; d++) {
                    const size_t nd = strlen(SECAO_DIF[d]);
                    if (n <= nd || strncmp(a0 + 1, SECAO_DIF[d], nd) != 0) continue;
                    for (int i = 0; i < INSTR_N; i++) {
                        const char *resto = SECAO_INSTR[i];
                        if (n - nd == strlen(resto) && strncmp(a0 + 1 + nd, resto, n - nd) == 0) {
                            secao = SEC_DIF;
                            dif   = &a->dif[i][d];
                        }
                    }
                }
            }
            continue;
        }

        switch (secao) {
        case SEC_SONG: linha_song(a, a0, b0);              break;
        case SEC_SYNC: ok = linha_sync(a, a0, b0);         break;
        case SEC_EVENTS: ok = linha_events(a, a0, b0);     break;
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
    a->corte_sustain = (uint32_t)(a->resolucao / 4);   /* .chart: sustains explícitos; corta só os mínimos */
    return ChartFile_finish(a, erro, n_erro);
}

/* ---------------------------------------------------------------------------
 * Para os leitores de formato
 * ------------------------------------------------------------------------- */
bool ChartFile_add_tempo(ArquivoChart *a, uint32_t tick, double bpm)
{
    if (bpm <= 0.0 || !cresce((void **)&a->tempos, &a->cap_tempos, a->n_tempos, sizeof(SegmentoTempo))) return bpm <= 0.0;
    /* por enquanto guarda o BPM*1000 em seg_por_tick; monta_mapa converte */
    a->tempos[a->n_tempos++] = (SegmentoTempo){ tick, 0.0, bpm * 1000.0 };
    return true;
}

bool ChartFile_add_compasso(ArquivoChart *a, uint32_t tick, int num, int den)
{
    if (!cresce((void **)&a->compassos, &a->cap_compassos, a->n_compassos, sizeof(Compasso))) return false;
    a->compassos[a->n_compassos++] = (Compasso){ tick, num, den };
    return true;
}

bool ChartFile_add_nota(Pista_Dif *d, uint32_t tick, uint32_t duracao, int traste)
{
    if (!cresce((void **)&d->notas, &d->cap_notas, d->n_notas, sizeof(NotaBruta))) return false;
    d->notas[d->n_notas++] = (NotaBruta){ tick, duracao, (uint8_t)traste };
    return true;
}

bool ChartFile_add_frase(Pista_Dif *d, uint32_t tick, uint32_t duracao)
{
    if (!cresce((void **)&d->frases, &d->cap_frases, d->n_frases, sizeof(FraseBruta))) return false;
    d->frases[d->n_frases++] = (FraseBruta){ tick, duracao };
    return true;
}

static bool guarda_secao(ArquivoChart *a, uint32_t tick, int compasso, const char *nome)
{
    if (!cresce((void **)&a->secoes, &a->cap_secoes, a->n_secoes, sizeof(SecaoBruta))) return false;
    SecaoBruta *s = &a->secoes[a->n_secoes++];
    *s = (SecaoBruta){ .tick = tick, .compasso = compasso };
    Texto_copia(s->nome, sizeof(s->nome), nome);
    return true;
}

/* "section Verse 1" (.chart, e .mid antigo entre colchetes) ou "prc_verse_1"
   (.mid do Rock Band 3) viram "Verse 1"; o resto do texto é ignorado */
bool ChartFile_add_secao(ArquivoChart *a, uint32_t tick, const char *texto, size_t n)
{
    char s[128];
    if (n >= sizeof(s)) n = sizeof(s) - 1;
    memcpy(s, texto, n);
    s[n] = '\0';
    char *p = s, *fim = s + strlen(s);
    while (*p == '[' || *p == '"' || isspace((unsigned char)*p)) p++;
    while (fim > p && (fim[-1] == ']' || fim[-1] == '"' || isspace((unsigned char)fim[-1]))) *--fim = '\0';
    if (strncmp(p, "section", 7) == 0 && (p[7] == ' ' || p[7] == '_')) p += 8;
    else if (strncmp(p, "prc_", 4) == 0)                               p += 4;
    else                                                                 return true;
    for (char *c = p; *c; c++)
        if (*c == '_') *c = ' ';
    while (*p == ' ') p++;
    if (!*p) return true;
    if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 'a' + 'A');
    return guarda_secao(a, tick, 0, p);
}

static int compara_secao(const void *x, const void *y)
{
    const SecaoBruta *a = x, *b = y;
    return (a->tick > b->tick) - (a->tick < b->tick);
}

/* Sem seções no arquivo, o modo prática ainda precisa de onde escolher: uma
   parte a cada 8 compassos, até a última nota (o nome fica vazio; quem
   mostra escreve "Compasso 9"). */
static bool secoes_por_compasso(ArquivoChart *a)
{
    uint32_t ultima = 0;
    for (int i = 0; i < INSTR_N; i++)
        for (int d = 0; d < DIF_N; d++)
            for (int k = 0; k < a->dif[i][d].n_notas; k++)
                if (a->dif[i][d].notas[k].tick > ultima) ultima = a->dif[i][d].notas[k].tick;

    int ts = -1;
    for (uint32_t tick = 0, compasso = 1; tick <= ultima; compasso++) {
        while (ts + 1 < a->n_compassos && a->compassos[ts + 1].tick <= tick) ts++;
        uint32_t tam = (uint32_t)(4 * a->resolucao);
        if (ts >= 0 && a->compassos[ts].den > 0)
            tam = (uint32_t)(a->compassos[ts].num * a->resolucao * 4 / a->compassos[ts].den);
        if (tam == 0) tam = (uint32_t)(4 * a->resolucao);
        if (compasso % 8 == 1 && !guarda_secao(a, tick, (int)compasso, "")) return false;
        tick += tam;
    }
    return true;
}

/* em ordem, uma por tick, com o tempo de cada uma */
static bool fecha_secoes(ArquivoChart *a)
{
    if (a->n_secoes > 0) qsort(a->secoes, (size_t)a->n_secoes, sizeof(SecaoBruta), compara_secao);
    int n = 0;
    for (int i = 0; i < a->n_secoes; i++)
        if (n == 0 || a->secoes[i].tick != a->secoes[n - 1].tick) a->secoes[n++] = a->secoes[i];
    a->n_secoes = n;
    if (n == 0 && !secoes_por_compasso(a)) return false;
    for (int i = 0; i < a->n_secoes; i++) a->secoes[i].t = ChartFile_tempo(a, a->secoes[i].tick) + a->offset;
    return true;
}

bool ChartFile_finish(ArquivoChart *a, char *erro, size_t n_erro)
{
    if (a->resolucao <= 0) a->resolucao = RESOLUCAO_PADRAO;
    if (a->corte_sustain == 0) a->corte_sustain = (uint32_t)(a->resolucao / 4);
    if (!monta_mapa(a)) {
        snprintf(erro, n_erro, "sem memoria no mapa de tempo");
        ChartFile_free(a);
        return false;
    }

    bool alguma = false;
    for (int i = 0; i < INSTR_N; i++) {
        for (int d = 0; d < DIF_N; d++) ordena(&a->dif[i][d]);
        alguma |= ChartFile_tem(a, (Instrumento)i);
    }
    if (!alguma) {
        snprintf(erro, n_erro, "o chart nao tem notas de guitarra, baixo, ritmica ou teclado");
        ChartFile_free(a);
        return false;
    }
    if (!fecha_secoes(a)) {
        snprintf(erro, n_erro, "sem memoria nas secoes");
        ChartFile_free(a);
        return false;
    }
    return true;
}

void ChartFile_free(ArquivoChart *a)
{
    free(a->tempos);
    free(a->compassos);
    free(a->secoes);
    for (int i = 0; i < INSTR_N; i++) {
        for (int d = 0; d < DIF_N; d++) {
            free(a->dif[i][d].notas);
            free(a->dif[i][d].frases);
        }
    }
    memset(a, 0, sizeof(*a));
}

static bool e_nota(const NotaBruta *b)
{
    return b->traste <= 4 || b->traste == BRUTA_ABERTA;
}

/* quantos ticks com nota de verdade (um acorde conta uma; marcadores sozinhos não) */
int ChartFile_contar(const ArquivoChart *a, Instrumento ins, Dificuldade d)
{
    const Pista_Dif *p = &a->dif[ins][d];
    int n = 0;
    uint32_t ultimo = 0;
    bool algum = false;
    for (int i = 0; i < p->n_notas; i++) {
        if (!e_nota(&p->notas[i])) continue;
        if (!algum || p->notas[i].tick != ultimo) n++;
        ultimo = p->notas[i].tick;
        algum  = true;
    }
    return n;
}

bool ChartFile_tem(const ArquivoChart *a, Instrumento ins)
{
    for (int d = 0; d < DIF_N; d++)
        if (ChartFile_contar(a, ins, (Dificuldade)d) > 0) return true;
    return false;
}

bool Chart_from_file(Chart *c, const ArquivoChart *a, Instrumento ins, Dificuldade d, double atraso_s)
{
    *c = (Chart){0};
    const Pista_Dif *p = &a->dif[ins][d];

    const int n = ChartFile_contar(a, ins, d);
    if (n == 0) return false;
    c->notas = calloc((size_t)n, sizeof(Nota));
    if (!c->notas) return false;

    const uint32_t corte = a->corte_sustain;
    const uint32_t limiar_hopo = (uint32_t)(a->resolucao * HOPO_LIMIAR + 0.5);
    const double desloca = a->offset + atraso_s;
    int k = -1, fr = 0;

    /* marcadores do tick atual (valem para a nota daquele tick) */
    bool inverte = false, forca_hopo = false, forca_palheta = false;
    uint32_t tick_nota[2] = { 0, 0 };   /* [0] = da nota k, [1] = da anterior */
    uint8_t  mascara_ant = 0;

    for (int i = 0; i <= p->n_notas; i++) {
        const bool fim_tick = i == p->n_notas || (i > 0 && p->notas[i].tick != p->notas[i - 1].tick);

        /* fechou o tick da nota k: decide se ela é HOPO */
        if (fim_tick && k >= 0 && tick_nota[0] == p->notas[i - 1].tick) {
            Nota *nt = &c->notas[k];
            if (nt->mascara) nt->flags &= (uint8_t)~NOTA_ABERTA;   /* aberta junto com traste: vale o acorde */
            const uint8_t m = (nt->flags & NOTA_ABERTA) ? 0x20 : nt->mascara;
            const bool acorde = (m & (m - 1)) != 0;
            /* HOPO natural: nota simples, perto da anterior e de cor diferente */
            bool hopo = k > 0 && !acorde && (mascara_ant & m) == 0 && tick_nota[0] - tick_nota[1] <= limiar_hopo;
            if (inverte)       hopo = !hopo;
            if (forca_hopo)    hopo = true;
            if (forca_palheta) hopo = false;
            if (hopo) nt->flags |= NOTA_HOPO;
            mascara_ant = m;
        }
        if (fim_tick) inverte = forca_hopo = forca_palheta = false;   /* marcador sem nota não vaza */
        if (i == p->n_notas) break;

        const NotaBruta *b = &p->notas[i];
        if (!e_nota(b)) {   /* marcador: vale para a nota do mesmo tick */
            if (b->traste == BRUTA_INVERTE_HOPO)  inverte = true;
            if (b->traste == BRUTA_FORCA_HOPO)    forca_hopo = true;
            if (b->traste == BRUTA_FORCA_PALHETA) forca_palheta = true;
            if (b->traste == BRUTA_TAP && k >= 0 && tick_nota[0] == b->tick) c->notas[k].flags |= NOTA_TAP;
            continue;
        }
        if (k < 0 || b->tick != tick_nota[0]) {   /* tick novo: nota nova */
            k++;
            tick_nota[1] = tick_nota[0];
            tick_nota[0] = b->tick;
            c->notas[k].t = ChartFile_tempo(a, b->tick) + desloca;

            /* frase de star power que contém este tick */
            while (fr < p->n_frases && p->frases[fr].tick + p->frases[fr].duracao <= b->tick) fr++;
            if (fr < p->n_frases && p->frases[fr].tick <= b->tick) {
                c->notas[k].flags |= NOTA_STAR;
                c->notas[k].frase  = (uint16_t)(fr + 1);
            }
        }
        Nota *nt = &c->notas[k];
        if (b->traste <= 4) nt->mascara |= (uint8_t)(1u << b->traste);
        else                nt->flags   |= NOTA_ABERTA;

        if (b->duracao >= corte) {
            const double dur = ChartFile_tempo(a, b->tick + b->duracao) - ChartFile_tempo(a, b->tick);
            if (dur > nt->duracao) nt->duracao = dur;   /* acorde: o sustain mais longo */
        }
    }
    c->n = n;

    /* marcadores que vêm ANTES da nota no mesmo tick (a ordenação põe 0..4 antes
       de 5..9, mas um tap no tick de uma aberta chega depois): uma segunda
       passada aplica os taps que ficaram para trás */
    for (int i = 0, j = 0; i < p->n_notas && j < c->n; i++) {
        const NotaBruta *b = &p->notas[i];
        if (b->traste != BRUTA_TAP) continue;
        while (j < c->n && ChartFile_tempo(a, b->tick) + desloca > c->notas[j].t + 1e-9) j++;
        if (j < c->n && fabs(ChartFile_tempo(a, b->tick) + desloca - c->notas[j].t) < 1e-9) c->notas[j].flags |= NOTA_TAP;
    }

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
