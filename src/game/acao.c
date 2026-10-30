#include "game/acao.h"
#include "game/texto.h"

#include <stddef.h>

typedef struct {
    const char *id;
    const char *rotulo;   /* só ASCII: a fonte de debug da SDL não desenha acentos */
} NomeAcao;

static const NomeAcao NOMES[ACAO_N] = {
    [ACAO_VERDE]         = { "verde",         N_("Verde") },
    [ACAO_VERMELHO]      = { "vermelho",      N_("Vermelho") },
    [ACAO_AMARELO]       = { "amarelo",       N_("Amarelo") },
    [ACAO_AZUL]          = { "azul",          N_("Azul") },
    [ACAO_LARANJA]       = { "laranja",       N_("Laranja") },
    [ACAO_PALHETA_CIMA]  = { "palheta_cima",  N_("Palheta cima") },
    [ACAO_PALHETA_BAIXO] = { "palheta_baixo", N_("Palheta baixo") },
    [ACAO_STAR_POWER]    = { "star_power",    N_("Star power") },
    [ACAO_WHAMMY]        = { "whammy",        N_("Whammy") },
    [ACAO_PAUSA]         = { "pausa",         N_("Pausa") },
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
