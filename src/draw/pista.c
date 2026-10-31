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
 * Tudo aqui é função do estado do jogo (g->t, notas, último acerto...):
 * nenhuma animação guarda estado próprio (ARQUITETURA 7.5 e 7.6).
 *
 * Cada jogador tem a sua pista (j). Com mais de um, elas encolhem e ficam lado
 * a lado; os tamanhos em pixels fixos (sombras, chamas) encolhem junto (s). */

#include "draw/draw_internal.h"

#define PISTA_Y_BASE     PISTA_Y_ACERTO
#define PISTA_Y_TOPO      40.0f   /* fim visível da pista */
#define PISTA_LARG_BASE  470.0f   /* com um jogador */
#define PISTA_LARG_TOPO  150.0f
#define TELA_LARG        640.0f
#define PISTA_TIRAS       16      /* subdivisões ao longo de z (ARQUITETURA 7.2) */
#define CAUDA_TIRAS       24      /* subdivisões de um sustain (para ondular) */

#define EXPLOSAO_DURACAO  0.30    /* segundos da animação de acerto */

static const SDL_Color COR_APAGADA = { 110, 110, 120, 255 };   /* nota perdida */
static const SDL_Color COR_ABERTA  = { 175,  80, 255, 255 };   /* nota aberta (sem traste) */

typedef struct {
    float cx, y_base, larg_base;
    float k;       /* quanto a pista "afunda": 1/f(1) - 1 */
    float y_hor;   /* horizonte, acima da tela */
    float s;       /* tamanho perto da pista de um jogador só (1 = igual) */
} Pista;

/* larguras na base e no topo, por número de jogadores: cada pista ocupa
   quase toda a sua fatia da tela (640 / n) */
static const float LARG_BASE[5] = { PISTA_LARG_BASE, PISTA_LARG_BASE, 300.0f, 205.0f, 155.0f };
static const float LARG_TOPO[5] = { PISTA_LARG_TOPO, PISTA_LARG_TOPO, 100.0f,  70.0f,  52.0f };

static int limita_n(int n)
{
    return n < 1 ? 1 : (n > 4 ? 4 : n);
}

float pista_centro(int pos, int n)
{
    n = limita_n(n);
    return TELA_LARG * ((float)pos + 0.5f) / (float)n;   /* 1: 320; 2: 160 e 480; 4: 80, 240... */
}

float pista_largura(int n)
{
    return LARG_BASE[limita_n(n)];
}

static Pista monta_pista(int pos, int n)
{
    n = limita_n(n);
    const float f1 = LARG_TOPO[n] / LARG_BASE[n];   /* escala no topo */
    Pista p;
    p.cx        = pista_centro(pos, n);
    p.y_base    = PISTA_Y_BASE;
    p.larg_base = LARG_BASE[n];
    p.k         = 1.0f / f1 - 1.0f;
    /* o horizonte é o y onde f = 0; sai de y_topo = y_hor + (y_base - y_hor) * f1 */
    p.y_hor     = (PISTA_Y_TOPO - PISTA_Y_BASE * f1) / (1.0f - f1);
    p.s         = LARG_BASE[n] / PISTA_LARG_BASE;
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

/* largura de uma faixa (1/5 da pista) na linha de acerto */
static float largura_faixa(const Pista *p)
{
    return p->larg_base / 5.0f;
}

/* centro da faixa de um traste (0 = verde); no modo canhoto a ordem inverte */
static float u_da_faixa(const Game *g, int traste)
{
    const int coluna = g->opcoes.canhoto ? 4 - traste : traste;
    return (coluna + 0.5f) / 5.0f;
}

/* tempo da música -> profundidade na pista */
static float z_do_tempo(const Game *g, double t)
{
    return (float)((t - g->t) / Game_janela_visivel(g));
}

/* some suavemente nos últimos 30% da pista */
static float some_no_topo(float z)
{
    const float a = (1.0f - z) / 0.3f;
    return a < 0.0f ? 0.0f : (a > 1.0f ? 1.0f : a);
}

/* ---------------------------------------------------------------------------
 * Corpo, linhas
 * ------------------------------------------------------------------------- */

/* o corpo da pista, em tiras (para o degradê do topo seguir a perspectiva) */
static void corpo(SDL_Renderer *ren, const Pista *p, SDL_Color cor)
{
    SDL_Vertex v[2 * (PISTA_TIRAS + 1)];
    int idx[6 * PISTA_TIRAS];

    for (int i = 0; i <= PISTA_TIRAS; i++) {
        const float z = (float)i / PISTA_TIRAS;
        const SDL_FColor c = cor_f(cor_alfa(cor, some_no_topo(z)));
        v[2 * i]     = (SDL_Vertex){ ponto(p, 0.0f, z), c, { 0, 0 } };
        v[2 * i + 1] = (SDL_Vertex){ ponto(p, 1.0f, z), c, { 0, 0 } };
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

static void batidas(SDL_Renderer *ren, const Pista *p, const Game *g, const Jogador *j)
{
    for (int k = 0; k < j->chart.n_batidas; k++) {
        const float z = z_do_tempo(g, j->chart.batidas[k].t);
        if (z < 0.0f) continue;
        if (z > 1.0f) break;
        const bool forte = j->chart.batidas[k].forte;
        travessa(ren, p, z, forte ? 4.0f : 2.0f, 0.0f,
                 cor_alfa((SDL_Color){ 255, 255, 255, forte ? 120 : 55 }, some_no_topo(z)));
    }
}

/* ---------------------------------------------------------------------------
 * Sustain: uma tira que segue a faixa de t0 a t1. Enquanto está sendo
 * segurado, ondula com o whammy.
 * ------------------------------------------------------------------------- */
static void cauda(SDL_Renderer *ren, const Pista *p, const Game *g, const Jogador *j, float u, double t0, double t1,
                  SDL_Color cor, bool viva)
{
    float z0 = z_do_tempo(g, t0), z1 = z_do_tempo(g, t1);
    if (z0 < 0.0f && viva) z0 = 0.0f;      /* segurando: a cauda sai da linha de acerto */
    if (z0 < -0.05f) z0 = -0.05f;
    if (z1 > 1.0f) z1 = 1.0f;
    if (z1 <= z0) return;

    SDL_Vertex v[2 * (CAUDA_TIRAS + 1)];
    int idx[6 * CAUDA_TIRAS];
    const float meia = 0.035f;   /* meia largura da cauda, em u (fração da pista) */
    const float onda = viva ? 0.025f * j->whammy : 0.0f;

    for (int i = 0; i <= CAUDA_TIRAS; i++) {
        const float z  = z0 + (z1 - z0) * (float)i / CAUDA_TIRAS;
        const float du = onda * SDL_sinf(z * 40.0f - (float)g->t * 25.0f);
        const SDL_FColor c = cor_f(cor_alfa(cor, some_no_topo(z)));
        v[2 * i]     = (SDL_Vertex){ ponto(p, u - meia + du, z), c, { 0, 0 } };
        v[2 * i + 1] = (SDL_Vertex){ ponto(p, u + meia + du, z), c, { 0, 0 } };
    }
    for (int i = 0; i < CAUDA_TIRAS; i++) {
        const int e0 = 2 * i, d0 = e0 + 1, e1 = e0 + 2, d1 = e0 + 3;
        idx[6 * i + 0] = e0; idx[6 * i + 1] = d0; idx[6 * i + 2] = e1;
        idx[6 * i + 3] = e1; idx[6 * i + 4] = d0; idx[6 * i + 5] = d1;
    }
    SDL_RenderGeometry(ren, NULL, v, 2 * (CAUDA_TIRAS + 1), idx, 6 * CAUDA_TIRAS);
}

/* Voltando da pausa, a pista mostra de novo o que já tinha sido tocado, como
   um "fantasma" para tocar junto: as notas acertadas, apagadas, até o ponto
   em que o jogo tinha parado. */
#define FANTASMA 0.35f

static bool fantasma(const Game *g, const Nota *n)
{
    return n->estado == NOTA_ACERTADA && g->t < g->livre_ate && n->t > g->t - 0.05;
}

static void caudas(SDL_Renderer *ren, const Pista *p, const Game *g, const Jogador *j, int ultima)
{
    for (int i = j->primeira_visivel; i <= ultima; i++) {
        const Nota *n = &j->chart.notas[i];
        if (n->duracao <= 0.0) continue;
        const double fim = n->t + n->duracao;

        /* aberta: uma cauda só, no meio */
        const bool aberta = n->flags & NOTA_ABERTA;
        for (int f = 0; f < ACAO_N_TRASTES; f++) {
            if (aberta ? f != 2 : !(n->mascara & (1u << f))) continue;
            const float u = u_da_faixa(g, f);
            const SDL_Color c = (n->flags & NOTA_STAR) ? COR_CIANO : (aberta ? COR_ABERTA : COR_TRASTE[f]);

            if (n->estado == NOTA_PENDENTE) {
                cauda(ren, p, g, j, u, n->t, fim, cor_alfa(c, 0.75f), false);
            } else if (n->estado == NOTA_PERDIDA) {
                cauda(ren, p, g, j, u, n->t, fim, cor_alfa(COR_APAGADA, 0.5f), false);
            } else if (n->segurando && n->t > g->t) {   /* voltando da pausa: o sustain ainda vem aí */
                cauda(ren, p, g, j, u, n->t, fim, cor_mistura(c, COR_TEXTO, 0.35f), false);
            } else if (n->segurando) {
                cauda(ren, p, g, j, u, g->t, fim, cor_mistura(c, COR_TEXTO, 0.35f), true);
            } else if (fantasma(g, n)) {
                cauda(ren, p, g, j, u, n->t, n->solto, cor_alfa(c, FANTASMA), false);
                if (n->solto < fim) cauda(ren, p, g, j, u, n->solto, fim, cor_alfa(COR_APAGADA, 0.5f), false);
            } else if (n->solto < fim) {   /* soltou antes do fim: o resto passa apagado */
                cauda(ren, p, g, j, u, n->solto, fim, cor_alfa(COR_APAGADA, 0.5f), false);
            }
        }
    }
}

/* ---------------------------------------------------------------------------
 * Notas, alvos, efeitos
 * ------------------------------------------------------------------------- */
#define NOTA_LARG 0.72f   /* largura da nota, em fração da largura da faixa */

/* palheta = nota comum (precisaria de palhetada): miolo preto com o centro
 * branco, como no Guitar Hero. HOPO e tap (não precisam de palhetada): o
 * miolo colorido. No modo controle todas se tocam igual; é a informação de
 * como a música foi escrita, e vale para o modo guitarra. */
static void nota(SDL_Renderer *ren, const Pista *p, float u, float z, SDL_Color cor, bool star, bool palheta)
{
    const float f = escala(p, z);
    const float w = NOTA_LARG * largura_faixa(p) * f;
    const float h = w * 0.42f;   /* achatada: estamos olhando a pista de cima */
    const float a = some_no_topo(z) * cor.a / 255.0f;   /* o alfa da cor vale para a nota toda */
    cor.a = 255;
    const SDL_FPoint c = ponto(p, u, z);

    elipse(ren, c.x, c.y + h * 0.12f, w + 4.0f * f, h + 4.0f * f, cor_alfa((SDL_Color){ 0, 0, 0, 200 }, a));
    if (star) {   /* nota de star power: anel ciano e miolo quase branco */
        elipse(ren, c.x, c.y, w + 3.0f * f, h + 3.0f * f, cor_alfa(COR_CIANO, a));
        cor = cor_mistura(cor, COR_TEXTO, 0.55f);
    }
    elipse(ren, c.x, c.y, w, h, cor_alfa(cor, a));
    if (palheta) {
        elipse(ren, c.x, c.y - h * 0.04f, w * 0.60f, h * 0.58f, cor_alfa((SDL_Color){ 15, 10, 20, 255 }, a));
        elipse(ren, c.x, c.y - h * 0.06f, w * 0.36f, h * 0.34f, cor_alfa(COR_TEXTO, a));
    } else {
        elipse(ren, c.x, c.y - h * 0.08f, w * 0.62f, h * 0.55f, cor_alfa(cor_mistura(cor, COR_TEXTO, 0.45f), a));
        elipse(ren, c.x, c.y - h * 0.10f, w * 0.26f, h * 0.24f, cor_alfa(COR_TEXTO, a * 0.85f));
    }
}

/* nota aberta: uma barra que atravessa as cinco faixas */
static void nota_aberta(SDL_Renderer *ren, const Pista *p, float z, SDL_Color cor, bool star)
{
    const float f = escala(p, z);
    const float a = some_no_topo(z) * cor.a / 255.0f;
    cor.a = 255;
    const SDL_FPoint e = ponto(p, 0.06f, z), d = ponto(p, 0.94f, z);
    const float h = 12.0f * f * p->s;
    const SDL_FRect r = { e.x, e.y - h / 2, d.x - e.x, h };
    ret_arredondado(ren, (SDL_FRect){ r.x - 2, r.y + 1, r.w + 4, r.h + 3 }, h / 2, cor_alfa((SDL_Color){ 0, 0, 0, 200 }, a));
    ret_arredondado(ren, r, h / 2, cor_alfa(star ? cor_mistura(cor, COR_TEXTO, 0.5f) : cor, a));
    ret_arredondado(ren, (SDL_FRect){ r.x + 6 * p->s, r.y + h * 0.2f, r.w - 12 * p->s, h * 0.3f }, h * 0.15f,
                    cor_alfa(COR_TEXTO, 0.6f * a));
}

static void notas(SDL_Renderer *ren, const Pista *p, const Game *g, const Jogador *j, int ultima)
{
    /* da mais longe para a mais perto: a de perto fica por cima */
    for (int i = ultima; i >= j->primeira_visivel; i--) {
        const Nota *n = &j->chart.notas[i];
        const bool sombra = fantasma(g, n);
        if (n->estado == NOTA_ACERTADA && !sombra) continue;   /* acertada some (vira explosão) */
        const float z = z_do_tempo(g, n->t);
        if (z < -0.08f || z > 1.0f) continue;

        const bool  perdida = n->estado == NOTA_PERDIDA;
        const float alfa    = sombra ? FANTASMA : 1.0f;
        if (n->flags & NOTA_ABERTA) {
            nota_aberta(ren, p, z, perdida ? cor_alfa(COR_APAGADA, 0.7f) : cor_alfa(COR_ABERTA, alfa),
                        !perdida && !sombra && (n->flags & NOTA_STAR));
            continue;
        }
        for (int f = 0; f < ACAO_N_TRASTES; f++) {
            if (!(n->mascara & (1u << f))) continue;
            nota(ren, p, u_da_faixa(g, f), z, perdida ? cor_alfa(COR_APAGADA, 0.7f) : cor_alfa(COR_TRASTE[f], alfa),
                 !perdida && !sombra && (n->flags & NOTA_STAR),
                 !(n->flags & (NOTA_HOPO | NOTA_TAP)));   /* palhetada? */
        }
    }
}

/* O alvo na linha de acerto. Segurado: "afunda" e acende. */
static void alvo(SDL_Renderer *ren, const Pista *p, float u, SDL_Color cor, bool segurado)
{
    const float w = 0.80f * largura_faixa(p) * (segurado ? 0.94f : 1.0f);
    const float h = w * 0.42f;
    const SDL_FPoint c = ponto(p, u, 0.0f);
    const float dy = segurado ? 2.0f * p->s : 0.0f;

    elipse(ren, c.x, c.y + 3.0f * p->s, w + 8.0f * p->s, h + 6.0f * p->s, (SDL_Color){ 0, 0, 0, 170 });
    elipse(ren, c.x, c.y + dy, w, h, segurado ? cor_mistura(cor, COR_TEXTO, 0.25f) : cor);
    if (segurado) {
        elipse(ren, c.x, c.y + dy, w * 0.70f, h * 0.70f, cor_mistura(cor, COR_TEXTO, 0.60f));
    } else {
        elipse(ren, c.x, c.y, w * 0.72f, h * 0.72f, (SDL_Color){ 22, 14, 34, 255 });
        elipse(ren, c.x, c.y, w * 0.30f, h * 0.30f, cor_alfa(cor, 0.5f));
    }
}

/* Acerto: um anel que se abre e uma chama que sobe, por EXPLOSAO_DURACAO. */
static void explosao(SDL_Renderer *ren, const Pista *p, float u, SDL_Color cor, double idade)
{
    const float k = (float)(idade / EXPLOSAO_DURACAO);   /* 0 -> 1 */
    const float w = 0.80f * largura_faixa(p);
    const SDL_FPoint c = ponto(p, u, 0.0f);

    elipse(ren, c.x, c.y, w * (1.0f + 0.9f * k), w * 0.42f * (1.0f + 0.9f * k),
           cor_alfa(cor_mistura(cor, COR_TEXTO, 0.3f), 0.55f * (1.0f - k)));
    for (int i = 0; i < 3; i++) {
        const float s = 1.0f - 0.25f * i;
        elipse(ren, c.x, c.y - (10.0f + 45.0f * k * s + 8.0f * i) * p->s, w * 0.55f * s * (1.0f - 0.6f * k),
               w * 0.40f * s * (1.0f - 0.4f * k), cor_alfa(cor_mistura(cor, COR_TEXTO, 0.2f * i), 0.8f * (1.0f - k)));
    }
}

/* sustain segurado: o alvo "queima" pulsando enquanto durar */
static void chama_sustain(SDL_Renderer *ren, const Pista *p, const Game *g, float u, SDL_Color cor)
{
    const float pulso = 0.5f + 0.5f * SDL_sinf((float)g->t * 30.0f);
    const float w = 0.80f * largura_faixa(p);
    const SDL_FPoint c = ponto(p, u, 0.0f);
    elipse(ren, c.x, c.y - 6.0f * p->s, w * (0.9f + 0.15f * pulso), w * 0.5f, cor_alfa(cor, 0.35f + 0.25f * pulso));
    elipse(ren, c.x, c.y - (14.0f + 6.0f * pulso) * p->s, w * 0.35f, w * 0.30f, cor_alfa(COR_TEXTO, 0.5f));
}

static void efeitos(SDL_Renderer *ren, const Pista *p, const Game *g, const Jogador *j, int ultima)
{
    uint8_t sustentando = 0;   /* faixas com sustain sendo segurado */
    for (int i = j->primeira_visivel; i <= ultima; i++)
        if (j->chart.notas[i].segurando && j->chart.notas[i].t <= g->t)
            sustentando |= (j->chart.notas[i].flags & NOTA_ABERTA) ? 0x04 : j->chart.notas[i].mascara;

    for (int f = 0; f < ACAO_N_TRASTES; f++) {
        const float u = u_da_faixa(g, f);
        if (sustentando & (1u << f)) chama_sustain(ren, p, g, u, COR_TRASTE[f]);
        const double idade = g->t - j->acerto_t[f];
        if (idade >= 0.0 && idade < EXPLOSAO_DURACAO) explosao(ren, p, u, COR_TRASTE[f], idade);
    }
}

/* ---------------------------------------------------------------------------
 * A pista inteira, de trás para frente
 * ------------------------------------------------------------------------- */
void pista_desenha(SDL_Renderer *ren, const Game *g, const Jogador *j, int pos, int n)
{
    const Pista p = monta_pista(pos, n);
    const int k = (int)(j - g->jog);
    const SDL_Color borda = j->sp_ativo ? COR_CIANO : COR_JOGADOR[k >= 0 && k < 4 ? k : 0];

    /* última nota que aparece (o começo é j->primeira_visivel, mantido pelo jogo) */
    const double limite = g->t + Game_janela_visivel(g);
    int ultima = j->primeira_visivel - 1;
    while (ultima + 1 < j->chart.n && j->chart.notas[ultima + 1].t <= limite) ultima++;

    /* 1. corpo (azulado com o star power ativo), divisórias, bordas */
    corpo(ren, &p, j->sp_ativo ? (SDL_Color){ 10, 28, 60, 235 } : (SDL_Color){ 12, 8, 24, 230 });
    for (int i = 1; i < 5; i++) trilho(ren, &p, i / 5.0f, 2.0f, (SDL_Color){ 255, 255, 255, 50 });
    trilho(ren, &p, 0.0f, 5.0f * p.s, borda);
    trilho(ren, &p, 1.0f, 5.0f * p.s, borda);

    /* 2. batidas descendo e a linha de acerto */
    batidas(ren, &p, g, j);
    travessa(ren, &p, 0.0f, 6.0f * p.s, 0.02f, cor_alfa(j->sp_ativo ? COR_CIANO : COR_TEXTO, 0.8f));

    /* 3. caudas dos sustains, alvos, notas por cima, efeitos por cima de tudo */
    caudas(ren, &p, g, j, ultima);
    for (int i = 0; i < ACAO_N_TRASTES; i++)
        alvo(ren, &p, u_da_faixa(g, i), COR_TRASTE[i], (j->segurados & ACAO_BIT(i)) != 0);
    notas(ren, &p, g, j, ultima);
    efeitos(ren, &p, g, j, ultima);
}
