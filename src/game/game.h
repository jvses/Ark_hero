#ifndef ARK_GAME_H
#define ARK_GAME_H

/* O estado do jogo e a lógica que o altera. Sem SDL: recebe ações e tempo,
 * produz estado. O draw lê este estado (const) e desenha. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game/acao.h"

typedef enum {
    TELA_INICIO,      /* menu principal */
    TELA_CONFIG,      /* configurações */
    TELA_CONTROLES,   /* remapear botões */
    TELA_JOGO,        /* a pista (por enquanto, de teste) */
    TELA_N
} Tela;

/* ---------------------------------------------------------------------------
 * Menus: listas de itens, definidas como dados em menu.c.
 * Todas as telas de menu navegam do mesmo jeito (game.c).
 * ------------------------------------------------------------------------- */
typedef enum {
    ITEM_BOTAO,   /* confirmar executa alguma coisa */
    ITEM_OPCAO    /* esquerda/direita muda um valor */
} TipoItem;

typedef enum {
    ID_JOGAR, ID_MUSICAS, ID_CONFIG, ID_SAIR,
    ID_CONTROLES, ID_VELOCIDADE, ID_CANHOTO, ID_FPS, ID_VOLUME, ID_CALIBRAR,
    ID_VOLTAR, ID_RESTAURAR,
    ID_CONTINUAR, ID_REINICIAR, ID_SAIR_MENU,
    ID_MAPEAR      /* ID_MAPEAR + Acao: remapear aquela ação */
} IdItem;

typedef struct {
    const char *rotulo;    /* só ASCII (fonte de debug) */
    int         id;        /* IdItem, ou ID_MAPEAR + Acao */
    TipoItem    tipo;
    bool        em_breve;  /* aparece, mas ainda não faz nada */
} ItemMenu;

typedef struct {
    const char     *titulo;
    const ItemMenu *itens;
    int             n;
} Menu;

/* ---------------------------------------------------------------------------
 * Opções do jogador (ainda não são salvas em arquivo; isso vem com a Lua)
 * ------------------------------------------------------------------------- */
#define GAME_VELOCIDADE_MIN 1
#define GAME_VELOCIDADE_MAX 10

typedef struct {
    int  velocidade;    /* 1..10: quão rápido as notas descem */
    bool canhoto;       /* inverte a ordem das faixas */
    bool mostrar_fps;
    int  volume;        /* 0..100, sem efeito até o áudio existir */
} Opcoes;

/* Pedidos: coisas que o game decide mas não pode fazer sozinho, porque são
 * da plataforma (remapear um botão, fechar o programa). O main.c lê e atende
 * no mesmo frame. Valem só por um frame, como uma caixa de saída. */
typedef enum {
    PEDIDO_NENHUM,
    PEDIDO_MAPEAR,               /* pedido_acao = qual ação remapear */
    PEDIDO_RESTAURAR_CONTROLES,
    PEDIDO_SAIR
} Pedido;

typedef struct {
    Tela    tela;
    int     sel[TELA_N];     /* item selecionado em cada tela (volta onde estava) */
    Opcoes  opcoes;

    /* tempo */
    double  t_total;         /* segundos desde o início: animações de menu */
    double  t;               /* posição na pista (depois: na música) */

    /* gameplay */
    bool     pausado;
    int      sel_pausa;
    uint32_t segurados;      /* ações seguradas neste frame (o draw acende as faixas) */
    double   t_palheta;      /* t da última palhetada (o draw faz o "flash") */

    /* aviso curto na tela ("Em breve") */
    const char *aviso;
    double      aviso_ate;   /* em t_total */

    /* saída para o main.c */
    Pedido  pedido;
    Acao    pedido_acao;
} Game;

void Game_init(Game *g);
void Game_update(Game *g, const Botoes *b, double dt);

/* Para o draw (e para os testes) */
const Menu *Game_menu(const Game *g);            /* menu visível agora, ou NULL */
int         Game_menu_sel(const Game *g);        /* item selecionado nele */
double      Game_janela_visivel(const Game *g);  /* segundos de pista visíveis */

/* menu.c */
const Menu *Menu_da_tela(Tela t);   /* NULL para TELA_JOGO */
const Menu *Menu_pausa(void);
void        Menu_valor(const Game *g, int id, char *buf, size_t n);   /* texto de um ITEM_OPCAO */

#endif
