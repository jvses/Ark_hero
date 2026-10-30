#ifndef ARK_INPUT_H
#define ARK_INPUT_H

/* Teclado + controles -> ações do jogo (game/acao.h), para até 4 jogadores.
 *
 * Cada jogador tem um espaço: um controle (o primeiro conectado vai para o
 * primeiro espaço livre) ou o teclado, e um PERFIL, a configuração dele: o
 * botão do controle e a tecla de cada ação, o analógico do whammy, o estilo
 * dos botões frontais e o modo (controle ou guitarra).
 *
 * O perfil é da CONTA de quem joga (Input_use_account): <PrefPath>/contas/
 * <código do nome>.cfg. O convidado usa o perfil do espaço: controles.cfg
 * (jogador 1), controles2.cfg, controles3.cfg e controles4.cfg. No console,
 * <PrefPath> é <pasta do jogo>/conf/arkhero/.
 *
 * O analógico escolhido faz o whammy (para os lados) e o star power (empurrão
 * forte para cima), além dos botões. O mapeamento pode ser trocado em tempo
 * de execução (Input_capture_start).
 *
 * O teclado é um aparelho só, dividido entre os espaços marcados como de
 * teclado (Input_set_keyboard_seat): só eles recebem as teclas de jogo, e
 * cada tecla deve ser de um jogador só (Input_keys_state). As teclas de menu
 * (setas, Enter, Esc) são do próprio teclado, entregues à parte pelo
 * Input_frame. */

#include <SDL3/SDL.h>

#include "game/acao.h"

/* "Botões físicos" do controle: os SDL_GamepadButton (0..COUNT-1) e mais dois
 * para os gatilhos, que a SDL entrega como eixo e não como botão. */
#define INPUT_FISICO_NENHUM (-1)
#define INPUT_FISICO_L2     ((int)SDL_GAMEPAD_BUTTON_COUNT)
#define INPUT_FISICO_R2     ((int)SDL_GAMEPAD_BUTTON_COUNT + 1)

#define INPUT_CAPTURA_SEGUNDOS 5   /* tempo para apertar o botão novo */

/* Os botões frontais são guardados pela POSIÇÃO física (sul = o de baixo,
 * leste = o da direita...), com os valores SDL_GAMEPAD_BUTTON_SOUTH/EAST/WEST/
 * NORTH. A SDL nomeia pela posição num controle de Xbox, mas alguns
 * mapeamentos (o do R36S no PortMaster) nomeiam pela LETRA impressa, no
 * padrão Nintendo, e aí o "b" da SDL é o botão de baixo. O input desfaz isso
 * (Input.trocado), para "sul" ser o botão de baixo em qualquer aparelho. */
enum { FRONTAIS_AUTO, FRONTAIS_XBOX, FRONTAIS_NINTENDO, FRONTAIS_N };

typedef struct {
    int          pad;     /* botão físico do controle, ou INPUT_FISICO_NENHUM */
    SDL_Scancode tecla;   /* SDL_SCANCODE_UNKNOWN = nenhuma */
    bool         cru;     /* veio de um controles.cfg antigo, com nome da SDL: converte ao conectar */
} Mapeamento;

#define INPUT_MAX_JOGADORES 4

/* um controle conectado, no espaço de um jogador */
typedef struct {
    SDL_Gamepad   *gamepad;           /* NULL = nenhum controle neste espaço */
    SDL_JoystickID which;
    bool           trocado;           /* nomeia os frontais pela letra (Nintendo) */
    bool           gatilho[2];        /* L2/R2 apertados? (para achar a borda do eixo) */
    Sint16         eixo[SDL_GAMEPAD_AXIS_COUNT];
    bool           alavanca_cima;     /* o empurrão para cima está valendo? (histerese) */
} Controle;

/* a configuração de um jogador */
typedef struct {
    Mapeamento     mapa[ACAO_N];
    int            analogico;         /* 0 = esquerdo, 1 = direito */
    int            frontais;          /* FRONTAIS_AUTO / _XBOX / _NINTENDO */
    bool           guitarra;          /* modo guitarra: a palhetada é obrigatória */
    char           conta[24];         /* de quem é ("" = o convidado deste espaço) */
    char           arquivo[512];      /* onde fica ("" = não salva) */
} Perfil;

typedef struct {
    Controle       ctl[INPUT_MAX_JOGADORES];
    Perfil         perfil[INPUT_MAX_JOGADORES];
    bool           no_teclado[INPUT_MAX_JOGADORES];   /* espaços que jogam no teclado (o controle novo pula esses) */
    char           pasta[512];        /* <PrefPath> ("" = sem pasta: nada é salvo) */
    uint32_t       bordas[INPUT_MAX_JOGADORES];   /* ações apertadas desde o último Input_frame */
    uint32_t       bordas_teclado;                /* idem, das teclas de menu */
    bool           sair;              /* Select + Start, em qualquer controle */

    int            capturando;        /* ação esperando um botão novo, ou -1 */
    int            captura_jogador;   /* de quem é o botão novo */
    Uint64         captura_inicio_ms;

    char           ultimo[64];        /* último botão apertado, para conferir na tela */
    bool           log_apertos;       /* ARKHERO_LOG_INPUT=1: cada aperto vai para o log */
} Input;

bool   Input_init(Input *inp);
void   Input_quit(Input *inp);
void   Input_event(Input *inp, const SDL_Event *ev);   /* no SDL_AppEvent: só anota */
/* no SDL_AppIterate, uma vez por frame: os botões de cada jogador e, à parte,
   as teclas de menu do teclado */
void   Input_frame(Input *inp, Botoes saida[INPUT_MAX_JOGADORES], Botoes *menu);

/* configuração de um jogador (j = 0..3) */
void   Input_capture_start(Input *inp, int j, Acao a);   /* o próximo botão/tecla do jogador j vira o de "a" */
double Input_capture_left(const Input *inp);             /* segundos restantes, 0 se não está capturando */
void   Input_reset_defaults(Input *inp, int j);          /* volta ao padrão e salva */
void   Input_toggle_stick(Input *inp, int j);            /* troca o analógico do whammy e salva */
void   Input_cycle_layout(Input *inp, int j);            /* botões frontais: automático / Xbox / Nintendo */
void   Input_toggle_mode(Input *inp, int j);             /* modo controle / guitarra, e salva */
bool   Input_guitar_mode(const Input *inp, int j);

/* o perfil do espaço j passa a ser o da conta (NULL ou "" = o convidado do
   espaço). Uma conta sem arquivo usa o perfil do convidado, e o arquivo dela
   é criado na primeira mudança. Não faz nada se já é esse. */
void   Input_use_account(Input *inp, int j, const char *conta);
void   Input_set_keyboard_seat(Input *inp, int j, bool teclado);   /* o espaço j joga no teclado? */
int    Input_keys_state(const Input *inp, int j);   /* 0 = dá para jogar, 1 = sem teclas nos trastes, 2 = tecla repetida */

/* Nomes para mostrar na tela: "L1", "R2", "B"... e "A", "Return"... */
const char *Input_pad_label(const Input *inp, int j, Acao a);
const char *Input_key_label(const Input *inp, int j, Acao a);
const char *Input_stick_label(const Input *inp, int j);   /* "Esquerdo" / "Direito" */
const char *Input_layout_label(const Input *inp, int j);  /* "Auto (Nintendo)"... */
const char *Input_gamepad_name(const Input *inp, int j);  /* NULL = sem controle */

#endif
