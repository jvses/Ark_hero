/* O juiz, no "modo controle" (o padrão do jogo):
 *
 *  - apertar o botão de um traste no tempo da nota já é tocar a nota;
 *    não existe palhetada. A janela é de ±100 ms, ou ±70 ms no modo precisão;
 *  - acorde: a nota é acertada quando os botões apertados são EXATAMENTE os
 *    dela, sendo o último apertado dentro da janela. Um botão a mais (vermelho
 *    + amarelo + azul num acorde vermelho + azul) erra o acorde. O botão de um
 *    sustain ainda segurado não conta como "a mais";
 *  - apertar um botão sem nota quebra o combo (opção "Penalidade", ligada
 *    por padrão). Segurar um botão e apertar só o outro continua valendo para
 *    acordes repetidos: o que vale é o botão NOVO estar na nota e os outros
 *    estarem segurados;
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
    return i + 1 >= g->chart.n || g->chart.notas[i + 1].frase != g->chart.notas[i].frase;
}

/* a nota usa o traste f? (a aberta aceita qualquer um) */
static bool usa_traste(const Nota *n, int f)
{
    return (n->flags & NOTA_ABERTA) || (n->mascara & (1u << f));
}

/* os botões da nota estão todos apertados? (a aberta: qualquer botão) */
static bool firme(const Nota *n, uint32_t botoes)
{
    if (n->flags & NOTA_ABERTA) return (botoes & ACAO_BITS_TRASTES) != 0;
    return (botoes & n->mascara) == n->mascara;
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
        if (usa_traste(n, f)) g->acerto_t[f] = g->t;
    g->ultimo_julgamento_t = g->t;
    g->ultimo_erro_tempo   = erro;
    g->ultimo_foi_erro     = false;
    g->soma_erro  += erro;
    g->soma_erro2 += erro * erro;
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
        if (n->t > g->t + Game_janela_acerto(g)) break;   /* daqui pra frente, cedo demais */
        if (n->estado != NOTA_PENDENTE) continue;
        if (n->t < g->t - Game_janela_acerto(g)) continue; /* tarde demais: a varredura cuida */
        if (usa_traste(n, f)) return i;
    }
    return -1;
}

/* acorde de verdade (dois ou mais trastes; a aberta não é) */
static bool acorde(const Nota *n)
{
    return !(n->flags & NOTA_ABERTA) && (n->mascara & (n->mascara - 1)) != 0;
}

/* os botões que estão segurando um sustain agora */
static uint32_t sustentando(const Game *g)
{
    uint32_t m = 0;
    for (int i = g->primeira_visivel; i < g->chart.n; i++) {
        const Nota *n = &g->chart.notas[i];
        if (n->t > g->t) break;
        if (n->segurando) m |= (n->flags & NOTA_ABERTA) ? 0 : n->mascara;
    }
    return m;
}

/* apertou um botão sem nota nenhuma para ele */
static void toque_errado(Game *g)
{
    g->combo = 0;
    g->toques_errados++;
    g->ultimo_julgamento_t = g->t;
    g->ultimo_foi_erro     = true;
    emite(g, EV_TOQUE_ERRADO, -1, 0.0);
}

void Judge_toques(Game *g, uint32_t apertados, uint32_t segurados)
{
    const uint32_t novos = apertados & ACAO_BITS_TRASTES;
    /* "segurados" vem do estado do frame; um toque mais curto que um frame só
       aparece em "apertados", por isso os dois juntos */
    const uint32_t firmes = (segurados | apertados) & ACAO_BITS_TRASTES;
    /* para os acordes: o que está apertado, sem os botões de sustains em curso */
    const uint32_t livres = firmes & ~sustentando(g);
    uint32_t usados = 0;     /* botões que já serviram a um acerto neste frame */
    bool     punido = false; /* no máximo uma penalidade por frame */

    for (int f = 0; f < ACAO_N_TRASTES; f++) {
        if (!(novos & (1u << f)) || (usados & (1u << f))) continue;
        const int i = nota_do_traste(g, f);
        if (i < 0) {                                    /* nenhuma nota para este botão */
            if (g->opcoes.penalidade && !punido) { toque_errado(g); punido = true; }
            continue;
        }
        const Nota *n = &g->chart.notas[i];
        if (acorde(n) && (livres & ~n->mascara)) {      /* acorde com botão a mais: errou o acorde */
            if (g->opcoes.penalidade && !punido) { toque_errado(g); punido = true; }
            usados |= novos;                            /* este frame não acerta mais nada */
            continue;
        }
        if (firme(n, firmes)) {                         /* acorde completo (ou nota simples) */
            acerta(g, i);
            /* os outros botões do acorde, apertados junto, já foram usados aqui:
               não procuram outra nota nem contam como erro */
            usados |= (n->flags & NOTA_ABERTA) ? ACAO_BITS_TRASTES : n->mascara;
        }
        /* senão: acorde pela metade; o próximo botão apertado completa */
    }
}

void Judge_sustains(Game *g, uint32_t segurados, double dt)
{
    for (int i = g->primeira_visivel; i < g->chart.n; i++) {
        Nota *n = &g->chart.notas[i];
        if (n->t > g->t + Game_janela_acerto(g)) break;
        if (!n->segurando) continue;

        const double fim = n->t + n->duracao;
        const bool segura = firme(n, segurados);

        if (g->t >= fim) {                            /* chegou ao fim */
            n->segurando = false;
            n->solto     = fim;
            const double parte = dt - (g->t - fim);   /* só o pedaço antes do fim */
            if (parte > 0.0) emite(g, EV_SUSTAIN, i, parte);
        } else if (!segura) {                         /* soltou */
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
        if (n->t >= g->t - Game_janela_acerto(g)) break;
        if (n->estado == NOTA_PENDENTE) erra(g, i);
    }
    avanca_proxima(g);
}
