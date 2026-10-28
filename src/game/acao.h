#ifndef ARK_ACAO_H
#define ARK_ACAO_H

/* O contrato entre o input e o jogo.
 *
 * Uma Acao é o que o jogador QUER fazer ("traste verde", "confirmar"), sem
 * saber qual botão ou tecla foi usado. O input traduz botão -> ação; o game
 * só conhece ações. Por isso este header não inclui nada da SDL. */

#include <stdint.h>

typedef enum {
    /* gameplay: aparecem na tela de Controles e podem ser remapeadas */
    ACAO_VERDE,
    ACAO_VERMELHO,
    ACAO_AMARELO,
    ACAO_AZUL,
    ACAO_LARANJA,
    ACAO_PALHETA_CIMA,
    ACAO_PALHETA_BAIXO,
    ACAO_STAR_POWER,
    ACAO_WHAMMY,          /* no controle vem do analógico; esta ação é a tecla */
    ACAO_PAUSA,

    /* navegação de menu: fixas, para ninguém se trancar fora do jogo */
    ACAO_CIMA,
    ACAO_BAIXO,
    ACAO_ESQUERDA,
    ACAO_DIREITA,
    ACAO_CONFIRMA,
    ACAO_VOLTA,

    ACAO_N
} Acao;

#define ACAO_N_TRASTES     5                 /* VERDE..LARANJA, nessa ordem */
#define ACAO_N_REMAPEAVEIS (ACAO_PAUSA + 1)  /* VERDE..PAUSA */

/* cada ação ocupa um bit: dá para guardar todas num inteiro só */
#define ACAO_BIT(a)     (1u << (a))
#define ACAO_BITS_TRASTES (ACAO_BIT(ACAO_N_TRASTES) - 1u)   /* os 5 primeiros bits */

/* O que o input entrega ao jogo uma vez por frame. As duas leituras existem
 * sempre, para todos os botões; quem decide qual usar é a tela do jogo:
 *   segurados -> movimento contínuo, trastes segurados
 *   apertados -> menus, confirmar, palhetada, toggles */
typedef struct {
    uint32_t segurados;   /* bit ligado = está apertado agora */
    uint32_t apertados;   /* bit ligado = foi apertado desde o frame anterior (false -> true) */
    float    whammy;      /* 0..1: quanto a alavanca está puxada (analógico ou tecla) */
} Botoes;

const char *Acao_id(Acao a);       /* nome no arquivo de controles: "verde" */
const char *Acao_rotulo(Acao a);   /* nome na tela: "Verde" */

#endif
