#include "game/game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AVISO_DURACAO 1.5   /* segundos que um aviso fica na tela */
#define FIM_ESPERA    1.5   /* segundos depois da última nota até a tela de fim */
#define REPETE_ESPERA 0.40  /* segurar Cima/Baixo: começa a repetir depois disso... */
#define REPETE_PASSO  0.07  /* ...e anda um item a cada tanto */

void Game_init(Game *g)
{
    *g = (Game){0};
    g->musica = -1;
    g->tela = TELA_INICIO;
    g->opcoes.velocidade  = 5;
    g->opcoes.canhoto     = false;
    g->opcoes.mostrar_fps = true;
    g->opcoes.volume      = 80;
    g->opcoes.organizar   = ORGANIZAR_PASTAS;
    g->opcoes.penalidade  = true;
    g->opcoes.mostrar_julgamento = false;
    g->opcoes.ajuste_ms   = 0;
    Chart_demo(&g->chart);   /* se falhar, o chart fica vazio e a pista também */
    Biblioteca_monta_menu(g);
}

void Game_quit(Game *g)
{
    Chart_free(&g->chart);
    ChartFile_free(&g->arquivo);
    Biblioteca_free(g);
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
    switch (g->tela) {
    case TELA_JOGO:
        if (g->terminou) return Menu_fim();
        return g->pausado ? Menu_pausa() : NULL;
    case TELA_MUSICAS:    return &g->menu_musicas;
    case TELA_DIFICULDADE: return &g->menu_dif;
    case TELA_CARREGANDO: return NULL;
    default:              return Menu_da_tela(g->tela);
    }
}

int Game_item_acao(int id)
{
    return (id >= ID_MAPEAR && id < ID_MAPEAR + ACAO_N) ? id - ID_MAPEAR : -1;
}

int Game_item_musica(int id)
{
    return id >= ID_MUSICA ? id - ID_MUSICA : -1;
}

int Game_item_pasta(int id)
{
    return (id >= ID_PASTA && id < ID_MUSICA) ? id - ID_PASTA : -1;
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

double Game_janela_acerto(const Game *g)
{
    return g->opcoes.precisao ? GAME_JANELA_PRECISAO : GAME_JANELA_NORMAL;
}

double Game_janela_perfeito(const Game *g)
{
    return Game_janela_acerto(g) * GAME_PERFEITO;
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
    g->novo_recorde = false;
    Chart_reset(&g->chart);
    g->t                = g->chart.inicio;   /* negativo: tempo de preparação antes da música */
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
    g->soma_erro        = 0.0;
    g->soma_erro2       = 0.0;
    g->pausas           = 0;
    g->toques_errados   = 0;
    for (int f = 0; f < 5; f++) g->acerto_t[f] = -1.0e9;   /* "nunca": sem animação no início */
    g->ultimo_julgamento_t = -1.0e9;
}

static void vai_para(Game *g, Tela t)
{
    g->tela = t;
    if (t == TELA_JOGO) comeca_partida(g);
}

void Game_play(Game *g)
{
    vai_para(g, TELA_JOGO);
}

static void volta(Game *g)
{
    switch (g->tela) {
    case TELA_MUSICAS:     if (!Biblioteca_sobe(g)) vai_para(g, TELA_INICIO); break;
    case TELA_DIFICULDADE: vai_para(g, TELA_MUSICAS);     break;
    case TELA_CARREGANDO:
        g->pedido = PEDIDO_CANCELAR_CARGA;
        vai_para(g, TELA_DIFICULDADE);
        break;
    case TELA_CONFIG:
        g->pedido = PEDIDO_SALVAR_OPCOES;   /* saindo das configurações: grava */
        vai_para(g, TELA_INICIO);
        break;
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
    case ID_PRECISAO:   o->precisao    = !o->precisao;    break;
    case ID_PENALIDADE: o->penalidade  = !o->penalidade;  break;
    case ID_JULGAMENTO: o->mostrar_julgamento = !o->mostrar_julgamento; break;
    case ID_AJUSTE:     o->ajuste_ms = limita(o->ajuste_ms + GAME_AJUSTE_PASSO * passo, -GAME_AJUSTE_MAX, GAME_AJUSTE_MAX); break;
    case ID_ORGANIZAR:
        o->organizar = (o->organizar + passo + ORGANIZAR_N) % ORGANIZAR_N;
        g->pasta_atual[0] = '\0';
        Biblioteca_monta_menu(g);
        g->pedido = PEDIDO_SALVAR_OPCOES;
        break;
    case ID_FPS:        o->mostrar_fps = !o->mostrar_fps; break;
    case ID_VOLUME:     o->volume = limita(o->volume + 10 * passo, 0, 100); break;
    case ID_ANALOGICO:  g->pedido = PEDIDO_TROCAR_ANALOGICO; break;
    case ID_FRONTAIS:   g->pedido = PEDIDO_TROCAR_FRONTAIS;  break;
    default: break;
    }
}

/* ---------------------------------------------------------------------------
 * Músicas e dificuldades
 * ------------------------------------------------------------------------- */
bool Game_open_chart(Game *g, const char *texto, size_t tamanho)
{
    ChartFile_free(&g->arquivo);
    char erro[96];
    /* .mid começa com "MThd"; o resto é .chart (texto) */
    const int nota_sp = (g->musica >= 0 && g->musica < g->n_musicas) ? g->musicas[g->musica].nota_sp : 116;
    if (tamanho >= 4 && memcmp(texto, "MThd", 4) == 0)
        g->arquivo_ok = ChartFile_parse_mid(&g->arquivo, (const unsigned char *)texto, tamanho, nota_sp, erro, sizeof(erro));
    else
        g->arquivo_ok = ChartFile_parse(&g->arquivo, texto, tamanho, erro, sizeof(erro));
    if (!g->arquivo_ok) {
        avisa(g, "Nao consegui ler o chart");
        return false;
    }

    /* da mais difícil para a mais fácil, só as que existem */
    int n = 0;
    for (int d = DIF_EXPERT; d >= DIF_FACIL; d--) {
        const int notas = ChartFile_contar(&g->arquivo, (Dificuldade)d);
        if (notas == 0) continue;
        snprintf(g->rotulos_dif[n], sizeof(g->rotulos_dif[n]), "%-8s %5d notas", Dificuldade_nome((Dificuldade)d), notas);
        g->itens_dif[n] = (ItemMenu){ g->rotulos_dif[n], ID_DIFICULDADE + d, ITEM_BOTAO, false };
        n++;
    }
    g->itens_dif[n++] = (ItemMenu){ "Voltar", ID_VOLTAR, ITEM_BOTAO, false };
    g->menu_dif = (Menu){ "Dificuldade", g->itens_dif, n };
    g->sel[TELA_DIFICULDADE] = 0;
    vai_para(g, TELA_DIFICULDADE);
    return true;
}

static void escolhe_dificuldade(Game *g, Dificuldade d)
{
    const InfoMusica *m = &g->musicas[g->musica];
    Chart_free(&g->chart);
    if (!Chart_from_file(&g->chart, &g->arquivo, d, m->atraso_ms / 1000.0)) {
        avisa(g, "Dificuldade vazia");
        Chart_demo(&g->chart);
        return;
    }
    snprintf(g->chart.titulo, sizeof(g->chart.titulo), "%.63s (%.8s)", m->titulo, Dificuldade_nome(d));
    g->dificuldade   = d;
    g->pedido        = PEDIDO_CARREGAR_MUSICA;   /* o main.c decodifica o áudio; depois Game_audio_ready */
    g->pedido_indice = g->musica;
    vai_para(g, TELA_CARREGANDO);
}

void Game_audio_ready(Game *g, bool ok, double duracao)
{
    if (g->tela != TELA_CARREGANDO) return;
    g->chart.duracao_audio = ok ? duracao : 0.0;
    if (!ok) avisa(g, "Sem audio: tocando com o metronomo");
    vai_para(g, TELA_JOGO);
}

static void executa(Game *g, const ItemMenu *item)
{
    /* as faixas de ids, da maior para a menor (game.h) */
    if (Game_item_musica(item->id) >= 0) {
        g->musica        = Game_item_musica(item->id);
        g->pedido        = PEDIDO_ABRIR_MUSICA;   /* o main.c lê o notes.chart e chama Game_open_chart */
        g->pedido_indice = g->musica;
        return;
    }
    if (Game_item_pasta(item->id) >= 0) {
        Biblioteca_entra(g, Game_item_pasta(item->id));
        return;
    }
    if (item->id >= ID_DIFICULDADE) {
        escolhe_dificuldade(g, (Dificuldade)(item->id - ID_DIFICULDADE));
        return;
    }
    if (Game_item_acao(item->id) >= 0) {
        g->pedido      = PEDIDO_MAPEAR;
        g->pedido_acao = (Acao)Game_item_acao(item->id);
        return;
    }

    switch (item->id) {
    case ID_JOGAR:     vai_para(g, TELA_MUSICAS);   break;
    case ID_DEMO:
        g->musica = -1;
        Chart_free(&g->chart);
        Chart_demo(&g->chart);
        vai_para(g, TELA_JOGO);
        break;
    case ID_CONFIG:    vai_para(g, TELA_CONFIG);    break;
    case ID_CONTROLES: vai_para(g, TELA_CONTROLES); break;
    case ID_AUDIO_TESTE:    vai_para(g, TELA_AUDIO);         break;
    case ID_AUDIO_CARREGAR: g->pedido = PEDIDO_AUDIO_CARREGAR; break;
    case ID_AUDIO_TOCAR:    g->pedido = PEDIDO_AUDIO_TOCAR;    break;
    case ID_VOLTAR:    volta(g);                    break;
    case ID_SAIR:      g->pedido = PEDIDO_SAIR;     break;
    case ID_PROCURAR:  g->pedido = PEDIDO_PROCURAR_MUSICAS; avisa(g, "Procurando musicas..."); break;
    case ID_RESTAURAR:
        g->pedido = PEDIDO_RESTAURAR_CONTROLES;
        avisa(g, "Controles restaurados");
        break;
    case ID_CONTINUAR: g->pausado = false;          break;
    case ID_REINICIAR: comeca_partida(g);           break;
    case ID_SAIR_MENU: vai_para(g, TELA_MUSICAS);   break;
    default:
        if (item->tipo == ITEM_OPCAO) ajusta(g, item->id, +1);   /* confirmar numa opção = avançar */
        break;
    }
}

/* ---------------------------------------------------------------------------
 * Navegação: igual para todos os menus. Só usa "apertados" (a borda): segurar
 * a seta não pode fazer a seleção correr 60 itens por segundo.
 * ------------------------------------------------------------------------- */
/* Apertou agora: anda 1. Segurando: depois de REPETE_ESPERA, anda 1 a cada
 * REPETE_PASSO. Assim dá para descer uma lista comprida sem apertar 100 vezes,
 * e um toque continua andando só um. */
static bool repete(Game *g, const Botoes *b, Acao a, int i, double dt)
{
    if (b->apertados & ACAO_BIT(a)) {
        g->segura_t[i] = 0.0;
        g->repete_t[i] = 0.0;
        return true;
    }
    if (!(b->segurados & ACAO_BIT(a))) {
        g->segura_t[i] = 0.0;
        return false;
    }
    g->segura_t[i] += dt;
    if (g->segura_t[i] < REPETE_ESPERA) return false;
    g->repete_t[i] += dt;
    if (g->repete_t[i] < REPETE_PASSO) return false;
    g->repete_t[i] -= REPETE_PASSO;
    return true;
}

static void navega(Game *g, const Menu *m, int *sel, const Botoes *b, double dt)
{
    const uint32_t ap = b->apertados;

    /* (sel + n - 1) % n em vez de sel - 1: nunca fica negativo */
    if (repete(g, b, ACAO_CIMA, 0, dt))  *sel = (*sel + m->n - 1) % m->n;
    if (repete(g, b, ACAO_BAIXO, 1, dt)) *sel = (*sel + 1) % m->n;

    const ItemMenu *item = &m->itens[*sel];
    const int passo = ((ap & ACAO_BIT(ACAO_DIREITA))  ? 1 : 0)
                    - ((ap & ACAO_BIT(ACAO_ESQUERDA)) ? 1 : 0);

    if (item->tipo == ITEM_OPCAO && passo != 0) {
        if (item->em_breve) avisa(g, "Em breve");
        else                ajusta(g, item->id, passo);
    } else if (g->tela == TELA_MUSICAS && passo != 0) {
        /* na lista de músicas, Esq/Dir pulam uma página inteira (sem dar a volta) */
        *sel += passo * GAME_PAGINA;
        if (*sel < 0) *sel = 0;
        if (*sel > m->n - 1) *sel = m->n - 1;
        item = &m->itens[*sel];
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
        navega(g, Menu_fim(), &g->sel_pausa, b, dt);
        return;
    }
    if (g->pausado) {
        if (b->apertados & ACAO_BIT(ACAO_PAUSA)) { g->pausado = false; return; }
        navega(g, Menu_pausa(), &g->sel_pausa, b, dt);
        return;
    }
    if (b->apertados & ACAO_BIT(ACAO_PAUSA)) {
        g->pausado   = true;
        g->sel_pausa = 0;
        g->pausas++;
        return;
    }

    /* com áudio, o tempo é o do áudio; ele nunca volta para trás */
    /* o ajuste de tempo desloca o relógio do áudio: com +48 ms, o jogo "vê" a
       música 48 ms à frente, compensando o atraso entre o som sair e chegar */
    double novo = g->relogio_audio ? g->t_audio + g->opcoes.ajuste_ms / 1000.0 : g->t + dt;
    if (novo < g->t) novo = g->t;
    const double passo = novo - g->t;
    g->t      = novo;
    g->whammy = b->whammy;

    Judge_toques(g, b->apertados, b->segurados);
    Judge_sustains(g, b->segurados, passo);
    Judge_varre_erros(g);
    star_power(g, b, dt);
    avanca_visiveis(g);

    /* acaba quando acabam as notas e a música (o que vier depois) */
    const double fim = g->chart.fim + FIM_ESPERA > g->chart.duracao_audio ? g->chart.fim + FIM_ESPERA
                                                                          : g->chart.duracao_audio;
    if (g->proxima >= g->chart.n && g->t > fim) {
        g->terminou  = true;
        g->sel_pausa = 0;
        Biblioteca_fim_partida(g);   /* recorde? */
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
    } else if (g->tela == TELA_CARREGANDO) {
        if (b->apertados & ACAO_BIT(ACAO_VOLTA)) volta(g);
    } else {
        const Menu *m = Game_menu(g);
        if (m) navega(g, m, &g->sel[g->tela], b, dt);
    }
}
