/* Leitura do notes.mid (Rock Band / Guitar Hero / Clone Hero).
 *
 * Um MIDI é uma lista de trilhas; cada trilha, uma lista de eventos com o
 * tempo em ticks desde o evento anterior (número de tamanho variável). O que
 * interessa ao jogo:
 *
 *   qualquer trilha      andamento (meta 0x51) e compasso (meta 0x58)
 *   "PART GUITAR"        guitarra (ou "T1 GEMS", nos charts antigos do GH1/GH2)
 *   "PART BASS"          baixo        "PART RHYTHM"       guitarra rítmica
 *   "PART GUITAR COOP"   coop         "PART KEYS"         teclado de 5 notas
 *   (todas com o mesmo esquema de notas abaixo)
 *   "EVENTS"             as partes da música: texto "[section verse_1]" ou "[prc_verse_1]"
 *
 * Dentro da trilha da guitarra, cada dificuldade é uma faixa de notas MIDI:
 *
 *              verde..laranja   força HOPO   força palhetada   aberta*
 *   Fácil         60..64            65             66             59
 *   Médio         72..76            77             78             71
 *   Difícil       84..88            89             90             83
 *   Expert        96..100          101            102             95
 *
 *   116 (ou o "multiplier_note" do song.ini)  frase de star power
 *   104                                        tap (todas as dificuldades)
 *   * só com o evento de texto [ENHANCED_OPENS]; o Phase Shift marca abertas
 *     e taps com SysEx ("PS"), que também é lido.
 *
 * Nota MIDI é um intervalo (liga ... desliga): a duração vira o sustain, e os
 * marcadores (forçar, tap, star power) valem para as notas dentro deles. */

#include "game/chartfile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int BASE_DIF[DIF_N] = { 60, 72, 84, 96 };
#define NOTA_TAP_MIDI 104

/* um intervalo de nota MIDI já fechado */
typedef struct {
    uint32_t ini, fim;
    uint8_t  nota;
} Intervalo;

typedef struct {
    Intervalo *v;
    int        n, cap;
} Intervalos;

static bool guarda(Intervalos *l, uint32_t ini, uint32_t fim, uint8_t nota)
{
    if (l->n >= l->cap) {
        const int cap = l->cap ? l->cap * 2 : 256;
        Intervalo *p = realloc(l->v, sizeof(Intervalo) * (size_t)cap);
        if (!p) return false;
        l->v   = p;
        l->cap = cap;
    }
    l->v[l->n++] = (Intervalo){ ini, fim, nota };
    return true;
}

/* o texto s aparece nos n bytes de d? */
static bool contem(const unsigned char *d, size_t n, const char *s)
{
    const size_t k = strlen(s);
    for (size_t i = 0; i + k <= n; i++)
        if (memcmp(d + i, s, k) == 0) return true;
    return false;
}

/* ---------------------------------------------------------------------------
 * Leitura de bytes com limite (um arquivo truncado não pode ler fora)
 * ------------------------------------------------------------------------- */
typedef struct {
    const unsigned char *p, *fim;
    bool ok;
} Leitor;

static uint32_t byte(Leitor *r)
{
    if (r->p >= r->fim) { r->ok = false; return 0; }
    return *r->p++;
}

static uint32_t be(Leitor *r, int n)   /* inteiro big-endian de n bytes */
{
    uint32_t v = 0;
    while (n--) v = (v << 8) | byte(r);
    return v;
}

static uint32_t vlq(Leitor *r)   /* número de tamanho variável: 7 bits por byte */
{
    uint32_t v = 0;
    for (int i = 0; i < 4; i++) {
        const uint32_t b = byte(r);
        v = (v << 7) | (b & 0x7f);
        if (!(b & 0x80)) return v;
    }
    r->ok = false;
    return v;
}

/* ---------------------------------------------------------------------------
 * Uma trilha
 * ------------------------------------------------------------------------- */
typedef struct {
    char       nome[64];
    Intervalos notas;          /* notas MIDI fechadas */
    Intervalos sysex;          /* aberta/tap do Phase Shift: nota = tipo | (dificuldade << 4) */
    bool       aberta_extra;   /* [ENHANCED_OPENS] */
} Trilha;

static bool le_trilha(ArquivoChart *a, Leitor *r, Trilha *t)
{
    uint32_t tick = 0, liga[128], liga_sx[16 * 16];
    bool     ligada[128] = { false }, ligada_sx[16 * 16] = { false };
    uint32_t status = 0;

    while (r->ok && r->p < r->fim) {
        tick += vlq(r);
        uint32_t b = byte(r);

        if (b == 0xFF) {                                   /* meta-evento */
            const uint32_t tipo = byte(r), n = vlq(r);
            if ((size_t)(r->fim - r->p) < n) { r->ok = false; break; }
            const unsigned char *d = r->p;
            r->p += n;
            if (tipo == 0x2F) break;                        /* fim da trilha */
            if (tipo == 0x51 && n == 3) {                   /* andamento: microssegundos por semínima */
                const uint32_t us = ((uint32_t)d[0] << 16) | ((uint32_t)d[1] << 8) | d[2];
                if (us > 0 && !ChartFile_add_tempo(a, tick, 60000000.0 / us)) return false;
            } else if (tipo == 0x58 && n >= 2) {            /* compasso: numerador, expoente do denominador */
                if (!ChartFile_add_compasso(a, tick, d[0], 1 << (d[1] < 8 ? d[1] : 2))) return false;
            } else if (tipo == 0x03 && !t->nome[0]) {      /* nome da trilha */
                snprintf(t->nome, sizeof(t->nome), "%.*s", (int)n, (const char *)d);
            } else if (tipo == 0x01 || tipo == 0x06) {      /* texto ou marcador */
                if (n >= 14 && contem(d, n, "ENHANCED_OPENS")) t->aberta_extra = true;
                else if (!ChartFile_add_secao(a, tick, (const char *)d, n)) return false;   /* "[section Intro]" */
            }
            continue;
        }
        if (b == 0xF0 || b == 0xF7) {                       /* SysEx */
            const uint32_t n = vlq(r);
            if ((size_t)(r->fim - r->p) < n) { r->ok = false; break; }
            const unsigned char *d = r->p;
            r->p += n;
            /* Phase Shift: 'P' 'S' 00 00 <dificuldade> <tipo> <liga/desliga> F7 */
            if (n >= 7 && d[0] == 'P' && d[1] == 'S' && d[2] == 0 && d[3] == 0 && (d[5] == 1 || d[5] == 4)) {
                const int dif = d[4] == 0xFF ? 15 : (d[4] & 0x0F);
                const int k = dif * 16 + d[5];
                if (d[6]) { ligada_sx[k] = true; liga_sx[k] = tick; }
                else if (ligada_sx[k]) {
                    ligada_sx[k] = false;
                    if (!guarda(&t->sysex, liga_sx[k], tick, (uint8_t)(d[5] | (dif << 4)))) return false;
                }
            }
            continue;
        }

        if (b & 0x80) status = b;           /* status novo */
        else          r->p--;               /* "running status": repete o anterior */
        if (!status) { r->ok = false; break; }

        const uint32_t tipo = status & 0xF0;
        if (tipo == 0xC0 || tipo == 0xD0) { byte(r); continue; }   /* 1 byte de dado */
        const uint32_t nota = byte(r) & 0x7F, vel = byte(r);
        if (tipo != 0x80 && tipo != 0x90) continue;                  /* outros: 2 bytes, ignorados */

        const bool liga_nota = tipo == 0x90 && vel > 0;   /* "liga com velocidade 0" = desliga */
        if (ligada[nota]) {                                /* fecha a anterior (liga de novo também fecha) */
            ligada[nota] = false;
            if (!guarda(&t->notas, liga[nota], tick, (uint8_t)nota)) return false;
        }
        if (liga_nota) {
            ligada[nota] = true;
            liga[nota]   = tick;
        }
    }
    return r->ok;
}

/* ---------------------------------------------------------------------------
 * As notas da guitarra -> as dificuldades do ArquivoChart
 * ------------------------------------------------------------------------- */

/* algum intervalo da lista contém "tick"? */
static bool dentro(const Intervalos *l, uint32_t tick)
{
    for (int i = 0; i < l->n; i++)
        if (l->v[i].ini <= tick && tick < l->v[i].fim) return true;
    return false;
}

/* só os intervalos de uma nota MIDI (as listas de marcadores ficam curtas) */
static bool filtra(const Intervalos *todos, uint8_t nota, Intervalos *saida)
{
    for (int i = 0; i < todos->n; i++)
        if (todos->v[i].nota == nota && !guarda(saida, todos->v[i].ini, todos->v[i].fim, nota)) return false;
    return true;
}

static bool dentro_sysex(const Intervalos *l, int dif, int tipo, uint32_t tick)
{
    for (int i = 0; i < l->n; i++) {
        const int d = l->v[i].nota >> 4, tp = l->v[i].nota & 0x0F;
        if (tp == tipo && (d == dif || d == 15) && l->v[i].ini <= tick && tick < l->v[i].fim) return true;
    }
    return false;
}

static bool monta_instrumento(ArquivoChart *a, Pista_Dif *pistas, const Trilha *t, int nota_sp)
{
    (void)a;
    /* os marcadores separados: [d][0] força HOPO, [d][1] força palhetada; e o tap */
    Intervalos forca[DIF_N][2], tap;
    memset(forca, 0, sizeof(forca));
    memset(&tap, 0, sizeof(tap));
    bool ok = filtra(&t->notas, NOTA_TAP_MIDI, &tap);
    for (int d = 0; d < DIF_N && ok; d++)
        ok = filtra(&t->notas, (uint8_t)(BASE_DIF[d] + 5), &forca[d][0]) &&
             filtra(&t->notas, (uint8_t)(BASE_DIF[d] + 6), &forca[d][1]);

    for (int i = 0; i < t->notas.n && ok; i++) {
        const Intervalo *iv = &t->notas.v[i];
        if (iv->nota == nota_sp) {                          /* star power: vale para todas */
            for (int d = 0; d < DIF_N && ok; d++) ok = ChartFile_add_frase(&pistas[d], iv->ini, iv->fim - iv->ini);
            continue;
        }
        for (int d = 0; d < DIF_N && ok; d++) {
            const int k = (int)iv->nota - BASE_DIF[d];
            int traste = -1;
            if (k >= 0 && k <= 4)                 traste = k;
            else if (k == -1 && t->aberta_extra)  traste = BRUTA_ABERTA;
            if (traste < 0) continue;

            /* aberta do Phase Shift: o verde dentro do SysEx de aberta */
            if (traste == 0 && dentro_sysex(&t->sysex, d, 1, iv->ini)) traste = BRUTA_ABERTA;
            Pista_Dif *p = &pistas[d];
            ok = ChartFile_add_nota(p, iv->ini, iv->fim - iv->ini, traste);

            /* marcadores que cobrem esta nota */
            if (ok && dentro(&forca[d][0], iv->ini)) ok = ChartFile_add_nota(p, iv->ini, 0, BRUTA_FORCA_HOPO);
            if (ok && dentro(&forca[d][1], iv->ini)) ok = ChartFile_add_nota(p, iv->ini, 0, BRUTA_FORCA_PALHETA);
            if (ok && (dentro(&tap, iv->ini) || dentro_sysex(&t->sysex, d, 4, iv->ini)))
                ok = ChartFile_add_nota(p, iv->ini, 0, BRUTA_TAP);
        }
    }

    for (int d = 0; d < DIF_N; d++) { free(forca[d][0].v); free(forca[d][1].v); }
    free(tap.v);
    return ok;
}

/* o nome da trilha diz o instrumento; -1 = trilha que não interessa (bateria, voz, eventos) */
static int instrumento_da_trilha(const char *nome)
{
    if (strcmp(nome, "PART GUITAR") == 0 || strcmp(nome, "T1 GEMS") == 0) return INSTR_GUITARRA;
    if (strcmp(nome, "PART BASS") == 0)        return INSTR_BAIXO;
    if (strcmp(nome, "PART RHYTHM") == 0)      return INSTR_RITMICA;
    if (strcmp(nome, "PART GUITAR COOP") == 0) return INSTR_COOP;
    if (strcmp(nome, "PART KEYS") == 0)        return INSTR_TECLADO;
    return -1;
}

bool ChartFile_parse_mid(ArquivoChart *a, const unsigned char *dados, size_t tamanho, int nota_sp,
                         char *erro, size_t n_erro)
{
    memset(a, 0, sizeof(*a));
    Leitor r = { dados, dados + tamanho, true };

    if (tamanho < 14 || memcmp(dados, "MThd", 4) != 0) {
        snprintf(erro, n_erro, "nao e um arquivo MIDI");
        return false;
    }
    r.p += 4;
    const uint32_t tam_cab = be(&r, 4);
    be(&r, 2);                                   /* formato (0 ou 1): tanto faz */
    const uint32_t n_trilhas = be(&r, 2), divisao = be(&r, 2);
    r.p = dados + 8 + tam_cab;
    if (divisao & 0x8000 || divisao == 0) {
        snprintf(erro, n_erro, "MIDI com tempo SMPTE (nao suportado)");
        return false;
    }
    a->resolucao     = (int)divisao;
    a->corte_sustain = (uint32_t)(a->resolucao / 3);   /* no .mid toda nota tem duração: as curtas não são sustain */
    if (nota_sp <= 0) nota_sp = 116;

    bool achou = false, ok = true, lido[INSTR_N] = { false };
    for (uint32_t i = 0; i < n_trilhas && ok && r.p + 8 <= r.fim; i++) {
        if (memcmp(r.p, "MTrk", 4) != 0) break;
        r.p += 4;
        const uint32_t n = be(&r, 4);
        if ((size_t)(r.fim - r.p) < n) { ok = false; break; }
        Leitor t = { r.p, r.p + n, true };
        r.p += n;

        Trilha tr;
        memset(&tr, 0, sizeof(tr));
        ok = le_trilha(a, &t, &tr);
        const int ins = instrumento_da_trilha(tr.nome);
        if (ok && ins >= 0 && !lido[ins]) {
            ok        = monta_instrumento(a, a->dif[ins], &tr, nota_sp);
            lido[ins] = true;
            achou     = true;
        }
        free(tr.notas.v);
        free(tr.sysex.v);
    }

    if (!ok) {
        snprintf(erro, n_erro, "MIDI corrompido ou sem memoria");
        ChartFile_free(a);
        return false;
    }
    if (!achou) {
        snprintf(erro, n_erro, "o MIDI nao tem guitarra, baixo, ritmica nem teclado");
        ChartFile_free(a);
        return false;
    }
    return ChartFile_finish(a, erro, n_erro);
}
