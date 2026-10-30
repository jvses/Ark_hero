/* Texto com a fonte de pixel do jogo (assets/fonts/).
 *
 * A fonte é desenhada UMA vez, no Draw_init: o stb_truetype rasteriza cada
 * caractere que ela tem numa célula de uma textura (o "atlas"). Depois, cada
 * letra na tela é só uma cópia de um retângulo dessa textura, como uma imagem.
 *
 * A fonte é de pixel, desenhada numa grade de 12x16: no tamanho nativo cada
 * pixel dela cai num pixel da tela, sem meio-tom. Por isso a escala é sempre
 * inteira (1 = 12x16, 2 = 24x32...) e a textura amplia sem suavizar (NEAREST):
 * os pixels só ficam maiores. Ela é monoespaçada: todo caractere avança 12.
 *
 * Sem o arquivo da fonte, o texto sai na fonte de debug da SDL (8x8, só
 * ASCII), esticada para a mesma célula: feio, mas o layout continua certo. */

#include "draw/draw_internal.h"

/* o stb_truetype com a memória e a matemática da SDL (nada de libm a mais) */
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_malloc(x, u)  ((void)(u), SDL_malloc(x))
#define STBTT_free(x, u)    ((void)(u), SDL_free(x))
#define STBTT_ifloor(x)     ((int)SDL_floor(x))
#define STBTT_iceil(x)      ((int)SDL_ceil(x))
#define STBTT_sqrt(x)       SDL_sqrt(x)
#define STBTT_pow(x, y)     SDL_pow(x, y)
#define STBTT_fmod(x, y)    SDL_fmod(x, y)
#define STBTT_cos(x)        SDL_cos(x)
#define STBTT_acos(x)       SDL_acos(x)
#define STBTT_fabs(x)       SDL_fabs(x)
#define STBTT_assert(x)     SDL_assert(x)
#define STBTT_strlen(x)     SDL_strlen(x)
#define STBTT_memcpy        SDL_memcpy
#define STBTT_memset        SDL_memset
#include "stb_truetype.h"

#define ATLAS_COLUNAS 32   /* células por linha da textura */
#define MAX_GLIFOS    1024

/* as faixas do Unicode que vale a pena procurar na fonte: latim (com os
   acentos), grego, cirílico, pontuação, setas, blocos e símbolos */
static const Uint32 FAIXAS[][2] = {
    { 0x0020, 0x007E }, { 0x00A0, 0x024F }, { 0x0370, 0x04FF }, { 0x2000, 0x206F },
    { 0x20A0, 0x20CF }, { 0x2100, 0x21FF }, { 0x2500, 0x27BF },
};

static struct {
    SDL_Texture *atlas;     /* NULL = sem a fonte: usa a de debug */
    Uint32       cp[MAX_GLIFOS];    /* os caracteres que existem, em ordem... */
    Uint16       celula[MAX_GLIFOS];/* ...e a célula de cada um no atlas */
    int          n;
    int          interrogacao;      /* a célula do '?', para o que faltar */
} F;

static int acha(Uint32 cp)
{
    int ini = 0, fim = F.n - 1;
    while (ini <= fim) {
        const int meio = (ini + fim) / 2;
        if (F.cp[meio] == cp) return F.celula[meio];
        if (F.cp[meio] < cp) ini = meio + 1;
        else                 fim = meio - 1;
    }
    return F.interrogacao;
}

bool fonte_init(SDL_Renderer *ren, const char *arquivo)
{
    size_t tam = 0;
    Uint8 *dados = SDL_LoadFile(arquivo, &tam);
    if (!dados) {
        SDL_Log("[fonte] nao li %s: %s (texto na fonte de debug)", arquivo, SDL_GetError());
        return false;
    }
    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, dados, stbtt_GetFontOffsetForIndex(dados, 0))) {
        SDL_Log("[fonte] %s nao e uma fonte TrueType valida", arquivo);
        SDL_free(dados);
        return false;
    }
    const float escala = stbtt_ScaleForPixelHeight(&info, TEXTO_H);
    int ascendente, descendente, espaco;
    stbtt_GetFontVMetrics(&info, &ascendente, &descendente, &espaco);
    const int base = (int)SDL_lroundf((float)ascendente * escala);   /* linha de base, a partir do topo da célula */

    /* quais caracteres a fonte tem (a ordem das faixas já deixa a lista ordenada) */
    F.n = 0;
    for (size_t f = 0; f < SDL_arraysize(FAIXAS); f++)
        for (Uint32 cp = FAIXAS[f][0]; cp <= FAIXAS[f][1] && F.n < MAX_GLIFOS; cp++)
            if (stbtt_FindGlyphIndex(&info, (int)cp)) F.cp[F.n++] = cp;

    const int cw = (int)TEXTO_W, ch = (int)TEXTO_H;
    const int linhas = (F.n + ATLAS_COLUNAS - 1) / ATLAS_COLUNAS;
    const int largura = ATLAS_COLUNAS * cw, altura = (linhas > 0 ? linhas : 1) * ch;
    Uint32 *pixels = SDL_calloc((size_t)largura * (size_t)altura, sizeof(Uint32));
    Uint8  *glifo  = SDL_malloc((size_t)(4 * cw) * (size_t)(4 * ch));   /* folga para um caractere largo */
    if (!pixels || !glifo) {
        SDL_free(pixels); SDL_free(glifo); SDL_free(dados);
        return false;
    }

    F.interrogacao = 0;
    for (int i = 0; i < F.n; i++) {
        F.celula[i] = (Uint16)i;
        if (F.cp[i] == '?') F.interrogacao = i;
        int x0, y0, x1, y1;
        stbtt_GetCodepointBitmapBox(&info, (int)F.cp[i], escala, escala, &x0, &y0, &x1, &y1);
        const int w = SDL_min(x1 - x0, 4 * cw), h = SDL_min(y1 - y0, 4 * ch);
        if (w <= 0 || h <= 0) continue;   /* espaço */
        stbtt_MakeCodepointBitmap(&info, glifo, w, h, w, escala, escala, (int)F.cp[i]);
        /* copia para a célula: a fonte é de pixel, então cada pixel é cheio ou vazio */
        const int cx = (i % ATLAS_COLUNAS) * cw, cy = (i / ATLAS_COLUNAS) * ch;
        for (int y = 0; y < h; y++) {
            const int py = base + y0 + y;
            if (py < 0 || py >= ch) continue;
            for (int x = 0; x < w; x++) {
                const int px = x0 + x;
                if (px < 0 || px >= cw || glifo[y * w + x] < 128) continue;
                pixels[(size_t)(cy + py) * (size_t)largura + (size_t)(cx + px)] = 0xFFFFFFFFu;   /* branco opaco */
            }
        }
    }
    SDL_free(glifo);
    SDL_free(dados);

    F.atlas = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, largura, altura);
    if (F.atlas) {
        SDL_UpdateTexture(F.atlas, NULL, pixels, largura * (int)sizeof(Uint32));
        SDL_SetTextureBlendMode(F.atlas, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(F.atlas, SDL_SCALEMODE_NEAREST);   /* ampliar sem borrar */
    }
    SDL_free(pixels);
    if (!F.atlas) {
        SDL_Log("[fonte] textura da fonte falhou: %s", SDL_GetError());
        return false;
    }
    SDL_Log("[fonte] %s: %d caracteres, atlas %dx%d", arquivo, F.n, largura, altura);
    return true;
}

void fonte_quit(void)
{
    if (F.atlas) SDL_DestroyTexture(F.atlas);
    F.atlas = NULL;
    F.n = 0;
}

/* escala inteira: só assim os pixels da fonte caem certos nos da tela */
static int inteira(float escala)
{
    const int e = (int)SDL_lroundf(escala);
    return e < 1 ? 1 : e;
}

float texto_largura(const char *s, float escala)
{
    return (float)SDL_utf8strlen(s) * TEXTO_W * (float)inteira(escala);   /* caracteres, não bytes */
}

void texto(SDL_Renderer *ren, float x, float y, float escala, SDL_Color cor, const char *s)
{
    const int e = inteira(escala);
    x = SDL_roundf(x);   /* posição inteira: senão o NEAREST "treme" os pixels */
    y = SDL_roundf(y);

    if (!F.atlas) {   /* sem a fonte: a de debug, esticada até a célula de 12x16 */
        const float ex = e * TEXTO_W / 8.0f, ey = e * TEXTO_H / 8.0f;
        SDL_SetRenderDrawColor(ren, cor.r, cor.g, cor.b, cor.a);
        SDL_SetRenderScale(ren, ex, ey);
        SDL_RenderDebugText(ren, x / ex, y / ey, s);
        SDL_SetRenderScale(ren, 1.0f, 1.0f);
        return;
    }

    SDL_SetTextureColorMod(F.atlas, cor.r, cor.g, cor.b);
    SDL_SetTextureAlphaMod(F.atlas, cor.a);
    const float w = TEXTO_W * (float)e, h = TEXTO_H * (float)e;
    const char *p = s;
    for (Uint32 cp; (cp = SDL_StepUTF8(&p, NULL)) != 0; x += w) {
        if (cp == ' ') continue;
        const int c = acha(cp);
        const SDL_FRect src = { (float)(c % ATLAS_COLUNAS) * TEXTO_W, (float)(c / ATLAS_COLUNAS) * TEXTO_H, TEXTO_W, TEXTO_H };
        SDL_RenderTexture(ren, F.atlas, &src, &(SDL_FRect){ x, y, w, h });
    }
}

void texto_centro(SDL_Renderer *ren, float cx, float y, float escala, SDL_Color cor, const char *s)
{
    texto(ren, cx - texto_largura(s, escala) / 2.0f, y, escala, cor, s);
}

void texto_sombra(SDL_Renderer *ren, float x, float y, float escala, SDL_Color cor, const char *s)
{
    const float d = (float)inteira(escala);   /* a sombra anda um pixel da fonte */
    texto(ren, x + d, y + d, escala, cor_alfa((SDL_Color){ 0, 0, 0, 255 }, cor.a / 255.0f * 0.8f), s);
    texto(ren, x, y, escala, cor, s);
}

/* copia s cortado com ".." para caber em "largura" pixels (cortando entre
   caracteres, nunca no meio de um acento em UTF-8) */
void texto_corta(char *destino, size_t n, const char *s, float largura, float escala)
{
    const int cabe = (int)(largura / (TEXTO_W * (float)inteira(escala)));
    SDL_strlcpy(destino, s ? s : "", n);
    if (cabe <= 3 || (int)SDL_utf8strlen(destino) <= cabe) return;
    const char *p = destino;
    for (int i = 0; i < cabe - 2; i++) SDL_StepUTF8(&p, NULL);
    SDL_strlcpy(destino + (p - destino), "..", n - (size_t)(p - destino));
}
