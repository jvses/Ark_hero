#ifndef ARK_INPUT_H
#define ARK_INPUT_H

/* Teclado + gamepad -> ações do jogo (game/acao.h).
 *
 * Cada ação tem um botão de controle e uma tecla. O analógico escolhido
 * (esquerdo ou direito) faz o whammy (para os lados) e o star power (empurrão
 * forte para cima), além dos botões. O mapeamento pode ser
 * trocado em tempo de execução (Input_capture_start) e fica salvo em
 * <PrefPath>/controles.cfg; no console, <pasta do jogo>/conf/arkhero/. */

#include <SDL3/SDL.h>

#include "game/acao.h"

/* "Botões físicos" do controle: os SDL_GamepadButton (0..COUNT-1) e mais dois
 * para os gatilhos, que a SDL entrega como eixo e não como botão. */
#define INPUT_FISICO_NENHUM (-1)
#define INPUT_FISICO_L2     ((int)SDL_GAMEPAD_BUTTON_COUNT)
#define INPUT_FISICO_R2     ((int)SDL_GAMEPAD_BUTTON_COUNT + 1)

#define INPUT_CAPTURA_SEGUNDOS 5   /* tempo para apertar o botão novo */

typedef struct {
    int          pad;     /* botão físico do controle, ou INPUT_FISICO_NENHUM */
    SDL_Scancode tecla;   /* SDL_SCANCODE_UNKNOWN = nenhuma */
} Mapeamento;

typedef struct {
    SDL_Gamepad   *gamepad;
    SDL_JoystickID which;
    bool           sair;              /* Select + Start */

    Mapeamento     mapa[ACAO_N];
    uint32_t       bordas;            /* ações apertadas desde o último Input_frame */
    bool           gatilho[2];        /* L2/R2 apertados? (para achar a borda do eixo) */

    /* analógico: para os lados = whammy; empurrão forte para cima = star power */
    int            analogico;         /* 0 = esquerdo, 1 = direito */
    Sint16         eixo[SDL_GAMEPAD_AXIS_COUNT];
    bool           alavanca_cima;     /* o empurrão para cima está valendo? (histerese) */

    int            capturando;        /* ação esperando um botão novo, ou -1 */
    Uint64         captura_inicio_ms;

    char           ultimo[48];        /* último botão apertado, para conferir na tela */
    char           arquivo[512];      /* caminho do controles.cfg ("" = não salva) */
} Input;

bool   Input_init(Input *inp);
void   Input_quit(Input *inp);
void   Input_event(Input *inp, const SDL_Event *ev);   /* no SDL_AppEvent: só anota */
Botoes Input_frame(Input *inp);                        /* no SDL_AppIterate: uma vez por frame */

void   Input_capture_start(Input *inp, Acao a);   /* o próximo botão/tecla vira o de "a" */
double Input_capture_left(const Input *inp);      /* segundos restantes, 0 se não está capturando */
void   Input_reset_defaults(Input *inp);          /* volta ao padrão e salva */
void   Input_toggle_stick(Input *inp);            /* troca o analógico do whammy e salva */

/* Nomes para mostrar na tela: "L1", "R2", "B"... e "A", "Return"... */
const char *Input_pad_label(const Input *inp, Acao a);
const char *Input_key_label(const Input *inp, Acao a);
const char *Input_stick_label(const Input *inp);   /* "Esquerdo" / "Direito" */

#endif
