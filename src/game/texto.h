#ifndef ARK_TEXTO_H
#define ARK_TEXTO_H

/* Os textos da tela em outros idiomas.
 *
 * O texto em português é a CHAVE: no código se escreve T("Jogar"), e T devolve
 * a tradução do idioma carregado, ou o próprio texto se não houver tradução.
 * Assim o código continua legível, e o que faltar traduzir aparece em
 * português, sem quebrar nada.
 *
 * Um idioma é um arquivo de texto (assets/lang/en.txt...), uma linha por texto:
 *     idioma = English
 *     Jogar = Play
 *     %d musica(s) = %d song(s)
 * Os "%d", "%s"... de cada texto têm de continuar na tradução, na mesma ordem:
 * se não estiverem, a tradução é ignorada (um %s no lugar de um %d derrubaria
 * o jogo). Linhas começando com # são comentários.
 *
 * O texto é UTF-8: a fonte do jogo (assets/fonts/) tem os acentos do latim, o
 * grego e o cirílico; o que ela não tiver aparece como '?'.
 *
 * Um idioma por vez, para o jogo todo: por isso o estado é global (o draw e
 * os menus usam sem precisar carregar nada junto). */

#include <stdbool.h>
#include <stddef.h>

/* marca um texto para tradução onde ele não pode ser traduzido na hora (uma
   tabela, como os itens de menu): quem mostra é que chama T() */
#define N_(s) s

const char *T(const char *pt);

/* troca o idioma pelo do arquivo; nome = o "idioma = ..." dele. Devolve quantas
   traduções valeram; *ignoradas = as que tinham os %d/%s diferentes. */
int  Texto_carrega(const char *conteudo, size_t tamanho, char *nome, size_t n_nome, int *ignoradas);
void Texto_limpa(void);   /* de volta ao português */

/* só o nome ("idioma = ...") de um arquivo de idioma, sem carregá-lo */
bool Texto_nome(const char *conteudo, size_t tamanho, char *nome, size_t n_nome);

/* copia um texto de fora (song.ini, nomes) para mostrar: mantém o UTF-8,
   converte Latin-1 e corta sem partir um caractere no meio */
void Texto_copia(char *destino, size_t max, const char *origem);

#endif
