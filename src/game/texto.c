#include "game/texto.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *pt;   /* a chave: o texto em português */
    const char *tr;   /* a tradução */
} Par;

static char *buffer;   /* o arquivo inteiro: as chaves e traduções apontam para dentro dele */
static Par  *pares;    /* ordenados pela chave: busca binária */
static int   n_pares;

/* ---------------------------------------------------------------------------
 * Cópia segura de um texto para a tela. UTF-8 válido passa como está; um byte
 * que não forma UTF-8 é tratado como Latin-1 (song.ini antigos gravam "é"
 * como o byte 0xE9) e convertido. O corte, se faltar espaço, nunca parte um
 * caractere no meio.
 * ------------------------------------------------------------------------- */
static bool continua(unsigned char c)
{
    return (c & 0xC0) == 0x80;
}

/* quantos bytes tem o caractere UTF-8 válido que começa em p (0 = não é UTF-8) */
static int tamanho_utf8(const unsigned char *p)
{
    if (p[0] < 0x80) return 1;
    if (p[0] >= 0xC2 && p[0] <= 0xDF && continua(p[1])) return 2;
    if ((p[0] & 0xF0) == 0xE0 && continua(p[1]) && continua(p[2])) return 3;
    if (p[0] >= 0xF0 && p[0] <= 0xF4 && continua(p[1]) && continua(p[2]) && continua(p[3])) return 4;
    return 0;
}

void Texto_copia(char *destino, size_t max, const char *origem)
{
    size_t k = 0;
    const unsigned char *p = (const unsigned char *)origem;
    while (*p) {
        const int n = tamanho_utf8(p);
        if (n > 0) {
            if (k + (size_t)n >= max) break;
            memcpy(destino + k, p, (size_t)n);
            k += (size_t)n;
            p += n;
        } else {                                        /* Latin-1 -> UTF-8: dois bytes */
            if (k + 2 >= max) break;
            destino[k++] = (char)(0xC0 | (*p >> 6));
            destino[k++] = (char)(0x80 | (*p & 0x3F));
            p++;
        }
    }
    destino[k] = '\0';
}

/* ---------------------------------------------------------------------------
 * Os %d / %s de um texto, em ordem e sem largura nem precisão ("%-8s" e "%s"
 * valem o mesmo: o que importa é o tipo do argumento). "%%" não conta.
 * ------------------------------------------------------------------------- */
static void formatos(const char *s, char *saida, size_t n)
{
    size_t k = 0;
    for (; *s; s++) {
        if (*s != '%') continue;
        if (s[1] == '%') { s++; continue; }
        s++;
        while (*s && strchr("-+ #0123456789.", *s)) s++;           /* flags, largura, precisão */
        while (*s && strchr("*hlLzjt", *s)) {                       /* '*' consome um int; tamanho */
            if (k + 1 < n) saida[k++] = *s;
            s++;
        }
        if (!*s) break;
        if (k + 2 < n) { saida[k++] = *s; saida[k++] = ','; }      /* o tipo: d, s, f... */
    }
    saida[k] = '\0';
}

static bool mesmos_formatos(const char *a, const char *b)
{
    char fa[64], fb[64];
    formatos(a, fa, sizeof(fa));
    formatos(b, fb, sizeof(fb));
    return strcmp(fa, fb) == 0;
}

/* ---------------------------------------------------------------------------
 * Leitura do arquivo
 * ------------------------------------------------------------------------- */
static char *apara(char *s)
{
    while (*s == ' ' || *s == '\t') s++;
    char *fim = s + strlen(s);
    while (fim > s && (fim[-1] == ' ' || fim[-1] == '\t' || fim[-1] == '\r')) *--fim = '\0';
    return s;
}

/* uma linha "chave = valor": separa (no próprio texto) e devolve false se não for */
static bool separa(char *linha, char **chave, char **valor)
{
    linha = apara(linha);
    if (!*linha || *linha == '#') return false;
    char *igual = strstr(linha, " = ");   /* o primeiro " = ": a chave pode ter "=" grudado */
    if (!igual) return false;
    *igual  = '\0';
    *chave  = apara(linha);
    *valor  = apara(igual + 3);
    return **chave && **valor;
}

static int compara(const void *x, const void *y)
{
    return strcmp(((const Par *)x)->pt, ((const Par *)y)->pt);
}

void Texto_limpa(void)
{
    free(buffer);
    free(pares);
    buffer  = NULL;
    pares   = NULL;
    n_pares = 0;
}

bool Texto_nome(const char *conteudo, size_t tamanho, char *nome, size_t n_nome)
{
    const char *p = conteudo, *fim = conteudo + tamanho;
    while (p < fim) {
        const char *eol = memchr(p, '\n', (size_t)(fim - p));
        if (!eol) eol = fim;
        char linha[160];
        const size_t t = (size_t)(eol - p) < sizeof(linha) - 1 ? (size_t)(eol - p) : sizeof(linha) - 1;
        memcpy(linha, p, t);
        linha[t] = '\0';
        p = eol + 1;
        char *chave, *valor;
        if (separa(linha, &chave, &valor) && strcmp(chave, "idioma") == 0) {
            Texto_copia(nome, n_nome, valor);
            return true;
        }
    }
    return false;
}

int Texto_carrega(const char *conteudo, size_t tamanho, char *nome, size_t n_nome, int *ignoradas)
{
    Texto_limpa();
    if (ignoradas) *ignoradas = 0;
    if (nome && n_nome) nome[0] = '\0';

    /* uma cópia: as chaves e traduções ficam apontando para dentro dela */
    buffer = malloc(tamanho + 1);
    if (!buffer) return 0;
    memcpy(buffer, conteudo, tamanho);
    buffer[tamanho] = '\0';

    int linhas = 1;
    for (const char *c = buffer; *c; c++) linhas += *c == '\n';
    pares = malloc(sizeof(Par) * (size_t)linhas);
    if (!pares) { Texto_limpa(); return 0; }

    for (char *linha = buffer, *prox; linha; linha = prox) {
        prox = strchr(linha, '\n');
        if (prox) *prox++ = '\0';
        char *chave, *valor;
        if (!separa(linha, &chave, &valor)) continue;
        if (strcmp(chave, "idioma") == 0) {
            if (nome && n_nome) Texto_copia(nome, n_nome, valor);
            continue;
        }
        if (!mesmos_formatos(chave, valor)) {
            if (ignoradas) (*ignoradas)++;
            continue;
        }
        pares[n_pares++] = (Par){ chave, valor };
    }
    if (n_pares > 0) qsort(pares, (size_t)n_pares, sizeof(Par), compara);
    return n_pares;
}

const char *T(const char *pt)
{
    if (!pt || n_pares == 0) return pt;
    int ini = 0, fim = n_pares - 1;
    while (ini <= fim) {
        const int meio = (ini + fim) / 2;
        const int c = strcmp(pt, pares[meio].pt);
        if (c == 0) return pares[meio].tr;
        if (c < 0) fim = meio - 1;
        else       ini = meio + 1;
    }
    return pt;
}
