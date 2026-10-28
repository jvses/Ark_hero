/* Os menus do jogo, como dados. Para criar um item novo basta acrescentar uma
 * linha aqui e tratar o id em executa() / ajusta() no game.c.
 * Rótulos só em ASCII: a fonte de debug da SDL não tem acentos. */

#include "game/game.h"

#include <stdio.h>

#define N_ITENS(v) ((int)(sizeof(v) / sizeof((v)[0])))

static const ItemMenu ITENS_INICIO[] = {
    { "Jogar (pista de teste)", ID_JOGAR,   ITEM_BOTAO, false },
    { "Biblioteca de musicas",  ID_MUSICAS, ITEM_BOTAO, true  },
    { "Configuracoes",          ID_CONFIG,  ITEM_BOTAO, false },
    { "Sair",                   ID_SAIR,    ITEM_BOTAO, false },
};

static const ItemMenu ITENS_CONFIG[] = {
    { "Controles",           ID_CONTROLES,  ITEM_BOTAO, false },
    { "Modo de jogo",        ID_MODO,       ITEM_OPCAO, false },
    { "Velocidade da pista", ID_VELOCIDADE, ITEM_OPCAO, false },
    { "Modo canhoto",        ID_CANHOTO,    ITEM_OPCAO, false },
    { "Mostrar FPS",         ID_FPS,        ITEM_OPCAO, false },
    { "Volume",              ID_VOLUME,     ITEM_OPCAO, false },
    { "Teste de audio",      ID_AUDIO_TESTE, ITEM_BOTAO, false },
    { "Calibrar atraso",     ID_CALIBRAR,   ITEM_BOTAO, true  },
    { "Voltar",              ID_VOLTAR,     ITEM_BOTAO, false },
};

static const ItemMenu ITENS_CONTROLES[] = {
    { "Verde",           ID_MAPEAR + ACAO_VERDE,         ITEM_BOTAO, false },
    { "Vermelho",        ID_MAPEAR + ACAO_VERMELHO,      ITEM_BOTAO, false },
    { "Amarelo",         ID_MAPEAR + ACAO_AMARELO,       ITEM_BOTAO, false },
    { "Azul",            ID_MAPEAR + ACAO_AZUL,          ITEM_BOTAO, false },
    { "Laranja",         ID_MAPEAR + ACAO_LARANJA,       ITEM_BOTAO, false },
    /* Palheta cima/baixo voltam aqui junto com o modo guitarra */
    { "Star power",      ID_MAPEAR + ACAO_STAR_POWER,    ITEM_BOTAO, false },
    { "Whammy",          ID_MAPEAR + ACAO_WHAMMY,        ITEM_BOTAO, false },
    { "Pausa",           ID_MAPEAR + ACAO_PAUSA,         ITEM_BOTAO, false },
    { "Analogico",       ID_ANALOGICO,                   ITEM_OPCAO, false },
    { "Restaurar padrao", ID_RESTAURAR,                  ITEM_BOTAO, false },
    { "Voltar",          ID_VOLTAR,                      ITEM_BOTAO, false },
};

static const ItemMenu ITENS_AUDIO[] = {
    { "Carregar a musica de teste", ID_AUDIO_CARREGAR, ITEM_BOTAO, false },
    { "Tocar / pausar",             ID_AUDIO_TOCAR,    ITEM_BOTAO, false },
    { "Voltar",                     ID_VOLTAR,         ITEM_BOTAO, false },
};

static const ItemMenu ITENS_PAUSA[] = {
    { "Continuar",       ID_CONTINUAR, ITEM_BOTAO, false },
    { "Reiniciar",       ID_REINICIAR, ITEM_BOTAO, false },
    { "Sair para o menu", ID_SAIR_MENU, ITEM_BOTAO, false },
};

static const ItemMenu ITENS_FIM[] = {
    { "Jogar de novo",    ID_REINICIAR, ITEM_BOTAO, false },
    { "Sair para o menu", ID_SAIR_MENU, ITEM_BOTAO, false },
};

static const Menu MENUS[TELA_N] = {
    [TELA_INICIO]    = { "ARK HERO",      ITENS_INICIO,    N_ITENS(ITENS_INICIO) },
    [TELA_CONFIG]    = { "Configuracoes", ITENS_CONFIG,    N_ITENS(ITENS_CONFIG) },
    [TELA_CONTROLES] = { "Controles",     ITENS_CONTROLES, N_ITENS(ITENS_CONTROLES) },
    [TELA_AUDIO]     = { "Teste de audio", ITENS_AUDIO,    N_ITENS(ITENS_AUDIO) },
    [TELA_JOGO]      = { NULL, NULL, 0 },
};

static const Menu MENU_PAUSA = { "Pausa", ITENS_PAUSA, N_ITENS(ITENS_PAUSA) };
static const Menu MENU_FIM   = { "Fim da musica", ITENS_FIM, N_ITENS(ITENS_FIM) };

const Menu *Menu_da_tela(Tela t)
{
    if (t < 0 || t >= TELA_N || MENUS[t].n == 0) return NULL;
    return &MENUS[t];
}

const Menu *Menu_pausa(void)
{
    return &MENU_PAUSA;
}

const Menu *Menu_fim(void)
{
    return &MENU_FIM;
}

void Menu_valor(const Game *g, int id, char *buf, size_t n)
{
    const Opcoes *o = &g->opcoes;
    switch (id) {
    case ID_MODO:       snprintf(buf, n, "%s", "Controle");                break;
    case ID_VELOCIDADE: snprintf(buf, n, "%d", o->velocidade);            break;
    case ID_CANHOTO:    snprintf(buf, n, "%s", o->canhoto ? "Sim" : "Nao"); break;
    case ID_FPS:        snprintf(buf, n, "%s", o->mostrar_fps ? "Sim" : "Nao"); break;
    case ID_VOLUME:     snprintf(buf, n, "%d%%", o->volume);              break;
    default:            if (n > 0) buf[0] = '\0';                         break;
    }
}
