/* A biblioteca de músicas do lado do jogo: a lista que o main.c entrega, o
 * menu montado a partir dela (por pasta, por título ou por artista) e os
 * recordes. Sem SDL e sem arquivo: quem acha as pastas e grava os recordes é
 * a plataforma. */

#include "game/game.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const NOME_ORGANIZAR[ORGANIZAR_N] = { "Pastas", "Titulo", "Artista" };

const char *Biblioteca_nome_organizar(int modo)
{
    return (modo >= 0 && modo < ORGANIZAR_N) ? NOME_ORGANIZAR[modo] : "?";
}

/* copia trocando o que não for ASCII por '?': a fonte de debug não desenha acento */
static void copia_ascii(char *destino, size_t max, const char *origem)
{
    size_t k = 0;
    for (const unsigned char *p = (const unsigned char *)origem; *p && k + 1 < max; p++) {
        if (*p < 0x80)                destino[k++] = (char)*p;
        else if ((*p & 0xC0) == 0xC0) destino[k++] = '?';   /* um '?' por caractere UTF-8 */
    }
    destino[k] = '\0';
}

/* comparação sem diferenciar maiúsculas (strcasecmp não é C11 puro) */
static int compara_sem_caixa(const char *a, const char *b)
{
    for (;; a++, b++) {
        const int x = tolower((unsigned char)*a), y = tolower((unsigned char)*b);
        if (x != y || !x) return x - y;
    }
}

/* garante espaço para o índice n (dobra até caber: a lista pode pular de tamanho) */
static bool cresce(void **v, int *cap, int n, size_t tam)
{
    if (n < *cap) return true;
    int nova = *cap ? *cap : 64;
    while (nova <= n) nova *= 2;
    void *p = realloc(*v, (size_t)nova * tam);
    if (!p) return false;
    *v   = p;
    *cap = nova;
    return true;
}

/* ---------------------------------------------------------------------------
 * A lista
 * ------------------------------------------------------------------------- */
void Game_clear_songs(Game *g)
{
    g->n_musicas = 0;
}

void Game_add_song(Game *g, const SongIni *ini, const char *nome_pasta, const char *grupo)
{
    if (!cresce((void **)&g->musicas, &g->cap_musicas, g->n_musicas, sizeof(InfoMusica))) return;
    InfoMusica *m = &g->musicas[g->n_musicas++];
    memset(m, 0, sizeof(*m));
    copia_ascii(m->titulo, sizeof(m->titulo), ini->nome[0] ? ini->nome : nome_pasta);
    copia_ascii(m->artista, sizeof(m->artista), ini->artista);
    copia_ascii(m->charter, sizeof(m->charter), ini->charter);
    copia_ascii(m->grupo, sizeof(m->grupo), grupo ? grupo : "");
    snprintf(m->rotulo_artista, sizeof(m->rotulo_artista), "%s - %s", m->artista[0] ? m->artista : "?", m->titulo);
    m->duracao_ms  = ini->duracao_ms;
    m->atraso_ms   = ini->atraso_ms;
    m->nota_sp     = ini->nota_sp > 0 ? ini->nota_sp : 116;
    m->recorde_dif = -1;
}

void Game_set_record(Game *g, int musica, long pontos, int dif, int pct, bool fc)
{
    if (musica < 0 || musica >= g->n_musicas) return;
    InfoMusica *m  = &g->musicas[musica];
    m->recorde     = pontos;
    m->recorde_dif = dif;
    m->recorde_pct = pct;
    m->recorde_fc  = fc;
}

/* fim da partida: foi recorde? (só músicas de verdade; a demonstração não conta) */
void Biblioteca_fim_partida(Game *g)
{
    g->novo_recorde = false;
    if (g->musica < 0 || g->musica >= g->n_musicas) return;
    InfoMusica *m = &g->musicas[g->musica];
    const long pontos = (long)g->pontos;
    if (pontos <= m->recorde) return;

    const int total = g->acertos + g->erros;
    g->novo_recorde     = true;
    g->recorde_anterior = m->recorde;
    Game_set_record(g, g->musica, pontos, (int)g->dificuldade,
                    total ? (100 * g->acertos + total / 2) / total : 0, g->erros == 0 && total > 0);
    g->pedido        = PEDIDO_SALVAR_RECORDE;   /* o main.c grava o arquivo */
    g->pedido_indice = g->musica;
}

/* ---------------------------------------------------------------------------
 * Modo pastas: as subpastas da pasta atual, com quantas músicas há dentro
 * (contando as sub-subpastas)
 * ------------------------------------------------------------------------- */

/* se o grupo da música fica dentro da pasta atual, devolve o primeiro trecho
   depois dela ("Tier 1" de "GH3/Tier 1" quando a atual é "GH3"); NULL se não */
static const char *filho(const char *atual, const char *grupo, size_t *tam)
{
    const size_t n = strlen(atual);
    const char *resto;
    if (n == 0) {
        resto = grupo;
    } else {
        if (strncmp(grupo, atual, n) != 0 || grupo[n] != '/') return NULL;
        resto = grupo + n + 1;
    }
    if (!*resto) return NULL;
    const char *barra = strchr(resto, '/');
    *tam = barra ? (size_t)(barra - resto) : strlen(resto);
    return resto;
}

static int compara_nomes(const void *x, const void *y)
{
    return compara_sem_caixa((const char *)x, (const char *)y);
}

static void acha_subpastas(Game *g)
{
    g->n_pastas = 0;
    for (int i = 0; i < g->n_musicas; i++) {
        size_t tam;
        const char *f = filho(g->pasta_atual, g->musicas[i].grupo, &tam);
        if (!f) continue;
        char nome[96];
        snprintf(nome, sizeof(nome), "%.*s", (int)tam, f);
        int k = 0;
        while (k < g->n_pastas && strcmp(g->pastas_nome[k], nome) != 0) k++;
        if (k == g->n_pastas) {
            if (!cresce((void **)&g->pastas_nome, &g->cap_pastas, g->n_pastas, sizeof(g->pastas_nome[0]))) return;
            strcpy(g->pastas_nome[g->n_pastas++], nome);
        }
    }
    if (g->n_pastas > 0) qsort(g->pastas_nome, (size_t)g->n_pastas, sizeof(g->pastas_nome[0]), compara_nomes);

    g->pastas_rotulo = realloc(g->pastas_rotulo, sizeof(g->pastas_rotulo[0]) * (size_t)(g->n_pastas ? g->n_pastas : 1));
    if (!g->pastas_rotulo) { g->n_pastas = 0; return; }
    for (int k = 0; k < g->n_pastas; k++) {
        char dentro[256];
        snprintf(dentro, sizeof(dentro), "%s%s%s", g->pasta_atual, g->pasta_atual[0] ? "/" : "", g->pastas_nome[k]);
        const size_t n = strlen(dentro);
        int conta = 0;
        for (int i = 0; i < g->n_musicas; i++) {
            const char *gr = g->musicas[i].grupo;
            if (strncmp(gr, dentro, n) == 0 && (gr[n] == '\0' || gr[n] == '/')) conta++;
        }
        snprintf(g->pastas_rotulo[k], sizeof(g->pastas_rotulo[0]), "[%s]  %d", g->pastas_nome[k], conta);
    }
}

static int compara_artista(const void *x, const void *y)
{
    const InfoMusica *a = *(const InfoMusica *const *)x, *b = *(const InfoMusica *const *)y;
    const int c = compara_sem_caixa(a->artista, b->artista);
    return c ? c : compara_sem_caixa(a->titulo, b->titulo);
}

/* ---------------------------------------------------------------------------
 * O menu da lista
 * ------------------------------------------------------------------------- */
void Biblioteca_monta_menu(Game *g)
{
    const int modo = g->opcoes.organizar;
    if (modo == ORGANIZAR_PASTAS) acha_subpastas(g);
    else                          g->n_pastas = 0;

    /* organizar + demonstração + pastas + músicas + procurar + voltar */
    const int precisa = g->n_musicas + g->n_pastas + 4;
    if (!cresce((void **)&g->itens_musicas, &g->cap_itens, precisa - 1, sizeof(ItemMenu))) return;

    int n = 0;
    const bool na_raiz = g->pasta_atual[0] == '\0' || modo != ORGANIZAR_PASTAS;
    g->itens_musicas[n++] = (ItemMenu){ "Organizar por", ID_ORGANIZAR, ITEM_OPCAO, false };
    if (na_raiz) g->itens_musicas[n++] = (ItemMenu){ "Demonstracao (sem musica)", ID_DEMO, ITEM_BOTAO, false };

    if (modo == ORGANIZAR_PASTAS) {
        for (int k = 0; k < g->n_pastas; k++)
            g->itens_musicas[n++] = (ItemMenu){ g->pastas_rotulo[k], ID_PASTA + k, ITEM_BOTAO, false };
        for (int i = 0; i < g->n_musicas; i++)   /* as músicas desta pasta (já em ordem de título) */
            if (strcmp(g->musicas[i].grupo, g->pasta_atual) == 0)
                g->itens_musicas[n++] = (ItemMenu){ g->musicas[i].titulo, ID_MUSICA + i, ITEM_BOTAO, false };
    } else if (modo == ORGANIZAR_TITULO) {
        for (int i = 0; i < g->n_musicas; i++)
            g->itens_musicas[n++] = (ItemMenu){ g->musicas[i].titulo, ID_MUSICA + i, ITEM_BOTAO, false };
    } else {
        if (!cresce((void **)&g->ordem, &g->cap_ordem, g->n_musicas, sizeof(g->ordem[0]))) return;
        for (int i = 0; i < g->n_musicas; i++) g->ordem[i] = &g->musicas[i];
        if (g->n_musicas > 0) qsort(g->ordem, (size_t)g->n_musicas, sizeof(g->ordem[0]), compara_artista);
        for (int i = 0; i < g->n_musicas; i++)
            g->itens_musicas[n++] = (ItemMenu){ g->ordem[i]->rotulo_artista, ID_MUSICA + (int)(g->ordem[i] - g->musicas),
                                                ITEM_BOTAO, false };
    }

    if (na_raiz) g->itens_musicas[n++] = (ItemMenu){ "Procurar musicas novas", ID_PROCURAR, ITEM_BOTAO, false };
    g->itens_musicas[n++] = (ItemMenu){ na_raiz ? "Voltar" : "Voltar (pasta de cima)", ID_VOLTAR, ITEM_BOTAO, false };
    g->menu_musicas = (Menu){ "Musicas", g->itens_musicas, n };
    if (g->sel[TELA_MUSICAS] >= n) g->sel[TELA_MUSICAS] = n - 1;
}

void Game_songs_done(Game *g)
{
    /* a pasta atual pode ter sumido na procura nova: volta para a raiz */
    bool existe = g->pasta_atual[0] == '\0';
    const size_t n = strlen(g->pasta_atual);
    for (int i = 0; i < g->n_musicas && !existe; i++) {
        const char *gr = g->musicas[i].grupo;
        existe = strncmp(gr, g->pasta_atual, n) == 0 && (gr[n] == '\0' || gr[n] == '/');
    }
    if (!existe) g->pasta_atual[0] = '\0';
    Biblioteca_monta_menu(g);
}

/* entra na subpasta k */
void Biblioteca_entra(Game *g, int k)
{
    if (k < 0 || k >= g->n_pastas) return;
    char nova[sizeof(g->pasta_atual) + sizeof(g->pastas_nome[0]) + 1];
    snprintf(nova, sizeof(nova), "%s%s%s", g->pasta_atual, g->pasta_atual[0] ? "/" : "", g->pastas_nome[k]);
    if (strlen(nova) >= sizeof(g->pasta_atual)) return;   /* fundo demais: cortar levaria à pasta errada */
    strcpy(g->pasta_atual, nova);
    g->sel[TELA_MUSICAS] = 1;   /* o primeiro item depois de "Organizar por" */
    Biblioteca_monta_menu(g);
}

/* sobe uma pasta, deixando selecionada a que acabou de sair. false = já na raiz */
bool Biblioteca_sobe(Game *g)
{
    if (g->opcoes.organizar != ORGANIZAR_PASTAS || g->pasta_atual[0] == '\0') return false;
    char saiu[128];
    char *barra = strrchr(g->pasta_atual, '/');
    snprintf(saiu, sizeof(saiu), "%s", barra ? barra + 1 : g->pasta_atual);
    if (barra) *barra = '\0';
    else       g->pasta_atual[0] = '\0';
    Biblioteca_monta_menu(g);
    for (int i = 0; i < g->menu_musicas.n; i++) {
        const int k = Game_item_pasta(g->menu_musicas.itens[i].id);
        if (k >= 0 && strcmp(g->pastas_nome[k], saiu) == 0) g->sel[TELA_MUSICAS] = i;
    }
    return true;
}

void Biblioteca_free(Game *g)
{
    free(g->musicas);
    free(g->itens_musicas);
    free(g->pastas_nome);
    free(g->pastas_rotulo);
    free(g->ordem);
    g->musicas = NULL; g->itens_musicas = NULL; g->pastas_nome = NULL; g->pastas_rotulo = NULL; g->ordem = NULL;
    g->n_musicas = g->cap_musicas = g->cap_itens = g->n_pastas = g->cap_pastas = g->cap_ordem = 0;
}
