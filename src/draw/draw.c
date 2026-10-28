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
    [TELA_INICIO]    = IMAGEM_INICIO,
    [TELA_CONFIG]    = IMAGEM_CONFIG,
    [TELA_CONTROLES] = IMAGEM_CONFIG,
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
        const float y = g->tela == TELA_CONTROLES ? 26 : (g->tela == TELA_JOGO ? 120 : 40);
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
    texto(ren, r.x + 18, y_texto, MENU_ESCALA, c, item->rotulo);

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
    const float lin_h = 36, lin_esp = 6;
    const float painel_w = 480;
    const float painel_h = m->n * (lin_h + lin_esp) + 24;
    const float y0 = g->tela == TELA_INICIO ? 140 : (g->tela == TELA_JOGO ? 170 : 110);
    const SDL_FRect painel = { (LARG - painel_w) / 2, y0, painel_w, painel_h };

    ret_arredondado(ren, painel, 14, COR_PAINEL);
    contorno_arredondado(ren, painel, 14, cor_alfa(COR_ACENTO, 0.6f));

    for (int i = 0; i < m->n; i++) {
        const SDL_FRect r = { painel.x + 12, painel.y + 12 + i * (lin_h + lin_esp), painel.w - 24, lin_h };
        char valor[32];
        Menu_valor(g, m->itens[i].id, valor, sizeof(valor));
        linha(ren, g, r, &m->itens[i], i == sel, valor, 0, NULL, 0);
    }
}

/* A tela de controles: cada ação com o botão do controle e a tecla. */
static void menu_controles(SDL_Renderer *ren, const Game *g, const Hud *h, const Menu *m, int sel)
{
    const float lin_h = 27, lin_esp = 3;
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
    if (g->tela == TELA_JOGO && !g->pausado) {
        SDL_snprintf(dica, sizeof(dica), "[%s/%s] pausa   [Select+Start] sair",
                     h->pad[ACAO_PAUSA], h->tecla[ACAO_PAUSA]);
    } else {
        SDL_snprintf(dica, sizeof(dica), "[%s/%s] confirmar   [%s/%s] voltar   [Esq/Dir] ajustar",
                     h->pad[ACAO_CONFIRMA], h->tecla[ACAO_CONFIRMA], h->pad[ACAO_VOLTA], h->tecla[ACAO_VOLTA]);
    }
    ret_degrade(ren, (SDL_FRect){ 0, ALT - 26, LARG, 26 }, (SDL_Color){ 0, 0, 0, 0 }, (SDL_Color){ 0, 0, 0, 200 });
    texto_centro(ren, LARG / 2, ALT - 14, 1, COR_TEXTO_FRACO, dica);
}

static void hud_jogo(SDL_Renderer *ren, const Game *g)
{
    texto_sombra(ren, 12, 10, 2, COR_TEXTO, "PISTA DE TESTE");
    texto(ren, 12, 32, 1, COR_TEXTO_FRACO, "notas de demonstracao, sem musica");

    char t[32];
    SDL_snprintf(t, sizeof(t), "%5.1f s", g->t);
    texto(ren, 12, 46, 1, COR_TEXTO_FRACO, t);
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
        if (g->pausado) {
            escurece(ren, 0.55f);
            titulo(ren, g, m);
            menu_lista(ren, g, m, Game_menu_sel(g));
        }
    } else if (m) {
        titulo(ren, g, m);
        if (g->tela == TELA_CONTROLES) menu_controles(ren, g, h, m, Game_menu_sel(g));
        else                           menu_lista(ren, g, m, Game_menu_sel(g));
    }

    rodape(ren, g, h);
    aviso(ren, g);
    if (h->capturando >= 0) captura(ren, h);
    fps(ren, g, h);
}
