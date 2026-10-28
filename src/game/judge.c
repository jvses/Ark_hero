/* O juiz, no "modo controle" (o padrão do jogo):
 *
 *  - apertar o botão de um traste no tempo da nota já é tocar a nota;
 *    não existe palhetada;
 *  - acorde: a nota é acertada quando todos os botões dela estão apertados,
 *    sendo o último apertado dentro da janela de tempo;
 *  - apertar um botão sem nota não é punido (como no modo controle do
 *    Clone Hero); o erro é deixar a nota passar;
 *  - sustain: depois de acertar a cabeça, segure os botões até o fim.
 *    Soltar antes não quebra o combo, só para de somar pontos.
 *
 * O tempo de cada aperto é o do frame (resolução de ~16 ms). Os timestamps
 * exatos dos eventos (ARQUITETURA 4.2) entram junto com o relógio do áudio. */

#include "game/game.h"

static void emite(Game *g, TipoEvento tipo, int nota, double valor)
{
    if (g->n_eventos >= GAME_MAX_EVENTOS) return;
    g->eventos[g->n_eventos++] = (EventoJogo){ tipo, nota, valor };
}

/* a nota i é a última da sua frase de star power? */
static bool fim_de_frase(const Game *g, int i)
{
    return i + 1 >= g->chart.n || !(g->chart.notas[i + 1].flags & NOTA_STAR);
}

static void avanca_proxima(Game *g)
{
    while (g->proxima < g->chart.n && g->chart.notas[g->proxima].estado != NOTA_PENDENTE)
        g->proxima++;
}

static void acerta(Game *g, int i)
{
    Nota *n = &g->chart.notas[i];
    const double erro = g->t - n->t;

    n->estado    = NOTA_ACERTADA;
    n->segurando = n->duracao > 0.0;
    g->combo++;
    if (g->combo > g->maior_combo) g->maior_combo = g->combo;
    g->acertos++;

    for (int f = 0; f < 5; f++)
        if (n->mascara & (1u << f)) g->acerto_t[f] = g->t;
    g->ultimo_julgamento_t = g->t;
    g->ultimo_erro_tempo   = erro;
    g->ultimo_foi_erro     = false;
    emite(g, EV_ACERTO, i, erro);

    if ((n->flags & NOTA_STAR) && fim_de_frase(g, i)) {
        if (!g->sp_frase_falhou) {
            g->sp_medidor += GAME_SP_FRASE;
            if (g->sp_medidor > 1.0f) g->sp_medidor = 1.0f;
            emite(g, EV_FRASE_STAR, i, 0.0);
        }
        g->sp_frase_falhou = false;
    }
    avanca_proxima(g);
}

static void erra(Game *g, int i)
{
    Nota *n = &g->chart.notas[i];
    n->estado = NOTA_PERDIDA;
    g->combo  = 0;
    g->erros++;
    g->ultimo_julgamento_t = g->t;
    g->ultimo_foi_erro     = true;
    emite(g, EV_ERRO, i, 0.0);

    if (n->flags & NOTA_STAR) {
        /* frase perdida; no fim dela, a próxima começa limpa */
        g->sp_frase_falhou = !fim_de_frase(g, i);
    }
}

/* primeira nota pendente, dentro da janela, que usa o traste f */
static int nota_do_traste(const Game *g, int f)
{
    for (int i = g->proxima; i < g->chart.n; i++) {
        const Nota *n = &g->chart.notas[i];
        if (n->t > g->t + GAME_JANELA_ACERTO) break;   /* daqui pra frente, cedo demais */
        if (n->estado != NOTA_PENDENTE) continue;
        if (n->t < g->t - GAME_JANELA_ACERTO) continue; /* tarde demais: a varredura cuida */
        if (n->mascara & (1u << f)) return i;
    }
    return -1;
}

void Judge_toques(Game *g, uint32_t apertados, uint32_t segurados)
{
    const uint32_t novos = apertados & ACAO_BITS_TRASTES;
    /* "segurados" vem do estado do frame; um toque mais curto que um frame só
       aparece em "apertados", por isso os dois juntos */
    const uint32_t firmes = (segurados | apertados) & ACAO_BITS_TRASTES;

    for (int f = 0; f < ACAO_N_TRASTES; f++) {
        if (!(novos & (1u << f))) continue;
        const int i = nota_do_traste(g, f);
        if (i < 0) continue;   /* aperto sem nota: sem punição */

        const uint8_t m = g->chart.notas[i].mascara;
        if ((firmes & m) == m) acerta(g, i);   /* acorde completo (ou nota simples) */
        /* senão: acorde pela metade; o próximo botão apertado completa */
    }
}

void Judge_sustains(Game *g, uint32_t segurados, double dt)
{
    for (int i = g->primeira_visivel; i < g->chart.n; i++) {
        Nota *n = &g->chart.notas[i];
        if (n->t > g->t + GAME_JANELA_ACERTO) break;
        if (!n->segurando) continue;

        const double fim = n->t + n->duracao;
        const bool firme = (segurados & n->mascara) == n->mascara;

        if (g->t >= fim) {                            /* chegou ao fim */
            n->segurando = false;
            n->solto     = fim;
            const double parte = dt - (g->t - fim);   /* só o pedaço antes do fim */
            if (parte > 0.0) emite(g, EV_SUSTAIN, i, parte);
        } else if (!firme) {                          /* soltou */
            n->segurando = false;
            n->solto     = (fim - g->t <= GAME_SUSTAIN_FOLGA) ? fim : g->t;
        } else {
            emite(g, EV_SUSTAIN, i, dt);
            /* whammy num sustain de star power enche o medidor */
            if ((n->flags & NOTA_STAR) && g->whammy > 0.2f) {
                g->sp_medidor += (float)dt * 0.08f * g->whammy;
                if (g->sp_medidor > 1.0f) g->sp_medidor = 1.0f;
            }
        }
    }
}

void Judge_varre_erros(Game *g)
{
    for (int i = g->proxima; i < g->chart.n; i++) {
        const Nota *n = &g->chart.notas[i];
        if (n->t >= g->t - GAME_JANELA_ACERTO) break;
        if (n->estado == NOTA_PENDENTE) erra(g, i);
    }
    avanca_proxima(g);
}
