/* A tela de jogadores e as contas.
 *
 * Antes da lista de músicas, cada um diz que vai jogar, com que conta e em
 * que modo. Cada espaço (J1..J4) é um cartão, atendido por um aparelho só:
 *   - o controle daquele espaço: Confirmar entra. Se o controle some, o
 *     jogador sai sozinho;
 *   - ou o teclado: Esq/Dir escolhe um cartão sem controle e Enter entra
 *     nele. O teclado atende um cartão por vez; quem fica pronto passa a vez.
 *     Para ficar pronto, o perfil precisa de teclas próprias (nenhuma
 *     repetida com outro jogador de teclado): senão, Controles.
 * Dentro do cartão, uma lista: a conta, o modo (controle ou guitarra), os
 * controles (a tela de controles desse jogador) e "pronto". Cima/Baixo
 * escolhe a linha; Confirmar entra no modo de escolha dela, ou executa;
 * Voltar desfaz, desfaz o pronto e, de novo, sai. Nada precisa de Esq/Dir:
 * dá para fazer tudo com a palhetada e os trastes verde e vermelho.
 * Todos prontos: uma contagem curta, e a lista de músicas. Quem escolheu
 * aqui joga as próximas músicas, até voltar para esta tela.
 *
 * Contas: o convidado (vários jogadores podem usar ao mesmo tempo) e os nomes
 * salvos no aparelho (cada um com um jogador só). Cada conta tem o seu perfil
 * de controles, com o modo (o main.c troca o perfil quando a conta muda); o
 * convidado usa o perfil do espaço. "+ novo jogador" abre um teclado na tela
 * para escrever o nome. */

#include "game/game.h"

#include <stdio.h>
#include <string.h>

/* o teclado da tela: 4 linhas de 13 teclas, em maiúsculas e minúsculas */
static const char *const TECLAS[2][4 * GAME_TECLAS_LINHA] = {
    { "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
      "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
      "Á", "É", "Í", "Ó", "Ú", "Â", "Ê", "Ô", "Ã", "Õ", "Ç", "Ü", "Ñ",
      "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "-", "_", "." },
    { "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
      "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",
      "á", "é", "í", "ó", "ú", "â", "ê", "ô", "ã", "õ", "ç", "ü", "ñ",
      "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "-", "_", "." },
};

const char *Game_key_text(const Game *g, int i)
{
    if (i >= 0 && i < 4 * GAME_TECLAS_LINHA) return TECLAS[g->nome_minusculas ? 1 : 0][i];
    switch (i) {
    case TECLA_ESPACO: return N_("espaço");
    case TECLA_CAIXA:  return "aA";
    case TECLA_APAGA:  return N_("apagar");
    case TECLA_OK:     return "OK";
    default:           return "";
    }
}

/* ---------------------------------------------------------------------------
 * Contas
 * ------------------------------------------------------------------------- */
void Game_add_account(Game *g, const char *nome)
{
    if (!nome || !*nome || g->n_contas >= GAME_MAX_CONTAS) return;
    for (int i = 0; i < g->n_contas; i++)
        if (strcmp(g->contas[i], nome) == 0) return;   /* já existe */
    Texto_copia(g->contas[g->n_contas++], GAME_NOME, nome);
}

const char *Game_player_name(const Game *g, int k)
{
    const int c = g->jog[k].conta;
    return (c >= 1 && c <= g->n_contas) ? g->contas[c - 1] : NULL;
}

static int acha_conta(const Game *g, const char *nome)
{
    for (int i = 0; i < g->n_contas; i++)
        if (nome[0] && strcmp(g->contas[i], nome) == 0) return i + 1;
    return 0;
}

/* a conta c (um nome) já está com outro jogador da sessão? (o convidado pode repetir) */
static bool conta_em_uso(const Game *g, int c, int k)
{
    if (c < 1 || c > g->n_contas) return false;
    for (int o = 0; o < GAME_MAX_JOGADORES; o++)
        if (o != k && g->jog[o].na_sessao && g->jog[o].conta == c) return true;
    return false;
}

/* a próxima conta (passo +1) ou a anterior (-1): convidado, os nomes livres
   e "+ novo jogador" (n_contas + 1), em roda */
static void troca_conta(Game *g, int k, int passo)
{
    const int n = g->n_contas + 2;
    int c = g->jog[k].conta;
    for (int t = 0; t < n; t++) {
        c = (c + passo + n) % n;
        if (c == 0 || c == g->n_contas + 1 || !conta_em_uso(g, c, k)) break;
    }
    g->jog[k].conta = c;
}

/* ---------------------------------------------------------------------------
 * Entrar, sair
 * ------------------------------------------------------------------------- */
static bool sessao_vazia(const Game *g)
{
    for (int k = 0; k < GAME_MAX_JOGADORES; k++)
        if (g->jog[k].na_sessao) return false;
    return true;
}

bool Game_teclado_livre(const Game *g, int k)
{
    return !g->jog[k].na_sessao && (!g->aparelhos || !g->conectado[k]);
}

/* entra com a última conta que usou (se ela ainda existe e está livre) */
static void entra(Game *g, int k, bool teclado)
{
    Jogador *j = &g->jog[k];
    j->na_sessao    = true;
    j->no_teclado   = teclado;
    j->pronto       = false;
    j->escolhendo   = false;
    j->linha_sessao = SESSAO_PRONTO;   /* com a conta de sempre, um Confirmar basta */
    const int c = acha_conta(g, g->opcoes.conta[k]);
    j->conta = conta_em_uso(g, c, k) ? 0 : c;
}

/* fora da sessão, o espaço volta ao convidado: o perfil da conta não fica em
   dois espaços ao mesmo tempo */
void Jogadores_sai(Game *g, int k)
{
    Jogador *j = &g->jog[k];
    j->na_sessao  = false;
    j->no_teclado = false;
    j->pronto     = false;
    j->escolhendo = false;
    j->conta      = 0;
    if (g->nome_jogador == k) g->nome_jogador = -1;
}

/* Chegando na tela: sai quem perdeu o aparelho; sem ninguém, entra quem
 * apertou "Jogar" (o controle dele, ou o teclado num cartão sem controle). */
void Jogadores_entra(Game *g)
{
    g->contagem     = 0.0;
    g->nome_jogador = -1;
    for (int k = 0; k < GAME_MAX_JOGADORES; k++) {
        g->jog[k].pronto     = false;
        g->jog[k].escolhendo = false;
        if (g->jog[k].na_sessao && !Game_present(g, k)) Jogadores_sai(g, k);
    }
    if (!sessao_vazia(g)) return;
    const int o = g->origem;
    if (o >= 0 && o < GAME_MAX_JOGADORES) {
        entra(g, o, false);
    } else if (o == GAME_TECLADO) {
        for (int k = 0; k < GAME_MAX_JOGADORES; k++)
            if (Game_teclado_livre(g, k)) { entra(g, k, true); g->cursor_teclado = k; break; }
    }
}

static void abre_nome(Game *g, int k)
{
    g->nome_jogador    = k;
    g->nome_novo[0]    = '\0';
    g->nome_cursor     = 0;
    g->nome_minusculas = false;
}

static void avisa(Game *g, const char *texto)
{
    g->aviso     = texto;
    g->aviso_ate = g->t_total + 1.5;
}

/* as teclas deixam este jogador jogar? (só conta para quem está no teclado) */
static bool teclas_prontas(const Game *g, int k)
{
    return !g->jog[k].no_teclado || !g->aparelhos || g->teclas[k] == TECLAS_OK;
}

#define BIT(a) ACAO_BIT(ACAO_##a)

/* Os botões de quem está na sessão, no cartão dele. Devolve false se saiu da tela. */
static bool cartao(Game *g, int k, uint32_t ap)
{
    Jogador *j = &g->jog[k];
    const int lado = ((ap & BIT(DIREITA)) ? 1 : 0) - ((ap & BIT(ESQUERDA)) ? 1 : 0);

    /* o modo de escolha da conta: Cima/Baixo mudam, Confirmar fica, Voltar desfaz */
    if (j->escolhendo) {
        const int passo = lado + ((ap & BIT(CIMA)) ? 1 : 0) - ((ap & BIT(BAIXO)) ? 1 : 0);
        if (passo) troca_conta(g, k, passo);
        if (ap & BIT(CONFIRMA)) {
            j->escolhendo = false;
            if (j->conta == g->n_contas + 1) abre_nome(g, k);   /* "+ novo jogador" */
        } else if (ap & BIT(VOLTA)) {
            j->conta      = j->antes[0];
            j->escolhendo = false;
        }
        return true;
    }
    if (ap & BIT(VOLTA)) {
        if (j->pronto) j->pronto = false;
        else           Jogadores_sai(g, k);
        return true;
    }
    if (j->pronto) return true;

    if (ap & BIT(CIMA))  j->linha_sessao = (j->linha_sessao + SESSAO_N - 1) % SESSAO_N;
    if (ap & BIT(BAIXO)) j->linha_sessao = (j->linha_sessao + 1) % SESSAO_N;
    if (lado && j->linha_sessao == SESSAO_CONTA) troca_conta(g, k, lado);
    if (lado && j->linha_sessao == SESSAO_MODO)  g->trocar_modo |= 1u << k;
    if (!(ap & BIT(CONFIRMA))) return true;

    switch (j->linha_sessao) {
    case SESSAO_CONTA:
        j->escolhendo = true;
        j->antes[0]   = j->conta;
        break;
    case SESSAO_MODO:   /* o modo é do perfil (da conta): o main.c troca e salva */
        g->trocar_modo |= 1u << k;
        break;
    case SESSAO_CONTROLES:
        g->controles_jogador = k;
        g->controles_volta   = TELA_JOGADORES;
        Game_vai_para(g, TELA_CONTROLES);
        return false;
    default:   /* pronto */
        if (j->conta == g->n_contas + 1) abre_nome(g, k);   /* "+ novo jogador" sem nome ainda */
        else if (teclas_prontas(g, k))   j->pronto = true;
        else                             avisa(g, N_("Ajuste as teclas em Controles"));
        break;
    }
    return true;
}

/* um cartão de fora da sessão: Confirmar entra; Voltar, sem ninguém, sai da tela */
static bool fora(Game *g, int k, uint32_t ap, bool teclado)
{
    if (ap & BIT(CONFIRMA)) {
        entra(g, k, teclado);
    } else if ((ap & BIT(VOLTA)) && sessao_vazia(g)) {
        Game_vai_para(g, TELA_INICIO);
        return false;
    }
    return true;
}

/* o teclado pode atender o cartão k: um jogador de teclado, ou um cartão livre sem controle */
static bool do_teclado(const Game *g, int k)
{
    return g->jog[k].na_sessao ? g->jog[k].no_teclado : Game_teclado_livre(g, k);
}

/* o próximo cartão do teclado a partir de k, para o lado de passo (-1 = nenhum) */
static int proximo_teclado(const Game *g, int k, int passo)
{
    for (int t = 1; t <= GAME_MAX_JOGADORES; t++) {
        const int o = ((k + passo * t) % GAME_MAX_JOGADORES + GAME_MAX_JOGADORES) % GAME_MAX_JOGADORES;
        if (do_teclado(g, o)) return o;
    }
    return -1;
}

/* O teclado: Esq/Dir escolhe o cartão; o resto vai para ele. Quem fica
   pronto passa a vez para o próximo jogador de teclado que falta. */
static bool atende_teclado(Game *g, uint32_t ap)
{
    int k = g->cursor_teclado;
    if (k < 0 || k >= GAME_MAX_JOGADORES || !do_teclado(g, k)) k = g->cursor_teclado = proximo_teclado(g, -1, +1);
    if (k < 0) {   /* nenhum cartão para o teclado (4 controles) */
        if ((ap & BIT(VOLTA)) && sessao_vazia(g)) { Game_vai_para(g, TELA_INICIO); return false; }
        return true;
    }
    Jogador *j = &g->jog[k];
    if (!j->escolhendo && (ap & (BIT(ESQUERDA) | BIT(DIREITA)))) {
        g->cursor_teclado = proximo_teclado(g, k, (ap & BIT(DIREITA)) ? +1 : -1);
        return true;
    }
    if (!j->na_sessao) return fora(g, k, ap, true);

    const bool antes = j->pronto;
    if (!cartao(g, k, ap)) return false;
    if (!antes && j->pronto) {
        for (int o = proximo_teclado(g, k, +1); o >= 0 && o != k; o = proximo_teclado(g, o, +1))
            if (g->jog[o].na_sessao && !g->jog[o].pronto) { g->cursor_teclado = o; break; }
    }
    return true;
}

void Jogadores_update(Game *g, const Botoes *b, int n, const Botoes *tec, double dt)
{
    /* escrevendo um nome: só o aparelho de quem escreve mexe */
    if (g->nome_jogador >= 0) {
        const int k = g->nome_jogador;
        const Botoes *bk = g->jog[k].no_teclado ? tec : (k < n ? &b[k] : NULL);
        if (bk) Jogadores_nome_update(g, bk, dt);
        return;
    }

    for (int k = 0; k < GAME_MAX_JOGADORES; k++) {
        if (g->jog[k].no_teclado) continue;   /* esses, o teclado atende */
        const uint32_t ap = k < n ? b[k].apertados : 0;
        if (!(g->jog[k].na_sessao ? cartao(g, k, ap) : fora(g, k, ap, false))) return;
    }
    if (tec && !atende_teclado(g, tec->apertados)) return;

    /* todos os que entraram estão prontos: contagem, e as músicas */
    bool alguem = false, todos = true;
    for (int k = 0; k < GAME_MAX_JOGADORES; k++) {
        if (!g->jog[k].na_sessao) continue;
        alguem = true;
        todos &= g->jog[k].pronto;
    }
    if (!alguem || !todos || g->nome_jogador >= 0) {
        g->contagem = 0.0;
        return;
    }
    g->contagem += dt;
    if (g->contagem < GAME_CONTAGEM) return;

    /* lembra a conta de cada um para a próxima vez */
    for (int k = 0; k < GAME_MAX_JOGADORES; k++) {
        const char *nome = g->jog[k].na_sessao ? Game_player_name(g, k) : NULL;
        if (g->jog[k].na_sessao) snprintf(g->opcoes.conta[k], sizeof(g->opcoes.conta[k]), "%s", nome ? nome : "");
    }
    g->pedido = PEDIDO_SALVAR_OPCOES;
    Game_vai_para(g, TELA_MUSICAS);
}

/* ---------------------------------------------------------------------------
 * O teclado da tela: 4 linhas de letras e uma de teclas especiais (espaço,
 * maiúsculas, apagar, OK). Voltar apaga a última letra; com o nome vazio,
 * desiste. Com setas, anda na grade; no modo guitarra (sem Esq/Dir), a
 * palhetada anda tecla por tecla, na ordem de leitura (segurar acelera).
 * ------------------------------------------------------------------------- */
static int letras(const char *s)
{
    int n = 0;
    for (; *s; s++) n += ((unsigned char)*s & 0xC0) != 0x80;   /* conta o começo de cada caractere */
    return n;
}

static void apaga_ultima(char *s)
{
    size_t n = strlen(s);
    while (n > 0 && ((unsigned char)s[n - 1] & 0xC0) == 0x80) n--;   /* os bytes do meio de um acento */
    if (n > 0) n--;
    s[n] = '\0';
}

static void conclui_nome(Game *g)
{
    const int k = g->nome_jogador;
    char *s = g->nome_novo;
    size_t n = strlen(s);
    while (n > 0 && s[n - 1] == ' ') s[--n] = '\0';   /* sem espaço no fim */
    g->nome_jogador = -1;
    if (n == 0) { g->jog[k].conta = 0; return; }

    int c = acha_conta(g, s);
    if (c && conta_em_uso(g, c, k)) {
        avisa(g, N_("Esse nome já está jogando"));
        g->jog[k].conta = 0;
        return;
    }
    if (!c) {
        if (g->n_contas >= GAME_MAX_CONTAS) {
            avisa(g, N_("Contas demais"));
            g->jog[k].conta = 0;
            return;
        }
        Game_add_account(g, s);
        c = g->n_contas;
        g->pedido = PEDIDO_SALVAR_CONTAS;   /* o main.c grava jogadores.txt */
    }
    g->jog[k].conta = c;
}

void Jogadores_nome_update(Game *g, const Botoes *b, double dt)
{
    const uint32_t ap = b->apertados;
    const int L = GAME_TECLAS_LINHA;
    int c = g->nome_cursor;
    const bool especial = c >= 4 * L;
    const int linha = especial ? 4 : c / L, coluna = especial ? c - 4 * L : c % L;

    if (g->guitarra[g->nome_jogador]) {
        const int passo = Game_repete(g, b, ACAO_BAIXO, dt) - Game_repete(g, b, ACAO_CIMA, dt)
                        + ((ap & ACAO_BIT(ACAO_DIREITA)) ? 1 : 0) - ((ap & ACAO_BIT(ACAO_ESQUERDA)) ? 1 : 0);
        c = ((c + passo) % GAME_TECLAS_N + GAME_TECLAS_N) % GAME_TECLAS_N;
    } else {
        if (ap & ACAO_BIT(ACAO_ESQUERDA)) c = especial ? 4 * L + (coluna + 3) % 4 : linha * L + (coluna + L - 1) % L;
        if (ap & ACAO_BIT(ACAO_DIREITA))  c = especial ? 4 * L + (coluna + 1) % 4 : linha * L + (coluna + 1) % L;
        if (ap & ACAO_BIT(ACAO_CIMA)) {
            if (especial)       c = 3 * L + coluna * L / 4;        /* da linha de baixo para a coluna parecida */
            else if (linha > 0) c -= L;
            else                c = 4 * L + coluna * 4 / L;
        }
        if (ap & ACAO_BIT(ACAO_BAIXO)) {
            if (especial)       c = coluna * L / 4;
            else if (linha < 3) c += L;
            else                c = 4 * L + coluna * 4 / L;
        }
    }
    g->nome_cursor = c;

    char *nome = g->nome_novo;
    if (ap & ACAO_BIT(ACAO_VOLTA)) {
        if (nome[0]) apaga_ultima(nome);
        else         { g->jog[g->nome_jogador].conta = 0; g->nome_jogador = -1; }   /* desistiu */
        return;
    }
    if (!(ap & ACAO_BIT(ACAO_CONFIRMA))) return;
    switch (c) {
    case TECLA_CAIXA: g->nome_minusculas = !g->nome_minusculas; return;
    case TECLA_APAGA: apaga_ultima(nome); return;
    case TECLA_OK:    conclui_nome(g); return;
    default: break;
    }
    const char *t = c == TECLA_ESPACO ? " " : TECLAS[g->nome_minusculas ? 1 : 0][c];
    if (c == TECLA_ESPACO && !nome[0]) return;   /* sem espaço no começo */
    if (letras(nome) >= GAME_NOME_LETRAS || strlen(nome) + strlen(t) >= GAME_NOME) return;
    strcat(nome, t);
}
