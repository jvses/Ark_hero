#include "game/acao.h"

#include <stddef.h>

typedef struct {
    const char *id;
    const char *rotulo;   /* só ASCII: a fonte de debug da SDL não desenha acentos */
} NomeAcao;

static const NomeAcao NOMES[ACAO_N] = {
    [ACAO_VERDE]         = { "verde",         "Verde" },
    [ACAO_VERMELHO]      = { "vermelho",      "Vermelho" },
    [ACAO_AMARELO]       = { "amarelo",       "Amarelo" },
    [ACAO_AZUL]          = { "azul",          "Azul" },
    [ACAO_LARANJA]       = { "laranja",       "Laranja" },
    [ACAO_PALHETA_CIMA]  = { "palheta_cima",  "Palheta cima" },
    [ACAO_PALHETA_BAIXO] = { "palheta_baixo", "Palheta baixo" },
    [ACAO_STAR_POWER]    = { "star_power",    "Star power" },
    [ACAO_WHAMMY]        = { "whammy",        "Whammy" },
    [ACAO_PAUSA]         = { "pausa",         "Pausa" },
    [ACAO_CIMA]          = { "cima",          "Cima" },
    [ACAO_BAIXO]         = { "baixo",         "Baixo" },
    [ACAO_ESQUERDA]      = { "esquerda",      "Esquerda" },
    [ACAO_DIREITA]       = { "direita",       "Direita" },
    [ACAO_CONFIRMA]      = { "confirma",      "Confirmar" },
    [ACAO_VOLTA]         = { "volta",         "Voltar" },
};

const char *Acao_id(Acao a)
{
    return (a >= 0 && a < ACAO_N) ? NOMES[a].id : "?";
}

const char *Acao_rotulo(Acao a)
{
    return (a >= 0 && a < ACAO_N) ? NOMES[a].rotulo : "?";
}
