#include "game/game.h"

#define AVISO_DURACAO 1.5   /* segundos que um aviso fica na tela */
#define FIM_ESPERA    1.5   /* segundos depois da última nota até a tela de fim */

void Game_init(Game *g)
{
    *g = (Game){0};
    g->tela = TELA_INICIO;
    g->opcoes.velocidade  = 5;
    g->opcoes.canhoto     = false;
    g->opcoes.mostrar_fps = true;
    g->opcoes.volume      = 80;
    Chart_demo(&g->chart);   /* se falhar, o chart fica vazio e a pista também */
}

void Game_quit(Game *g)
{
    Chart_free(&g->chart);
}

bool Game_load_chart(Game *g, const Nota *notas, int n)
{
    Chart_free(&g->chart);
    return Chart_from_notes(&g->chart, notas, n);
}

/* ---------------------------------------------------------------------------
 * Consultas
 * ------------------------------------------------------------------------- */

const Menu *Game_menu(const Game *g)
{
    if (g->tela == TELA_JOGO) {
        if (g->terminou) return Menu_fim();
        return g->pausado ? Menu_pausa() : NULL;
    }
    return Menu_da_tela(g->tela);
}

int Game_menu_sel(const Game *g)
{
    return g->tela == TELA_JOGO ? g->sel_pausa : g->sel[g->tela];
}

/* Quantos segundos de música cabem na pista. Velocidade maior = janela menor
 * = notas descendo mais rápido. */
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

/* começa (ou recomeça) a partida do zero */
static void comeca_partida(Game *g)
{
    g->partida++;            /* o main.c vê a mudança e começa o som do zero */
    Chart_reset(&g->chart);
    g->t                = 0.0;
    g->proxima          = 0;
    g->primeira_visivel = 0;
    g->pausado          = false;
    g->terminou         = false;
    g->sel_pausa        = 0;
    g->pontos           = 0.0;
    g->multiplicador    = 1;
    g->combo = g->maior_combo = g->acertos = g->erros = 0;
    g->sp_medidor       = 0.0f;
    g->sp_ativo         = false;
    g->sp_frase_falhou  = false;
    for (int f = 0; f < 5; f++) g->acerto_t[f] = -1.0e9;   /* "nunca": sem animação no início */
    g->ultimo_julgamento_t = -1.0e9;
}

static void vai_para(Game *g, Tela t)
{
    g->tela = t;
    if (t == TELA_JOGO) comeca_partida(g);
}

static void volta(Game *g)
{
    switch (g->tela) {
    case TELA_CONFIG:    vai_para(g, TELA_INICIO); break;
    case TELA_CONTROLES: vai_para(g, TELA_CONFIG); break;
    case TELA_AUDIO:     vai_para(g, TELA_CONFIG); break;
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
    case ID_MODO:       avisa(g, "Modo guitarra: em breve"); break;
    case ID_VELOCIDADE: o->velocidade = limita(o->velocidade + passo, GAME_VELOCIDADE_MIN, GAME_VELOCIDADE_MAX); break;
    case ID_CANHOTO:    o->canhoto     = !o->canhoto;     break;
    case ID_FPS:        o->mostrar_fps = !o->mostrar_fps; break;
    case ID_VOLUME:     o->volume = limita(o->volume + 10 * passo, 0, 100); break;
    case ID_ANALOGICO:  g->pedido = PEDIDO_TROCAR_ANALOGICO; break;
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
    case ID_AUDIO_TESTE:    vai_para(g, TELA_AUDIO);         break;
    case ID_AUDIO_CARREGAR: g->pedido = PEDIDO_AUDIO_CARREGAR; break;
    case ID_AUDIO_TOCAR:    g->pedido = PEDIDO_AUDIO_TOCAR;    break;
    case ID_VOLTAR:    volta(g);                    break;
    case ID_SAIR:      g->pedido = PEDIDO_SAIR;     break;
    case ID_RESTAURAR:
        g->pedido = PEDIDO_RESTAURAR_CONTROLES;
        avisa(g, "Controles restaurados");
        break;
    case ID_CONTINUAR: g->pausado = false;          break;
    case ID_REINICIAR: comeca_partida(g);           break;
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

/* ---------------------------------------------------------------------------
 * A partida
 * ------------------------------------------------------------------------- */

static void star_power(Game *g, const Botoes *b, double dt)
{
    if ((b->apertados & ACAO_BIT(ACAO_STAR_POWER)) && !g->sp_ativo && g->sp_medidor >= GAME_SP_MINIMO) {
        g->sp_ativo = true;
        if (g->n_eventos < GAME_MAX_EVENTOS)
            g->eventos[g->n_eventos++] = (EventoJogo){ EV_STAR_ATIVOU, -1, 0.0 };
    }
    if (g->sp_ativo) {
        g->sp_medidor -= (float)dt / GAME_SP_DURACAO;
        if (g->sp_medidor <= 0.0f) {
            g->sp_medidor = 0.0f;
            g->sp_ativo   = false;
        }
    }
}

/* notas que já saíram de vista de vez (o draw e o juiz começam daqui) */
static void avanca_visiveis(Game *g)
{
    const double margem = 0.2 * Game_janela_visivel(g);
    while (g->primeira_visivel < g->chart.n) {
        const Nota *n = &g->chart.notas[g->primeira_visivel];
        if (n->segurando || n->t + n->duracao >= g->t - margem) break;
        g->primeira_visivel++;
    }
}

static void atualiza_jogo(Game *g, const Botoes *b, double dt)
{
    if (g->terminou) {
        navega(g, Menu_fim(), &g->sel_pausa, b);
        return;
    }
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

    /* com áudio, o tempo é o do áudio; ele nunca volta para trás */
    double novo = g->relogio_audio ? g->t_audio : g->t + dt;
    if (novo < g->t) novo = g->t;
    const double passo = novo - g->t;
    g->t      = novo;
    g->whammy = b->whammy;

    Judge_toques(g, b->apertados, b->segurados);
    Judge_sustains(g, b->segurados, passo);
    Judge_varre_erros(g);
    star_power(g, b, dt);
    avanca_visiveis(g);

    if (g->proxima >= g->chart.n && g->t > g->chart.fim + FIM_ESPERA) {
        g->terminou  = true;
        g->sel_pausa = 0;
    }
}

void Game_update(Game *g, const Botoes *b, double dt)
{
    g->pedido    = PEDIDO_NENHUM;   /* pedidos e eventos valem só pelo frame */
    g->n_eventos = 0;
    g->t_total  += dt;
    g->segurados = b->segurados;

    if (g->aviso && g->t_total > g->aviso_ate) g->aviso = NULL;

    if (g->tela == TELA_JOGO) {
        atualiza_jogo(g, b, dt);
    } else {
        const Menu *m = Menu_da_tela(g->tela);
        if (m) navega(g, m, &g->sel[g->tela], b);
    }
}
