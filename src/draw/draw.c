/* O roteiro de cada frame: fundo, tela, sobreposições, HUD. */

#include "draw/draw_internal.h"

#define LARG 640.0f
#define ALT  480.0f

static const char *const ARQUIVO_IMAGEM[IMAGEM_N] = {
    [IMAGEM_INICIO] = "vaporwave_guitar1.png",
    [IMAGEM_JOGO]   = "vaporwave_guitar2.png",
    [IMAGEM_CONFIG] = "vaporwave_guitar3.png",
};

static const int IMAGEM_DA_TELA[TELA_N] = {
    [TELA_INICIO]      = IMAGEM_INICIO,
    [TELA_MUSICAS]     = IMAGEM_INICIO,
    [TELA_JOGADORES]   = IMAGEM_INICIO,
    [TELA_PREPARAR]    = IMAGEM_INICIO,
    [TELA_CARREGANDO]  = IMAGEM_JOGO,
    [TELA_CONFIG]    = IMAGEM_CONFIG,
    [TELA_CONTROLES] = IMAGEM_CONFIG,
    [TELA_AUDIO]     = IMAGEM_CONFIG,
    [TELA_JOGO]      = IMAGEM_JOGO,
};

/* ---------------------------------------------------------------------------
 * Ciclo de vida: as texturas são do draw. Criadas aqui, uma vez, e
 * destruídas no Draw_quit, que roda antes do Video_quit (o renderer).
 * ------------------------------------------------------------------------- */
static SDL_Texture *carrega_imagem(SDL_Renderer *ren, const char *pasta, const char *nome)
{
    char caminho[512];
    SDL_snprintf(caminho, sizeof(caminho), "%s%s", pasta, nome);

    SDL_Surface *s = SDL_LoadPNG(caminho);    /* arquivo -> pixels na RAM */
    if (!s) {
        SDL_Log("[draw] nao carreguei %s: %s", caminho, SDL_GetError());
        return NULL;
    }
    SDL_Texture *t = SDL_CreateTextureFromSurface(ren, s);   /* RAM -> GPU */
    SDL_DestroySurface(s);
    if (!t) {
        SDL_Log("[draw] textura de %s falhou: %s", caminho, SDL_GetError());
        return NULL;
    }
    SDL_SetTextureBlendMode(t, SDL_BLENDMODE_NONE);   /* fundo opaco: o mais barato */
    return t;
}

bool Draw_init(Draw *d, SDL_Renderer *ren, const char *pasta_assets)
{
    /* sem isto, o alfa das cores de SDL_RenderGeometry/FillRect é ignorado */
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    /* imagem que falta não impede o jogo: o fundo vira um degradê */
    char pasta[512];
    SDL_snprintf(pasta, sizeof(pasta), "%simages/", pasta_assets);
    for (int i = 0; i < IMAGEM_N; i++)
        d->imagem[i] = carrega_imagem(ren, pasta, ARQUIVO_IMAGEM[i]);

    /* a fonte também: sem ela, o texto sai na de debug da SDL */
    SDL_snprintf(pasta, sizeof(pasta), "%sfonts/terminal16unicode.ttf", pasta_assets);
    fonte_init(ren, pasta);
    return true;
}

void Draw_quit(Draw *d)
{
    fonte_quit();
    for (int i = 0; i < IMAGEM_N; i++) {
        if (d->imagem[i]) SDL_DestroyTexture(d->imagem[i]);
        d->imagem[i] = NULL;
    }
}

/* ---------------------------------------------------------------------------
 * Fundo
 * ------------------------------------------------------------------------- */
static void fundo(const Draw *d, SDL_Renderer *ren, const Game *g)
{
    SDL_Texture *t = d->imagem[IMAGEM_DA_TELA[g->tela]];
    if (!t) {
        ret_degrade(ren, (SDL_FRect){ 0, 0, LARG, ALT }, (SDL_Color){ 40, 10, 70, 255 },
                    (SDL_Color){ 10, 5, 20, 255 });
        return;
    }

    /* recorta o centro da imagem em 4:3, para não distorcer se ela for de outra proporção */
    float w, h;
    SDL_GetTextureSize(t, &w, &h);
    SDL_FRect src = { 0, 0, w, h };
    if (w / h > LARG / ALT) { src.w = h * LARG / ALT; src.x = (w - src.w) / 2.0f; }
    else                    { src.h = w * ALT / LARG; src.y = (h - src.h) / 2.0f; }

    /* escurece o fundo na pista, para as notas se destacarem */
    const Uint8 brilho = g->tela == TELA_JOGO ? 105 : 190;
    SDL_SetTextureColorMod(t, brilho, brilho, brilho);
    SDL_RenderTexture(ren, t, &src, NULL);
}

/* ---------------------------------------------------------------------------
 * Menus
 *
 * Todo texto é a fonte do jogo (fonte.c): 12x16 por caractere na escala 1,
 * 24x32 na 2 (títulos), 36x48 na 3 (o nome do jogo).
 * ------------------------------------------------------------------------- */

/* pulsa entre 0 e 1, para a seleção "respirar" */
static float pulso(const Game *g)
{
    return 0.5f + 0.5f * SDL_sinf((float)g->t_total * 5.0f);
}

/* uma linha de texto que, se passar da largura, é cortada com ".." */
static void texto_cortado(SDL_Renderer *ren, float x, float y, float largura, SDL_Color cor, const char *s)
{
    char c[160];
    texto_corta(c, sizeof(c), s, largura, 1);
    texto(ren, x, y, 1, cor, c);
}

static void texto_centro_cortado(SDL_Renderer *ren, float cx, float y, float largura, SDL_Color cor, const char *s)
{
    char c[160];
    texto_corta(c, sizeof(c), s, largura, 1);
    texto_centro(ren, cx, y, 1, cor, c);
}

static void escurece(SDL_Renderer *ren, float alfa)
{
    ret(ren, (SDL_FRect){ 0, 0, LARG, ALT }, cor_alfa((SDL_Color){ 0, 0, 0, 255 }, alfa));
}

static void titulo(SDL_Renderer *ren, const Game *g, const Menu *m)
{
    if (g->tela == TELA_INICIO) {
        const float x = LARG / 2 - texto_largura(m->titulo, 3) / 2;
        texto(ren, x + 3, 43, 3, COR_ACENTO, m->titulo);   /* a "sombra" rosa, do estilo vaporwave */
        texto(ren, x, 40, 3, COR_TEXTO, m->titulo);
        texto_centro(ren, LARG / 2, 100, 1, COR_CIANO, T("um jogo de ritmo para o R36S"));
    } else {
        float y = 34;
        if (g->tela == TELA_CONTROLES)  y = 24;
        else if (g->tela == TELA_JOGO)  y = g->terminou ? 34 : 116;
        const char *t = T(m->titulo);
        texto_sombra(ren, LARG / 2 - texto_largura(t, 2) / 2, y, 2, COR_TEXTO, t);
    }
}

/* Uma linha de menu. valor/valor2: texto à direita (opção, botão mapeado). */
static void linha(SDL_Renderer *ren, const Game *g, SDL_FRect r, const ItemMenu *item, bool sel,
                  const char *valor, float x_valor, const char *valor2, float x_valor2)
{
    const float y_texto = r.y + (r.h - TEXTO_H) / 2.0f;

    /* no modo de escolha (ITEM_VALOR), a linha fica ciano e o valor ganha ▲▼ */
    const bool escolhendo = sel && g->escolhendo && item->tipo == ITEM_VALOR;
    if (sel) {
        ret_arredondado(ren, r, 8, cor_alfa(escolhendo ? COR_CIANO : COR_ACENTO, 0.80f + 0.20f * pulso(g)));
        contorno_arredondado(ren, r, 8, cor_alfa(COR_TEXTO, 0.5f + 0.5f * pulso(g)));
    }

    SDL_Color c = item->em_breve ? COR_TEXTO_FRACO : (sel ? COR_TEXTO_ESCURO : COR_TEXTO);
    if (!sel && Game_item_pasta(item->id) >= 0) c = COR_CIANO;   /* pastas se destacam das músicas */

    /* músicas e pastas são nomes: só os outros itens se traduzem. O valor da
       direita é desenhado primeiro, para o rótulo saber quanto espaço sobra. */
    float fim_rotulo = r.x + r.w - 14;
    if (valor && *valor && !item->em_breve) {
        char buf[64];
        if (escolhendo) SDL_snprintf(buf, sizeof(buf), "▲ %s ▼", valor);
        else            SDL_snprintf(buf, sizeof(buf), "%s", valor);
        const float x = x_valor > 0 ? x_valor : r.x + r.w - texto_largura(buf, 1) - 14;
        texto(ren, x, y_texto, 1, c, buf);
        fim_rotulo = x - 12;
    }
    if (valor2 && *valor2) texto(ren, x_valor2, y_texto, 1, c, valor2);
    if (item->em_breve) {
        const char *tag = T("em breve");
        texto(ren, r.x + r.w - texto_largura(tag, 1) - 14, y_texto, 1, sel ? COR_TEXTO_ESCURO : COR_TEXTO_FRACO, tag);
        fim_rotulo = r.x + r.w - texto_largura(tag, 1) - 26;
    }

    const bool nome = Game_item_musica(item->id) >= 0 || Game_item_pasta(item->id) >= 0;
    char rotulo[160];   /* comprido (nome de música): cortado com ".." para não invadir o valor */
    texto_corta(rotulo, sizeof(rotulo), nome ? item->rotulo : T(item->rotulo), fim_rotulo - (r.x + 18), 1);
    texto(ren, r.x + 18, y_texto, 1, c, rotulo);
}

static void menu_lista(SDL_Renderer *ren, const Game *g, const Menu *m, int sel)
{
    /* listas compridas (as músicas) mostram uma janela que acompanha a seleção */
    const int max_vis = g->tela == TELA_MUSICAS ? 7 : 10;
    const int vis = m->n < max_vis ? m->n : max_vis;
    int primeiro = sel - vis / 2;
    if (primeiro > m->n - vis) primeiro = m->n - vis;
    if (primeiro < 0) primeiro = 0;

    /* menus compridos ficam mais justos para caber */
    const float lin_h   = vis > 8 ? 26 : (vis > 6 ? 29 : 36);
    const float lin_esp = vis > 8 ? 3 : (vis > 6 ? 4 : 6);
    const float painel_w = g->tela == TELA_MUSICAS ? 560 : 480;
    const float painel_h = vis * (lin_h + lin_esp) + 24;
    float y0 = 110;
    if (g->tela == TELA_INICIO)       y0 = 140;
    else if (g->tela == TELA_MUSICAS) y0 = 94;
    else if (g->tela == TELA_JOGO)    y0 = g->terminou ? (Game_n_jogando(g) > 1 ? 308 : 262) : 170;
    const SDL_FRect painel = { (LARG - painel_w) / 2, y0, painel_w, painel_h };

    ret_arredondado(ren, painel, 14, COR_PAINEL);
    contorno_arredondado(ren, painel, 14, cor_alfa(COR_ACENTO, 0.6f));

    for (int k = 0; k < vis; k++) {
        const int i = primeiro + k;
        const SDL_FRect r = { painel.x + 12, painel.y + 12 + k * (lin_h + lin_esp), painel.w - 24, lin_h };
        char valor[32];
        Menu_valor(g, m->itens[i].id, valor, sizeof(valor));
        linha(ren, g, r, &m->itens[i], i == sel, valor, 0, NULL, 0);
    }
    /* setas: tem mais lista para cima / para baixo */
    if (primeiro > 0)
        texto_centro(ren, LARG / 2, painel.y - 6, 1, COR_TEXTO_FRACO, "▲");
    if (primeiro + vis < m->n)
        texto_centro(ren, LARG / 2, painel.y + painel.h - 10, 1, COR_TEXTO_FRACO, "▼");
}

static void hora(char *buf, size_t n, double t);

/* embaixo da lista de músicas: artista, charter e duração da selecionada */
static void info_musica(SDL_Renderer *ren, const Game *g)
{
    const int sel = Game_menu_sel(g);
    const int id  = g->menu_musicas.itens[sel].id;
    const SDL_FRect p = { 40, 400, LARG - 80, 48 };
    const float largura = p.w - 28;
    ret_arredondado(ren, p, 10, COR_PAINEL);

    char s[160], c[160], dur[16];
    if (id == ID_DEMO) {
        SDL_strlcpy(s, T("Notas de exemplo com metrônomo, para testar"), sizeof(s));
    } else if (Game_item_musica(id) >= 0) {
        const InfoMusica *mu = &g->musicas[Game_item_musica(id)];
        hora(dur, sizeof(dur), mu->duracao_ms / 1000.0);
        SDL_snprintf(s, sizeof(s), "%s  -  %s", mu->artista[0] ? mu->artista : "?", mu->duracao_ms ? dur : "?");
        char linha2[160];
        if (mu->recorde > 0 && mu->recorde_nome[0]) {
            SDL_snprintf(linha2, sizeof(linha2), T("recorde: %ld de %s (%s %s, %d%%%s)"), mu->recorde, mu->recorde_nome,
                         T(Instrumento_nome((Instrumento)mu->recorde_instr)), T(Dificuldade_nome((Dificuldade)mu->recorde_dif)),
                         mu->recorde_pct, mu->recorde_fc ? ", FC" : "");
            texto_corta(c, sizeof(c), linha2, largura, 1);
            texto(ren, p.x + 14, p.y + 26, 1, (SDL_Color){ 255, 220, 90, 255 }, c);
        } else if (mu->recorde > 0) {
            SDL_snprintf(linha2, sizeof(linha2), T("recorde: %ld (%s %s, %d%%%s)"), mu->recorde,
                         T(Instrumento_nome((Instrumento)mu->recorde_instr)), T(Dificuldade_nome((Dificuldade)mu->recorde_dif)),
                         mu->recorde_pct, mu->recorde_fc ? ", FC" : "");
            texto_corta(c, sizeof(c), linha2, largura, 1);
            texto(ren, p.x + 14, p.y + 26, 1, (SDL_Color){ 255, 220, 90, 255 }, c);
        } else if (mu->charter[0]) {
            SDL_snprintf(linha2, sizeof(linha2), T("chart: %s"), mu->charter);
            texto_corta(c, sizeof(c), linha2, largura, 1);
            texto(ren, p.x + 14, p.y + 26, 1, COR_TEXTO_FRACO, c);
        }
    } else if (Game_item_pasta(id) >= 0) {
        SDL_strlcpy(s, T("Pasta: entre para ver as músicas dela"), sizeof(s));
    } else if (id == ID_ORGANIZAR) {
        SDL_strlcpy(s, T("←/→: por pasta, título (A-Z) ou artista"), sizeof(s));
    } else {
        s[0] = '\0';
    }
    if (id == ID_PROCURAR)
        SDL_strlcpy(s, T("Procura de novo em songs/ e no pastas.txt"), sizeof(s));
    texto_corta(c, sizeof(c), s, largura, 1);
    texto(ren, p.x + 14, p.y + 6, 1, COR_TEXTO, c);
}

/* em cima da lista: em que posição estamos e se a procura está andando */
static void estado_lista(SDL_Renderer *ren, const Game *g, const Hud *h)
{
    char s[96], c[96];
    SDL_snprintf(s, sizeof(s), "%d/%d", Game_menu_sel(g) + 1, g->menu_musicas.n);
    const float x_contador = LARG - 40 - texto_largura(s, 1);
    texto(ren, x_contador, 72, 1, COR_TEXTO_FRACO, s);

    SDL_Color cor = COR_TEXTO_FRACO;
    if (h->procurando) {
        SDL_snprintf(s, sizeof(s), T("procurando músicas... %d encontrada(s)"), h->achadas);
        cor = cor_alfa(COR_CIANO, 0.6f + 0.4f * pulso(g));
    } else if (g->n_musicas == 0) {
        SDL_strlcpy(s, T("nenhuma música em songs/"), sizeof(s));
    } else if (g->opcoes.organizar == ORGANIZAR_PASTAS && g->pasta_atual[0]) {
        SDL_snprintf(s, sizeof(s), T("pasta: %s"), g->pasta_atual);
        cor = COR_CIANO;
    } else {
        SDL_snprintf(s, sizeof(s), T("%d música(s)"), g->n_musicas);
    }
    texto_corta(c, sizeof(c), s, x_contador - 52, 1);
    texto(ren, 40, 72, 1, cor, c);
}

/* o título da música escolhida (grande, se couber) e o artista embaixo */
static void subtitulo_musica(SDL_Renderer *ren, const Game *g, float y)
{
    if (g->musica < 0 || g->musica >= g->n_musicas) return;
    const InfoMusica *mu = &g->musicas[g->musica];
    const float e = texto_largura(mu->titulo, 2) <= LARG - 32 ? 2 : 1;
    char c[96];
    texto_corta(c, sizeof(c), mu->titulo, LARG - 32, e);
    texto_centro(ren, LARG / 2, y, e, COR_CIANO, c);
    if (mu->artista[0]) {
        texto_corta(c, sizeof(c), mu->artista, LARG - 32, 1);
        texto_centro(ren, LARG / 2, y + TEXTO_H * e + 2, 1, COR_TEXTO_FRACO, c);
    }
}

/* carregando: o título e a barra de progresso da decodificação */
static void tela_carregando(SDL_Renderer *ren, const Game *g, const Hud *h)
{
    const char *carregando = T("Carregando");
    texto_sombra(ren, LARG / 2 - texto_largura(carregando, 2) / 2, 110, 2, COR_TEXTO, carregando);
    subtitulo_musica(ren, g, 170);

    const SDL_FRect barra = { 120, 260, LARG - 240, 14 };
    ret_arredondado(ren, barra, 7, (SDL_Color){ 40, 25, 60, 230 });
    ret_arredondado(ren, (SDL_FRect){ barra.x, barra.y, barra.w * h->audio.progresso, barra.h }, 7, COR_CIANO);

    char s[96];
    SDL_snprintf(s, sizeof(s), T("decodificando o áudio: %.0f%%"), 100.0f * h->audio.progresso);
    texto_centro(ren, LARG / 2, 282, 1, COR_TEXTO_FRACO, s);

    float y = 320;
    for (int k = 0; k < GAME_MAX_JOGADORES; k++) {
        const Jogador *j = &g->jog[k];
        if (!j->ativo) continue;
        SDL_snprintf(s, sizeof(s), "J%d  %s  %s", k + 1, T(Instrumento_nome(j->instrumento)), T(Dificuldade_nome(j->dificuldade)));
        texto_centro(ren, LARG / 2, y, 1, COR_JOGADOR[k], s);
        y += 18;
    }
}

/* ---------------------------------------------------------------------------
 * Preparação: um cartão por jogador, numa grade 2x2, com o instrumento e a
 * dificuldade de cada um.
 * ------------------------------------------------------------------------- */
/* "J2 João", ou "JOGADOR 2" para o convidado */
static void cabecalho(char *s, size_t n, const Game *g, int k)
{
    const char *nome = Game_player_name(g, k);
    if (nome) SDL_snprintf(s, n, "J%d %s", k + 1, nome);
    else      SDL_snprintf(s, n, T("JOGADOR %d"), k + 1);
}

/* o nome curto, para o placar: o da conta, ou "J2" */
static void nome_curto(char *s, size_t n, const Game *g, int k)
{
    const char *nome = Game_player_name(g, k);
    if (nome) SDL_snprintf(s, n, "%s", nome);
    else      SDL_snprintf(s, n, "J%d", k + 1);
}

static SDL_FRect cartao(int k)
{
    return (SDL_FRect){ 16 + (float)(k % 2) * 308, 98 + (float)(k / 2) * 176, 300, 166 };
}

/* Uma linha do cartão: rótulo à esquerda e valor à direita, numa caixa. A
 * linha escolhida fica da cor do jogador; no modo de escolha, ciano, com ▲▼
 * no valor. Sem valor (um botão), o rótulo vai no meio. */
static void linha_cartao(SDL_Renderer *ren, const Game *g, SDL_FRect r, float y, SDL_Color cj, const char *rotulo,
                         const char *valor, bool sel, bool escolhendo, bool apagada)
{
    const SDL_FRect caixa = { r.x + 8, y, r.w - 16, 22 };
    if (escolhendo) ret_arredondado(ren, caixa, 8, cor_alfa(COR_CIANO, 0.80f + 0.20f * pulso(g)));
    else if (sel)   ret_arredondado(ren, caixa, 8, cor_alfa(cj, 0.75f + 0.25f * pulso(g)));
    else            contorno_arredondado(ren, caixa, 8, cor_alfa(COR_TEXTO_FRACO, apagada ? 0.2f : 0.45f));
    const bool escuro = sel || escolhendo;
    const SDL_Color c  = escuro ? COR_TEXTO_ESCURO : (apagada ? COR_TEXTO_FRACO : COR_TEXTO);
    const SDL_Color cr = escuro ? COR_TEXTO_ESCURO : COR_TEXTO_FRACO;
    if (!valor) {
        texto_centro_cortado(ren, caixa.x + caixa.w / 2, caixa.y + 3, caixa.w - 16, c, rotulo);
        return;
    }
    if (escolhendo) {   /* escolhendo, o valor ocupa a caixa toda, entre as setas */
        texto(ren, caixa.x + 8, caixa.y + 3, 1, c, "▲");
        texto(ren, caixa.x + caixa.w - 8 - TEXTO_W, caixa.y + 3, 1, c, "▼");
        texto_centro_cortado(ren, caixa.x + caixa.w / 2, caixa.y + 3, caixa.w - 16 - 4 * TEXTO_W, c, valor);
        return;
    }
    char cv[64];
    texto_corta(cv, sizeof(cv), valor, caixa.w - 16 - 6 * TEXTO_W, 1);   /* o rótulo fica com pelo menos 5 letras */
    const float wv = texto_largura(cv, 1);
    texto(ren, caixa.x + caixa.w - 8 - wv, caixa.y + 3, 1, c, cv);
    texto_cortado(ren, caixa.x + 8, caixa.y + 3, caixa.w - 28 - wv, cr, rotulo);
}

/* a etiqueta "teclado" na borda de cima: o cartão que o teclado está atendendo */
static void etiqueta_teclado(SDL_Renderer *ren, SDL_FRect r, SDL_Color cj)
{
    const char *tag = T("teclado");
    const float cx = r.x + r.w / 2;
    const SDL_FRect t = { cx - texto_largura(tag, 1) / 2 - 8, r.y - 10, texto_largura(tag, 1) + 16, 18 };
    ret_arredondado(ren, t, 6, cj);
    texto_centro(ren, cx, t.y + 1, 1, COR_TEXTO_ESCURO, tag);
}

/* o cabeçalho do cartão: quem é (à esquerda) e com que aparelho (à direita) */
static void topo_cartao(SDL_Renderer *ren, const Game *g, const Hud *h, SDL_FRect r, int k, const char *quem, bool dentro)
{
    texto(ren, r.x + 12, r.y + 8, 1, dentro ? COR_JOGADOR[k] : COR_TEXTO_FRACO, quem);
    const char *aparelho = g->jog[k].no_teclado && dentro ? T("teclado") : (h->controle[k] ? h->controle[k] : T("sem controle"));
    const float x = r.x + 12 + texto_largura(quem, 1) + 12;
    char c[96];
    texto_corta(c, sizeof(c), aparelho, r.x + r.w - 12 - x, 1);
    texto(ren, r.x + r.w - 12 - texto_largura(c, 1), r.y + 8, 1, COR_TEXTO_FRACO, c);
}

/* um cartão vazio: o "+" e como entrar */
static void cartao_vazio(SDL_Renderer *ren, const Game *g, const Hud *h, SDL_FRect r, int k, bool teclado)
{
    const float cx = r.x + r.w / 2;
    char s[96];
    texto_centro(ren, cx, r.y + 40, 3, cor_alfa(COR_JOGADOR[k], 0.35f + 0.25f * pulso(g)), "+");
    if (h->controle[k]) {
        SDL_snprintf(s, sizeof(s), T("[%s] entra"), T(h->entra_pad[k]));
        texto_centro_cortado(ren, cx, r.y + 100, r.w - 24, COR_TEXTO_FRACO, s);
    } else if (teclado) {
        SDL_snprintf(s, sizeof(s), T("[%s] joga no teclado"), h->tecla[ACAO_CONFIRMA]);
        texto_centro_cortado(ren, cx, r.y + 100, r.w - 24, COR_TEXTO, s);
    } else {
        texto_centro_cortado(ren, cx, r.y + 100, r.w - 24, COR_TEXTO_FRACO, T("conecte um controle"));
    }
}

static void cartao_jogador(SDL_Renderer *ren, const Game *g, const Hud *h, int k)
{
    const Jogador *j = &g->jog[k];
    const SDL_FRect r = cartao(k);
    const float cx = r.x + r.w / 2;
    const SDL_Color cj = COR_JOGADOR[k];
    char s[96], c[96];

    /* quem atende este jogador: o próprio controle, ou o teclado (um por vez) */
    const bool no_teclado = j->no_teclado;
    const bool na_vez = no_teclado && Game_vez_teclado(g) == k;

    ret_arredondado(ren, r, 12, j->ativo ? COR_PAINEL : cor_alfa(COR_PAINEL, 0.5f));
    contorno_arredondado(ren, r, 12, cor_alfa(cj, j->pronto || na_vez ? 0.6f + 0.4f * pulso(g) : (j->ativo ? 0.8f : 0.3f)));
    if (na_vez) etiqueta_teclado(ren, r, cj);
    cabecalho(s, sizeof(s), g, k);
    topo_cartao(ren, g, h, r, k, s, j->ativo);

    if (!j->ativo) {
        texto_centro(ren, cx, r.y + 40, 3, cor_alfa(cj, 0.35f + 0.25f * pulso(g)), "+");
        if (j->na_sessao && Game_present(g, k)) {   /* ficou fora desta música: pode voltar */
            SDL_snprintf(s, sizeof(s), T("[%s] entra"), no_teclado ? h->tecla[ACAO_CONFIRMA] : T(h->entra_pad[k]));
            texto_centro_cortado(ren, cx, r.y + 100, r.w - 24, COR_TEXTO_FRACO, s);
            texto_centro_cortado(ren, cx, r.y + 118, r.w - 24, COR_TEXTO_FRACO, T("fora desta música"));
        }
        return;
    }

    /* as linhas: instrumento, dificuldade e pronto */
    const bool mexe = !j->pronto && (!no_teclado || na_vez);   /* um de teclado só mexe na sua vez */
    const bool apagada = !mexe;
    linha_cartao(ren, g, r, r.y + 32, cj, T("instrumento"), T(Instrumento_nome(j->instrumento)),
                 mexe && j->linha_prep == PREP_INSTRUMENTO && !j->escolhendo,
                 mexe && j->linha_prep == PREP_INSTRUMENTO && j->escolhendo, apagada);
    linha_cartao(ren, g, r, r.y + 58, cj, T("dificuldade"), T(Dificuldade_nome(j->dificuldade)),
                 mexe && j->linha_prep == PREP_DIFICULDADE && !j->escolhendo,
                 mexe && j->linha_prep == PREP_DIFICULDADE && j->escolhendo, apagada);
    if (j->pronto)
        texto_centro(ren, cx, r.y + 86, 2, cor_alfa(COR_CIANO, 0.6f + 0.4f * pulso(g)), T("PRONTO!"));
    else
        linha_cartao(ren, g, r, r.y + 84, cj, T("Pronto"), NULL, mexe && j->linha_prep == PREP_PRONTO, false, apagada);

    /* embaixo: quantas notas e o modo; e a dica de quem espera */
    const int notas = g->notas[j->instrumento][j->dificuldade];
    if (notas == 1) SDL_strlcpy(c, T("1 nota"), sizeof(c));
    else            SDL_snprintf(c, sizeof(c), T("%d notas"), notas);
    SDL_snprintf(s, sizeof(s), "%s · %s", c, T(g->guitarra[k] ? "modo guitarra" : "modo controle"));
    texto_centro_cortado(ren, cx, r.y + 122, r.w - 24, COR_TEXTO_FRACO, s);
    if (no_teclado && !na_vez && !j->pronto) texto_centro_cortado(ren, cx, r.y + 142, r.w - 24, COR_TEXTO_FRACO, T("espera o teclado"));
}

/* ---------------------------------------------------------------------------
 * Tela de jogadores: quem joga, com que conta e em que modo. Mesmos cartões
 * da preparação.
 * ------------------------------------------------------------------------- */
static void cartao_sessao(SDL_Renderer *ren, const Game *g, const Hud *h, int k)
{
    const Jogador *j = &g->jog[k];
    const SDL_FRect r = cartao(k);
    const float cx = r.x + r.w / 2;
    const SDL_Color cj = COR_JOGADOR[k];
    char s[96];

    /* o cartão que o teclado atende (as dicas do teclado só depois que alguém usou um) */
    const bool cursor = g->teclado_usado && g->cursor_teclado == k &&
                        (j->na_sessao ? j->no_teclado : Game_teclado_livre(g, k));

    ret_arredondado(ren, r, 12, j->na_sessao ? COR_PAINEL : cor_alfa(COR_PAINEL, 0.5f));
    contorno_arredondado(ren, r, 12, cor_alfa(cj, j->pronto || cursor ? 0.6f + 0.4f * pulso(g) : (j->na_sessao ? 0.8f : 0.3f)));
    if (cursor) etiqueta_teclado(ren, r, cj);
    SDL_snprintf(s, sizeof(s), T("JOGADOR %d"), k + 1);
    topo_cartao(ren, g, h, r, k, s, j->na_sessao);

    if (!j->na_sessao) {
        cartao_vazio(ren, g, h, r, k, cursor);
        return;
    }

    /* as linhas: conta, modo, controles e pronto */
    const char *nome  = Game_player_name(g, k);
    const char *conta = nome ? nome : (j->conta == g->n_contas + 1 ? T("+ novo jogador") : T("Convidado"));
    const bool mexe   = !j->pronto && (!j->no_teclado || g->cursor_teclado == k);
    const int  l      = j->linha_sessao;
    linha_cartao(ren, g, r, r.y + 32, cj, T("conta"), conta, mexe && l == SESSAO_CONTA && !j->escolhendo,
                 mexe && l == SESSAO_CONTA && j->escolhendo, !mexe);
    linha_cartao(ren, g, r, r.y + 58, cj, T("modo"), T(g->guitarra[k] ? "Guitarra" : "Controle"),
                 mexe && l == SESSAO_MODO, false, !mexe);

    /* as teclas de quem joga no teclado: sem elas (ou repetidas), não dá para ficar pronto */
    const char *aviso = NULL;
    if (j->no_teclado && g->aparelhos && g->teclas[k] == TECLAS_FALTAM)    aviso = T("sem teclas");
    if (j->no_teclado && g->aparelhos && g->teclas[k] == TECLAS_REPETIDAS) aviso = T("teclas repetidas");

    if (j->pronto) {
        texto_centro(ren, cx, r.y + 92, 2, cor_alfa(COR_CIANO, 0.6f + 0.4f * pulso(g)), T("PRONTO!"));
        return;
    }
    linha_cartao(ren, g, r, r.y + 84, cj, T("Controles"), aviso ? "!" : "", mexe && l == SESSAO_CONTROLES, false, !mexe);
    linha_cartao(ren, g, r, r.y + 110, cj, j->conta == g->n_contas + 1 ? T("Escrever o nome") : T("Pronto"), NULL,
                 mexe && l == SESSAO_PRONTO, false, !mexe);
    if (aviso)   /* o "!" da linha de Controles: o que falta */
        texto_centro_cortado(ren, cx, r.y + 140, r.w - 24, COR_ACENTO, aviso);
    else if (j->no_teclado && !mexe)
        texto_centro_cortado(ren, cx, r.y + 140, r.w - 24, COR_TEXTO_FRACO, T("espera o teclado"));
}

/* o teclado da tela, para escrever o nome de uma conta nova */
static void teclado_nome(SDL_Renderer *ren, const Game *g, const Hud *h)
{
    const int k = g->nome_jogador;
    escurece(ren, 0.6f);
    const SDL_FRect p = { 36, 70, LARG - 72, 340 };
    ret_arredondado(ren, p, 14, (SDL_Color){ 25, 12, 45, 245 });
    contorno_arredondado(ren, p, 14, COR_JOGADOR[k]);

    char s[96];
    SDL_snprintf(s, sizeof(s), T("Nome do jogador %d"), k + 1);
    texto_centro(ren, LARG / 2, p.y + 10, 1, COR_JOGADOR[k], s);

    /* o nome, com o cursor piscando */
    const SDL_FRect campo = { p.x + 40, p.y + 32, p.w - 80, 40 };
    ret_arredondado(ren, campo, 8, (SDL_Color){ 0, 0, 0, 160 });
    SDL_snprintf(s, sizeof(s), "%s%s", g->nome_novo, SDL_fmod(g->t_total, 1.0) < 0.5 ? "_" : " ");
    texto_centro(ren, LARG / 2, campo.y + 4, 2, COR_TEXTO, s);

    /* as teclas: 4 linhas de 13 e a linha das especiais */
    const float tw = 40, th = 30, x0 = LARG / 2 - GAME_TECLAS_LINHA * tw / 2, y0 = p.y + 88;
    for (int i = 0; i < GAME_TECLAS_N; i++) {
        SDL_FRect t;
        if (i < 4 * GAME_TECLAS_LINHA) t = (SDL_FRect){ x0 + (i % GAME_TECLAS_LINHA) * tw, y0 + (i / GAME_TECLAS_LINHA) * (th + 4), tw - 4, th };
        else                           t = (SDL_FRect){ x0 + (i - 4 * GAME_TECLAS_LINHA) * (GAME_TECLAS_LINHA * tw / 4), y0 + 4 * (th + 4) + 6,
                                                        GAME_TECLAS_LINHA * tw / 4 - 4, th };
        const bool sel = i == g->nome_cursor;
        if (sel) ret_arredondado(ren, t, 6, cor_alfa(COR_JOGADOR[k], 0.8f + 0.2f * pulso(g)));
        else     contorno_arredondado(ren, t, 6, cor_alfa(COR_TEXTO_FRACO, 0.5f));
        const char *rotulo = i < 4 * GAME_TECLAS_LINHA ? Game_key_text(g, i) : T(Game_key_text(g, i));
        texto_centro(ren, t.x + t.w / 2, t.y + (t.h - TEXTO_H) / 2, 1, sel ? COR_TEXTO_ESCURO : COR_TEXTO, rotulo);
    }

    const bool tecl = g->jog[k].no_teclado || !h->controle[k];
    SDL_snprintf(s, sizeof(s), g->guitarra[k] ? T("[↑/↓] tecla  [%s] escreve  [%s] apaga") : T("[%s] escreve  [%s] apaga"),
                 tecl ? h->tecla[ACAO_CONFIRMA] : T(h->entra_pad[k]), tecl ? h->tecla[ACAO_VOLTA] : T(h->pad[ACAO_VOLTA]));
    texto_centro_cortado(ren, LARG / 2, p.y + p.h - 26, p.w - 24, COR_TEXTO_FRACO, s);
}

static void tela_jogadores(SDL_Renderer *ren, const Game *g, const Hud *h)
{
    const char *t = T("Jogadores");
    texto_sombra(ren, LARG / 2 - texto_largura(t, 2) / 2, 34, 2, COR_TEXTO, t);
    for (int k = 0; k < GAME_MAX_JOGADORES; k++) cartao_sessao(ren, g, h, k);
    if (g->contagem > 0.0) {   /* todos prontos: a barra enche até as músicas */
        const float frac = (float)(g->contagem / GAME_CONTAGEM);
        ret(ren, (SDL_FRect){ 16, 444, (LARG - 32) * (frac > 1 ? 1 : frac), 4 }, COR_CIANO);
    }
    if (g->nome_jogador >= 0) teclado_nome(ren, g, h);
}

static void tela_preparar(SDL_Renderer *ren, const Game *g, const Hud *h)
{
    subtitulo_musica(ren, g, 30);
    for (int k = 0; k < GAME_MAX_JOGADORES; k++) cartao_jogador(ren, g, h, k);
}

/* A tela de controles: cada ação com o botão do controle e a tecla. A lista
 * é mais comprida que a tela: mostra uma janela que acompanha a seleção. */
#define CONTROLES_VISIVEIS 13

static void menu_controles(SDL_Renderer *ren, const Game *g, const Hud *h, const Menu *m, int sel)
{
    const float lin_h = 22, lin_esp = 2;
    const int vis = m->n < CONTROLES_VISIVEIS ? m->n : CONTROLES_VISIVEIS;
    int primeiro = sel - vis / 2;
    if (primeiro > m->n - vis) primeiro = m->n - vis;
    if (primeiro < 0) primeiro = 0;
    const SDL_FRect painel = { 30, 64, LARG - 60, vis * (lin_h + lin_esp) + 34 };
    const float x_pad = painel.x + 300, x_tecla = painel.x + 450;

    ret_arredondado(ren, painel, 14, COR_PAINEL);
    contorno_arredondado(ren, painel, 14, cor_alfa(COR_JOGADOR[h->jogador], 0.6f));
    texto(ren, x_pad, painel.y + 6, 1, COR_CIANO, T("CONTROLE"));
    texto(ren, x_tecla, painel.y + 6, 1, COR_CIANO, T("TECLADO"));

    /* o controle do jogador escolhido (o primeiro item troca de jogador) */
    char c[64];
    texto_corta(c, sizeof(c), h->controle[h->jogador] ? h->controle[h->jogador] : T("sem controle: só o teclado"),
                x_pad - painel.x - 24, 1);
    texto(ren, painel.x + 12, painel.y + 6, 1, COR_JOGADOR[h->jogador], c);

    char valor[32];
    for (int v = 0; v < vis; v++) {
        const int i = primeiro + v;
        const ItemMenu *item = &m->itens[i];
        const SDL_FRect r = { painel.x + 10, painel.y + 26 + v * (lin_h + lin_esp), painel.w - 20, lin_h };
        const char *pad = NULL, *tecla = NULL;
        const int acao = Game_item_acao(item->id);
        if (acao >= 0) {
            pad   = T(h->pad[acao]);
            tecla = h->tecla[acao];
        } else if (item->id == ID_ANALOGICO) {
            pad = T(h->analogico);   /* whammy (para os lados) e star power (para cima) */
        } else if (item->id == ID_FRONTAIS) {
            pad = h->frontais;       /* Xbox: a SDL nomeia pela posição; Nintendo: pela letra */
        } else if (item->id == ID_JOGADOR) {
            Menu_valor(g, item->id, valor, sizeof(valor));
            pad = valor;
        }
        linha(ren, g, r, item, i == sel, pad, x_pad, tecla, x_tecla);

        /* bolinha da cor do traste ao lado do nome */
        if (acao >= 0 && acao < ACAO_N_TRASTES)
            elipse(ren, r.x + 9, r.y + r.h / 2, 9, 9, COR_TRASTE[acao]);
    }

    if (primeiro > 0)            texto_centro(ren, LARG / 2, painel.y + 14, 1, COR_TEXTO_FRACO, "▲");
    if (primeiro + vis < m->n)   texto_centro(ren, LARG / 2, painel.y + painel.h - 12, 1, COR_TEXTO_FRACO, "▼");

    char ultimo[96];
    SDL_snprintf(ultimo, sizeof(ultimo), T("último botão apertado: %s"), h->ultimo ? h->ultimo : "-");
    texto_corta(c, sizeof(c), ultimo, painel.w - 24, 1);
    texto(ren, painel.x + 12, painel.y + painel.h + 6, 1, COR_TEXTO_FRACO, c);
}

/* relógio em m:ss.s */
static void hora(char *buf, size_t n, double t)
{
    if (t < 0) t = 0;
    SDL_snprintf(buf, n, "%d:%04.1f", (int)(t / 60.0), SDL_fmod(t, 60.0));
}



/* Tela de teste de áudio: o que foi achado, quanto a carga demorou e os
 * dois relógios (cru e suave) andando lado a lado. */
static void painel_audio(SDL_Renderer *ren, const HudAudio *a)
{
    const SDL_FRect p = { 40, 264, LARG - 80, 182 };
    ret_arredondado(ren, p, 12, COR_PAINEL);
    contorno_arredondado(ren, p, 12, cor_alfa(COR_CIANO, 0.6f));

    char s[160];
    float y = p.y + 10;
    const float x = p.x + 16, largura = p.w - 32;

    if (!a->ok) {
        texto_cortado(ren, x, y, largura, (SDL_Color){ 255, 90, 90, 255 }, T("Sem dispositivo de áudio: o jogo segue mudo."));
        return;
    }
    SDL_snprintf(s, sizeof(s), T("Saída: %s, buffer de %d frames (%.0f ms)"), a->driver, a->buffer_frames,
                 1000.0 * a->buffer_frames / 48000.0);
    texto_cortado(ren, x, y, largura, COR_TEXTO_FRACO, s);
    y += 24;

    if (!a->musica) {
        SDL_snprintf(s, sizeof(s), T("Nenhuma música em %s"), a->pasta ? a->pasta : "-");
        texto_cortado(ren, x, y, largura, COR_TEXTO, s);
        y += 18;
        texto_cortado(ren, x, y, largura, COR_TEXTO_FRACO, T("(notes.chart + song.ini + áudio)"));
        return;
    }
    texto_cortado(ren, x, y, largura, COR_TEXTO, a->musica);
    y += 18;
    texto_cortado(ren, x, y, largura, COR_TEXTO_FRACO, a->faixas);
    y += 18;
    if (a->ignoradas) {
        SDL_snprintf(s, sizeof(s), T("(%d faixa(s) com formato não suportado)"), a->ignoradas);
        texto_cortado(ren, x, y, largura, COR_TEXTO_FRACO, s);
    }
    y += 22;

    if (a->carga == 1) {   /* carregando: barra de progresso */
        const char *d = T("Decodificando...");
        texto(ren, x, y, 1, COR_TEXTO, d);
        const float bx = x + texto_largura(d, 1) + 12;
        const SDL_FRect barra = { bx, y + 3, p.x + p.w - 16 - bx, 10 };
        ret_arredondado(ren, barra, 5, (SDL_Color){ 60, 40, 80, 255 });
        ret_arredondado(ren, (SDL_FRect){ barra.x, barra.y, barra.w * a->progresso, barra.h }, 5, COR_CIANO);
    } else if (a->carga_info) {
        texto_cortado(ren, x, y, largura, a->carga == 3 ? (SDL_Color){ 255, 90, 90, 255 } : COR_TEXTO, a->carga_info);
    }
    y += 22;

    if (a->tocando) {
        char t[16], cru[16], dur[16];
        hora(t, sizeof(t), a->t_suave);
        hora(cru, sizeof(cru), a->t_cru);
        hora(dur, sizeof(dur), a->duracao);
        SDL_snprintf(s, sizeof(s), a->pausado ? T("Pausado %s / %s") : T("Tocando %s / %s"), t, dur);
        texto_cortado(ren, x, y, largura, COR_CIANO, s);
        y += 18;
        SDL_snprintf(s, sizeof(s), T("relógio cru %s  suave-cru: %+.1f ms"), cru, (a->t_suave - a->t_cru) * 1000.0);
        texto_cortado(ren, x, y, largura, COR_TEXTO_FRACO, s);
        y += 22;

        const float frac = a->duracao > 0 ? (float)(a->t_suave / a->duracao) : 0.0f;
        const SDL_FRect barra = { x, y, p.w - 32, 6 };
        ret(ren, barra, (SDL_Color){ 60, 40, 80, 255 });
        ret(ren, (SDL_FRect){ barra.x, barra.y, barra.w * (frac < 0 ? 0 : (frac > 1 ? 1 : frac)), barra.h }, COR_ACENTO);
    }
}

/* ---------------------------------------------------------------------------
 * Sobreposições
 * ------------------------------------------------------------------------- */

static void captura(SDL_Renderer *ren, const Hud *h)
{
    escurece(ren, 0.6f);
    const SDL_FRect p = { 40, 150, LARG - 80, 180 };
    ret_arredondado(ren, p, 14, (SDL_Color){ 25, 12, 45, 245 });
    contorno_arredondado(ren, p, 14, COR_CIANO);

    char s[96], c[96];
    SDL_snprintf(s, sizeof(s), T("Jogador %d: aperte o novo botão ou tecla para"), h->captura_jogador + 1);
    texto_corta(c, sizeof(c), s, p.w - 24, 1);
    texto_centro(ren, LARG / 2, p.y + 16, 1, COR_TEXTO, c);

    const SDL_Color cor = h->capturando < ACAO_N_TRASTES ? COR_TRASTE[h->capturando] : COR_ACENTO;
    texto_centro(ren, LARG / 2, p.y + 42, 2, cor, T(Acao_rotulo((Acao)h->capturando)));
    const char *l1 = h->capturando == ACAO_WHAMMY ? T("mexa o analógico que vai ser o whammy") : T("Esc cancela; sem resposta, cancela sozinho");
    const char *l2 = h->capturando == ACAO_WHAMMY ? T("(ou aperte um botão)") : NULL;
    texto_corta(c, sizeof(c), l1, p.w - 24, 1);
    texto_centro(ren, LARG / 2, p.y + 92, 1, COR_TEXTO_FRACO, c);
    if (l2) texto_centro(ren, LARG / 2, p.y + 110, 1, COR_TEXTO_FRACO, l2);

    /* barra do tempo restante */
    const float frac = (float)(h->captura_resta / 5.0);
    const SDL_FRect fundo_barra = { p.x + 30, p.y + 146, p.w - 60, 10 };
    ret_arredondado(ren, fundo_barra, 5, (SDL_Color){ 60, 40, 80, 255 });
    ret_arredondado(ren, (SDL_FRect){ fundo_barra.x, fundo_barra.y, fundo_barra.w * frac, fundo_barra.h },
                    5, COR_CIANO);
}

static void aviso(SDL_Renderer *ren, const Game *g)
{
    if (!g->aviso) return;
    const char *aviso = T(g->aviso);
    const float resta = (float)(g->aviso_ate - g->t_total);
    const float a = resta < 0.3f ? resta / 0.3f : 1.0f;   /* some nos últimos 300 ms */
    const float w = texto_largura(aviso, 1) + 40;
    const SDL_FRect r = { (LARG - w) / 2, ALT - 82, w, 28 };
    ret_arredondado(ren, r, 10, cor_alfa((SDL_Color){ 0, 0, 0, 220 }, a));
    contorno_arredondado(ren, r, 10, cor_alfa(COR_CIANO, a));
    texto_centro(ren, LARG / 2, r.y + 6, 1, cor_alfa(COR_TEXTO, a), aviso);
}

/* Dicas de botões no rodapé, com os botões de verdade do mapeamento atual.
 * Uma linha só, de no máximo ~52 caracteres: o que passar é cortado. */
static void rodape(SDL_Renderer *ren, const Game *g, const Hud *h)
{
    char dica[160];
    /* os botões de confirmar e voltar: os do controle e, se alguém já usou o
       teclado, as teclas também ("Sul/Enter") */
    const char *sul = T(h->pad[ACAO_CONFIRMA]), *leste = T(h->pad[ACAO_VOLTA]);
    char ok[48], volta[48];
    if (g->teclado_usado) {
        SDL_snprintf(ok, sizeof(ok), "%s/%s", sul, h->tecla[ACAO_CONFIRMA]);
        SDL_snprintf(volta, sizeof(volta), "%s/%s", leste, h->tecla[ACAO_VOLTA]);
    } else {
        SDL_strlcpy(ok, sul, sizeof(ok));
        SDL_strlcpy(volta, leste, sizeof(volta));
    }

    if (g->tela == TELA_MUSICAS) {
        SDL_snprintf(dica, sizeof(dica), T("[%s] escolher  [%s] voltar  [←/→] página"), sul, leste);
    } else if (g->tela == TELA_JOGO && !g->pausado && !g->terminou) {
        if (Game_n_jogando(g) > 1) return;   /* o espaço embaixo das pistas é do placar de cada um */
        SDL_snprintf(dica, sizeof(dica), T("[%s] pausa  [%s] star power"),
                     T(h->pad[ACAO_PAUSA]), T(h->pad[ACAO_STAR_POWER]));
    } else if (g->tela == TELA_JOGADORES && g->nome_jogador >= 0) {
        return;   /* o teclado da tela tem as dicas dele */
    } else if (g->tela == TELA_JOGADORES || g->tela == TELA_PREPARAR) {
        /* o líder pronto, esperando quem entrou: ele pode começar sem os outros */
        const int lider = Game_leader(g);
        bool falta = false;
        for (int k = 0; k < GAME_MAX_JOGADORES; k++) falta |= k != lider && g->jog[k].ativo && !g->jog[k].pronto;
        if (g->tela == TELA_JOGADORES && g->contagem > 0.0)
            SDL_strlcpy(dica, T("todos prontos: escolha a música..."), sizeof(dica));
        else if (g->tela == TELA_PREPARAR && lider >= 0 && g->jog[lider].pronto && falta)
            SDL_snprintf(dica, sizeof(dica), T("J%d: [%s] de novo começa sem os outros"), lider + 1,
                         g->jog[lider].no_teclado ? h->tecla[ACAO_CONFIRMA] : T(h->entra_pad[lider]));
        else if (g->tela == TELA_JOGADORES && g->teclado_usado)   /* comprido demais com os dois: só as teclas */
            SDL_snprintf(dica, sizeof(dica), T("[←/→] cartão  [↑/↓] linha  [%s] ok  [%s] volta"),
                         h->tecla[ACAO_CONFIRMA], h->tecla[ACAO_VOLTA]);
        else
            SDL_snprintf(dica, sizeof(dica), T("[↑/↓] linha  [%s] ok  [%s] volta"), ok, volta);
    } else if (g->escolhendo) {
        SDL_snprintf(dica, sizeof(dica), T("[↑/↓] muda  [%s] confirma  [%s] desfaz"), ok, volta);
    } else {
        SDL_snprintf(dica, sizeof(dica), T("[%s] ok  [%s] volta"), ok, volta);
    }
    char cabe_dica[160];   /* uma tradução comprida não sai da tela: é cortada */
    texto_corta(cabe_dica, sizeof(cabe_dica), dica, LARG - 16, 1);
    ret_degrade(ren, (SDL_FRect){ 0, ALT - 30, LARG, 30 }, (SDL_Color){ 0, 0, 0, 0 }, (SDL_Color){ 0, 0, 0, 210 });
    texto_centro(ren, LARG / 2, ALT - 21, 1, COR_TEXTO_FRACO, cabe_dica);
}

/* ---------------------------------------------------------------------------
 * HUD da partida
 * ------------------------------------------------------------------------- */
static SDL_Color cor_do_multiplicador(const Jogador *j)
{
    if (j->sp_ativo) return COR_CIANO;
    switch (j->multiplicador) {
    case 2:  return COR_TRASTE[4];                    /* laranja */
    case 3:  return COR_TRASTE[0];                    /* verde   */
    case 4:  return (SDL_Color){ 190, 100, 255, 255 }; /* roxo    */
    default: return COR_TEXTO;
    }
}

/* o primeiro jogador ativo (com um só, é ele quem aparece) */
static const Jogador *jogador_unico(const Game *g)
{
    for (int k = 0; k < GAME_MAX_JOGADORES; k++)
        if (g->jog[k].ativo) return &g->jog[k];
    return &g->jog[0];
}

/* medidor de star power: barra vertical à esquerda, com a marca dos 50% */
static void medidor_star(SDL_Renderer *ren, const Game *g, const Jogador *j)
{
    const SDL_FRect r = { 22, 180, 16, 200 };
    const float cheio = r.h * j->sp_medidor;
    const bool pronto = !j->sp_ativo && j->sp_medidor >= GAME_SP_MINIMO;
    const float brilho = 0.5f + 0.5f * SDL_sinf((float)g->t_total * 8.0f);

    const SDL_FRect moldura = { r.x - 3, r.y - 3, r.w + 6, r.h + 6 };
    ret_arredondado(ren, moldura, 6, (SDL_Color){ 0, 0, 0, 170 });
    contorno_arredondado(ren, moldura, 6, cor_alfa(COR_TEXTO_FRACO, 0.8f));
    if (cheio > 0.5f) {
        const SDL_Color c = (j->sp_ativo || pronto) ? cor_alfa(COR_CIANO, pronto ? 0.6f + 0.4f * brilho : 1.0f)
                                                     : (SDL_Color){ 60, 140, 200, 255 };
        ret(ren, (SDL_FRect){ r.x, r.y + r.h - cheio, r.w, cheio }, c);
    }
    ret(ren, (SDL_FRect){ r.x - 5, r.y + r.h * (1.0f - GAME_SP_MINIMO), r.w + 10, 2 }, COR_TEXTO);
    texto(ren, r.x - 10, r.y + r.h + 8, 1, COR_TEXTO_FRACO, T("STAR"));

    if (j->sp_ativo)  texto_sombra(ren, 10, r.y - 28, 1, COR_CIANO, T("STAR POWER!"));
    else if (pronto)  texto_sombra(ren, 10, r.y - 28, 1, cor_alfa(COR_CIANO, 0.5f + 0.5f * brilho), T("PRONTO!"));

    /* whammy: barrinha horizontal embaixo */
    if (j->whammy > 0.0f) {
        ret(ren, (SDL_FRect){ r.x - 6, r.y + r.h + 28, 40, 4 }, (SDL_Color){ 255, 255, 255, 40 });
        ret(ren, (SDL_FRect){ r.x - 6, r.y + r.h + 28, 40 * j->whammy, 4 }, COR_ACENTO);
    }
}

/* "Perfeito / Bom / Errou" sobre a linha de acerto, sumindo em meio segundo */
static void julgamento(SDL_Renderer *ren, const Game *g, const Jogador *j, float cx)
{
    if (!g->opcoes.mostrar_julgamento) return;   /* opcional: polui a pista */
    const double idade = g->t - j->ultimo_julgamento_t;
    if (idade < 0.0 || idade > 0.5) return;
    const float a = 1.0f - (float)(idade / 0.5);
    const float y = 310.0f - 14.0f * (float)(idade / 0.5);

    if (j->ultimo_foi_erro) {
        texto_centro(ren, cx, y, 1, cor_alfa((SDL_Color){ 255, 80, 80, 255 }, a), T("ERROU"));
        return;
    }
    const bool perfeito = SDL_fabs(j->ultimo_erro_tempo) <= Game_janela_perfeito(g);
    const char *t = T(perfeito ? "PERFEITO" : "BOM");
    texto_sombra(ren, cx - texto_largura(t, 1) / 2, y, 1, cor_alfa(perfeito ? COR_CIANO : COR_TEXTO, a), t);

    char ms[24];   /* negativo = adiantado: ajuda a perceber o atraso da tela/som */
    SDL_snprintf(ms, sizeof(ms), "%+.0f ms", j->ultimo_erro_tempo * 1000.0);
    texto_centro(ren, cx, y + 18, 1, cor_alfa(COR_TEXTO_FRACO, a), ms);
}

/* um jogador: o placar grande à direita e o medidor à esquerda */
static void hud_um(SDL_Renderer *ren, const Game *g, const Jogador *j)
{
    char s[160], c[160];

    /* música, parte e tempo, no canto */
    const char *titulo = g->musica < 0 ? T(g->titulo) : g->titulo;   /* a demonstração não é um nome */
    if (j->instrumento == INSTR_GUITARRA)
        SDL_snprintf(s, sizeof(s), "%s (%s)", titulo, T(Dificuldade_nome(j->dificuldade)));
    else
        SDL_snprintf(s, sizeof(s), "%s (%s %s)", titulo, T(Instrumento_nome(j->instrumento)),
                     T(Dificuldade_nome(j->dificuldade)));
    texto_corta(c, sizeof(c), s, LARG - 110, 1);
    texto_sombra(ren, 10, 6, 1, COR_TEXTO, c);
    hora(s, sizeof(s), g->t);
    const char *nome = Game_player_name(g, (int)(j - g->jog));
    if (nome) SDL_snprintf(c, sizeof(c), "%s   %s", s, nome);
    else      SDL_snprintf(c, sizeof(c), "%s", s);
    texto(ren, 10, 24, 1, COR_TEXTO_FRACO, c);

    /* pontos, multiplicador e combo: à direita, perto da altura das notas que
       importam (mais para baixo fica mais fácil de olhar sem se perder na pista).
       A coluna termina onde a pista começa a alargar (y ~ 340). */
    const float cx = 578;
    ret_arredondado(ren, (SDL_FRect){ 524, 184, 108, 162 }, 10, (SDL_Color){ 0, 0, 0, 120 });

    texto_centro(ren, cx, 190, 1, COR_TEXTO_FRACO, T("PONTOS"));
    SDL_snprintf(s, sizeof(s), "%ld", (long)j->pontos);
    texto_sombra(ren, cx - texto_largura(s, 1) / 2, 208, 1, COR_TEXTO, s);

    /* o multiplicador cresce um pouco quando muda, e volta em 250 ms */
    const double desde = g->t_total - j->mult_mudou_t;
    const float pop = desde >= 0.0 && desde < 0.25 ? 1.0f + 0.3f * (float)(1.0 - desde / 0.25) : 1.0f;
    const SDL_Color cm = cor_do_multiplicador(j);
    elipse(ren, cx, 258, 58 * pop, 58 * pop, (SDL_Color){ 0, 0, 0, 170 });
    elipse(ren, cx, 258, 52 * pop, 52 * pop, cor_alfa(cm, 0.25f));
    SDL_snprintf(s, sizeof(s), "x%d", j->multiplicador);
    texto_centro(ren, cx, 242, 2, cm, s);

    texto_centro(ren, cx, 290, 1, COR_TEXTO_FRACO, T("COMBO"));
    SDL_snprintf(s, sizeof(s), "%d", j->combo);
    texto_centro(ren, cx, 308, texto_largura(s, 2) <= 100 ? 2 : 1, COR_TEXTO, s);

    medidor_star(ren, g, j);
    julgamento(ren, g, j, LARG / 2);
}

/* vários jogadores: quem é em cima da pista; embaixo, os pontos, o
   multiplicador e o combo, e a barra de star power na borda da tela */
static void hud_varios(SDL_Renderer *ren, const Game *g, const Jogador *j, int k, int pos, int n)
{
    const float cx = pista_centro(pos, n), larg = 0.9f * pista_largura(n);   /* folga para a vizinha */
    const float x0 = cx - larg / 2, x1 = cx + larg / 2;
    const SDL_Color cj = COR_JOGADOR[k];
    char s[64], c[64];

    nome_curto(s, sizeof(s), g, k);
    texto_corta(c, sizeof(c), s, larg, 1);
    texto_centro(ren, cx, 22, 1, j->sp_ativo ? COR_CIANO : cj, c);
    texto_corta(c, sizeof(c), T(Instrumento_nome(j->instrumento)), larg, 1);
    texto_centro(ren, cx, 38, 1, COR_TEXTO_FRACO, c);

    julgamento(ren, g, j, cx);
    if (g->pausado || g->terminou) return;   /* embaixo fica o rodapé do menu; o resultado mostra os números */

    SDL_snprintf(s, sizeof(s), "%ld", (long)j->pontos);
    texto_sombra(ren, cx - texto_largura(s, 1) / 2, PISTA_Y_ACERTO + 12, 1, COR_TEXTO, s);
    SDL_snprintf(s, sizeof(s), "x%d", j->multiplicador);
    texto(ren, x0, PISTA_Y_ACERTO + 30, 1, cor_do_multiplicador(j), s);
    SDL_snprintf(s, sizeof(s), "%d", j->combo);
    texto(ren, x1 - texto_largura(s, 1), PISTA_Y_ACERTO + 30, 1, COR_TEXTO_FRACO, s);

    /* star power: barra horizontal, com a marca dos 50%; pisca quando dá para ativar */
    const SDL_FRect r = { x0, ALT - 8, larg, 5 };
    const bool pronto = !j->sp_ativo && j->sp_medidor >= GAME_SP_MINIMO;
    ret(ren, r, (SDL_Color){ 0, 0, 0, 170 });
    if (j->sp_medidor > 0.0f)
        ret(ren, (SDL_FRect){ r.x, r.y, r.w * j->sp_medidor, r.h },
            (j->sp_ativo || pronto) ? cor_alfa(COR_CIANO, pronto ? 0.6f + 0.4f * pulso(g) : 1.0f)
                                    : (SDL_Color){ 60, 140, 200, 255 });
    ret(ren, (SDL_FRect){ r.x + r.w * GAME_SP_MINIMO - 1, r.y - 2, 2, r.h + 4 }, COR_TEXTO);
}

static void hud_jogo(SDL_Renderer *ren, const Game *g)
{
    const int n = Game_n_jogando(g);
    if (n <= 1) {
        hud_um(ren, g, jogador_unico(g));
        return;
    }
    for (int k = 0, pos = 0; k < GAME_MAX_JOGADORES; k++)
        if (g->jog[k].ativo) hud_varios(ren, g, &g->jog[k], k, pos++, n);
}

static void pistas(SDL_Renderer *ren, const Game *g)
{
    const int n = Game_n_jogando(g);
    if (n <= 1) {
        pista_desenha(ren, g, jogador_unico(g), 0, 1);
        return;
    }
    for (int k = 0, pos = 0; k < GAME_MAX_JOGADORES; k++)
        if (g->jog[k].ativo) pista_desenha(ren, g, &g->jog[k], pos++, n);
}

static int porcentagem(const Jogador *j)
{
    const int total = j->acertos + j->erros;
    return total ? (100 * j->acertos + total / 2) / total : 0;
}

/* tela de fim: o resumo da partida (o menu vem por baixo, no Draw_frame) */
static void resultado_um(SDL_Renderer *ren, const Game *g, const Jogador *j)
{
    char s[96], c[96];
    const int total = j->acertos + j->erros;
    const SDL_FRect p = { 40, 84, LARG - 80, 160 };
    ret_arredondado(ren, p, 14, COR_PAINEL);
    contorno_arredondado(ren, p, 14, cor_alfa(COR_CIANO, 0.7f));

    SDL_snprintf(s, sizeof(s), T("%ld pontos"), (long)j->pontos);
    texto_sombra(ren, LARG / 2 - texto_largura(s, 2) / 2, p.y + 12, 2, COR_TEXTO, s);
    SDL_snprintf(s, sizeof(s), T("Notas: %d / %d  (%d%%)"), j->acertos, total, porcentagem(j));
    texto_centro(ren, LARG / 2, p.y + 54, 1, COR_TEXTO, s);
    SDL_snprintf(s, sizeof(s), T("Maior combo: %d"), j->maior_combo);
    texto_centro(ren, LARG / 2, p.y + 74, 1, COR_TEXTO, s);

    /* o tempo médio dos acertos: a pista para o "Ajuste de tempo" */
    if (j->acertos >= 20) {
        const int media = (int)SDL_lround(j->soma_erro / j->acertos * 1000.0);
        const int sugere = g->opcoes.ajuste_ms - (media / GAME_AJUSTE_PASSO) * GAME_AJUSTE_PASSO;
        if (media <= -10 || media >= 10)
            SDL_snprintf(s, sizeof(s), media < 0 ? T("%d ms adiantado: tente Ajuste de tempo %+d")
                                                 : T("%d ms atrasado: tente Ajuste de tempo %+d"),
                         media < 0 ? -media : media, sugere);
        else
            SDL_snprintf(s, sizeof(s), T("acertos no tempo certo (%+d ms em média)"), media);
        texto_corta(c, sizeof(c), s, p.w - 24, 1);
        texto_centro(ren, LARG / 2, p.y + 100, 1, COR_TEXTO_FRACO, c);
    }
    if (j->novo_recorde) {
        const float b = 0.6f + 0.4f * pulso(g);
        SDL_snprintf(s, sizeof(s), j->erros == 0 && total ? T("NOVO RECORDE!  FULL COMBO!  (antes: %ld)")
                                                           : T("NOVO RECORDE!  (antes: %ld)"), j->recorde_anterior);
        texto_corta(c, sizeof(c), s, p.w - 24, 1);
        texto_centro(ren, LARG / 2, p.y + 124, 1, cor_alfa((SDL_Color){ 255, 220, 90, 255 }, b), c);
    } else {
        texto_corta(c, sizeof(c), j->erros == 0 && total ? T("FULL COMBO!") : (g->musica < 0 ? T(g->titulo) : g->titulo),
                    p.w - 24, 1);
        texto_centro(ren, LARG / 2, p.y + 124, 1, COR_TEXTO_FRACO, c);
    }
}

/* vários jogadores: uma coluna para cada um */
static void resultado_varios(SDL_Renderer *ren, const Game *g, int n)
{
    char s[64], c[64];
    const SDL_FRect p = { 20, 76, LARG - 40, 222 };
    ret_arredondado(ren, p, 14, COR_PAINEL);
    contorno_arredondado(ren, p, 14, cor_alfa(COR_CIANO, 0.7f));
    texto_corta(c, sizeof(c), g->musica < 0 ? T(g->titulo) : g->titulo, p.w - 24, 1);
    texto_centro(ren, LARG / 2, p.y + 6, 1, COR_TEXTO_FRACO, c);

    const float w = p.w / (float)n;
    for (int k = 0, pos = 0; k < GAME_MAX_JOGADORES; k++) {
        const Jogador *j = &g->jog[k];
        if (!j->ativo) continue;
        const float cx = p.x + w * ((float)pos++ + 0.5f);
        const int total = j->acertos + j->erros;

        SDL_snprintf(s, sizeof(s), "J%d", k + 1);
        texto_centro(ren, cx, p.y + 26, 2, COR_JOGADOR[k], s);
        nome_curto(s, sizeof(s), g, k);
        texto_centro_cortado(ren, cx, p.y + 62, w - 12, COR_JOGADOR[k], s);
        texto_centro_cortado(ren, cx, p.y + 78, w - 12, COR_TEXTO, T(Instrumento_nome(j->instrumento)));
        texto_centro_cortado(ren, cx, p.y + 94, w - 12, COR_TEXTO_FRACO, T(Dificuldade_nome(j->dificuldade)));
        SDL_snprintf(s, sizeof(s), "%ld", (long)j->pontos);
        texto_centro(ren, cx, p.y + 114, texto_largura(s, 2) <= w - 12 ? 2 : 1, COR_TEXTO, s);
        SDL_snprintf(s, sizeof(s), T("notas %d%%"), porcentagem(j));
        texto_centro_cortado(ren, cx, p.y + 150, w - 12, COR_TEXTO, s);
        SDL_snprintf(s, sizeof(s), T("combo %d"), j->maior_combo);
        texto_centro_cortado(ren, cx, p.y + 166, w - 12, COR_TEXTO_FRACO, s);
        if (j->erros == 0 && total) texto_centro_cortado(ren, cx, p.y + 184, w - 12, COR_CIANO, T("FULL COMBO!"));
        if (j->novo_recorde)
            texto_centro_cortado(ren, cx, p.y + 202, w - 12, cor_alfa((SDL_Color){ 255, 220, 90, 255 }, 0.6f + 0.4f * pulso(g)), T("RECORDE!"));
    }
}

static void resultado(SDL_Renderer *ren, const Game *g)
{
    const int n = Game_n_jogando(g);
    if (n <= 1) resultado_um(ren, g, jogador_unico(g));
    else        resultado_varios(ren, g, n);
}

static void fps(SDL_Renderer *ren, const Game *g, const Hud *h)
{
    if (!g->opcoes.mostrar_fps) return;
    char s[24];
    SDL_snprintf(s, sizeof(s), "%.0f FPS", h->fps);
    texto(ren, LARG - texto_largura(s, 1) - 6, 4, 1, (SDL_Color){ 255, 230, 80, 255 }, s);
}

/* ---------------------------------------------------------------------------
 * O frame
 * ------------------------------------------------------------------------- */
void Draw_frame(const Draw *d, SDL_Renderer *ren, const Game *g, const Hud *h)
{
    fundo(d, ren, g);

    const Menu *m = Game_menu(g);
    if (g->tela == TELA_JOGO) {
        pistas(ren, g);
        hud_jogo(ren, g);
        if (g->terminou) {
            escurece(ren, 0.65f);
            titulo(ren, g, m);
            resultado(ren, g);
            menu_lista(ren, g, m, Game_menu_sel(g));
        } else if (g->pausado) {
            escurece(ren, 0.55f);
            titulo(ren, g, m);
            menu_lista(ren, g, m, Game_menu_sel(g));
        }
    } else if (m) {
        titulo(ren, g, m);
        if (g->tela == TELA_CONTROLES) menu_controles(ren, g, h, m, Game_menu_sel(g));
        else                           menu_lista(ren, g, m, Game_menu_sel(g));
        if (g->tela == TELA_AUDIO)       painel_audio(ren, &h->audio);
        if (g->tela == TELA_MUSICAS)     { info_musica(ren, g); estado_lista(ren, g, h); }
    } else if (g->tela == TELA_JOGADORES) {
        tela_jogadores(ren, g, h);
    } else if (g->tela == TELA_PREPARAR) {
        tela_preparar(ren, g, h);
    } else if (g->tela == TELA_CARREGANDO) {
        tela_carregando(ren, g, h);
    }

    rodape(ren, g, h);
    aviso(ren, g);
    if (h->capturando >= 0) captura(ren, h);
    fps(ren, g, h);
}
