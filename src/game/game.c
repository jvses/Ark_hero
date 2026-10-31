#include "game/game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AVISO_DURACAO 1.5   /* segundos que um aviso fica na tela */
#define FIM_ESPERA    1.5   /* segundos depois da última nota até a tela de fim */
#define REPETE_ESPERA 0.40  /* segurar Cima/Baixo: começa a repetir depois disso... */
#define REPETE_PASSO  0.07  /* ...e anda um item a cada tanto... */
#define REPETE_RAPIDO 2.0   /* ...e, segurando mais que isso, */
#define REPETE_CORRE  0.02  /* um a cada tanto (uma lista comprida, na guitarra, sem Esq/Dir) */

static void libera_charts(Game *g)
{
    for (int k = 0; k < GAME_MAX_JOGADORES; k++) Chart_free(&g->jog[k].chart);
}

/* o que é de todos sai dos charts dos jogadores: a partida começa no começo
   mais cedo e termina no fim mais tarde */
static void junta_charts(Game *g)
{
    bool primeiro = true;
    for (int k = 0; k < GAME_MAX_JOGADORES; k++) {
        const Jogador *j = &g->jog[k];
        if (!j->ativo) continue;
        if (primeiro || j->chart.inicio < g->inicio) g->inicio = j->chart.inicio;
        if (primeiro || j->chart.fim > g->fim)       g->fim    = j->chart.fim;
        primeiro = false;
    }
    if (primeiro) g->inicio = g->fim = 0.0;
}

/* o primeiro da sessão que tem o aparelho (ninguém: o jogador 1) */
static int primeiro_da_sessao(const Game *g)
{
    for (int k = 0; k < GAME_MAX_JOGADORES; k++)
        if (g->jog[k].na_sessao && Game_present(g, k)) return k;
    return 0;
}

/* a demonstração: um jogador só, sem áudio */
static void carrega_demo(Game *g)
{
    libera_charts(g);
    const int um = primeiro_da_sessao(g);
    for (int k = 0; k < GAME_MAX_JOGADORES; k++) g->jog[k].ativo = k == um;
    Chart_demo(&g->jog[um].chart);   /* se falhar, o chart fica vazio e a pista também */
    snprintf(g->titulo, sizeof(g->titulo), "%s", N_("Demonstração"));
    g->duracao_audio = 0.0;
    junta_charts(g);
}

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
    g->opcoes.som_erro    = false;
    snprintf(g->opcoes.idioma, sizeof(g->opcoes.idioma), "%s", "pt");
    Game_add_language(g, "pt", "Português");   /* o embutido: os textos do código */
    for (int k = 0; k < GAME_MAX_JOGADORES; k++) {
        g->jog[k].instrumento  = INSTR_GUITARRA;
        g->jog[k].dificuldade  = DIF_EXPERT;
        g->jog[k].linha_sessao = SESSAO_PRONTO;
        g->jog[k].linha_prep   = PREP_PRONTO;
    }
    g->nome_jogador    = -1;
    g->cursor_teclado  = -1;
    g->controles_volta = TELA_CONFIG;
    carrega_demo(g);
    Biblioteca_monta_menu(g);
}

void Game_quit(Game *g)
{
    libera_charts(g);
    ChartFile_free(&g->arquivo);
    Biblioteca_free(g);
}

bool Game_load_chart(Game *g, const Nota *notas, int n)
{
    libera_charts(g);
    for (int k = 0; k < GAME_MAX_JOGADORES; k++) g->jog[k].ativo = k == 0;
    const bool ok = Chart_from_notes(&g->jog[0].chart, notas, n);
    snprintf(g->titulo, sizeof(g->titulo), "%s", "Teste");
    g->duracao_audio = 0.0;
    junta_charts(g);
    return ok;
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
    case TELA_JOGADORES:  return NULL;   /* telas próprias: um cartão por jogador */
    case TELA_PREPARAR:   return NULL;
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

void Game_add_language(Game *g, const char *codigo, const char *nome)
{
    if (g->n_idiomas >= GAME_MAX_IDIOMAS) return;
    snprintf(g->idiomas[g->n_idiomas], sizeof(g->idiomas[0]), "%s", codigo);
    Texto_copia(g->nomes_idioma[g->n_idiomas], sizeof(g->nomes_idioma[0]), nome);
    g->n_idiomas++;
}

/* o idioma escolhido, na lista (0 = português, se ele não estiver lá) */
int Game_language_index(const Game *g)
{
    for (int i = 0; i < g->n_idiomas; i++)
        if (strcmp(g->idiomas[i], g->opcoes.idioma) == 0) return i;
    return 0;
}

/* O aparelho do jogador k está aí? Quem entrou pelo teclado, sempre; os
   outros, se o controle do espaço deles está conectado. (Sem informação,
   nos testes: sim.) */
bool Game_present(const Game *g, int k)
{
    return !g->aparelhos || g->jog[k].no_teclado || g->conectado[k];
}

void Game_set_devices(Game *g, const bool *conectado, const int *teclas, const bool *guitarra)
{
    g->aparelhos = true;
    for (int k = 0; k < GAME_MAX_JOGADORES; k++) {
        g->conectado[k] = conectado[k];
        g->teclas[k]    = teclas[k];
        g->guitarra[k]  = guitarra[k];
    }
}

/* Quem manda (volta para a lista, começa sem os outros): o primeiro jogador
   da partida que ainda tem o aparelho. Era sempre o J1; agora, se o
   controle do J1 some, o J2 assume. */
int Game_leader(const Game *g)
{
    for (int k = 0; k < GAME_MAX_JOGADORES; k++)
        if (g->jog[k].ativo && Game_present(g, k)) return k;
    return -1;
}

int Game_n_jogando(const Game *g)
{
    int n = 0;
    for (int k = 0; k < GAME_MAX_JOGADORES; k++) n += g->jog[k].ativo;
    return n;
}

/* o primeiro jogador de teclado que entrou e ainda não está pronto */
int Game_vez_teclado(const Game *g)
{
    for (int k = 0; k < GAME_MAX_JOGADORES; k++)
        if (g->jog[k].no_teclado && g->jog[k].ativo && !g->jog[k].pronto) return k;
    return -1;
}

/* alguém da sessão joga no teclado? (senão, as teclas de menu ajudam o líder) */
static bool divide_teclado(const Game *g)
{
    for (int k = 0; k < GAME_MAX_JOGADORES; k++)
        if (g->jog[k].no_teclado && g->jog[k].na_sessao) return true;
    return false;
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

/* o placar e a pista do jogador do zero (o chart e as escolhas ficam) */
static void zera_jogador(Jogador *j)
{
    Chart_reset(&j->chart);
    j->proxima          = 0;
    j->primeira_visivel = 0;
    j->segurados        = 0;
    j->whammy           = 0.0f;
    j->pontos           = 0.0;
    j->mult_mudou_t     = -1.0e9;
    j->multiplicador    = 1;
    j->combo = j->maior_combo = j->acertos = j->erros = j->toques_errados = 0;
    j->sp_medidor       = 0.0f;
    j->sp_ativo         = false;
    j->sp_frase_falhou  = false;
    for (int f = 0; f < 5; f++) j->acerto_t[f] = -1.0e9;   /* "nunca": sem animação no início */
    j->ultimo_julgamento_t = -1.0e9;
    j->ultimo_erro_tempo   = 0.0;
    j->ultimo_foi_erro     = false;
    j->soma_erro        = 0.0;
    j->soma_erro2       = 0.0;
    j->novo_recorde     = false;
    j->recorde_anterior = 0;
    j->palhetada_t      = -1.0;
    j->hopo_t           = -1.0e9;
    j->trastes_antes    = 0;
}

/* começa (ou recomeça) a partida do zero */
static void comeca_partida(Game *g)
{
    junta_charts(g);
    g->partida++;            /* o main.c vê a mudança e começa o som do zero */
    g->t         = g->inicio;   /* negativo: tempo de preparação antes da música */
    g->pausado   = false;
    g->terminou  = false;
    g->sel_pausa = 0;
    g->pausas    = 0;
    for (int k = 0; k < GAME_MAX_JOGADORES; k++) zera_jogador(&g->jog[k]);
}

void Game_vai_para(Game *g, Tela t)
{
    g->tela       = t;
    g->escolhendo = false;
    if (t == TELA_JOGO) comeca_partida(g);
    if (t == TELA_JOGADORES) Jogadores_entra(g);
    if (t == TELA_PREPARAR) {
        for (int k = 0; k < GAME_MAX_JOGADORES; k++) {
            g->jog[k].pronto     = false;
            g->jog[k].escolhendo = false;
            g->jog[k].linha_prep = PREP_PRONTO;   /* a mesma escolha da música anterior: um Confirmar */
        }
    }
}

void Game_play(Game *g)
{
    Game_vai_para(g, TELA_JOGO);
}

static void volta(Game *g)
{
    switch (g->tela) {
    case TELA_MUSICAS:    if (!Biblioteca_sobe(g)) Game_vai_para(g, TELA_JOGADORES); break;
    case TELA_JOGADORES:  Game_vai_para(g, TELA_INICIO); break;
    case TELA_PREPARAR:   Game_vai_para(g, TELA_MUSICAS); break;
    case TELA_CARREGANDO:
        g->pedido = PEDIDO_CANCELAR_CARGA;
        Game_vai_para(g, TELA_PREPARAR);
        break;
    case TELA_CONFIG:
        g->pedido = PEDIDO_SALVAR_OPCOES;   /* saindo das configurações: grava */
        Game_vai_para(g, TELA_INICIO);
        break;
    case TELA_CONTROLES:
        if (g->controles_volta == TELA_JOGADORES) {
            g->tela       = TELA_JOGADORES;   /* sem Jogadores_entra: quem estava pronto continua */
            g->escolhendo = false;
        } else {
            Game_vai_para(g, TELA_CONFIG);
        }
        break;
    case TELA_AUDIO:     Game_vai_para(g, TELA_CONFIG); break;
    case TELA_JOGO:      g->pausado = false;       break;   /* "voltar" na pausa = continuar */
    default:             break;
    }
}

static int limita(int v, int min, int max)
{
    return v < min ? min : (v > max ? max : v);
}

/* muda o valor de um ITEM_OPCAO / ITEM_VALOR: passo = -1 ou +1 */
static void ajusta(Game *g, int id, int passo)
{
    Opcoes *o = &g->opcoes;
    switch (id) {
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
    case ID_SOM_ERRO:
        o->som_erro = !o->som_erro;
        if (o->som_erro) g->pedido = PEDIDO_TOCAR_SOM_ERRO;   /* para saber como ele soa */
        break;
    case ID_JOGADOR:
        g->controles_jogador = (g->controles_jogador + passo + GAME_MAX_JOGADORES) % GAME_MAX_JOGADORES;
        break;
    case ID_IDIOMA: {
        int i = Game_language_index(g);
        i = (i + passo + g->n_idiomas) % g->n_idiomas;
        snprintf(o->idioma, sizeof(o->idioma), "%s", g->idiomas[i]);
        g->pedido = PEDIDO_TROCAR_IDIOMA;   /* o main.c lê o arquivo do idioma */
        break;
    }
    case ID_ANALOGICO:  g->pedido = PEDIDO_TROCAR_ANALOGICO; break;
    case ID_FRONTAIS:   g->pedido = PEDIDO_TROCAR_FRONTAIS;  break;
    default: break;
    }
}

/* ---------------------------------------------------------------------------
 * Músicas e a preparação: cada jogador escolhe o instrumento e a
 * dificuldade, entre as partes que o chart tem.
 * ------------------------------------------------------------------------- */
static bool tem_instrumento(const Game *g, int i)
{
    for (int d = 0; d < DIF_N; d++)
        if (g->notas[i][d] > 0) return true;
    return false;
}

/* a escolha do jogador existe neste chart? senão, a mais parecida: o primeiro
   instrumento que existe, e a dificuldade mais perto da que ele tinha */
static void escolha_valida(Game *g, Jogador *j)
{
    if (!tem_instrumento(g, j->instrumento)) {
        for (int i = 0; i < INSTR_N; i++)
            if (tem_instrumento(g, i)) { j->instrumento = (Instrumento)i; break; }
    }
    const int *n = g->notas[j->instrumento];
    if (n[j->dificuldade] > 0) return;
    for (int d = j->dificuldade; d >= 0; d--)        /* primeiro uma mais fácil... */
        if (n[d] > 0) { j->dificuldade = (Dificuldade)d; return; }
    for (int d = j->dificuldade; d < DIF_N; d++)     /* ...senão uma mais difícil */
        if (n[d] > 0) { j->dificuldade = (Dificuldade)d; return; }
}

static void troca_instrumento(Game *g, Jogador *j, int passo)
{
    int i = j->instrumento;
    for (int k = 0; k < INSTR_N; k++) {
        i = (i + passo + INSTR_N) % INSTR_N;
        if (tem_instrumento(g, i)) break;
    }
    j->instrumento = (Instrumento)i;
    escolha_valida(g, j);
}

/* Esq = mais fácil, Dir = mais difícil, pulando as que não existem (sem dar a volta) */
static void troca_dificuldade(Game *g, Jogador *j, int passo)
{
    for (int d = j->dificuldade + passo; d >= 0 && d < DIF_N; d += passo)
        if (g->notas[j->instrumento][d] > 0) { j->dificuldade = (Dificuldade)d; return; }
}

/* a escolha da linha do cartão, para o lado de passo */
static void muda_prep(Game *g, Jogador *j, int passo)
{
    if (j->linha_prep == PREP_INSTRUMENTO) troca_instrumento(g, j, passo);
    if (j->linha_prep == PREP_DIFICULDADE) troca_dificuldade(g, j, passo);
}

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
        avisa(g, N_("Não consegui ler o chart"));
        return false;
    }

    for (int i = 0; i < INSTR_N; i++)
        for (int d = 0; d < DIF_N; d++) g->notas[i][d] = ChartFile_contar(&g->arquivo, (Instrumento)i, (Dificuldade)d);

    /* joga quem está na sessão (tela de jogadores) e tem o aparelho; sem
       ninguém (os testes, que pulam aquela tela), o jogador 1 */
    bool alguem = false;
    for (int k = 0; k < GAME_MAX_JOGADORES; k++) {
        g->jog[k].ativo = g->jog[k].na_sessao && Game_present(g, k);
        alguem |= g->jog[k].ativo;
        escolha_valida(g, &g->jog[k]);
    }
    if (!alguem) g->jog[0].na_sessao = g->jog[0].ativo = true;
    Game_vai_para(g, TELA_PREPARAR);
    return true;
}

/* todos prontos: monta o chart de cada um e pede o áudio */
static void monta_partida(Game *g)
{
    const bool tem_musica = g->musica >= 0 && g->musica < g->n_musicas;
    const double atraso = tem_musica ? g->musicas[g->musica].atraso_ms / 1000.0 : 0.0;
    libera_charts(g);
    for (int k = 0; k < GAME_MAX_JOGADORES; k++) {
        Jogador *j = &g->jog[k];
        if (!j->ativo) continue;
        if (!Chart_from_file(&j->chart, &g->arquivo, j->instrumento, j->dificuldade, atraso)) {
            avisa(g, N_("Essa parte do chart está vazia"));
            j->pronto = false;
            return;
        }
    }
    snprintf(g->titulo, sizeof(g->titulo), "%s", tem_musica ? g->musicas[g->musica].titulo : "?");
    g->duracao_audio = 0.0;
    junta_charts(g);
    g->pedido        = PEDIDO_CARREGAR_MUSICA;   /* o main.c decodifica o áudio; depois Game_audio_ready */
    g->pedido_indice = g->musica;
    Game_vai_para(g, TELA_CARREGANDO);
}

static void entra(Game *g, Jogador *j)
{
    j->ativo      = true;
    j->pronto     = false;
    j->escolhendo = false;
    j->linha_prep = PREP_PRONTO;
    escolha_valida(g, j);
}

#define BIT(a) ACAO_BIT(ACAO_##a)

/* Os botões de um jogador desta música, no cartão dele. Cima/Baixo escolhe a
 * linha (instrumento, dificuldade, pronto); Confirmar entra no modo de
 * escolha dela, onde Cima/Baixo mudam o valor, Confirmar fica e Voltar
 * desfaz. Esq/Dir, de quem tem, mudam o valor direto. Fora do modo de
 * escolha, Voltar desfaz o pronto e, de novo, deixa este jogador fora desta
 * música (o líder volta para a lista). Devolve false se saiu da tela. */
static bool prep_botoes(Game *g, int k, uint32_t ap)
{
    Jogador *j = &g->jog[k];
    const int lado = ((ap & BIT(DIREITA)) ? 1 : 0) - ((ap & BIT(ESQUERDA)) ? 1 : 0);
    if (j->escolhendo) {
        const int passo = lado + ((ap & BIT(CIMA)) ? 1 : 0) - ((ap & BIT(BAIXO)) ? 1 : 0);
        if (passo) muda_prep(g, j, passo);
        if (ap & BIT(CONFIRMA)) {
            j->escolhendo = false;
        } else if (ap & BIT(VOLTA)) {
            j->instrumento = (Instrumento)j->antes[0];
            j->dificuldade = (Dificuldade)j->antes[1];
            j->escolhendo  = false;
        }
        return true;
    }
    const bool lider = k == Game_leader(g);
    if (ap & BIT(VOLTA)) {
        if (j->pronto)   j->pronto = false;
        else if (!lider) j->ativo  = false;
        else             { Game_vai_para(g, TELA_MUSICAS); return false; }
        return true;
    }
    if (j->pronto) {
        if ((ap & BIT(CONFIRMA)) && lider) {   /* o líder confirma de novo: começa sem quem não ficou pronto */
            for (int o = 0; o < GAME_MAX_JOGADORES; o++)
                if (!g->jog[o].pronto) g->jog[o].ativo = false;
        }
        return true;
    }
    if (ap & BIT(CIMA))  j->linha_prep = (j->linha_prep + PREP_N - 1) % PREP_N;
    if (ap & BIT(BAIXO)) j->linha_prep = (j->linha_prep + 1) % PREP_N;
    if (lado) muda_prep(g, j, lado);
    if (ap & BIT(CONFIRMA)) {
        if (j->linha_prep == PREP_PRONTO) {
            j->pronto = true;
        } else {
            j->escolhendo = true;
            j->antes[0]   = (int)j->instrumento;
            j->antes[1]   = (int)j->dificuldade;
        }
    }
    return true;
}

/* Um jogador de controle, com os próprios botões. Devolve false se saiu da tela. */
static bool prepara_controle(Game *g, int k, uint32_t ap)
{
    Jogador *j = &g->jog[k];
    if (!j->ativo) {   /* quem é da sessão e ficou de fora desta música pode voltar */
        if (j->na_sessao && Game_present(g, k) && (ap & BIT(CONFIRMA))) entra(g, j);
        return true;
    }
    return prep_botoes(g, k, ap);
}

/* O teclado dividido: atende um jogador de teclado por vez, na ordem (J1,
 * J2...), com as mesmas linhas; quem fica pronto passa a vez ao próximo.
 * Sem ninguém na vez: Esc desfaz o último pronto e Enter traz de volta quem
 * tinha ficado de fora. Devolve false se saiu da tela. */
static bool prepara_teclado(Game *g, uint32_t ap)
{
    const int k = Game_vez_teclado(g);
    if (k >= 0) return prep_botoes(g, k, ap);
    if (ap & ACAO_BIT(ACAO_VOLTA)) {
        for (int o = GAME_MAX_JOGADORES - 1; o >= 0; o--) {
            if (g->jog[o].no_teclado && g->jog[o].ativo && g->jog[o].pronto) {
                g->jog[o].pronto = false;
                return true;
            }
        }
        Game_vai_para(g, TELA_MUSICAS);
        return false;
    }
    if (ap & ACAO_BIT(ACAO_CONFIRMA)) {
        for (int o = 0; o < GAME_MAX_JOGADORES; o++)
            if (g->jog[o].no_teclado && g->jog[o].na_sessao && !g->jog[o].ativo) { entra(g, &g->jog[o]); break; }
    }
    return true;
}

/* A tela de preparação: quem está na sessão (tela de jogadores) escolhe
 * instrumento e dificuldade para esta música, no seu cartão (prep_botoes).
 * Os jogadores de teclado, um de cada vez. Quando todos os que vão jogar
 * estão prontos, a partida começa; se alguém sumiu, o líder confirma de novo
 * e começa sem ele. Sem ninguém no teclado, as teclas de menu valem como as
 * do líder (no PC, quem joga no controle pode usar o Enter). */
static void prepara(Game *g, const Botoes *b, int n, const Botoes *teclado)
{
    const bool dividido = divide_teclado(g);
    const int  lider    = Game_leader(g);
    for (int k = 0; k < GAME_MAX_JOGADORES; k++) {
        if (g->jog[k].no_teclado) continue;   /* esses, o teclado atende */
        uint32_t ap = k < n ? b[k].apertados : 0;
        if (k == lider && teclado && !dividido) ap |= teclado->apertados;
        if (!prepara_controle(g, k, ap)) return;
    }
    if (teclado && dividido && !prepara_teclado(g, teclado->apertados)) return;

    for (int k = 0; k < GAME_MAX_JOGADORES; k++)
        if (g->jog[k].ativo && !g->jog[k].pronto) return;
    monta_partida(g);
}

void Game_audio_ready(Game *g, bool ok, double duracao)
{
    if (g->tela != TELA_CARREGANDO) return;
    g->duracao_audio = ok ? duracao : 0.0;
    if (!ok) avisa(g, N_("Sem áudio: tocando com o metrônomo"));
    Game_vai_para(g, TELA_JOGO);
}

static void executa(Game *g, const ItemMenu *item)
{
    /* as faixas de ids, da maior para a menor (game.h) */
    if (Game_item_musica(item->id) >= 0) {
        g->musica        = Game_item_musica(item->id);
        g->pedido        = PEDIDO_ABRIR_MUSICA;   /* o main.c lê o chart e chama Game_open_chart */
        g->pedido_indice = g->musica;
        return;
    }
    if (Game_item_pasta(item->id) >= 0) {
        Biblioteca_entra(g, Game_item_pasta(item->id));
        return;
    }
    if (Game_item_acao(item->id) >= 0) {
        g->pedido      = PEDIDO_MAPEAR;
        g->pedido_acao = (Acao)Game_item_acao(item->id);
        return;
    }

    switch (item->id) {
    case ID_JOGAR:     Game_vai_para(g, TELA_JOGADORES); break;
    case ID_DEMO:
        g->musica = -1;
        carrega_demo(g);
        Game_vai_para(g, TELA_JOGO);
        break;
    case ID_CONFIG:    Game_vai_para(g, TELA_CONFIG);    break;
    case ID_CONTROLES:
        g->controles_volta = TELA_CONFIG;
        Game_vai_para(g, TELA_CONTROLES);
        break;
    case ID_AUDIO_TESTE:    Game_vai_para(g, TELA_AUDIO);         break;
    case ID_AUDIO_CARREGAR: g->pedido = PEDIDO_AUDIO_CARREGAR; break;
    case ID_AUDIO_TOCAR:    g->pedido = PEDIDO_AUDIO_TOCAR;    break;
    case ID_VOLTAR:    volta(g);                    break;
    case ID_SAIR:      g->pedido = PEDIDO_SAIR;     break;
    case ID_PROCURAR:  g->pedido = PEDIDO_PROCURAR_MUSICAS; avisa(g, N_("Procurando músicas...")); break;
    case ID_RESTAURAR:
        g->pedido = PEDIDO_RESTAURAR_CONTROLES;
        avisa(g, N_("Controles restaurados"));
        break;
    case ID_CONTINUAR: g->pausado = false;          break;
    case ID_REINICIAR: comeca_partida(g);           break;
    case ID_SAIR_MENU: Game_vai_para(g, TELA_MUSICAS);   break;
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
 * REPETE_PASSO, e depois de REPETE_RAPIDO, bem mais rápido. Assim dá para
 * descer uma lista comprida sem apertar 100 vezes (e sem Esq/Dir, numa
 * guitarra), e um toque continua andando só um. */
int Game_repete(Game *g, const Botoes *b, Acao a, double dt)
{
    const int i = a == ACAO_CIMA ? 0 : 1;
    if (b->apertados & ACAO_BIT(a)) {
        g->segura_t[i] = 0.0;
        g->repete_t[i] = 0.0;
        return 1;
    }
    if (!(b->segurados & ACAO_BIT(a))) {
        g->segura_t[i] = 0.0;
        return 0;
    }
    g->segura_t[i] += dt;
    if (g->segura_t[i] < REPETE_ESPERA) return 0;
    const double passo = g->segura_t[i] < REPETE_RAPIDO ? REPETE_PASSO : REPETE_CORRE;
    int n = 0;
    for (g->repete_t[i] += dt; g->repete_t[i] >= passo; g->repete_t[i] -= passo) n++;
    return n;
}

/* muda o valor passo a passo (cada ajusta() anda um) */
static void ajusta_varios(Game *g, int id, int passos)
{
    for (; passos > 0; passos--) ajusta(g, id, +1);
    for (; passos < 0; passos++) ajusta(g, id, -1);
}

/* Voltar no modo de escolha: o valor de antes */
static void desfaz_escolha(Game *g)
{
    const bool idioma = strcmp(g->opcoes.idioma, g->opcoes_antes.idioma) != 0;
    g->opcoes            = g->opcoes_antes;
    g->controles_jogador = g->jogador_antes;
    g->escolhendo        = false;
    if (idioma) g->pedido = PEDIDO_TROCAR_IDIOMA;
}

static void navega(Game *g, const Menu *m, int *sel, const Botoes *b, double dt)
{
    const uint32_t ap   = b->apertados;
    const int      lado = ((ap & ACAO_BIT(ACAO_DIREITA)) ? 1 : 0) - ((ap & ACAO_BIT(ACAO_ESQUERDA)) ? 1 : 0);
    const int      sobe = Game_repete(g, b, ACAO_CIMA, dt), desce = Game_repete(g, b, ACAO_BAIXO, dt);

    /* o modo de escolha de um ITEM_VALOR: Cima/Baixo mudam, Confirmar fica, Voltar desfaz */
    if (g->escolhendo) {
        ajusta_varios(g, m->itens[*sel].id, sobe - desce + lado);
        if (ap & ACAO_BIT(ACAO_CONFIRMA))   g->escolhendo = false;
        else if (ap & ACAO_BIT(ACAO_VOLTA)) desfaz_escolha(g);
        return;
    }

    /* um toque dá a volta na lista; segurando, para no fim */
    if (sobe || desce) {
        const bool toque = ap & (ACAO_BIT(ACAO_CIMA) | ACAO_BIT(ACAO_BAIXO));
        *sel += desce - sobe;
        if (toque)              *sel = (*sel % m->n + m->n) % m->n;
        else if (*sel < 0)      *sel = 0;
        else if (*sel >= m->n)  *sel = m->n - 1;
    }

    const ItemMenu *item = &m->itens[*sel];
    if (item->tipo != ITEM_BOTAO && lado != 0) {
        if (item->em_breve) avisa(g, N_("Em breve"));
        else                ajusta(g, item->id, lado);
    } else if (g->tela == TELA_MUSICAS && lado != 0) {
        /* na lista de músicas, Esq/Dir pulam uma página inteira (sem dar a volta) */
        *sel += lado * GAME_PAGINA;
        if (*sel < 0) *sel = 0;
        if (*sel > m->n - 1) *sel = m->n - 1;
        item = &m->itens[*sel];
    }

    if (ap & ACAO_BIT(ACAO_CONFIRMA)) {
        if (item->em_breve) {
            avisa(g, N_("Em breve"));
        } else if (item->tipo == ITEM_VALOR) {
            g->escolhendo    = true;
            g->opcoes_antes  = g->opcoes;
            g->jogador_antes = g->controles_jogador;
        } else {
            executa(g, item);
        }
    } else if (ap & ACAO_BIT(ACAO_VOLTA)) {
        volta(g);
    }
}

/* ---------------------------------------------------------------------------
 * A partida
 * ------------------------------------------------------------------------- */

static void star_power(Game *g, Jogador *j, const Botoes *b, double dt)
{
    if ((b->apertados & ACAO_BIT(ACAO_STAR_POWER)) && !j->sp_ativo && j->sp_medidor >= GAME_SP_MINIMO) {
        j->sp_ativo = true;
        if (g->n_eventos < GAME_MAX_EVENTOS)
            g->eventos[g->n_eventos++] = (EventoJogo){ EV_STAR_ATIVOU, (int)(j - g->jog), -1, 0.0 };
    }
    if (j->sp_ativo) {
        j->sp_medidor -= (float)dt / GAME_SP_DURACAO;
        if (j->sp_medidor <= 0.0f) {
            j->sp_medidor = 0.0f;
            j->sp_ativo   = false;
        }
    }
}

/* notas que já saíram de vista de vez (o draw e o juiz começam daqui) */
static void avanca_visiveis(const Game *g, Jogador *j)
{
    const double margem = 0.2 * Game_janela_visivel(g);
    while (j->primeira_visivel < j->chart.n) {
        const Nota *n = &j->chart.notas[j->primeira_visivel];
        if (n->segurando || n->t + n->duracao >= g->t - margem) break;
        j->primeira_visivel++;
    }
}

static void pausa(Game *g)
{
    g->pausado   = true;
    g->sel_pausa = 0;
    g->pausas++;
}

/* b = os botões de cada jogador; u = todos juntos (pausa e menus); teclado:
   as teclas de menu (o Esc também pausa, como em todo jogo de PC) */
static void atualiza_jogo(Game *g, const Botoes *b, int n, const Botoes *u, const Botoes *teclado, double dt)
{
    if (g->terminou) {
        navega(g, Menu_fim(), &g->sel_pausa, u, dt);
        return;
    }
    if (g->pausado) {
        if (u->apertados & ACAO_BIT(ACAO_PAUSA)) { g->pausado = false; return; }
        navega(g, Menu_pausa(), &g->sel_pausa, u, dt);
        return;
    }
    if ((u->apertados & ACAO_BIT(ACAO_PAUSA)) || (teclado && (teclado->apertados & ACAO_BIT(ACAO_VOLTA)))) {
        pausa(g);   /* qualquer jogador pausa */
        return;
    }

    /* com áudio, o tempo é o do áudio; ele nunca volta para trás */
    /* o ajuste de tempo desloca o relógio do áudio: com +48 ms, o jogo "vê" a
       música 48 ms à frente, compensando o atraso entre o som sair e chegar */
    double novo = g->relogio_audio ? g->t_audio + g->opcoes.ajuste_ms / 1000.0 : g->t + dt;
    if (novo < g->t) novo = g->t;
    const double passo = novo - g->t;
    g->t = novo;

    /* acaba quando acabam as notas de todos e a música (o que vier depois) */
    const double fim = g->fim + FIM_ESPERA > g->duracao_audio ? g->fim + FIM_ESPERA : g->duracao_audio;
    bool acabou = g->t > fim;

    static const Botoes NADA = {0};
    for (int k = 0; k < GAME_MAX_JOGADORES; k++) {
        Jogador *j = &g->jog[k];
        if (!j->ativo) continue;
        const Botoes *bk = k < n ? &b[k] : &NADA;
        j->whammy = bk->whammy;
        if (g->guitarra[k]) Judge_guitarra(g, j, bk->apertados, bk->segurados);
        else                Judge_toques(g, j, bk->apertados, bk->segurados);
        Judge_sustains(g, j, bk->segurados, passo);
        Judge_varre_erros(g, j);
        star_power(g, j, bk, dt);
        avanca_visiveis(g, j);
        if (j->proxima < j->chart.n) acabou = false;
    }

    if (acabou) {
        g->terminou  = true;
        g->sel_pausa = 0;
        Biblioteca_fim_partida(g);   /* recorde? */
    }
}

/* Um aparelho que sumiu (bateria, cabo, dongle): na tela de jogadores, o
 * jogador sai da sessão; na preparação, fica fora desta música (e o líder
 * passa para o próximo); na partida, ela pausa, e quem perdeu o controle
 * pode ligar outro (ele cai no primeiro espaço livre, o mesmo de antes).
 * Quem entrou pelo teclado nunca some. */
/* alguém da partida ainda tem com o que tocar? */
static bool alguem_tocando(const Game *g)
{
    for (int k = 0; k < GAME_MAX_JOGADORES; k++)
        if (g->jog[k].ativo && Game_present(g, k)) return true;
    return false;
}

static void confere_aparelhos(Game *g)
{
    if (!g->aparelhos) return;
    for (int k = 0; k < GAME_MAX_JOGADORES; k++) {
        const bool agora = Game_present(g, k);
        const bool sumiu = g->presente_antes[k] && !agora;
        g->presente_antes[k] = agora;
        Jogador *j = &g->jog[k];
        if (agora) continue;
        if (g->tela == TELA_JOGADORES && j->na_sessao) {
            Jogadores_sai(g, k);
        } else if (g->tela == TELA_PREPARAR && j->ativo) {
            j->ativo  = false;
            j->pronto = false;
        } else if (g->tela == TELA_JOGO && j->ativo && sumiu && !g->terminou && !g->pausado) {
            /* com mais alguém tocando a música segue: quem caiu só deixa as
               notas passarem. Sem ninguém, pausa. */
            if (alguem_tocando(g)) {
                avisa(g, N_("Um controle desconectou"));
            } else {
                pausa(g);
                avisa(g, N_("Um controle desconectou: a partida pausou"));
            }
        }
    }
}

void Game_update(Game *g, const Botoes *b, int n, const Botoes *teclado, double dt)
{
    g->pedido      = PEDIDO_NENHUM;   /* pedidos e eventos valem só pelo frame */
    g->trocar_modo = 0;
    g->n_eventos   = 0;
    g->t_total    += dt;

    /* quem apertou Confirmar (a tela de jogadores põe quem apertou "Jogar") */
    g->origem = -1;
    for (int k = 0; k < n && k < GAME_MAX_JOGADORES && g->origem < 0; k++)
        if (b[k].apertados & ACAO_BIT(ACAO_CONFIRMA)) g->origem = k;
    if (g->origem < 0 && teclado && (teclado->apertados & ACAO_BIT(ACAO_CONFIRMA))) g->origem = GAME_TECLADO;
    if (teclado && teclado->apertados) g->teclado_usado = true;

    /* nos menus, qualquer jogador mexe: vale a soma dos botões (e do teclado) */
    Botoes u = teclado ? *teclado : (Botoes){0};
    for (int k = 0; k < n && k < GAME_MAX_JOGADORES; k++) {
        u.apertados |= b[k].apertados;
        u.segurados |= b[k].segurados;
        if (b[k].whammy > u.whammy) u.whammy = b[k].whammy;
        g->jog[k].segurados = b[k].segurados;
    }

    confere_aparelhos(g);

    if (g->aviso && g->t_total > g->aviso_ate) g->aviso = NULL;

    if (g->tela == TELA_JOGO) {
        atualiza_jogo(g, b, n, &u, teclado, dt);
    } else if (g->tela == TELA_JOGADORES) {
        Jogadores_update(g, b, n, teclado, dt);
    } else if (g->tela == TELA_PREPARAR) {
        prepara(g, b, n, teclado);
    } else if (g->tela == TELA_CARREGANDO) {
        if (u.apertados & ACAO_BIT(ACAO_VOLTA)) volta(g);
    } else {
        const Menu *m = Game_menu(g);
        if (m) navega(g, m, &g->sel[g->tela], &u, dt);
    }
}
