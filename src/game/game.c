#include "game/game.h"

#define AVISO_DURACAO 1.5   /* segundos que um aviso fica na tela */

void Game_init(Game *g)
{
    *g = (Game){0};
    g->tela = TELA_INICIO;
    g->opcoes.velocidade  = 5;
    g->opcoes.canhoto     = false;
    g->opcoes.mostrar_fps = true;
    g->opcoes.volume      = 80;
    g->t_palheta = -1.0e9;   /* "nunca palhetou": o flash não aparece no início */
}

/* ---------------------------------------------------------------------------
 * Consultas
 * ------------------------------------------------------------------------- */

const Menu *Game_menu(const Game *g)
{
    if (g->tela == TELA_JOGO) return g->pausado ? Menu_pausa() : NULL;
    return Menu_da_tela(g->tela);
}

int Game_menu_sel(const Game *g)
{
    return g->tela == TELA_JOGO ? g->sel_pausa : g->sel[g->tela];
}

/* Quantos segundos de música cabem na pista. Velocidade maior = janela menor
 * = notas descendo mais rápido. Vai ser usado também pelo juiz. */
double Game_janela_visivel(const Game *g)
{
    return 3.0 - 0.2 * (g->opcoes.velocidade - 1);   /* 1 -> 3,0 s ... 10 -> 1,2 s */
}

/* ---------------------------------------------------------------------------
 * Ações dos itens de menu
 * ------------------------------------------------------------------------- */

static void avisa(Game *g, const char *texto)
{
    g->aviso     = texto;
    g->aviso_ate = g->t_total + AVISO_DURACAO;
}

static void vai_para(Game *g, Tela t)
{
    g->tela = t;
    if (t == TELA_JOGO) {
        g->t         = 0.0;
        g->pausado   = false;
        g->t_palheta = -1.0e9;
    }
}

static void volta(Game *g)
{
    switch (g->tela) {
    case TELA_CONFIG:    vai_para(g, TELA_INICIO); break;
    case TELA_CONTROLES: vai_para(g, TELA_CONFIG); break;
    case TELA_JOGO:      g->pausado = false;       break;   /* "voltar" na pausa = continuar */
    default:             break;
    }
}

static int limita(int v, int min, int max)
{
    return v < min ? min : (v > max ? max : v);
}

/* esquerda/direita num ITEM_OPCAO: passo = -1 ou +1 */
static void ajusta(Game *g, int id, int passo)
{
    Opcoes *o = &g->opcoes;
    switch (id) {
    case ID_VELOCIDADE: o->velocidade = limita(o->velocidade + passo, GAME_VELOCIDADE_MIN, GAME_VELOCIDADE_MAX); break;
    case ID_CANHOTO:    o->canhoto     = !o->canhoto;     break;
    case ID_FPS:        o->mostrar_fps = !o->mostrar_fps; break;
    case ID_VOLUME:     o->volume = limita(o->volume + 10 * passo, 0, 100); break;
    default: break;
    }
}

static void executa(Game *g, const ItemMenu *item)
{
    if (item->id >= ID_MAPEAR) {
        g->pedido      = PEDIDO_MAPEAR;
        g->pedido_acao = (Acao)(item->id - ID_MAPEAR);
        return;
    }

    switch (item->id) {
    case ID_JOGAR:     vai_para(g, TELA_JOGO);      break;
    case ID_CONFIG:    vai_para(g, TELA_CONFIG);    break;
    case ID_CONTROLES: vai_para(g, TELA_CONTROLES); break;
    case ID_VOLTAR:    volta(g);                    break;
    case ID_SAIR:      g->pedido = PEDIDO_SAIR;     break;
    case ID_RESTAURAR:
        g->pedido = PEDIDO_RESTAURAR_CONTROLES;
        avisa(g, "Controles restaurados");
        break;
    case ID_CONTINUAR: g->pausado = false;          break;
    case ID_REINICIAR: vai_para(g, TELA_JOGO);      break;
    case ID_SAIR_MENU: vai_para(g, TELA_INICIO);    break;
    default:
        if (item->tipo == ITEM_OPCAO) ajusta(g, item->id, +1);   /* confirmar numa opção = avançar */
        break;
    }
}

/* ---------------------------------------------------------------------------
 * Navegação: igual para todos os menus. Só usa "apertados" (a borda): segurar
 * a seta não pode fazer a seleção correr 60 itens por segundo.
 * ------------------------------------------------------------------------- */
static void navega(Game *g, const Menu *m, int *sel, const Botoes *b)
{
    const uint32_t ap = b->apertados;

    /* (sel + n - 1) % n em vez de sel - 1: nunca fica negativo */
    if (ap & ACAO_BIT(ACAO_CIMA))  *sel = (*sel + m->n - 1) % m->n;
    if (ap & ACAO_BIT(ACAO_BAIXO)) *sel = (*sel + 1) % m->n;

    const ItemMenu *item = &m->itens[*sel];
    const int passo = ((ap & ACAO_BIT(ACAO_DIREITA))  ? 1 : 0)
                    - ((ap & ACAO_BIT(ACAO_ESQUERDA)) ? 1 : 0);

    if (item->tipo == ITEM_OPCAO && passo != 0) {
        if (item->em_breve) avisa(g, "Em breve");
        else                ajusta(g, item->id, passo);
    }

    if (ap & ACAO_BIT(ACAO_CONFIRMA)) {
        if (item->em_breve) avisa(g, "Em breve");
        else                executa(g, item);
    } else if (ap & ACAO_BIT(ACAO_VOLTA)) {
        volta(g);
    }
}

static void atualiza_jogo(Game *g, const Botoes *b, double dt)
{
    if (g->pausado) {
        if (b->apertados & ACAO_BIT(ACAO_PAUSA)) { g->pausado = false; return; }
        navega(g, Menu_pausa(), &g->sel_pausa, b);
        return;
    }

    if (b->apertados & ACAO_BIT(ACAO_PAUSA)) {
        g->pausado   = true;
        g->sel_pausa = 0;
        return;
    }

    g->t += dt;   /* etapa 2: o tempo passa a vir do relógio do áudio */

    if (b->apertados & (ACAO_BIT(ACAO_PALHETA_CIMA) | ACAO_BIT(ACAO_PALHETA_BAIXO)))
        g->t_palheta = g->t;
}

void Game_update(Game *g, const Botoes *b, double dt)
{
    g->pedido  = PEDIDO_NENHUM;   /* pedidos valem só pelo frame em que foram feitos */
    g->t_total += dt;
    g->segurados = b->segurados;

    if (g->aviso && g->t_total > g->aviso_ate) g->aviso = NULL;

    if (g->tela == TELA_JOGO) {
        atualiza_jogo(g, b, dt);
    } else {
        const Menu *m = Menu_da_tela(g->tela);
        if (m) navega(g, m, &g->sel[g->tela], b);
    }
}
