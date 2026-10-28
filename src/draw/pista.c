/* A pista (highway) em perspectiva.
 *
 * Todo ponto da pista é dado por duas coordenadas:
 *   u: atravessando a pista, 0 = borda esquerda, 1 = borda direita
 *   z: ao longo dela, 0 = linha de acerto (embaixo), 1 = fim (em cima)
 *
 * Perspectiva: algo a uma distância maior aparece menor e mais perto do
 * horizonte, na mesma proporção. Com f(z) = 1 / (1 + k*z):
 *   largura(z) = largura_base * f(z)
 *   y(z)       = y_horizonte + (y_base - y_horizonte) * f(z)
 * Uma nota que anda com velocidade constante em z (o tempo) parece acelerar
 * quando chega perto, como numa estrada de verdade.
 *
 * Por enquanto as notas são de demonstração (um padrão fixo a 120 BPM);
 * na etapa do chart elas passam a vir do Game. */

#include "draw/draw_internal.h"

#define PISTA_Y_BASE     422.0f   /* linha de acerto */
#define PISTA_Y_TOPO      40.0f   /* fim visível da pista */
#define PISTA_LARG_BASE  470.0f
#define PISTA_LARG_TOPO  150.0f
#define PISTA_TIRAS       16      /* subdivisões ao longo de z (ARQUITETURA 7.2) */

#define DEMO_BPM        120.0
#define DEMO_INICIO       2.0     /* segundos até a primeira nota de demonstração */

typedef struct {
    float cx, y_base, larg_base;
    float k;       /* quanto a pista "afunda": 1/f(1) - 1 */
    float y_hor;   /* horizonte, acima da tela */
} Pista;

static Pista pista_padrao(void)
{
    const float f1 = PISTA_LARG_TOPO / PISTA_LARG_BASE;   /* escala no topo */
    Pista p;
    p.cx        = 320.0f;
    p.y_base    = PISTA_Y_BASE;
    p.larg_base = PISTA_LARG_BASE;
    p.k         = 1.0f / f1 - 1.0f;
    /* o horizonte é o y onde f = 0; sai de y_topo = y_hor + (y_base - y_hor) * f1 */
    p.y_hor     = (PISTA_Y_TOPO - PISTA_Y_BASE * f1) / (1.0f - f1);
    return p;
}

static float escala(const Pista *p, float z)
{
    return 1.0f / (1.0f + p->k * z);
}

static SDL_FPoint ponto(const Pista *p, float u, float z)
{
    const float f = escala(p, z);
    return (SDL_FPoint){ p->cx + (u - 0.5f) * p->larg_base * f,
                         p->y_hor + (p->y_base - p->y_hor) * f };
}

/* centro da faixa de um traste (0 = verde); no modo canhoto a ordem inverte */
static float u_da_faixa(const Game *g, int traste)
{
    const int coluna = g->opcoes.canhoto ? 4 - traste : traste;
    return (coluna + 0.5f) / 5.0f;
}

/* some suavemente nos últimos 30% da pista */
static float some_no_topo(float z)
{
    const float a = (1.0f - z) / 0.3f;
    return a < 0.0f ? 0.0f : (a > 1.0f ? 1.0f : a);
}

/* ---------------------------------------------------------------------------
 * Faixa entre u0 e u1, da base ao topo, em tiras (para o degradê seguir a
 * perspectiva). brilho = true: forte embaixo e some rápido (traste segurado).
 * ------------------------------------------------------------------------- */
static void faixa(SDL_Renderer *ren, const Pista *p, float u0, float u1, SDL_Color cor, bool brilho)
{
    SDL_Vertex v[2 * (PISTA_TIRAS + 1)];
    int idx[6 * PISTA_TIRAS];

    for (int i = 0; i <= PISTA_TIRAS; i++) {
        const float z = (float)i / PISTA_TIRAS;
        const float a = brilho ? (1.0f - z) * (1.0f - z) : some_no_topo(z);
        const SDL_FColor c = cor_f(cor_alfa(cor, a));
        v[2 * i]     = (SDL_Vertex){ ponto(p, u0, z), c, { 0, 0 } };
        v[2 * i + 1] = (SDL_Vertex){ ponto(p, u1, z), c, { 0, 0 } };
    }
    for (int i = 0; i < PISTA_TIRAS; i++) {
        const int e0 = 2 * i, d0 = e0 + 1, e1 = e0 + 2, d1 = e0 + 3;
        idx[6 * i + 0] = e0; idx[6 * i + 1] = d0; idx[6 * i + 2] = e1;
        idx[6 * i + 3] = e1; idx[6 * i + 4] = d0; idx[6 * i + 5] = d1;
    }
    SDL_RenderGeometry(ren, NULL, v, 2 * (PISTA_TIRAS + 1), idx, 6 * PISTA_TIRAS);
}

/* linha "longitudinal" (borda, divisória): retas continuam retas na
 * perspectiva, então bastam 4 vértices. Some no topo. */
static void trilho(SDL_Renderer *ren, const Pista *p, float u, float largura, SDL_Color cor)
{
    const SDL_FPoint b = ponto(p, u, 0.0f), t = ponto(p, u, 1.0f);
    const float hb = largura / 2.0f, ht = hb * escala(p, 1.0f);
    const SDL_FColor cb = cor_f(cor), ct = cor_f(cor_alfa(cor, 0.0f));
    const SDL_Vertex v[4] = {
        { { b.x - hb, b.y }, cb, { 0, 0 } }, { { b.x + hb, b.y }, cb, { 0, 0 } },
        { { t.x - ht, t.y }, ct, { 0, 0 } }, { { t.x + ht, t.y }, ct, { 0, 0 } },
    };
    const int idx[6] = { 0, 1, 2, 2, 1, 3 };
    SDL_RenderGeometry(ren, NULL, v, 4, idx, 6);
}

/* linha "transversal" (batida, linha de acerto) na profundidade z */
static void travessa(SDL_Renderer *ren, const Pista *p, float z, float altura, float sobra, SDL_Color cor)
{
    const SDL_FPoint e = ponto(p, -sobra, z), d = ponto(p, 1.0f + sobra, z);
    const float h = altura * escala(p, z) / 2.0f;
    ret(ren, (SDL_FRect){ e.x, e.y - h, d.x - e.x, 2.0f * h }, cor);
}

/* ---------------------------------------------------------------------------
 * Notas e alvos
 * ------------------------------------------------------------------------- */
#define NOTA_LARG 0.72f   /* largura da nota, em fração da largura da faixa */

static void nota(SDL_Renderer *ren, const Pista *p, float u, float z, SDL_Color cor)
{
    const float f = escala(p, z);
    const float w = NOTA_LARG * (p->larg_base / 5.0f) * f;
    const float h = w * 0.42f;   /* achatada: estamos olhando a pista de cima */
    const float a = some_no_topo(z);
    const SDL_FPoint c = ponto(p, u, z);

    elipse(ren, c.x, c.y + h * 0.12f, w + 4.0f * f, h + 4.0f * f, cor_alfa((SDL_Color){ 0, 0, 0, 200 }, a));
    elipse(ren, c.x, c.y, w, h, cor_alfa(cor, a));
    elipse(ren, c.x, c.y - h * 0.08f, w * 0.62f, h * 0.55f, cor_alfa(cor_mistura(cor, COR_TEXTO, 0.45f), a));
    elipse(ren, c.x, c.y - h * 0.10f, w * 0.26f, h * 0.24f, cor_alfa(COR_TEXTO, a * 0.85f));
}

static void alvo(SDL_Renderer *ren, const Pista *p, float u, SDL_Color cor, bool segurado)
{
    const float w = 0.80f * (p->larg_base / 5.0f) * (segurado ? 1.06f : 1.0f);
    const float h = w * 0.42f;
    const SDL_FPoint c = ponto(p, u, 0.0f);

    elipse(ren, c.x, c.y + 3.0f, w + 8.0f, h + 6.0f, (SDL_Color){ 0, 0, 0, 170 });
    elipse(ren, c.x, c.y, w, h, segurado ? cor_mistura(cor, COR_TEXTO, 0.25f) : cor);
    if (segurado) {
        elipse(ren, c.x, c.y, w * 0.70f, h * 0.70f, cor_mistura(cor, COR_TEXTO, 0.55f));
    } else {
        elipse(ren, c.x, c.y, w * 0.72f, h * 0.72f, (SDL_Color){ 22, 14, 34, 255 });
        elipse(ren, c.x, c.y, w * 0.30f, h * 0.30f, cor_alfa(cor, 0.5f));
    }
}

/* Padrão de demonstração: 16 batidas, cada número é uma máscara de faixas
 * (bit 0 = verde ... bit 4 = laranja); 0 = pausa. Tem acordes (3, 24). */
static const Uint8 DEMO[16] = { 1, 2, 4, 8, 16, 0, 3, 0, 1, 4, 16, 4, 2, 24, 0, 8 };

static void notas_demo(SDL_Renderer *ren, const Pista *p, const Game *g)
{
    const double batida = 60.0 / DEMO_BPM;
    const double janela = Game_janela_visivel(g);

    /* da mais longe para a mais perto: a de perto fica por cima */
    const int primeira = (int)SDL_floor((g->t - DEMO_INICIO) / batida);
    const int ultima   = (int)SDL_ceil((g->t + janela - DEMO_INICIO) / batida);
    for (int k = ultima; k >= primeira; k--) {
        if (k < 0) break;
        const double t_nota = DEMO_INICIO + k * batida;
        const float z = (float)((t_nota - g->t) / janela);
        if (z < -0.06f || z > 1.0f) continue;

        const Uint8 mascara = DEMO[k % 16];
        for (int i = 0; i < ACAO_N_TRASTES; i++) {
            if (mascara & (1u << i)) nota(ren, p, u_da_faixa(g, i), z, COR_TRASTE[i]);
        }
    }
}

/* ---------------------------------------------------------------------------
 * A pista inteira, de trás para frente
 * ------------------------------------------------------------------------- */
void pista_desenha(SDL_Renderer *ren, const Game *g)
{
    const Pista p = pista_padrao();

    /* 1. o corpo da pista */
    faixa(ren, &p, 0.0f, 1.0f, (SDL_Color){ 12, 8, 24, 230 }, false);

    /* 2. faixas acesas pelos trastes segurados */
    for (int i = 0; i < ACAO_N_TRASTES; i++) {
        if (!(g->segurados & ACAO_BIT(i))) continue;
        const float u = u_da_faixa(g, i);
        faixa(ren, &p, u - 0.1f, u + 0.1f, cor_alfa(COR_TRASTE[i], 0.45f), true);
    }

    /* 3. divisórias e bordas */
    for (int i = 1; i < 5; i++) trilho(ren, &p, i / 5.0f, 2.0f, (SDL_Color){ 255, 255, 255, 50 });
    trilho(ren, &p, 0.0f, 5.0f, COR_ACENTO);
    trilho(ren, &p, 1.0f, 5.0f, COR_ACENTO);

    /* 4. linhas de batida descendo (compasso de 4 mais forte) */
    const double batida = 60.0 / DEMO_BPM;
    const double janela = Game_janela_visivel(g);
    for (int k = (int)SDL_ceil(g->t / batida); ; k++) {
        const float z = (float)((k * batida - g->t) / janela);
        if (z > 1.0f) break;
        const bool compasso = (k % 4) == 0;
        travessa(ren, &p, z, compasso ? 4.0f : 2.0f, 0.0f,
                 cor_alfa((SDL_Color){ 255, 255, 255, compasso ? 120 : 55 }, some_no_topo(z)));
    }

    /* 5. linha de acerto: pisca na palhetada */
    const double desde = g->t - g->t_palheta;
    const float flash = desde < 0.12 ? (float)(1.0 - desde / 0.12) : 0.0f;
    travessa(ren, &p, 0.0f, 6.0f + 6.0f * flash, 0.02f,
             cor_mistura((SDL_Color){ 230, 230, 240, 200 }, COR_CIANO, flash));

    /* 6. alvos, depois as notas por cima */
    for (int i = 0; i < ACAO_N_TRASTES; i++)
        alvo(ren, &p, u_da_faixa(g, i), COR_TRASTE[i], (g->segurados & ACAO_BIT(i)) != 0);
    notas_demo(ren, &p, g);
}
