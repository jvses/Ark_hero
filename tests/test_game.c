/* Testes da lógica do jogo: sem janela, sem controle, sem SDL.
 * Cada teste monta um Game, simula botões e confere o estado.
 * Rodar: ./build/local/test_game  (ou ctest --test-dir build/local) */

#include <stdio.h>

#include "game/game.h"

static int falhas = 0;

/* não usa assert(): no build Release o NDEBUG desligaria todos os testes */
#define CONFERE(cond)                                                       \
    do {                                                                    \
        if (!(cond)) {                                                      \
            printf("  FALHOU %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
            falhas++;                                                       \
        }                                                                   \
    } while (0)

/* um frame em que a ação "a" foi apertada */
static void aperta(Game *g, Acao a)
{
    const Botoes b = { ACAO_BIT(a), ACAO_BIT(a) };
    Game_update(g, &b, 1.0 / 60.0);
}

/* frames sem apertar nada */
static void espera(Game *g, double segundos)
{
    const Botoes b = { 0, 0 };
    for (double t = 0; t < segundos; t += 1.0 / 60.0) Game_update(g, &b, 1.0 / 60.0);
}

static void teste_navegacao_da_volta(void)
{
    Game g;
    Game_init(&g);
    const int n = Game_menu(&g)->n;

    aperta(&g, ACAO_CIMA);                    /* do primeiro item, para cima... */
    CONFERE(Game_menu_sel(&g) == n - 1);      /* ...vai para o último, não para 255 */
    aperta(&g, ACAO_BAIXO);
    CONFERE(Game_menu_sel(&g) == 0);
}

static void teste_segurar_nao_corre_o_menu(void)
{
    Game g;
    Game_init(&g);
    const Botoes segurando = { ACAO_BIT(ACAO_BAIXO), 0 };   /* segurado, sem borda */
    for (int i = 0; i < 30; i++) Game_update(&g, &segurando, 1.0 / 60.0);
    CONFERE(Game_menu_sel(&g) == 0);
}

static void teste_jogar_e_pausar(void)
{
    Game g;
    Game_init(&g);
    aperta(&g, ACAO_CONFIRMA);                /* item 0: Jogar */
    CONFERE(g.tela == TELA_JOGO);

    espera(&g, 1.0);
    CONFERE(g.t > 0.9 && g.t < 1.1);

    aperta(&g, ACAO_PAUSA);
    CONFERE(g.pausado);
    const double t_pausa = g.t;
    espera(&g, 1.0);
    CONFERE(g.t == t_pausa);                  /* pausado, o tempo não anda */

    aperta(&g, ACAO_BAIXO);                   /* Reiniciar */
    aperta(&g, ACAO_BAIXO);                   /* Sair para o menu */
    aperta(&g, ACAO_CONFIRMA);
    CONFERE(g.tela == TELA_INICIO);
}

static void teste_em_breve_nao_faz_nada(void)
{
    Game g;
    Game_init(&g);
    aperta(&g, ACAO_BAIXO);                   /* Biblioteca de musicas (em breve) */
    aperta(&g, ACAO_CONFIRMA);
    CONFERE(g.tela == TELA_INICIO);
    CONFERE(g.aviso != NULL);
    espera(&g, 2.0);
    CONFERE(g.aviso == NULL);
}

static void teste_opcoes_e_voltar(void)
{
    Game g;
    Game_init(&g);
    aperta(&g, ACAO_BAIXO);
    aperta(&g, ACAO_BAIXO);                   /* Configuracoes */
    aperta(&g, ACAO_CONFIRMA);
    CONFERE(g.tela == TELA_CONFIG);

    aperta(&g, ACAO_BAIXO);                   /* Velocidade da pista */
    for (int i = 0; i < 20; i++) aperta(&g, ACAO_DIREITA);
    CONFERE(g.opcoes.velocidade == GAME_VELOCIDADE_MAX);   /* para no máximo */

    aperta(&g, ACAO_BAIXO);                   /* Modo canhoto */
    aperta(&g, ACAO_CONFIRMA);
    CONFERE(g.opcoes.canhoto);

    aperta(&g, ACAO_VOLTA);
    CONFERE(g.tela == TELA_INICIO);
    CONFERE(Game_menu_sel(&g) == 2);          /* voltou para onde estava */
}

static void teste_pedido_de_remapear(void)
{
    Game g;
    Game_init(&g);
    aperta(&g, ACAO_BAIXO);
    aperta(&g, ACAO_BAIXO);
    aperta(&g, ACAO_CONFIRMA);                /* Configuracoes */
    aperta(&g, ACAO_CONFIRMA);                /* Controles */
    CONFERE(g.tela == TELA_CONTROLES);

    aperta(&g, ACAO_BAIXO);                   /* Vermelho */
    aperta(&g, ACAO_CONFIRMA);
    CONFERE(g.pedido == PEDIDO_MAPEAR);
    CONFERE(g.pedido_acao == ACAO_VERMELHO);

    espera(&g, 0.1);
    CONFERE(g.pedido == PEDIDO_NENHUM);       /* pedido vale só um frame */
}

static void teste_trastes_segurados(void)
{
    Game g;
    Game_init(&g);
    aperta(&g, ACAO_CONFIRMA);
    const Botoes acorde = { ACAO_BIT(ACAO_VERDE) | ACAO_BIT(ACAO_AMARELO), 0 };
    Game_update(&g, &acorde, 1.0 / 60.0);
    CONFERE((g.segurados & ACAO_BITS_TRASTES) == 0x5);   /* bits 0 e 2 */
}

int main(void)
{
    teste_navegacao_da_volta();
    teste_segurar_nao_corre_o_menu();
    teste_jogar_e_pausar();
    teste_em_breve_nao_faz_nada();
    teste_opcoes_e_voltar();
    teste_pedido_de_remapear();
    teste_trastes_segurados();

    if (falhas) {
        printf("%d verificacao(oes) falharam\n", falhas);
        return 1;
    }
    puts("test_game: todos os testes passaram");
    return 0;
}
