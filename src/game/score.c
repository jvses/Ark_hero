/* Pontuação a partir dos eventos do frame. Regras padrão, em C; quando a
 * Lua entrar (regras.lua), ela pode substituir este arquivo, e ele continua
 * como reserva se o script falhar (ARQUITETURA 6.8). */

#include "game/game.h"

#define PONTOS_POR_NOTA        50.0   /* por botão: um acorde de 3 vale 150 */
#define PONTOS_SUSTAIN_POR_SEG 50.0
#define COMBO_POR_MULT         10     /* a cada 10 de combo, +1 no multiplicador */
#define MULT_MAX               4

static int botoes_na_nota(uint8_t mascara)
{
    int n = 0;
    for (; mascara; mascara >>= 1) n += mascara & 1u;
    return n;
}

int Score_multiplicador(const Jogador *j)
{
    int m = 1 + j->combo / COMBO_POR_MULT;
    if (m > MULT_MAX) m = MULT_MAX;
    return j->sp_ativo ? 2 * m : m;
}

/* cada evento conta para o jogador que o produziu */
void Score_apply_events(Game *g)
{
    for (int i = 0; i < g->n_eventos; i++) {
        const EventoJogo *e = &g->eventos[i];
        if (e->jogador < 0 || e->jogador >= GAME_MAX_JOGADORES) continue;
        Jogador *j = &g->jog[e->jogador];
        const int mult = Score_multiplicador(j);
        switch (e->tipo) {
        case EV_ACERTO:
        {
            const int b = botoes_na_nota(j->chart.notas[e->nota].mascara);
            j->pontos += PONTOS_POR_NOTA * (b > 0 ? b : 1) * mult;   /* a aberta vale como uma */
        }
            break;
        case EV_SUSTAIN:
            j->pontos += PONTOS_SUSTAIN_POR_SEG * e->valor * mult;
            break;
        default:
            break;
        }
    }
    for (int k = 0; k < GAME_MAX_JOGADORES; k++) {
        Jogador *j = &g->jog[k];
        if (!j->ativo) continue;
        const int novo = Score_multiplicador(j);
        if (novo != j->multiplicador) j->mult_mudou_t = g->t_total;   /* o draw anima a troca */
        j->multiplicador = novo;
    }
}
