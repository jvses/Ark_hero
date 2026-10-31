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
 * exatos dos eventos (ARQUITETURA 4.2) entram junto com o relógio do áudio.
 *
 * Cada função julga um jogador (j): a pista, o placar e o star power são dele;
 * o tempo (g->t) e as opções são de todos. */

#include "game/game.h"

static void emite(Game *g, const Jogador *j, TipoEvento tipo, int nota, double valor)
{
    if (g->n_eventos >= GAME_MAX_EVENTOS) return;
    g->eventos[g->n_eventos++] = (EventoJogo){ tipo, (int)(j - g->jog), nota, valor };
}

/* a nota i é a última da sua frase de star power? */
static bool fim_de_frase(const Jogador *j, int i)
{
    return i + 1 >= j->chart.n || j->chart.notas[i + 1].frase != j->chart.notas[i].frase;
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

static void avanca_proxima(Jogador *j)
{
    while (j->proxima < j->chart.n && j->chart.notas[j->proxima].estado != NOTA_PENDENTE)
        j->proxima++;
}

static void acerta(Game *g, Jogador *j, int i)
{
    Nota *n = &j->chart.notas[i];
    const double erro = g->t - n->t;

    n->estado    = NOTA_ACERTADA;
    n->segurando = n->duracao > 0.0;
    j->combo++;
    if (j->combo > j->maior_combo) j->maior_combo = j->combo;
    j->acertos++;

    for (int f = 0; f < 5; f++)
        if (usa_traste(n, f)) j->acerto_t[f] = g->t;
    j->ultimo_julgamento_t = g->t;
    j->ultimo_erro_tempo   = erro;
    j->ultimo_foi_erro     = false;
    j->soma_erro  += erro;
    j->soma_erro2 += erro * erro;
    emite(g, j, EV_ACERTO, i, erro);

    if ((n->flags & NOTA_STAR) && fim_de_frase(j, i)) {
        if (!j->sp_frase_falhou) {
            j->sp_medidor += GAME_SP_FRASE;
            if (j->sp_medidor > 1.0f) j->sp_medidor = 1.0f;
            emite(g, j, EV_FRASE_STAR, i, 0.0);
        }
        j->sp_frase_falhou = false;
    }
    avanca_proxima(j);
}

static void erra(Game *g, Jogador *j, int i)
{
    Nota *n = &j->chart.notas[i];
    n->estado = NOTA_PERDIDA;
    j->combo  = 0;
    j->erros++;
    j->ultimo_julgamento_t = g->t;
    j->ultimo_foi_erro     = true;
    emite(g, j, EV_ERRO, i, 0.0);

    if (n->flags & NOTA_STAR) {
        /* frase perdida; no fim dela, a próxima começa limpa */
        j->sp_frase_falhou = !fim_de_frase(j, i);
    }
}

/* primeira nota pendente, dentro da janela, que usa o traste f */
static int nota_do_traste(const Game *g, const Jogador *j, int f)
{
    for (int i = j->proxima; i < j->chart.n; i++) {
        const Nota *n = &j->chart.notas[i];
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
static uint32_t sustentando(const Game *g, const Jogador *j)
{
    uint32_t m = 0;
    for (int i = j->primeira_visivel; i < j->chart.n; i++) {
        const Nota *n = &j->chart.notas[i];
        if (n->t > g->t) break;
        if (n->segurando) m |= (n->flags & NOTA_ABERTA) ? 0 : n->mascara;
    }
    return m;
}

/* Apertar sem nota pune? Só com a opção ligada, e não enquanto o jogador
   ainda está voltando da pausa (ele toca junto o que já tinha tocado). */
static bool pune(const Game *g)
{
    return g->opcoes.penalidade && g->t >= g->livre_ate;
}

/* apertou um botão sem nota nenhuma para ele */
static void toque_errado(Game *g, Jogador *j)
{
    j->combo = 0;
    j->toques_errados++;
    j->ultimo_julgamento_t = g->t;
    j->ultimo_foi_erro     = true;
    emite(g, j, EV_TOQUE_ERRADO, -1, 0.0);
}

void Judge_toques(Game *g, Jogador *j, uint32_t apertados, uint32_t segurados)
{
    const uint32_t novos = apertados & ACAO_BITS_TRASTES;
    /* "segurados" vem do estado do frame; um toque mais curto que um frame só
       aparece em "apertados", por isso os dois juntos */
    const uint32_t firmes = (segurados | apertados) & ACAO_BITS_TRASTES;
    /* para os acordes: o que está apertado, sem os botões de sustains em curso */
    const uint32_t livres = firmes & ~sustentando(g, j);
    uint32_t usados = 0;     /* botões que já serviram a um acerto neste frame */
    bool     punido = false; /* no máximo uma penalidade por frame */

    for (int f = 0; f < ACAO_N_TRASTES; f++) {
        if (!(novos & (1u << f)) || (usados & (1u << f))) continue;
        const int i = nota_do_traste(g, j, f);
        if (i < 0) {                                    /* nenhuma nota para este botão */
            if (pune(g) && !punido) { toque_errado(g, j); punido = true; }
            continue;
        }
        const Nota *n = &j->chart.notas[i];
        if (acorde(n) && (livres & ~n->mascara)) {      /* acorde com botão a mais: errou o acorde */
            if (pune(g) && !punido) { toque_errado(g, j); punido = true; }
            usados |= novos;                            /* este frame não acerta mais nada */
            continue;
        }
        if (firme(n, firmes)) {                         /* acorde completo (ou nota simples) */
            acerta(g, j, i);
            /* os outros botões do acorde, apertados junto, já foram usados aqui:
               não procuram outra nota nem contam como erro */
            usados |= (n->flags & NOTA_ABERTA) ? ACAO_BITS_TRASTES : n->mascara;
        }
        /* senão: acorde pela metade; o próximo botão apertado completa */
    }
}

void Judge_sustains(Game *g, Jogador *j, uint32_t segurados, double dt)
{
    const bool guitarra = g->guitarra[j - g->jog];
    for (int i = j->primeira_visivel; i < j->chart.n; i++) {
        Nota *n = &j->chart.notas[i];
        if (n->t > g->t + Game_janela_acerto(g)) break;
        if (!n->segurando) continue;

        const double fim = n->t + n->duracao;
        /* na guitarra, o sustain de uma aberta é sem traste nenhum: não tem o que soltar */
        const bool segura = (guitarra && (n->flags & NOTA_ABERTA)) || firme(n, segurados);

        if (g->t >= fim) {                            /* chegou ao fim */
            n->segurando = false;
            n->solto     = fim;
            const double parte = dt - (g->t - fim);   /* só o pedaço antes do fim */
            if (parte > 0.0) emite(g, j, EV_SUSTAIN, i, parte);
        } else if (!segura) {                         /* soltou */
            n->segurando = false;
            n->solto     = (fim - g->t <= GAME_SUSTAIN_FOLGA) ? fim : g->t;
        } else {
            emite(g, j, EV_SUSTAIN, i, dt);
            /* whammy num sustain de star power enche o medidor */
            if ((n->flags & NOTA_STAR) && j->whammy > 0.2f) {
                j->sp_medidor += (float)dt * 0.08f * j->whammy;
                if (j->sp_medidor > 1.0f) j->sp_medidor = 1.0f;
            }
        }
    }
}

void Judge_varre_erros(Game *g, Jogador *j)
{
    for (int i = j->proxima; i < j->chart.n; i++) {
        const Nota *n = &j->chart.notas[i];
        if (n->t >= g->t - Game_janela_acerto(g)) break;
        if (n->estado == NOTA_PENDENTE) erra(g, j, i);
    }
    avanca_proxima(j);
}

/* ---------------------------------------------------------------------------
 * Modo guitarra: a palheta é que toca a nota.
 *   - palhetada com os trastes certos acerta. Nota simples: o traste dela é
 *     o mais alto segurado (os de baixo podem ficar: "ancorar"). Acorde: os
 *     trastes exatos. Aberta: nenhum traste;
 *   - a palhetada pode vir um pouco antes dos trastes: ela espera até
 *     GUITARRA_FOLGA para os trastes ficarem certos;
 *   - palhetada sem nota, ou que não acertou a tempo: erro (com a
 *     penalidade ligada, quebra o combo);
 *   - HOPO: com o combo de pé, trocar de traste já toca, sem palhetar. Tap:
 *     sempre. Uma palhetada logo depois de um HOPO tocado assim é ignorada
 *     (muita gente palheta por costume);
 *   - sustain estendido: o traste de um sustain ainda tocando não conta para
 *     a nota que vem por cima (nem como "a mais" num acorde, nem como o
 *     traste mais alto).
 * Os sustains e as notas perdidas são como no modo controle.
 * ------------------------------------------------------------------------- */
#define GUITARRA_FOLGA    0.050   /* palhetada antes dos trastes: espera isso */
#define GUITARRA_POS_HOPO 0.100   /* palhetada logo depois de um HOPO: ignorada */

static bool trastes_certos(const Nota *n, uint32_t f, uint32_t sustentados)
{
    f &= ~(sustentados & ~(uint32_t)n->mascara);   /* tira os trastes dos sustains que não são desta nota */
    if (n->flags & NOTA_ABERTA) return f == 0;
    if (acorde(n)) return f == n->mascara;
    if (!(f & n->mascara)) return false;
    return (f & ~((uint32_t)(n->mascara << 1) - 1u)) == 0;   /* nada acima do traste dela */
}

/* a primeira nota pendente dentro da janela (-1 = nenhuma) */
static int nota_na_janela(const Game *g, const Jogador *j)
{
    for (int i = j->proxima; i < j->chart.n; i++) {
        const Nota *n = &j->chart.notas[i];
        if (n->t > g->t + Game_janela_acerto(g)) return -1;
        if (n->estado == NOTA_PENDENTE && n->t >= g->t - Game_janela_acerto(g)) return i;
    }
    return -1;
}

static void erro_palhetada(Game *g, Jogador *j)
{
    if (pune(g)) toque_errado(g, j);
}

void Judge_guitarra(Game *g, Jogador *j, uint32_t apertados, uint32_t segurados)
{
    const uint32_t f = (segurados | apertados) & ACAO_BITS_TRASTES;
    const bool palhetou = apertados & (ACAO_BIT(ACAO_PALHETA_CIMA) | ACAO_BIT(ACAO_PALHETA_BAIXO));
    const bool mudou = f != j->trastes_antes;
    j->trastes_antes = f;
    const int i = nota_na_janela(g, j);
    const Nota *n = i >= 0 ? &j->chart.notas[i] : NULL;
    const uint32_t sus = sustentando(g, j);

    if (palhetou) {
        if (j->palhetada_t >= 0.0) {   /* a anterior não acertou nada e já veio outra */
            j->palhetada_t = -1.0;
            erro_palhetada(g, j);
        }
        if (n && trastes_certos(n, f, sus)) { acerta(g, j, i); return; }
        if (g->t - j->hopo_t <= GUITARRA_POS_HOPO) return;   /* sobra de um HOPO */
        if (n) j->palhetada_t = g->t;                        /* espera os trastes */
        else   erro_palhetada(g, j);
        return;
    }
    if (j->palhetada_t >= 0.0) {   /* palhetada esperando os trastes */
        if (n && trastes_certos(n, f, sus)) {
            j->palhetada_t = -1.0;
            acerta(g, j, i);
        } else if (g->t - j->palhetada_t > GUITARRA_FOLGA) {
            j->palhetada_t = -1.0;
            erro_palhetada(g, j);
        }
        return;
    }
    /* sem palhetar: HOPO (com o combo de pé) e tap, trocando de traste */
    if (mudou && n && ((n->flags & NOTA_TAP) || ((n->flags & NOTA_HOPO) && j->combo > 0)) && trastes_certos(n, f, sus)) {
        acerta(g, j, i);
        j->hopo_t = g->t;
    }
}
