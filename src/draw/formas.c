/* Primitivas de desenho. A SDL só desenha ponto, linha, retângulo e
 * triângulo; o resto é montado aqui, quase sempre com SDL_RenderGeometry. */

#include "draw/draw_internal.h"

/* ---------------------------------------------------------------------------
 * Paleta
 * ------------------------------------------------------------------------- */
const SDL_Color COR_TRASTE[5] = {
    {  60, 200,  80, 255 },   /* verde    */
    { 230,  60,  60, 255 },   /* vermelho */
    { 240, 210,  60, 255 },   /* amarelo  */
    {  60, 130, 240, 255 },   /* azul     */
    { 245, 140,  40, 255 },   /* laranja  */
};
const SDL_Color COR_ACENTO       = { 255,  70, 190, 255 };
const SDL_Color COR_CIANO        = {  80, 225, 255, 255 };
const SDL_Color COR_PAINEL       = {  18,   8,  36, 210 };
const SDL_Color COR_TEXTO        = { 245, 240, 255, 255 };
const SDL_Color COR_TEXTO_FRACO  = { 150, 135, 175, 255 };
const SDL_Color COR_TEXTO_ESCURO = {  25,  10,  35, 255 };

SDL_Color cor_alfa(SDL_Color c, float alfa)
{
    if (alfa < 0.0f) alfa = 0.0f;
    if (alfa > 1.0f) alfa = 1.0f;
    c.a = (Uint8)(c.a * alfa);
    return c;
}

SDL_Color cor_mistura(SDL_Color a, SDL_Color b, float t)
{
    return (SDL_Color){
        (Uint8)(a.r + (b.r - a.r) * t),
        (Uint8)(a.g + (b.g - a.g) * t),
        (Uint8)(a.b + (b.b - a.b) * t),
        (Uint8)(a.a + (b.a - a.a) * t),
    };
}

SDL_FColor cor_f(SDL_Color c)
{
    return (SDL_FColor){ c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f };
}

/* ---------------------------------------------------------------------------
 * Polígono convexo preenchido: um "leque" de triângulos saindo do ponto 0.
 * Serve para qualquer forma convexa (elipse, retângulo arredondado...).
 * ------------------------------------------------------------------------- */
#define CONVEXO_MAX 64

static void convexo(SDL_Renderer *ren, const SDL_FPoint *p, int n, SDL_Color cor)
{
    SDL_Vertex v[CONVEXO_MAX];
    int idx[(CONVEXO_MAX - 2) * 3];
    if (n < 3 || n > CONVEXO_MAX) return;

    const SDL_FColor fc = cor_f(cor);
    for (int i = 0; i < n; i++) {
        v[i].position  = p[i];
        v[i].color     = fc;
        v[i].tex_coord = (SDL_FPoint){ 0.0f, 0.0f };
    }
    for (int i = 0; i < n - 2; i++) {   /* triângulos (0, i+1, i+2) */
        idx[3 * i]     = 0;
        idx[3 * i + 1] = i + 1;
        idx[3 * i + 2] = i + 2;
    }
    SDL_RenderGeometry(ren, NULL, v, n, idx, (n - 2) * 3);
}

/* ---------------------------------------------------------------------------
 * Elipse: centro (cx, cy), largura w, altura h. Círculo quando w == h.
 * Forma paramétrica: x = cx + a*cos(θ), y = cy + b*sin(θ).
 * ------------------------------------------------------------------------- */
#define ELIPSE_SEGMENTOS 40   /* pontos na borda; de 32 a 64 já fica liso em 640x480 */

void elipse(SDL_Renderer *ren, float cx, float cy, float w, float h, SDL_Color cor)
{
    SDL_FPoint p[ELIPSE_SEGMENTOS];
    const float a = w / 2.0f;   /* semi-eixo horizontal */
    const float b = h / 2.0f;   /* semi-eixo vertical */

    for (int i = 0; i < ELIPSE_SEGMENTOS; i++) {
        const float teta = 2.0f * SDL_PI_F * (float)i / (float)ELIPSE_SEGMENTOS;
        p[i] = (SDL_FPoint){ cx + a * SDL_cosf(teta), cy + b * SDL_sinf(teta) };
    }
    convexo(ren, p, ELIPSE_SEGMENTOS, cor);
}

/* ---------------------------------------------------------------------------
 * Retângulos
 * ------------------------------------------------------------------------- */
void ret(SDL_Renderer *ren, SDL_FRect r, SDL_Color cor)
{
    SDL_SetRenderDrawColor(ren, cor.r, cor.g, cor.b, cor.a);
    SDL_RenderFillRect(ren, &r);
}

#define CANTO_SEGMENTOS 6   /* pontos por canto arredondado */

/* Contorno de um retângulo arredondado: 4 quartos de círculo, um por canto,
 * percorridos no sentido horário a partir do canto superior direito. */
static int pontos_arredondado(SDL_FRect r, float raio, SDL_FPoint *p)
{
    const float max = (r.w < r.h ? r.w : r.h) / 2.0f;
    if (raio > max) raio = max;

    const SDL_FPoint centros[4] = {
        { r.x + r.w - raio, r.y + raio },         /* superior direito */
        { r.x + r.w - raio, r.y + r.h - raio },   /* inferior direito */
        { r.x + raio,       r.y + r.h - raio },   /* inferior esquerdo */
        { r.x + raio,       r.y + raio },         /* superior esquerdo */
    };
    int n = 0;
    for (int c = 0; c < 4; c++) {
        const float inicio = -SDL_PI_F / 2.0f + c * SDL_PI_F / 2.0f;   /* -90°, 0°, 90°, 180° */
        for (int i = 0; i <= CANTO_SEGMENTOS; i++) {
            const float teta = inicio + (SDL_PI_F / 2.0f) * (float)i / (float)CANTO_SEGMENTOS;
            p[n++] = (SDL_FPoint){ centros[c].x + raio * SDL_cosf(teta),
                                   centros[c].y + raio * SDL_sinf(teta) };
        }
    }
    return n;   /* 4 * (CANTO_SEGMENTOS + 1) */
}

void ret_arredondado(SDL_Renderer *ren, SDL_FRect r, float raio, SDL_Color cor)
{
    SDL_FPoint p[4 * (CANTO_SEGMENTOS + 1)];
    const int n = pontos_arredondado(r, raio, p);
    convexo(ren, p, n, cor);
}

void contorno_arredondado(SDL_Renderer *ren, SDL_FRect r, float raio, SDL_Color cor)
{
    SDL_FPoint p[4 * (CANTO_SEGMENTOS + 1) + 1];
    const int n = pontos_arredondado(r, raio, p);
    p[n] = p[0];   /* fecha a volta */
    SDL_SetRenderDrawColor(ren, cor.r, cor.g, cor.b, cor.a);
    SDL_RenderLines(ren, p, n + 1);
}

/* degradê vertical: 4 vértices, a GPU interpola a cor entre eles */
void ret_degrade(SDL_Renderer *ren, SDL_FRect r, SDL_Color topo, SDL_Color base)
{
    const SDL_FColor ct = cor_f(topo), cb = cor_f(base);
    const SDL_Vertex v[4] = {
        { { r.x,       r.y },       ct, { 0, 0 } },
        { { r.x + r.w, r.y },       ct, { 0, 0 } },
        { { r.x,       r.y + r.h }, cb, { 0, 0 } },
        { { r.x + r.w, r.y + r.h }, cb, { 0, 0 } },
    };
    const int idx[6] = { 0, 1, 2, 2, 1, 3 };
    SDL_RenderGeometry(ren, NULL, v, 4, idx, 6);
}

/* ---------------------------------------------------------------------------
 * Texto
 *
 * SDL_RenderDebugText desenha 8x8 px por caractere. Para aumentar, a escala do
 * renderer multiplica TUDO, inclusive as coordenadas: por isso x e y são
 * divididos pela escala antes, e a escala volta a 1 logo depois.
 * ------------------------------------------------------------------------- */
float texto_largura(const char *s, float escala)
{
    return (float)SDL_utf8strlen(s) * TEXTO_PX * escala;   /* caracteres, não bytes */
}

void texto(SDL_Renderer *ren, float x, float y, float escala, SDL_Color cor, const char *s)
{
    SDL_SetRenderDrawColor(ren, cor.r, cor.g, cor.b, cor.a);
    if (escala == 1.0f) {
        SDL_RenderDebugText(ren, x, y, s);
        return;
    }
    SDL_SetRenderScale(ren, escala, escala);
    SDL_RenderDebugText(ren, x / escala, y / escala, s);
    SDL_SetRenderScale(ren, 1.0f, 1.0f);
}

void texto_centro(SDL_Renderer *ren, float cx, float y, float escala, SDL_Color cor, const char *s)
{
    texto(ren, cx - texto_largura(s, escala) / 2.0f, y, escala, cor, s);
}

void texto_sombra(SDL_Renderer *ren, float x, float y, float escala, SDL_Color cor, const char *s)
{
    texto(ren, x + escala, y + escala, escala, cor_alfa((SDL_Color){ 0, 0, 0, 255 }, cor.a / 255.0f * 0.8f), s);
    texto(ren, x, y, escala, cor, s);
}
