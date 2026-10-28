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
    [TELA_DIFICULDADE] = IMAGEM_INICIO,
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

bool Draw_init(Draw *d, SDL_Renderer *ren, const char *pasta_imagens)
{
    /* sem isto, o alfa das cores de SDL_RenderGeometry/FillRect é ignorado */
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    /* imagem que falta não impede o jogo: o fundo vira um degradê */
    for (int i = 0; i < IMAGEM_N; i++)
        d->imagem[i] = carrega_imagem(ren, pasta_imagens, ARQUIVO_IMAGEM[i]);
    return true;
}

void Draw_quit(Draw *d)
{
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
 * ------------------------------------------------------------------------- */
#define MENU_ESCALA 2.0f   /* texto dos itens: 16 px */

/* pulsa entre 0 e 1, para a seleção "respirar" */
static float pulso(const Game *g)
{
    return 0.5f + 0.5f * SDL_sinf((float)g->t_total * 5.0f);
}

static void titulo(SDL_Renderer *ren, const Game *g, const Menu *m)
{
    if (g->tela == TELA_INICIO) {
        texto_sombra(ren, LARG / 2 - texto_largura(m->titulo, 5) / 2 + 3, 46, 5, COR_ACENTO, m->titulo);
        texto(ren, LARG / 2 - texto_largura(m->titulo, 5) / 2, 43, 5, COR_TEXTO, m->titulo);
        texto_centro(ren, LARG / 2, 96, 1, COR_CIANO, "um jogo de ritmo para o R36S");
    } else {
        float y = 40;
        if (g->tela == TELA_CONTROLES)  y = 26;
        else if (g->tela == TELA_JOGO)  y = g->terminou ? 36 : 120;
        texto_sombra(ren, LARG / 2 - texto_largura(m->titulo, 3) / 2, y, 3, COR_TEXTO, m->titulo);
    }
}

/* Uma linha de menu. valor/valor2: texto à direita (opção, botão mapeado). */
static void linha(SDL_Renderer *ren, const Game *g, SDL_FRect r, const ItemMenu *item, bool sel,
                  const char *valor, float x_valor, const char *valor2, float x_valor2)
{
    const float y_texto = r.y + (r.h - TEXTO_PX * MENU_ESCALA) / 2.0f;

    if (sel) {
        ret_arredondado(ren, r, 8, cor_alfa(COR_ACENTO, 0.80f + 0.20f * pulso(g)));
        contorno_arredondado(ren, r, 8, cor_alfa(COR_TEXTO, 0.5f + 0.5f * pulso(g)));
    }

    const SDL_Color c = item->em_breve ? COR_TEXTO_FRACO : (sel ? COR_TEXTO_ESCURO : COR_TEXTO);

    /* rótulo comprido (nome de música) é cortado com ".." para não sair da linha */
    char rotulo[64];
    const int cabe = (int)((r.w - 36) / (TEXTO_PX * MENU_ESCALA));
    SDL_strlcpy(rotulo, item->rotulo, sizeof(rotulo));
    if (cabe > 3 && (int)SDL_strlen(rotulo) > cabe) SDL_strlcpy(rotulo + cabe - 2, "..", 3);
    texto(ren, r.x + 18, y_texto, MENU_ESCALA, c, rotulo);

    if (item->em_breve) {
        const char *tag = "em breve";
        texto(ren, r.x + r.w - texto_largura(tag, 1) - 14, r.y + (r.h - TEXTO_PX) / 2.0f, 1,
              sel ? COR_TEXTO_ESCURO : COR_TEXTO_FRACO, tag);
    }
    if (valor && *valor && !item->em_breve) {
        char buf[48];
        if (item->tipo == ITEM_OPCAO && sel) SDL_snprintf(buf, sizeof(buf), "< %s >", valor);
        else                                 SDL_snprintf(buf, sizeof(buf), "%s", valor);
        const float x = x_valor > 0 ? x_valor : r.x + r.w - texto_largura(buf, MENU_ESCALA) - 16;
        texto(ren, x, y_texto, MENU_ESCALA, c, buf);
    }
    if (valor2 && *valor2) texto(ren, x_valor2, y_texto, MENU_ESCALA, c, valor2);
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
    if (g->tela == TELA_INICIO)           y0 = 140;
    else if (g->tela == TELA_MUSICAS)     y0 = 86;
    else if (g->tela == TELA_DIFICULDADE) y0 = 140;
    else if (g->tela == TELA_JOGO)        y0 = g->terminou ? 262 : 170;
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
        texto_centro(ren, LARG / 2, painel.y + 2, 1, COR_TEXTO_FRACO, "^");
    if (primeiro + vis < m->n)
        texto_centro(ren, LARG / 2, painel.y + painel.h - 10, 1, COR_TEXTO_FRACO, "v");
}

static void hora(char *buf, size_t n, double t);

/* embaixo da lista de músicas: artista, charter e duração da selecionada */
static void info_musica(SDL_Renderer *ren, const Game *g)
{
    const int sel = Game_menu_sel(g);
    const int id  = g->menu_musicas.itens[sel].id;
    const SDL_FRect p = { 40, 404, LARG - 80, 40 };
    ret_arredondado(ren, p, 10, COR_PAINEL);

    char s[160], dur[16];
    if (id == ID_DEMO) {
        SDL_strlcpy(s, "Notas de exemplo com metronomo, para testar a jogabilidade", sizeof(s));
    } else if (id >= ID_MUSICA && id < ID_DIFICULDADE) {
        const InfoMusica *mu = &g->musicas[id - ID_MUSICA];
        hora(dur, sizeof(dur), mu->duracao_ms / 1000.0);
        SDL_snprintf(s, sizeof(s), "%s  -  %s", mu->artista[0] ? mu->artista : "?", mu->duracao_ms ? dur : "?");
        if (mu->charter[0]) {
            char ch[96];
            SDL_snprintf(ch, sizeof(ch), "chart: %s", mu->charter);
            texto(ren, p.x + 14, p.y + 24, 1, COR_TEXTO_FRACO, ch);
        }
    } else {
        s[0] = '\0';
    }
    if (id == ID_PROCURAR)
        SDL_strlcpy(s, "Procura de novo nas pastas (songs/ e as do pastas.txt)", sizeof(s));
    texto(ren, p.x + 14, p.y + 10, 1, COR_TEXTO, s);
}

/* em cima da lista: em que posição estamos e se a procura está andando */
static void estado_lista(SDL_Renderer *ren, const Game *g, const Hud *h)
{
    char s[64];
    SDL_snprintf(s, sizeof(s), "%d/%d", Game_menu_sel(g) + 1, g->menu_musicas.n);
    texto(ren, LARG - 40 - texto_largura(s, 1), 72, 1, COR_TEXTO_FRACO, s);

    if (h->procurando) {
        SDL_snprintf(s, sizeof(s), "procurando musicas... %d encontrada(s)", h->achadas);
        texto(ren, 40, 72, 1, cor_alfa(COR_CIANO, 0.6f + 0.4f * pulso(g)), s);
    } else if (g->n_musicas == 0) {
        texto(ren, 40, 72, 1, COR_TEXTO_FRACO, "nenhuma musica: copie pastas do Clone Hero para songs/");
    } else {
        SDL_snprintf(s, sizeof(s), "%d musica(s)", g->n_musicas);
        texto(ren, 40, 72, 1, COR_TEXTO_FRACO, s);
    }
}

static void subtitulo_musica(SDL_Renderer *ren, const Game *g, float y)
{
    if (g->musica < 0 || g->musica >= g->n_musicas) return;
    const InfoMusica *mu = &g->musicas[g->musica];
    texto_centro(ren, LARG / 2, y, 2, COR_CIANO, mu->titulo);
    if (mu->artista[0]) texto_centro(ren, LARG / 2, y + 22, 1, COR_TEXTO_FRACO, mu->artista);
}

/* carregando: o título e a barra de progresso da decodificação */
static void tela_carregando(SDL_Renderer *ren, const Game *g, const Hud *h)
{
    texto_sombra(ren, LARG / 2 - texto_largura("Carregando", 3) / 2, 120, 3, COR_TEXTO, "Carregando");
    subtitulo_musica(ren, g, 180);

    const SDL_FRect barra = { 120, 260, LARG - 240, 14 };
    ret_arredondado(ren, barra, 7, (SDL_Color){ 40, 25, 60, 230 });
    ret_arredondado(ren, (SDL_FRect){ barra.x, barra.y, barra.w * h->audio.progresso, barra.h }, 7, COR_CIANO);

    char s[48];
    SDL_snprintf(s, sizeof(s), "decodificando o audio: %.0f%%", 100.0f * h->audio.progresso);
    texto_centro(ren, LARG / 2, 288, 1, COR_TEXTO_FRACO, s);
    texto_centro(ren, LARG / 2, 330, 1, COR_TEXTO_FRACO, Dificuldade_nome(g->dificuldade));
}

/* A tela de controles: cada ação com o botão do controle e a tecla. */
static void menu_controles(SDL_Renderer *ren, const Game *g, const Hud *h, const Menu *m, int sel)
{
    const float lin_h = 25, lin_esp = 3;
    const SDL_FRect painel = { 30, 66, LARG - 60, m->n * (lin_h + lin_esp) + 34 };
    const float x_pad = painel.x + 300, x_tecla = painel.x + 420;

    ret_arredondado(ren, painel, 14, COR_PAINEL);
    contorno_arredondado(ren, painel, 14, cor_alfa(COR_ACENTO, 0.6f));
    texto(ren, x_pad, painel.y + 10, 1, COR_CIANO, "CONTROLE");
    texto(ren, x_tecla, painel.y + 10, 1, COR_CIANO, "TECLADO");

    for (int i = 0; i < m->n; i++) {
        const ItemMenu *item = &m->itens[i];
        const SDL_FRect r = { painel.x + 10, painel.y + 26 + i * (lin_h + lin_esp), painel.w - 20, lin_h };
        const char *pad = NULL, *tecla = NULL;
        if (item->id >= ID_MAPEAR) {
            pad   = h->pad[item->id - ID_MAPEAR];
            tecla = h->tecla[item->id - ID_MAPEAR];
        } else if (item->id == ID_ANALOGICO) {
            pad = h->analogico;   /* whammy (para os lados) e star power (para cima) */
        }
        linha(ren, g, r, item, i == sel, pad, x_pad, tecla, x_tecla);

        /* bolinha da cor do traste ao lado do nome */
        const int acao = item->id - ID_MAPEAR;
        if (item->id >= ID_MAPEAR && acao < ACAO_N_TRASTES)
            elipse(ren, r.x + 9, r.y + r.h / 2, 9, 9, COR_TRASTE[acao]);
    }

    char ultimo[80];
    SDL_snprintf(ultimo, sizeof(ultimo), "ultimo botao apertado: %s", h->ultimo ? h->ultimo : "-");
    texto(ren, painel.x + 12, painel.y + painel.h + 8, 1, COR_TEXTO_FRACO, ultimo);
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
    const SDL_FRect p = { 40, 268, LARG - 80, 176 };
    ret_arredondado(ren, p, 12, COR_PAINEL);
    contorno_arredondado(ren, p, 12, cor_alfa(COR_CIANO, 0.6f));

    char s[128];
    float y = p.y + 14;
    const float x = p.x + 16, passo = 16;

    if (!a->ok) {
        texto(ren, x, y, 1, (SDL_Color){ 255, 90, 90, 255 }, "Sem dispositivo de audio: o jogo roda sem som.");
        return;
    }
    SDL_snprintf(s, sizeof(s), "Saida: %s, buffer de %d frames (%.0f ms)", a->driver, a->buffer_frames,
                 1000.0 * a->buffer_frames / 48000.0);
    texto(ren, x, y, 1, COR_TEXTO_FRACO, s);
    y += passo + 6;

    if (!a->musica) {
        SDL_snprintf(s, sizeof(s), "Nenhuma musica em %s", a->pasta ? a->pasta : "-");
        texto(ren, x, y, 1, COR_TEXTO, s);
        texto(ren, x, y + passo, 1, COR_TEXTO_FRACO, "(uma pasta do Clone Hero: notes.chart + song.ini + audio)");
        return;
    }
    texto(ren, x, y, 1, COR_TEXTO, a->musica);
    y += passo;
    texto(ren, x, y, 1, COR_TEXTO_FRACO, a->faixas);
    if (a->ignoradas) {
        SDL_snprintf(s, sizeof(s), "(%d faixa(s) em mp3 ignorada(s): ainda nao suportado)", a->ignoradas);
        texto(ren, x, y + passo, 1, COR_TEXTO_FRACO, s);
    }
    y += 2 * passo + 4;

    if (a->carga == 1) {   /* carregando: barra de progresso */
        texto(ren, x, y, 1, COR_TEXTO, "Decodificando...");
        const SDL_FRect barra = { x + 140, y - 1, p.w - 180, 10 };
        ret_arredondado(ren, barra, 5, (SDL_Color){ 60, 40, 80, 255 });
        ret_arredondado(ren, (SDL_FRect){ barra.x, barra.y, barra.w * a->progresso, barra.h }, 5, COR_CIANO);
    } else if (a->carga_info) {
        texto(ren, x, y, 1, a->carga == 3 ? (SDL_Color){ 255, 90, 90, 255 } : COR_TEXTO, a->carga_info);
    }
    y += passo + 6;

    if (a->tocando) {
        char t[16], cru[16], dur[16];
        hora(t, sizeof(t), a->t_suave);
        hora(cru, sizeof(cru), a->t_cru);
        hora(dur, sizeof(dur), a->duracao);
        SDL_snprintf(s, sizeof(s), "%s %s / %s", a->pausado ? "Pausado" : "Tocando", t, dur);
        texto(ren, x, y, 2, COR_CIANO, s);
        SDL_snprintf(s, sizeof(s), "relogio cru %s   diferenca suave-cru: %+.1f ms", cru,
                     (a->t_suave - a->t_cru) * 1000.0);
        texto(ren, x, y + 22, 1, COR_TEXTO_FRACO, s);

        const float frac = a->duracao > 0 ? (float)(a->t_suave / a->duracao) : 0.0f;
        const SDL_FRect barra = { x, y + 38, p.w - 32, 6 };
        ret(ren, barra, (SDL_Color){ 60, 40, 80, 255 });
        ret(ren, (SDL_FRect){ barra.x, barra.y, barra.w * (frac < 0 ? 0 : (frac > 1 ? 1 : frac)), barra.h }, COR_ACENTO);
    }
}

/* ---------------------------------------------------------------------------
 * Sobreposições
 * ------------------------------------------------------------------------- */
static void escurece(SDL_Renderer *ren, float alfa)
{
    ret(ren, (SDL_FRect){ 0, 0, LARG, ALT }, cor_alfa((SDL_Color){ 0, 0, 0, 255 }, alfa));
}

static void captura(SDL_Renderer *ren, const Hud *h)
{
    escurece(ren, 0.6f);
    const SDL_FRect p = { 90, 160, LARG - 180, 160 };
    ret_arredondado(ren, p, 14, (SDL_Color){ 25, 12, 45, 245 });
    contorno_arredondado(ren, p, 14, COR_CIANO);

    texto_centro(ren, LARG / 2, p.y + 20, 1, COR_TEXTO, "Aperte o novo botao ou tecla para");

    const SDL_Color c = h->capturando < ACAO_N_TRASTES ? COR_TRASTE[h->capturando] : COR_ACENTO;
    texto_centro(ren, LARG / 2, p.y + 46, 3, c, Acao_rotulo((Acao)h->capturando));
    texto_centro(ren, LARG / 2, p.y + 92, 1, COR_TEXTO_FRACO, "Esc cancela (teclado)  -  sem resposta, cancela sozinho");

    /* barra do tempo restante */
    const float frac = (float)(h->captura_resta / 5.0);
    const SDL_FRect fundo_barra = { p.x + 30, p.y + 124, p.w - 60, 10 };
    ret_arredondado(ren, fundo_barra, 5, (SDL_Color){ 60, 40, 80, 255 });
    ret_arredondado(ren, (SDL_FRect){ fundo_barra.x, fundo_barra.y, fundo_barra.w * frac, fundo_barra.h },
                    5, COR_CIANO);
}

static void aviso(SDL_Renderer *ren, const Game *g)
{
    if (!g->aviso) return;
    const float resta = (float)(g->aviso_ate - g->t_total);
    const float a = resta < 0.3f ? resta / 0.3f : 1.0f;   /* some nos últimos 300 ms */
    const float w = texto_largura(g->aviso, 2) + 40;
    const SDL_FRect r = { (LARG - w) / 2, ALT - 84, w, 34 };
    ret_arredondado(ren, r, 10, cor_alfa((SDL_Color){ 0, 0, 0, 220 }, a));
    contorno_arredondado(ren, r, 10, cor_alfa(COR_CIANO, a));
    texto_centro(ren, LARG / 2, r.y + 9, 2, cor_alfa(COR_TEXTO, a), g->aviso);
}

/* dicas de botões no rodapé, com os botões de verdade do mapeamento atual */
static void rodape(SDL_Renderer *ren, const Game *g, const Hud *h)
{
    char dica[128];
    if (g->tela == TELA_MUSICAS) {
        SDL_snprintf(dica, sizeof(dica), "[%s] escolher   [%s] voltar   [Esq/Dir] pagina   segure Cima/Baixo para rolar",
                     h->pad[ACAO_CONFIRMA], h->pad[ACAO_VOLTA]);
    } else if (g->tela == TELA_JOGO && !g->pausado && !g->terminou) {
        SDL_snprintf(dica, sizeof(dica), "[%s] pausa   [%s / analog. cima] star power   [Select+Start] sair",
                     h->pad[ACAO_PAUSA], h->pad[ACAO_STAR_POWER]);
    } else {
        SDL_snprintf(dica, sizeof(dica), "[%s/%s] confirmar   [%s/%s] voltar   [Esq/Dir] ajustar",
                     h->pad[ACAO_CONFIRMA], h->tecla[ACAO_CONFIRMA], h->pad[ACAO_VOLTA], h->tecla[ACAO_VOLTA]);
    }
    ret_degrade(ren, (SDL_FRect){ 0, ALT - 26, LARG, 26 }, (SDL_Color){ 0, 0, 0, 0 }, (SDL_Color){ 0, 0, 0, 200 });
    texto_centro(ren, LARG / 2, ALT - 14, 1, COR_TEXTO_FRACO, dica);
}

/* ---------------------------------------------------------------------------
 * HUD da partida
 * ------------------------------------------------------------------------- */
static SDL_Color cor_do_multiplicador(const Game *g)
{
    if (g->sp_ativo) return COR_CIANO;
    switch (g->multiplicador) {
    case 2:  return COR_TRASTE[4];                    /* laranja */
    case 3:  return COR_TRASTE[0];                    /* verde   */
    case 4:  return (SDL_Color){ 190, 100, 255, 255 }; /* roxo    */
    default: return COR_TEXTO;
    }
}

/* medidor de star power: barra vertical à esquerda, com a marca dos 50% */
static void medidor_star(SDL_Renderer *ren, const Game *g)
{
    const SDL_FRect r = { 22, 180, 16, 200 };
    const float cheio = r.h * g->sp_medidor;
    const bool pronto = !g->sp_ativo && g->sp_medidor >= GAME_SP_MINIMO;
    const float brilho = 0.5f + 0.5f * SDL_sinf((float)g->t_total * 8.0f);

    const SDL_FRect moldura = { r.x - 3, r.y - 3, r.w + 6, r.h + 6 };
    ret_arredondado(ren, moldura, 6, (SDL_Color){ 0, 0, 0, 170 });
    contorno_arredondado(ren, moldura, 6, cor_alfa(COR_TEXTO_FRACO, 0.8f));
    if (cheio > 0.5f) {
        const SDL_Color c = (g->sp_ativo || pronto) ? cor_alfa(COR_CIANO, pronto ? 0.6f + 0.4f * brilho : 1.0f)
                                                     : (SDL_Color){ 60, 140, 200, 255 };
        ret(ren, (SDL_FRect){ r.x, r.y + r.h - cheio, r.w, cheio }, c);
    }
    ret(ren, (SDL_FRect){ r.x - 5, r.y + r.h * (1.0f - GAME_SP_MINIMO), r.w + 10, 2 }, COR_TEXTO);
    texto(ren, r.x - 6, r.y + r.h + 8, 1, COR_TEXTO_FRACO, "STAR");

    if (g->sp_ativo)  texto_sombra(ren, 12, r.y - 34, 1, COR_CIANO, "STAR POWER!");
    else if (pronto)  texto_sombra(ren, 12, r.y - 34, 1, cor_alfa(COR_CIANO, 0.5f + 0.5f * brilho), "PRONTO!");

    /* whammy: barrinha horizontal embaixo */
    if (g->whammy > 0.0f) {
        ret(ren, (SDL_FRect){ r.x - 6, r.y + r.h + 22, 40, 4 }, (SDL_Color){ 255, 255, 255, 40 });
        ret(ren, (SDL_FRect){ r.x - 6, r.y + r.h + 22, 40 * g->whammy, 4 }, COR_ACENTO);
    }
}

/* "Perfeito / Bom / Errou" sobre a linha de acerto, sumindo em meio segundo */
static void julgamento(SDL_Renderer *ren, const Game *g)
{
    const double idade = g->t - g->ultimo_julgamento_t;
    if (idade < 0.0 || idade > 0.5) return;
    const float a = 1.0f - (float)(idade / 0.5);
    const float y = 318.0f - 14.0f * (float)(idade / 0.5);

    if (g->ultimo_foi_erro) {
        texto_centro(ren, LARG / 2, y, 2, cor_alfa((SDL_Color){ 255, 80, 80, 255 }, a), "ERROU");
        return;
    }
    const bool perfeito = SDL_fabs(g->ultimo_erro_tempo) <= Game_janela_perfeito(g);
    texto_centro(ren, LARG / 2, y, 2, cor_alfa(perfeito ? COR_CIANO : COR_TEXTO, a), perfeito ? "PERFEITO" : "BOM");

    char ms[24];   /* negativo = adiantado: ajuda a perceber o atraso da tela/som */
    SDL_snprintf(ms, sizeof(ms), "%+.0f ms", g->ultimo_erro_tempo * 1000.0);
    texto_centro(ren, LARG / 2, y + 20, 1, cor_alfa(COR_TEXTO_FRACO, a), ms);
}

static void hud_jogo(SDL_Renderer *ren, const Game *g)
{
    char s[48];

    /* música e tempo, no canto */
    texto_sombra(ren, 12, 10, 1, COR_TEXTO, g->chart.titulo);
    hora(s, sizeof(s), g->t);
    texto(ren, 12, 22, 1, COR_TEXTO_FRACO, s);

    /* pontos, multiplicador e combo: à direita, perto da altura das notas que
       importam (mais para baixo fica mais fácil de olhar sem se perder na pista).
       A coluna termina onde a pista começa a alargar (y ~ 330). */
    const float cx = 578;
    ret_arredondado(ren, (SDL_FRect){ 524, 190, 108, 144 }, 10, (SDL_Color){ 0, 0, 0, 120 });

    texto_centro(ren, cx, 198, 1, COR_TEXTO_FRACO, "PONTOS");
    SDL_snprintf(s, sizeof(s), "%ld", (long)g->pontos);
    texto_sombra(ren, cx - texto_largura(s, 2) / 2, 210, 2, COR_TEXTO, s);

    /* o multiplicador cresce um pouco quando muda, e volta em 250 ms */
    const double desde = g->t_total - g->mult_mudou_t;
    const float pop = desde >= 0.0 && desde < 0.25 ? 1.0f + 0.3f * (float)(1.0 - desde / 0.25) : 1.0f;
    const SDL_Color cm = cor_do_multiplicador(g);
    elipse(ren, cx, 262, 58 * pop, 58 * pop, (SDL_Color){ 0, 0, 0, 170 });
    elipse(ren, cx, 262, 52 * pop, 52 * pop, cor_alfa(cm, 0.25f));
    SDL_snprintf(s, sizeof(s), "x%d", g->multiplicador);
    texto_centro(ren, cx, 251, 3, cm, s);

    texto_centro(ren, cx, 298, 1, COR_TEXTO_FRACO, "COMBO");
    SDL_snprintf(s, sizeof(s), "%d", g->combo);
    texto_centro(ren, cx, 310, 2, COR_TEXTO, s);

    medidor_star(ren, g);
    julgamento(ren, g);
}

/* tela de fim: o resumo da partida (o menu vem por baixo, no Draw_frame) */
static void resultado(SDL_Renderer *ren, const Game *g)
{
    char s[64];
    const int total = g->acertos + g->erros;
    const SDL_FRect p = { 120, 92, LARG - 240, 156 };
    ret_arredondado(ren, p, 14, COR_PAINEL);
    contorno_arredondado(ren, p, 14, cor_alfa(COR_CIANO, 0.7f));

    SDL_snprintf(s, sizeof(s), "%ld pontos", (long)g->pontos);
    texto_centro(ren, LARG / 2, p.y + 18, 3, COR_TEXTO, s);
    SDL_snprintf(s, sizeof(s), "Notas: %d / %d  (%d%%)", g->acertos, total,
                 total ? (100 * g->acertos + total / 2) / total : 0);
    texto_centro(ren, LARG / 2, p.y + 62, 2, COR_TEXTO, s);
    SDL_snprintf(s, sizeof(s), "Maior combo: %d", g->maior_combo);
    texto_centro(ren, LARG / 2, p.y + 92, 2, COR_TEXTO, s);
    texto_centro(ren, LARG / 2, p.y + 126, 1, COR_TEXTO_FRACO, g->erros == 0 && total ? "FULL COMBO!" : g->chart.titulo);
}

static void fps(SDL_Renderer *ren, const Game *g, const Hud *h)
{
    if (!g->opcoes.mostrar_fps) return;
    char s[24];
    SDL_snprintf(s, sizeof(s), "%.0f FPS", h->fps);
    texto(ren, LARG - texto_largura(s, 1) - 8, 6, 1, (SDL_Color){ 255, 230, 80, 255 }, s);
}

/* ---------------------------------------------------------------------------
 * O frame
 * ------------------------------------------------------------------------- */
void Draw_frame(const Draw *d, SDL_Renderer *ren, const Game *g, const Hud *h)
{
    fundo(d, ren, g);

    const Menu *m = Game_menu(g);
    if (g->tela == TELA_JOGO) {
        pista_desenha(ren, g);
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
        if (g->tela == TELA_DIFICULDADE) subtitulo_musica(ren, g, 84);
    } else if (g->tela == TELA_CARREGANDO) {
        tela_carregando(ren, g, h);
    }

    rodape(ren, g, h);
    aviso(ren, g);
    if (h->capturando >= 0) captura(ren, h);
    fps(ren, g, h);
}
