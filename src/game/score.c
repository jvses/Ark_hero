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

int Score_multiplicador(const Game *g)
{
    int m = 1 + g->combo / COMBO_POR_MULT;
    if (m > MULT_MAX) m = MULT_MAX;
    return g->sp_ativo ? 2 * m : m;
}

void Score_apply_events(Game *g)
{
    for (int i = 0; i < g->n_eventos; i++) {
        const EventoJogo *e = &g->eventos[i];
        const int mult = Score_multiplicador(g);
        switch (e->tipo) {
        case EV_ACERTO:
            g->pontos += PONTOS_POR_NOTA * botoes_na_nota(g->chart.notas[e->nota].mascara) * mult;
            break;
        case EV_SUSTAIN:
            g->pontos += PONTOS_SUSTAIN_POR_SEG * e->valor * mult;
            break;
        default:
            break;
        }
    }
    g->multiplicador = Score_multiplicador(g);
}
