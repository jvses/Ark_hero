/* Os menus fixos do jogo, como dados (o das músicas é montado em
 * biblioteca.c, a partir do que foi encontrado). Para criar um item novo basta acrescentar uma
 * linha aqui e tratar o id em executa() / ajusta() no game.c.
 * ITEM_OPCAO: Confirmar troca (Sim/Não, ou poucos valores em roda).
 * ITEM_VALOR: Confirmar entra no modo de escolha (muitos valores).
 * Os rótulos são as chaves das traduções (game/texto.h): N_() marca, o draw
 * traduz. */

#include "game/game.h"

#include <stdio.h>

#define N_ITENS(v) ((int)(sizeof(v) / sizeof((v)[0])))

static const ItemMenu ITENS_INICIO[] = {
    { N_("Jogar"),         ID_JOGAR,  ITEM_BOTAO, false },
    { N_("Prática"),       ID_PRATICA, ITEM_BOTAO, false },
    { N_("Configurações"), ID_CONFIG, ITEM_BOTAO, false },
    { N_("Sair"),          ID_SAIR,   ITEM_BOTAO, false },
};

static const ItemMenu ITENS_CONFIG[] = {
    { N_("Controles"),           ID_CONTROLES,  ITEM_BOTAO, false },
    { N_("Velocidade da pista"), ID_VELOCIDADE, ITEM_VALOR, false },
    { N_("Modo precisão"),       ID_PRECISAO,   ITEM_OPCAO, false },
    { N_("Penalidade (erro)"),   ID_PENALIDADE, ITEM_OPCAO, false },
    { N_("Mostrar acertos"),     ID_JULGAMENTO, ITEM_OPCAO, false },
    { N_("Ajuste de tempo"),     ID_AJUSTE,     ITEM_VALOR, false },
    { N_("Modo canhoto"),        ID_CANHOTO,    ITEM_OPCAO, false },
    { N_("Mostrar FPS"),         ID_FPS,        ITEM_OPCAO, false },
    { N_("Volume"),              ID_VOLUME,     ITEM_VALOR, false },
    { N_("Som de erro"),         ID_SOM_ERRO,   ITEM_OPCAO, false },
    { N_("Idioma"),              ID_IDIOMA,     ITEM_VALOR, false },
    { N_("Teste de áudio"),      ID_AUDIO_TESTE, ITEM_BOTAO, false },
    { N_("Voltar"),              ID_VOLTAR,     ITEM_BOTAO, false },
};

static const ItemMenu ITENS_CONTROLES[] = {
    { N_("Jogador"),         ID_JOGADOR,                     ITEM_VALOR, false },
    { N_("Verde"),           ID_MAPEAR + ACAO_VERDE,         ITEM_BOTAO, false },
    { N_("Vermelho"),        ID_MAPEAR + ACAO_VERMELHO,      ITEM_BOTAO, false },
    { N_("Amarelo"),         ID_MAPEAR + ACAO_AMARELO,       ITEM_BOTAO, false },
    { N_("Azul"),            ID_MAPEAR + ACAO_AZUL,          ITEM_BOTAO, false },
    { N_("Laranja"),         ID_MAPEAR + ACAO_LARANJA,       ITEM_BOTAO, false },
    { N_("Palheta cima"),    ID_MAPEAR + ACAO_PALHETA_CIMA,  ITEM_BOTAO, false },
    { N_("Palheta baixo"),   ID_MAPEAR + ACAO_PALHETA_BAIXO, ITEM_BOTAO, false },
    { N_("Star power"),      ID_MAPEAR + ACAO_STAR_POWER,    ITEM_BOTAO, false },
    { N_("Whammy"),          ID_MAPEAR + ACAO_WHAMMY,        ITEM_BOTAO, false },
    { N_("Pausa"),           ID_MAPEAR + ACAO_PAUSA,         ITEM_BOTAO, false },
    { N_("Analógico"),       ID_ANALOGICO,                   ITEM_OPCAO, false },
    { N_("Botões frontais"), ID_FRONTAIS,                    ITEM_OPCAO, false },
    { N_("Restaurar padrão"), ID_RESTAURAR,                  ITEM_BOTAO, false },
    { N_("Voltar"),          ID_VOLTAR,                      ITEM_BOTAO, false },
};

static const ItemMenu ITENS_AUDIO[] = {
    { N_("Carregar a música de teste"), ID_AUDIO_CARREGAR, ITEM_BOTAO, false },
    { N_("Tocar / pausar"),             ID_AUDIO_TOCAR,    ITEM_BOTAO, false },
    { N_("Voltar"),                     ID_VOLTAR,         ITEM_BOTAO, false },
};

/* a prática: o trecho e a velocidade, antes de começar */
static const ItemMenu ITENS_PRATICA[] = {
    { N_("Começo (A)"),            ID_TRECHO_A, ITEM_VALOR, false },
    { N_("Fim (B)"),               ID_TRECHO_B, ITEM_VALOR, false },
    { N_("Velocidade da música"),  ID_RITMO,    ITEM_VALOR, false },
    { N_("Começar"),               ID_COMECAR,  ITEM_BOTAO, false },
    { N_("Voltar"),                ID_VOLTAR,   ITEM_BOTAO, false },
};

static const ItemMenu ITENS_PAUSA[] = {
    { N_("Continuar"),       ID_CONTINUAR, ITEM_BOTAO, false },
    { N_("Reiniciar"),       ID_REINICIAR, ITEM_BOTAO, false },
    { N_("Sair para o menu"), ID_SAIR_MENU, ITEM_BOTAO, false },
};

static const ItemMenu ITENS_PAUSA_PRATICA[] = {
    { N_("Continuar"),             ID_CONTINUAR,       ITEM_BOTAO, false },
    { N_("Velocidade da música"),  ID_RITMO,           ITEM_VALOR, false },
    { N_("Marcar o começo aqui"),  ID_MARCA_A,         ITEM_BOTAO, false },
    { N_("Marcar o fim aqui"),     ID_MARCA_B,         ITEM_BOTAO, false },
    { N_("Escolher o trecho"),     ID_ESCOLHER_TRECHO, ITEM_BOTAO, false },
    { N_("Recomeçar o trecho"),    ID_REINICIAR,       ITEM_BOTAO, false },
    { N_("Sair para o menu"),      ID_SAIR_MENU,       ITEM_BOTAO, false },
};

static const ItemMenu ITENS_FIM[] = {
    { N_("Jogar de novo"),    ID_REINICIAR, ITEM_BOTAO, false },
    { N_("Sair para o menu"), ID_SAIR_MENU, ITEM_BOTAO, false },
};

static const Menu MENUS[TELA_N] = {
    [TELA_INICIO]    = { "ARK HERO",          ITENS_INICIO,    N_ITENS(ITENS_INICIO) },
    [TELA_CONFIG]    = { N_("Configurações"), ITENS_CONFIG,    N_ITENS(ITENS_CONFIG) },
    [TELA_CONTROLES] = { N_("Controles"),     ITENS_CONTROLES, N_ITENS(ITENS_CONTROLES) },
    [TELA_AUDIO]     = { N_("Teste de áudio"), ITENS_AUDIO,    N_ITENS(ITENS_AUDIO) },
    [TELA_PRATICA]   = { N_("Prática"),       ITENS_PRATICA,   N_ITENS(ITENS_PRATICA) },
    [TELA_JOGO]      = { NULL, NULL, 0 },
};

static const Menu MENU_PAUSA = { N_("Pausa"), ITENS_PAUSA, N_ITENS(ITENS_PAUSA) };
static const Menu MENU_PAUSA_PRATICA = { N_("Pausa"), ITENS_PAUSA_PRATICA, N_ITENS(ITENS_PAUSA_PRATICA) };
static const Menu MENU_FIM   = { N_("Fim da música"), ITENS_FIM, N_ITENS(ITENS_FIM) };

const Menu *Menu_da_tela(Tela t)
{
    if (t < 0 || t >= TELA_N || MENUS[t].n == 0) return NULL;
    return &MENUS[t];
}

const Menu *Menu_pausa(bool pratica)
{
    return pratica ? &MENU_PAUSA_PRATICA : &MENU_PAUSA;
}

/* "1:05" */
static void minutos(char *buf, size_t n, double t)
{
    const int s = t > 0.0 ? (int)t : 0;
    snprintf(buf, n, "%d:%02d", s / 60, s % 60);
}

/* uma ponta do trecho: o nome da seção, ou o tempo (marcado na pausa) */
static void ponta(const Game *g, int secao, double t, char *buf, size_t n)
{
    if (secao < 0 || secao >= g->arquivo.n_secoes) {
        minutos(buf, n, t);
        return;
    }
    const SecaoBruta *s = &g->arquivo.secoes[secao];
    if (s->nome[0]) snprintf(buf, n, "%s", s->nome);
    else            snprintf(buf, n, T("Compasso %d"), s->compasso);
}

const Menu *Menu_fim(void)
{
    return &MENU_FIM;
}

void Menu_valor(const Game *g, int id, char *buf, size_t n)
{
    const Opcoes *o = &g->opcoes;
    switch (id) {
    case ID_VELOCIDADE: snprintf(buf, n, "%d", o->velocidade);            break;
    case ID_CANHOTO:    snprintf(buf, n, "%s", T(o->canhoto ? "Sim" : "Não")); break;
    case ID_PRECISAO:   snprintf(buf, n, "%s", T(o->precisao ? "Sim" : "Não")); break;
    case ID_PENALIDADE: snprintf(buf, n, "%s", T(o->penalidade ? "Sim" : "Não")); break;
    case ID_JULGAMENTO: snprintf(buf, n, "%s", T(o->mostrar_julgamento ? "Sim" : "Não")); break;
    case ID_AJUSTE:     snprintf(buf, n, "%+d ms", o->ajuste_ms); break;
    case ID_ORGANIZAR:  snprintf(buf, n, "%s", T(Biblioteca_nome_organizar(o->organizar))); break;
    case ID_FPS:        snprintf(buf, n, "%s", T(o->mostrar_fps ? "Sim" : "Não")); break;
    case ID_VOLUME:     snprintf(buf, n, "%d%%", o->volume);              break;
    case ID_SOM_ERRO:   snprintf(buf, n, "%s", T(o->som_erro ? "Sim" : "Não")); break;
    case ID_JOGADOR: {   /* "J2 Ana", ou "J2" para o convidado */
        const char *nome = Game_player_name(g, g->controles_jogador);
        if (nome) snprintf(buf, n, "J%d %s", g->controles_jogador + 1, nome);
        else      snprintf(buf, n, "J%d", g->controles_jogador + 1);
        break;
    }
    case ID_IDIOMA:     snprintf(buf, n, "%s", g->nomes_idioma[Game_language_index(g)]); break;
    case ID_TRECHO_A:   ponta(g, g->pratica.a, g->pratica.ta, buf, n); break;
    case ID_TRECHO_B:   ponta(g, g->pratica.b, g->pratica.tb, buf, n); break;
    case ID_RITMO:      snprintf(buf, n, "%d%%", g->pratica.ritmo); break;
    default:            if (n > 0) buf[0] = '\0';                         break;
    }
}
