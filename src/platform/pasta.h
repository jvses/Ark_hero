#ifndef ARK_PASTA_H
#define ARK_PASTA_H

/* A pasta onde o jogo grava: opções, contas, recordes, controles, pastas.txt.
 * Termina com a barra; libere com SDL_free. NULL se não houver nenhuma.
 *
 * Linux: a do SDL_GetPrefPath, $XDG_DATA_HOME/arkhero/. No console o script
 * de launch aponta XDG_DATA_HOME para <pasta do jogo>/conf/, e tudo fica
 * dentro do port. No PC, ~/.local/share/arkhero/.
 *
 * Windows: <pasta do jogo>\conf\arkhero\, para o jogo ficar inteiro na pasta
 * dele, como no console. Se ela não puder ser criada (o jogo instalado em
 * Arquivos de Programas, por exemplo), vale a do sistema: %APPDATA%\arkhero\. */
char *Pasta_dados(void);

#endif
