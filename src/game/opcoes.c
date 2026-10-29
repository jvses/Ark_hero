/* As opções do jogador em texto, uma por linha ("velocidade = 5"). Quem lê e
 * grava o arquivo (opcoes.cfg) é o main.c; aqui só se converte. Opção que
 * faltar no arquivo, ou com valor estranho, fica com o valor que já tinha. */

#include "game/game.h"

#include <stdio.h>
#include <string.h>

int Opcoes_write(const Opcoes *o, char *buf, size_t n)
{
    return snprintf(buf, n,
                    "# Ark Hero - opcoes (gerado pelo jogo, pode editar)\n"
                    "velocidade = %d\n"
                    "precisao = %d\n"
                    "canhoto = %d\n"
                    "mostrar_fps = %d\n"
                    "volume = %d\n"
                    "organizar = %d\n"
                    "penalidade = %d\n"
                    "mostrar_acertos = %d\n"
                    "ajuste_ms = %d\n",
                    o->velocidade, o->precisao, o->canhoto, o->mostrar_fps, o->volume, o->organizar,
                    o->penalidade, o->mostrar_julgamento, o->ajuste_ms);
}

static int limita(int v, int min, int max)
{
    return v < min ? min : (v > max ? max : v);
}

void Opcoes_read(Opcoes *o, const char *texto, size_t tamanho)
{
    const char *p = texto, *fim = texto + tamanho;
    while (p < fim) {
        const char *eol = memchr(p, '\n', (size_t)(fim - p));
        if (!eol) eol = fim;
        char linha[96];
        const size_t t = (size_t)(eol - p) < sizeof(linha) - 1 ? (size_t)(eol - p) : sizeof(linha) - 1;
        memcpy(linha, p, t);
        linha[t] = '\0';
        p = eol + 1;

        char chave[32];
        int  v;
        if (linha[0] == '#' || sscanf(linha, " %31[a-z_] = %d", chave, &v) != 2) continue;   /* %d aceita o sinal */

        if      (strcmp(chave, "velocidade") == 0)  o->velocidade  = limita(v, GAME_VELOCIDADE_MIN, GAME_VELOCIDADE_MAX);
        else if (strcmp(chave, "precisao") == 0)    o->precisao    = v != 0;
        else if (strcmp(chave, "canhoto") == 0)     o->canhoto     = v != 0;
        else if (strcmp(chave, "mostrar_fps") == 0) o->mostrar_fps = v != 0;
        else if (strcmp(chave, "volume") == 0)      o->volume      = limita(v, 0, 100);
        else if (strcmp(chave, "organizar") == 0)   o->organizar   = limita(v, 0, ORGANIZAR_N - 1);
        else if (strcmp(chave, "penalidade") == 0)  o->penalidade  = v != 0;
        else if (strcmp(chave, "mostrar_acertos") == 0) o->mostrar_julgamento = v != 0;
        else if (strcmp(chave, "ajuste_ms") == 0)   o->ajuste_ms   = limita(v, -GAME_AJUSTE_MAX, GAME_AJUSTE_MAX);
    }
}
