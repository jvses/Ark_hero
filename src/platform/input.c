#include "platform/input.h"

#include "game/texto.h"   /* N_(): os rótulos para a tela são chaves de tradução */

#define GATILHO_LIGA    16000   /* eixo do gatilho acima disso = apertado */
#define GATILHO_DESLIGA  8000   /* ...e só solta abaixo disso (histerese) */

#define ALAVANCA_LIGA   (-26000)  /* analógico acima disso (Y negativo = cima) = star power */
#define ALAVANCA_DESLIGA (-16000)
#define WHAMMY_ZONA_MORTA 0.20f   /* folga do analógico parado, em fração do curso */

/* eixos (X, Y) de cada analógico: [0] = esquerdo, [1] = direito */
static const SDL_GamepadAxis EIXO_X[2] = { SDL_GAMEPAD_AXIS_LEFTX, SDL_GAMEPAD_AXIS_RIGHTX };
static const SDL_GamepadAxis EIXO_Y[2] = { SDL_GAMEPAD_AXIS_LEFTY, SDL_GAMEPAD_AXIS_RIGHTY };

/* ---------------------------------------------------------------------------
 * Mapeamento padrão. Os frontais são pela posição física (input.h):
 *
 *   posição   aqui    Xbox   PlayStation   R36S (letra)
 *   baixo     sul      A        X              B
 *   direita   leste    B        bola           A
 *   esquerda  oeste    X        quadrado       Y
 *   cima      norte    Y        triângulo      X
 *
 * Confirmar = sul e voltar = leste em qualquer controle (o costume do Xbox e
 * do PlayStation). Trastes no R36S: R1, R2, L2, L1 e o sul.
 * Teclado: para jogar e testar no PC. As teclas de menu ficam fora daqui
 * (TECLAS_MENU): elas são do teclado, não de um jogador.
 * ------------------------------------------------------------------------- */
static const Mapeamento PADRAO[ACAO_N] = {
    [ACAO_VERDE]         = { SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, SDL_SCANCODE_A },      /* R1 */
    [ACAO_VERMELHO]      = { INPUT_FISICO_R2,                   SDL_SCANCODE_S },      /* R2 */
    [ACAO_AMARELO]       = { INPUT_FISICO_L2,                   SDL_SCANCODE_D },      /* L2 */
    [ACAO_AZUL]          = { SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,  SDL_SCANCODE_F },      /* L1 */
    [ACAO_LARANJA]       = { SDL_GAMEPAD_BUTTON_SOUTH,          SDL_SCANCODE_G },      /* sul */
    [ACAO_PALHETA_CIMA]  = { SDL_GAMEPAD_BUTTON_DPAD_UP,        SDL_SCANCODE_UP },
    [ACAO_PALHETA_BAIXO] = { SDL_GAMEPAD_BUTTON_DPAD_DOWN,      SDL_SCANCODE_DOWN },
    [ACAO_STAR_POWER]    = { SDL_GAMEPAD_BUTTON_BACK,           SDL_SCANCODE_SPACE },  /* Select (e o analógico) */
    [ACAO_WHAMMY]        = { INPUT_FISICO_NENHUM,               SDL_SCANCODE_W },      /* no controle: o analógico */
    [ACAO_PAUSA]         = { SDL_GAMEPAD_BUTTON_START,          SDL_SCANCODE_ESCAPE },

    [ACAO_CIMA]          = { SDL_GAMEPAD_BUTTON_DPAD_UP,        SDL_SCANCODE_UNKNOWN },
    [ACAO_BAIXO]         = { SDL_GAMEPAD_BUTTON_DPAD_DOWN,      SDL_SCANCODE_UNKNOWN },
    [ACAO_ESQUERDA]      = { SDL_GAMEPAD_BUTTON_DPAD_LEFT,      SDL_SCANCODE_UNKNOWN },
    [ACAO_DIREITA]       = { SDL_GAMEPAD_BUTTON_DPAD_RIGHT,     SDL_SCANCODE_UNKNOWN },
    [ACAO_CONFIRMA]      = { SDL_GAMEPAD_BUTTON_SOUTH,          SDL_SCANCODE_UNKNOWN }, /* sul */
    [ACAO_VOLTA]         = { SDL_GAMEPAD_BUTTON_EAST,           SDL_SCANCODE_UNKNOWN }, /* leste */
};

/* As teclas de menu, fixas: são do TECLADO, não de um jogador. Num teclado
 * dividido entre vários jogadores, elas configuram um de cada vez na tela de
 * preparação; nos outros menus, valem como as de qualquer controle. */
static const SDL_Scancode TECLAS_MENU[ACAO_N] = {
    [ACAO_CIMA]     = SDL_SCANCODE_UP,
    [ACAO_BAIXO]    = SDL_SCANCODE_DOWN,
    [ACAO_ESQUERDA] = SDL_SCANCODE_LEFT,
    [ACAO_DIREITA]  = SDL_SCANCODE_RIGHT,
    [ACAO_CONFIRMA] = SDL_SCANCODE_RETURN,
    [ACAO_VOLTA]    = SDL_SCANCODE_ESCAPE,
};

/* Os outros jogadores usam o mesmo mapeamento no controle e começam sem
 * teclas: quem vai jogar no teclado escolhe as suas na tela de Controles. */
static void padrao(Perfil *p, int j)
{
    SDL_memcpy(p->mapa, PADRAO, sizeof(PADRAO));
    if (j > 0)
        for (int a = 0; a < ACAO_N; a++) p->mapa[a].tecla = SDL_SCANCODE_UNKNOWN;
    p->analogico = 0;
}

/* j fora de 0..3 vira o jogador 1 (nunca lê fora dos vetores) */
static int jog(int j)
{
    return (j >= 0 && j < INPUT_MAX_JOGADORES) ? j : 0;
}

/* ---------------------------------------------------------------------------
 * Nomes dos botões físicos
 * ------------------------------------------------------------------------- */

static bool frontal(int b)
{
    return b == SDL_GAMEPAD_BUTTON_SOUTH || b == SDL_GAMEPAD_BUTTON_EAST ||
           b == SDL_GAMEPAD_BUTTON_WEST || b == SDL_GAMEPAD_BUTTON_NORTH;
}

/* SDL <-> posição física: no estilo Nintendo, sul<->leste e oeste<->norte
   (a mesma troca serve nos dois sentidos) */
static int converte_frontal(bool trocado, int b)
{
    if (!trocado) return b;
    switch (b) {
    case SDL_GAMEPAD_BUTTON_SOUTH: return SDL_GAMEPAD_BUTTON_EAST;
    case SDL_GAMEPAD_BUTTON_EAST:  return SDL_GAMEPAD_BUTTON_SOUTH;
    case SDL_GAMEPAD_BUTTON_WEST:  return SDL_GAMEPAD_BUTTON_NORTH;
    case SDL_GAMEPAD_BUTTON_NORTH: return SDL_GAMEPAD_BUTTON_WEST;
    default:                       return b;
    }
}

static const char *const CARDEAIS[4] = { "sul", "leste", "oeste", "norte" };   /* SOUTH, EAST, WEST, NORTH */

/* nome no arquivo: os frontais pela posição ("sul"), o resto pelo nome da SDL
   ("leftshoulder", "lefttrigger"...) */
static const char *nome_pad(int fisico)
{
    if (frontal(fisico)) return CARDEAIS[fisico - SDL_GAMEPAD_BUTTON_SOUTH];
    if (fisico == INPUT_FISICO_L2) return SDL_GetGamepadStringForAxis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
    if (fisico == INPUT_FISICO_R2) return SDL_GetGamepadStringForAxis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
    if (fisico >= 0 && fisico < SDL_GAMEPAD_BUTTON_COUNT) {
        const char *s = SDL_GetGamepadStringForButton((SDL_GamepadButton)fisico);
        if (s) return s;
    }
    return "-";
}

/* o contrário: do nome no arquivo para o botão. -2 = nome desconhecido.
   *cru = era "a"/"b"/"x"/"y" (arquivo antigo): a posição depende do controle */
static int pad_do_nome(const char *nome, bool *cru)
{
    *cru = false;
    if (SDL_strcmp(nome, "-") == 0) return INPUT_FISICO_NENHUM;
    for (int i = 0; i < 4; i++)
        if (SDL_strcmp(nome, CARDEAIS[i]) == 0) return SDL_GAMEPAD_BUTTON_SOUTH + i;

    const SDL_GamepadButton b = SDL_GetGamepadButtonFromString(nome);
    if (b != SDL_GAMEPAD_BUTTON_INVALID) {
        *cru = frontal(b);
        return (int)b;
    }

    const SDL_GamepadAxis e = SDL_GetGamepadAxisFromString(nome);
    if (e == SDL_GAMEPAD_AXIS_LEFT_TRIGGER)  return INPUT_FISICO_L2;
    if (e == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) return INPUT_FISICO_R2;
    return -2;
}

/* nome curto para a tela: os frontais pela posição, que vale em qualquer
   controle, e o direcional em setas. Quem mostra traduz (T). */
static const char *rotulo_pad(int fisico)
{
    switch (fisico) {
    case INPUT_FISICO_NENHUM:                return "-";
    case INPUT_FISICO_L2:                    return "L2";
    case INPUT_FISICO_R2:                    return "R2";
    case SDL_GAMEPAD_BUTTON_SOUTH:           return N_("Sul");
    case SDL_GAMEPAD_BUTTON_EAST:            return N_("Leste");
    case SDL_GAMEPAD_BUTTON_WEST:            return N_("Oeste");
    case SDL_GAMEPAD_BUTTON_NORTH:           return N_("Norte");
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:   return "L1";
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:  return "R1";
    case SDL_GAMEPAD_BUTTON_LEFT_STICK:      return "L3";
    case SDL_GAMEPAD_BUTTON_RIGHT_STICK:     return "R3";
    case SDL_GAMEPAD_BUTTON_BACK:            return "Select";
    case SDL_GAMEPAD_BUTTON_START:           return "Start";
    case SDL_GAMEPAD_BUTTON_GUIDE:           return "Guide";
    case SDL_GAMEPAD_BUTTON_DPAD_UP:         return "↑";
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:       return "↓";
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:       return "←";
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:      return "→";
    default:                                 return nome_pad(fisico);
    }
}

static const char *nome_tecla(SDL_Scancode s)
{
    if (s == SDL_SCANCODE_UNKNOWN) return "-";
    const char *n = SDL_GetScancodeName(s);
    return (n && *n) ? n : "-";
}

/* para a tela: mais curto que o nome da SDL (que é o do arquivo, e fica) */
static const char *rotulo_tecla(SDL_Scancode s)
{
    if (s == SDL_SCANCODE_RETURN) return "Enter";
    if (s == SDL_SCANCODE_ESCAPE) return "Esc";
    return nome_tecla(s);
}

/* ---------------------------------------------------------------------------
 * Arquivo de controles: uma linha por ação remapeável
 *     verde|leftshoulder|A
 * ------------------------------------------------------------------------- */

static void salva(const Input *inp, int j)
{
    const Perfil *p = &inp->perfil[j];
    if (!p->arquivo[0]) return;

    char texto[2048];
    int usado = SDL_snprintf(texto, sizeof(texto),
                             "# Ark Hero - controles (gerado pelo jogo, pode editar)\n"
                             "# conta: %s\n"
                             "# acao|botao do controle|tecla\n"
                             "# frontais pela posicao: sul (baixo), leste (direita), oeste, norte\n",
                             p->conta[0] ? p->conta : "convidado");
    for (int a = 0; a < ACAO_N_REMAPEAVEIS && usado < (int)sizeof(texto); a++) {
        usado += SDL_snprintf(texto + usado, sizeof(texto) - (size_t)usado, "%s|%s|%s\n",
                              Acao_id((Acao)a), nome_pad(p->mapa[a].pad),
                              nome_tecla(p->mapa[a].tecla));
    }
    if (usado < (int)sizeof(texto)) {
        static const char *const FRONTAIS[FRONTAIS_N] = { "auto", "xbox", "nintendo" };
        usado += SDL_snprintf(texto + usado, sizeof(texto) - (size_t)usado,
                              "analogico|%s|-\nfrontais|%s|-\nmodo|%s|-\n", p->analogico ? "direito" : "esquerdo",
                              FRONTAIS[p->frontais], p->guitarra ? "guitarra" : "controle");
    }
    if (usado >= (int)sizeof(texto)) usado = (int)sizeof(texto) - 1;

    if (p->conta[0]) {   /* os perfis das contas ficam numa pasta própria */
        char pasta[600];
        SDL_snprintf(pasta, sizeof(pasta), "%scontas", inp->pasta);
        SDL_CreateDirectory(pasta);   /* já existir não é erro */
    }
    if (!SDL_SaveFile(p->arquivo, texto, (size_t)usado))
        SDL_Log("[input] nao consegui salvar %s: %s", p->arquivo, SDL_GetError());
}

static void carrega(Input *inp, int j)
{
    Perfil *p = &inp->perfil[j];
    if (!p->arquivo[0]) return;

    size_t tam = 0;
    char *texto = SDL_LoadFile(p->arquivo, &tam);   /* a SDL termina o texto com '\0' */
    if (!texto) {
        if (j == 0 && !p->conta[0]) SDL_Log("[input] sem %s: usando os controles padrao", p->arquivo);
        return;
    }

    char *fim_linha = NULL;
    for (char *linha = SDL_strtok_r(texto, "\r\n", &fim_linha); linha;
         linha = SDL_strtok_r(NULL, "\r\n", &fim_linha)) {
        if (linha[0] == '#') continue;

        char *fim_campo = NULL;
        const char *id    = SDL_strtok_r(linha, "|", &fim_campo);
        const char *pad   = SDL_strtok_r(NULL, "|", &fim_campo);
        const char *tecla = SDL_strtok_r(NULL, "|", &fim_campo);
        if (!id || !pad || !tecla) continue;

        if (SDL_strcmp(id, "analogico") == 0) {
            p->analogico = SDL_strcmp(pad, "direito") == 0 ? 1 : 0;
            continue;
        }
        if (SDL_strcmp(id, "frontais") == 0) {
            p->frontais = SDL_strcmp(pad, "xbox") == 0 ? FRONTAIS_XBOX
                        : SDL_strcmp(pad, "nintendo") == 0 ? FRONTAIS_NINTENDO : FRONTAIS_AUTO;
            continue;
        }
        if (SDL_strcmp(id, "modo") == 0) {
            p->guitarra = SDL_strcmp(pad, "guitarra") == 0;
            continue;
        }
        for (int a = 0; a < ACAO_N_REMAPEAVEIS; a++) {
            if (SDL_strcmp(id, Acao_id((Acao)a)) != 0) continue;

            bool cru;
            const int fisico = pad_do_nome(pad, &cru);
            if (fisico != -2) {
                p->mapa[a].pad = fisico;
                p->mapa[a].cru = cru;
            }
            p->mapa[a].tecla = SDL_strcmp(tecla, "-") == 0 ? SDL_SCANCODE_UNKNOWN
                                                           : SDL_GetScancodeFromName(tecla);
            break;
        }
    }
    SDL_free(texto);
    SDL_Log("[input] controles do jogador %d (%s) carregados de %s", j + 1, p->conta[0] ? p->conta : "convidado",
            p->arquivo);
}

/* ---------------------------------------------------------------------------
 * Captura: a próxima tecla ou botão vira o da ação escolhida.
 * Se o botão já era de outra ação remapeável, as duas trocam: assim nunca
 * ficam dois trastes no mesmo botão.
 * ------------------------------------------------------------------------- */

static void atribui_pad(Perfil *p, int a, int fisico)
{
    for (int outra = 0; outra < ACAO_N_REMAPEAVEIS; outra++) {
        if (outra != a && p->mapa[outra].pad == fisico) p->mapa[outra].pad = p->mapa[a].pad;
    }
    p->mapa[a].pad = fisico;
    SDL_Log("[input] %s -> controle %s", Acao_id((Acao)a), nome_pad(fisico));
}

static void salva(const Input *inp, int j);

/* Uma tecla é de um jogador só: se outro jogador de teclado já a usava, ela
 * sai de lá. (Sem isso, os dois recebiam o mesmo aperto, e quem não tinha
 * nota tomava a penalidade.) Dentro do mesmo jogador, as duas ações trocam.
 * Quem joga no controle não recebe teclas: as dele ficam como estão. */
static void atribui_tecla(Input *inp, int j, int a, SDL_Scancode tecla)
{
    Perfil *p = &inp->perfil[j];
    for (int outra = 0; outra < ACAO_N_REMAPEAVEIS; outra++) {
        if (outra != a && p->mapa[outra].tecla == tecla) p->mapa[outra].tecla = p->mapa[a].tecla;
    }
    p->mapa[a].tecla = tecla;
    SDL_Log("[input] jogador %d: %s -> tecla %s", j + 1, Acao_id((Acao)a), nome_tecla(tecla));

    for (int o = 0; o < INPUT_MAX_JOGADORES; o++) {
        if (o == j || !inp->no_teclado[o]) continue;
        bool tirou = false;
        for (int b = 0; b < ACAO_N_REMAPEAVEIS; b++) {
            if (inp->perfil[o].mapa[b].tecla != tecla) continue;
            inp->perfil[o].mapa[b].tecla = SDL_SCANCODE_UNKNOWN;
            SDL_Log("[input] a tecla %s saiu do jogador %d (%s)", nome_tecla(tecla), o + 1, Acao_id((Acao)b));
            tirou = true;
        }
        if (tirou) salva(inp, o);
    }
}

/* Arquivos de antes desta regra podem ter a mesma tecla em dois jogadores.
 * Fica com quem a escolheu de propósito (a tecla não é a padrão dele); se
 * nenhum ou os dois escolheram, fica com o de número menor. */
static void resolve_conflitos(Input *inp)
{
    for (int j = 0; j < INPUT_MAX_JOGADORES; j++) {
        for (int a = 0; a < ACAO_N_REMAPEAVEIS; a++) {
            const SDL_Scancode t = inp->perfil[j].mapa[a].tecla;
            bool saiu = t == SDL_SCANCODE_UNKNOWN;   /* a tecla de j saiu: nada mais a comparar */
            for (int o = j + 1; o < INPUT_MAX_JOGADORES && !saiu; o++) {
                for (int b = 0; b < ACAO_N_REMAPEAVEIS && !saiu; b++) {
                    if (inp->perfil[o].mapa[b].tecla != t) continue;
                    /* só o jogador 1 tem teclas padrão: nos outros, toda tecla foi escolhida */
                    saiu = j == 0 && PADRAO[a].tecla == t;
                    const int perde = saiu ? j : o;
                    inp->perfil[perde].mapa[saiu ? a : b].tecla = SDL_SCANCODE_UNKNOWN;
                    SDL_Log("[input] tecla %s estava no jogador %d e no %d: ficou com o %d", nome_tecla(t),
                            j + 1, o + 1, (saiu ? o : j) + 1);
                    salva(inp, perde);
                }
            }
        }
    }
}

void Input_capture_start(Input *inp, int j, Acao a)
{
    if (a < 0 || a >= ACAO_N_REMAPEAVEIS) return;
    inp->capturando        = (int)a;
    inp->captura_jogador   = jog(j);
    inp->captura_inicio_ms = SDL_GetTicks();
    SDL_zeroa(inp->bordas);
}

double Input_capture_left(const Input *inp)
{
    if (inp->capturando < 0) return 0.0;
    const double passou = (double)(SDL_GetTicks() - inp->captura_inicio_ms) / 1000.0;
    const double resta  = INPUT_CAPTURA_SEGUNDOS - passou;
    return resta > 0.0 ? resta : 0.0;
}

void Input_toggle_mode(Input *inp, int j)
{
    j = jog(j);
    inp->perfil[j].guitarra = !inp->perfil[j].guitarra;
    salva(inp, j);
    SDL_Log("[input] jogador %d: modo %s", j + 1, inp->perfil[j].guitarra ? "guitarra" : "controle");
}

bool Input_guitar_mode(const Input *inp, int j)
{
    return inp->perfil[jog(j)].guitarra;
}

void Input_toggle_stick(Input *inp, int j)
{
    j = jog(j);
    inp->perfil[j].analogico  = !inp->perfil[j].analogico;
    inp->ctl[j].alavanca_cima = false;
    salva(inp, j);
    SDL_Log("[input] jogador %d: whammy e star power no analogico %s", j + 1, Input_stick_label(inp, j));
}

/* nomes de controles que (no PortMaster) chamam os frontais pela letra
   impressa, no estilo Nintendo: o "b" da SDL é o botão de baixo */
static const char *const NINTENDO[] = {
    "GO-Super Gamepad", "GO-Advance Gamepad", "GO-Ultra Gamepad", "OpenSimHardware OSH PB Controller",
};

/* o controle conectado é do estilo Nintendo? */
static bool detecta_nintendo(SDL_Gamepad *pad)
{
    const char *nome = SDL_GetGamepadName(pad);
    for (size_t i = 0; nome && i < SDL_arraysize(NINTENDO); i++)
        if (SDL_strcmp(nome, NINTENDO[i]) == 0) return true;
    /* outros: "a" e "b" apontando para os botões crus 1 e 0, ao contrário do
       normal (no Xbox, a:b0 e b:b1), é o mesmo sinal */
    char *mapa = SDL_GetGamepadMapping(pad);
    const bool trocado = mapa && SDL_strstr(mapa, ",a:b1,") && SDL_strstr(mapa, ",b:b0,");
    SDL_free(mapa);
    return trocado;
}

static void decide_frontais(Input *inp, int j)
{
    Perfil   *p = &inp->perfil[j];
    Controle *c = &inp->ctl[j];
    if (p->frontais == FRONTAIS_XBOX)          c->trocado = false;
    else if (p->frontais == FRONTAIS_NINTENDO) c->trocado = true;
    else c->trocado = c->gamepad && detecta_nintendo(c->gamepad);

    /* mapeamentos de um arquivo antigo (com "a"/"b" da SDL): agora que o
       controle é conhecido, viram posição física de vez */
    bool converteu = false;
    for (int a = 0; a < ACAO_N; a++) {
        if (!p->mapa[a].cru || !c->gamepad) continue;
        p->mapa[a].pad = converte_frontal(c->trocado, p->mapa[a].pad);
        p->mapa[a].cru = false;
        converteu = true;
    }
    if (converteu) salva(inp, j);
    SDL_Log("[input] jogador %d: botoes frontais %s", j + 1, Input_layout_label(inp, j));
}

void Input_cycle_layout(Input *inp, int j)
{
    j = jog(j);
    inp->perfil[j].frontais = (inp->perfil[j].frontais + 1) % FRONTAIS_N;
    decide_frontais(inp, j);
    salva(inp, j);
}

/* o arquivo do perfil: o do convidado do espaço, ou o da conta (pelo código
   do nome: o nome pode ter acento, e o cartão SD pode não gostar) */
static void arquivo_do_perfil(const Input *inp, int j, const char *conta, char *buf, size_t n)
{
    if (!inp->pasta[0]) {
        buf[0] = '\0';
    } else if (conta[0]) {
        Uint32 h = 2166136261u;   /* FNV-1a */
        for (const unsigned char *c = (const unsigned char *)conta; *c; c++) h = (h ^ *c) * 16777619u;
        SDL_snprintf(buf, n, "%scontas/%08x.cfg", inp->pasta, (unsigned)h);
    } else if (j == 0) {
        SDL_snprintf(buf, n, "%scontroles.cfg", inp->pasta);
    } else {
        SDL_snprintf(buf, n, "%scontroles%d.cfg", inp->pasta, j + 1);
    }
}

static void decide_frontais(Input *inp, int j);

/* lê o perfil (o padrão, e por cima o arquivo) */
static void le_perfil(Input *inp, int j, const char *conta)
{
    Perfil *p = &inp->perfil[j];
    padrao(p, j);
    p->frontais = FRONTAIS_AUTO;
    p->guitarra = false;
    SDL_strlcpy(p->conta, conta, sizeof(p->conta));
    arquivo_do_perfil(inp, j, conta, p->arquivo, sizeof(p->arquivo));
    carrega(inp, j);
}

void Input_use_account(Input *inp, int j, const char *conta)
{
    j = jog(j);
    if (!conta) conta = "";
    Perfil *p = &inp->perfil[j];
    if (SDL_strcmp(p->conta, conta) == 0) return;

    char arquivo[512];
    arquivo_do_perfil(inp, j, conta, arquivo, sizeof(arquivo));
    if (!conta[0] || !arquivo[0] || SDL_GetPathInfo(arquivo, NULL)) {
        le_perfil(inp, j, conta);
    } else {   /* conta sem perfil ainda: usa o do convidado deste espaço; o arquivo dela nasce na primeira mudança */
        le_perfil(inp, j, "");
        SDL_strlcpy(p->conta, conta, sizeof(p->conta));
        SDL_strlcpy(p->arquivo, arquivo, sizeof(p->arquivo));
        SDL_Log("[input] %s ainda sem perfil: usa o do convidado do jogador %d", conta, j + 1);
    }
    inp->ctl[j].alavanca_cima = false;
    decide_frontais(inp, j);
}

void Input_set_keyboard_seat(Input *inp, int j, bool teclado)
{
    inp->no_teclado[jog(j)] = teclado;
}

int Input_keys_state(const Input *inp, int j)
{
    j = jog(j);
    const Perfil *p = &inp->perfil[j];
    bool tem = false;
    for (int a = 0; a < ACAO_N_TRASTES; a++) tem |= p->mapa[a].tecla != SDL_SCANCODE_UNKNOWN;
    if (!tem) return 1;
    for (int a = 0; a < ACAO_N_REMAPEAVEIS; a++) {
        const SDL_Scancode t = p->mapa[a].tecla;
        if (t == SDL_SCANCODE_UNKNOWN) continue;
        for (int o = 0; o < INPUT_MAX_JOGADORES; o++) {
            if (o == j || !inp->no_teclado[o]) continue;
            for (int b = 0; b < ACAO_N_REMAPEAVEIS; b++)
                if (inp->perfil[o].mapa[b].tecla == t) return 2;
        }
    }
    return 0;
}

void Input_reset_defaults(Input *inp, int j)
{
    j = jog(j);
    padrao(&inp->perfil[j], j);
    salva(inp, j);
    SDL_Log("[input] controles do jogador %d restaurados para o padrao", j + 1);
}

/* ---------------------------------------------------------------------------
 * Eventos: cada BUTTON_DOWN / KEY_DOWN já é uma borda false -> true.
 * Aqui só anotamos; quem usa é o Input_frame.
 * ------------------------------------------------------------------------- */

/* o espaço (jogador) do controle que mandou o evento; -1 = nenhum */
static int jogador_do_pad(const Input *inp, SDL_JoystickID which)
{
    for (int j = 0; j < INPUT_MAX_JOGADORES; j++)
        if (inp->ctl[j].gamepad && inp->ctl[j].which == which) return j;
    return -1;
}

static void aperto_pad(Input *inp, int j, int fisico)
{
    SDL_snprintf(inp->ultimo, sizeof(inp->ultimo), "J%d: %s [%s]", j + 1, rotulo_pad(fisico), nome_pad(fisico));
    if (inp->log_apertos) SDL_Log("[input] %llu ms: controle %s", (unsigned long long)SDL_GetTicks(), inp->ultimo);

    if (inp->capturando >= 0) {
        if (j != inp->captura_jogador) return;   /* o botão novo vem do controle de quem está configurando */
        atribui_pad(&inp->perfil[j], inp->capturando, fisico);
        inp->capturando = -1;
        salva(inp, j);
        return;
    }
    for (int a = 0; a < ACAO_N; a++) {
        if (inp->perfil[j].mapa[a].pad == fisico) inp->bordas[j] |= ACAO_BIT(a);
    }
}

/* o teclado é dividido: cada jogador de teclado vê as teclas do seu perfil */
static void aperto_tecla(Input *inp, SDL_Scancode tecla)
{
    SDL_snprintf(inp->ultimo, sizeof(inp->ultimo), "tecla %s", nome_tecla(tecla));
    if (inp->log_apertos) SDL_Log("[input] %llu ms: %s", (unsigned long long)SDL_GetTicks(), inp->ultimo);

    if (inp->capturando >= 0) {
        if (tecla != SDL_SCANCODE_ESCAPE) {           /* Esc cancela a captura */
            atribui_tecla(inp, inp->captura_jogador, inp->capturando, tecla);
            salva(inp, inp->captura_jogador);
        }
        inp->capturando = -1;
        return;
    }
    for (int j = 0; j < INPUT_MAX_JOGADORES; j++)
        for (int a = 0; a < ACAO_N_REMAPEAVEIS && inp->no_teclado[j]; a++)
            if (inp->perfil[j].mapa[a].tecla == tecla) inp->bordas[j] |= ACAO_BIT(a);
    for (int a = ACAO_N_REMAPEAVEIS; a < ACAO_N; a++)
        if (TECLAS_MENU[a] == tecla) inp->bordas_teclado |= ACAO_BIT(a);
}

/* L2/R2: a SDL manda movimento de eixo (0..32767). Vira botão aqui. */
static void eixo_gatilho(Input *inp, int j, int lado, Sint16 valor)
{
    Controle *c = &inp->ctl[j];
    const bool antes = c->gatilho[lado];
    const bool agora = antes ? valor > GATILHO_DESLIGA : valor > GATILHO_LIGA;
    c->gatilho[lado] = agora;
    if (agora && !antes) aperto_pad(inp, j, lado == 0 ? INPUT_FISICO_L2 : INPUT_FISICO_R2);
}

/* empurrão forte para cima no analógico = star power (com histerese, para
   não disparar duas vezes no mesmo empurrão). Não entra na captura. */
static void alavanca_vertical(Input *inp, int j, Sint16 valor)
{
    Controle *c = &inp->ctl[j];
    const bool antes = c->alavanca_cima;
    const bool agora = antes ? valor < ALAVANCA_DESLIGA : valor < ALAVANCA_LIGA;
    c->alavanca_cima = agora;
    if (agora && !antes && inp->capturando < 0) {
        inp->bordas[j] |= ACAO_BIT(ACAO_STAR_POWER);
        SDL_snprintf(inp->ultimo, sizeof(inp->ultimo), "J%d: analogico %s para cima", j + 1, Input_stick_label(inp, j));
    }
}

/* Um analógico mexido com vontade: aparece no "último botão" da tela de
 * Controles, e, se a captura do Whammy está esperando (deste jogador),
 * escolhe esse analógico para o whammy (e o star power para cima). */
static void analogico_mexeu(Input *inp, int j, int eixo, Sint16 valor)
{
    int lado = -1;
    if (eixo == SDL_GAMEPAD_AXIS_LEFTX || eixo == SDL_GAMEPAD_AXIS_LEFTY)   lado = 0;
    if (eixo == SDL_GAMEPAD_AXIS_RIGHTX || eixo == SDL_GAMEPAD_AXIS_RIGHTY) lado = 1;
    if (lado < 0 || (valor > -24000 && valor < 24000)) return;

    SDL_snprintf(inp->ultimo, sizeof(inp->ultimo), "J%d: analogico %s", j + 1, lado ? "direito" : "esquerdo");
    if (inp->capturando == ACAO_WHAMMY && inp->captura_jogador == j) {
        Perfil *p = &inp->perfil[j];
        p->analogico = lado;
        p->mapa[ACAO_WHAMMY].pad = INPUT_FISICO_NENHUM;   /* o whammy é o analógico, não um botão */
        inp->ctl[j].alavanca_cima = false;
        inp->capturando = -1;
        salva(inp, j);
        SDL_Log("[input] jogador %d: whammy -> analogico %s", j + 1, Input_stick_label(inp, j));
    }
}

/* controle novo: vai para o primeiro espaço livre (nem com controle, nem de
   quem joga no teclado) */
static void conecta(Input *inp, SDL_JoystickID which)
{
    if (jogador_do_pad(inp, which) >= 0) return;   /* já está aberto */
    int j = 0;
    while (j < INPUT_MAX_JOGADORES && (inp->ctl[j].gamepad || inp->no_teclado[j])) j++;
    if (j == INPUT_MAX_JOGADORES) {
        SDL_Log("[input] controle ignorado: os %d espacos estao ocupados", INPUT_MAX_JOGADORES);
        return;
    }
    Controle *c = &inp->ctl[j];
    SDL_zerop(c);
    c->gamepad = SDL_OpenGamepad(which);
    if (!c->gamepad) {
        SDL_Log("[input] nao abri o controle: %s", SDL_GetError());
        return;
    }
    c->which = which;
    /* o mapeamento diz qual botão físico a SDL chama de a/b/x/y: é a
       resposta para "o botão de baixo é o A ou o B?" */
    char *mapa = SDL_GetGamepadMapping(c->gamepad);
    SDL_Log("[input] controle do jogador %d: %s (tipo %s)", j + 1, SDL_GetGamepadName(c->gamepad),
            SDL_GetGamepadStringForType(SDL_GetGamepadType(c->gamepad)));
    SDL_Log("[input] mapeamento SDL: %s", mapa ? mapa : "(nenhum)");
    SDL_free(mapa);
    decide_frontais(inp, j);
}

/* Select ou Start servem de traste neste perfil? (aí eles não fecham o jogo) */
static bool sair_e_traste(const Perfil *p)
{
    for (int a = 0; a < ACAO_N_TRASTES; a++)
        if (p->mapa[a].pad == SDL_GAMEPAD_BUTTON_BACK || p->mapa[a].pad == SDL_GAMEPAD_BUTTON_START) return true;
    return false;
}

/* Um joystick que a SDL não sabe usar como controle (sem mapeamento): não
 * dá para jogar com ele, mas o log diz o nome e o GUID, para quem quiser
 * escrever um mapeamento em conf/arkhero/gamecontrollerdb.txt. */
static void joystick_sem_mapa(SDL_JoystickID which)
{
    if (SDL_IsGamepad(which)) return;
    char guid[64];
    SDL_GUIDToString(SDL_GetJoystickGUIDForID(which), guid, sizeof(guid));
    const char *nome = SDL_GetJoystickNameForID(which);
    SDL_Log("[input] joystick sem mapeamento de controle: %s (GUID %s). Para usar, ponha uma linha dele em "
            "gamecontrollerdb.txt, na pasta de dados do jogo", nome ? nome : "?", guid);
}

void Input_event(Input *inp, const SDL_Event *ev)
{
    switch (ev->type) {
    case SDL_EVENT_JOYSTICK_ADDED:
        joystick_sem_mapa(ev->jdevice.which);
        break;

    case SDL_EVENT_GAMEPAD_ADDED:
        conecta(inp, ev->gdevice.which);
        break;

    case SDL_EVENT_GAMEPAD_REMOVED: {
        const int j = jogador_do_pad(inp, ev->gdevice.which);
        if (j < 0) break;
        SDL_CloseGamepad(inp->ctl[j].gamepad);
        SDL_zero(inp->ctl[j]);
        SDL_Log("[input] controle do jogador %d desconectado", j + 1);
        break;
    }

    case SDL_EVENT_GAMEPAD_BUTTON_DOWN: {
        const int j = jogador_do_pad(inp, ev->gbutton.which);
        if (j < 0) break;
        SDL_Gamepad *pad = inp->ctl[j].gamepad;
        const SDL_GamepadButton b = (SDL_GamepadButton)ev->gbutton.button;

        /* Select + Start fecha o jogo, em qualquer tela (padrão dos ports).
           Menos quando esses dois são trastes no perfil do jogador: há
           controles que chamam L2/R2 de Select/Start, e um acorde fecharia o jogo. */
        if (((b == SDL_GAMEPAD_BUTTON_START && SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_BACK)) ||
             (b == SDL_GAMEPAD_BUTTON_BACK  && SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_START))) &&
            !sair_e_traste(&inp->perfil[j])) {
            inp->sair = true;
            break;
        }
        aperto_pad(inp, j, converte_frontal(inp->ctl[j].trocado, (int)b));   /* SDL -> posição física */
        break;
    }

    case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
        const int j = jogador_do_pad(inp, ev->gaxis.which);
        if (j < 0) break;
        Controle *c = &inp->ctl[j];
        if (ev->gaxis.axis < SDL_GAMEPAD_AXIS_COUNT) c->eixo[ev->gaxis.axis] = ev->gaxis.value;
        if (ev->gaxis.axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER)  eixo_gatilho(inp, j, 0, ev->gaxis.value);
        if (ev->gaxis.axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) eixo_gatilho(inp, j, 1, ev->gaxis.value);
        if (ev->gaxis.axis == EIXO_Y[inp->perfil[j].analogico]) alavanca_vertical(inp, j, ev->gaxis.value);
        analogico_mexeu(inp, j, ev->gaxis.axis, ev->gaxis.value);
        break;
    }

    case SDL_EVENT_KEY_DOWN:
        if (!ev->key.repeat) aperto_tecla(inp, ev->key.scancode);   /* repeat = tecla segurada, não é borda */
        break;

    default:
        break;
    }
}

/* ---------------------------------------------------------------------------
 * Uma vez por frame
 * ------------------------------------------------------------------------- */

static bool pad_segurado(const Controle *c, int fisico)
{
    if (!c->gamepad || fisico == INPUT_FISICO_NENHUM) return false;
    if (fisico == INPUT_FISICO_L2) return c->gatilho[0];
    if (fisico == INPUT_FISICO_R2) return c->gatilho[1];
    return SDL_GetGamepadButton(c->gamepad, (SDL_GamepadButton)converte_frontal(c->trocado, fisico));
}

void Input_frame(Input *inp, Botoes saida[INPUT_MAX_JOGADORES], Botoes *menu)
{
    SDL_memset(saida, 0, sizeof(Botoes) * INPUT_MAX_JOGADORES);
    *menu = (Botoes){0};

    /* esperando um botão novo: o jogo não recebe nada, para o menu não andar */
    if (inp->capturando >= 0) {
        if (Input_capture_left(inp) <= 0.0) {
            SDL_Log("[input] captura de %s expirou", Acao_id((Acao)inp->capturando));
            inp->capturando = -1;
        }
        SDL_zeroa(inp->bordas);
        inp->bordas_teclado = 0;
        return;
    }

    const bool *teclado = SDL_GetKeyboardState(NULL);

    /* as teclas de menu: de ninguém em particular */
    for (int a = ACAO_N_REMAPEAVEIS; a < ACAO_N; a++)
        if (teclado[TECLAS_MENU[a]]) menu->segurados |= ACAO_BIT(a);
    menu->apertados     = inp->bordas_teclado;
    inp->bordas_teclado = 0;
    for (int j = 0; j < INPUT_MAX_JOGADORES; j++) {
        const Perfil   *p = &inp->perfil[j];
        const Controle *c = &inp->ctl[j];
        Botoes         *b = &saida[j];

        /* "segurados" vem do estado atual; vários botões podem levar à mesma ação
           (as teclas, só nas ações de jogo e de quem joga no teclado: as de
           menu são do teclado) */
        for (int a = 0; a < ACAO_N; a++) {
            const SDL_Scancode t = a < ACAO_N_REMAPEAVEIS && inp->no_teclado[j] ? p->mapa[a].tecla : SDL_SCANCODE_UNKNOWN;
            if (pad_segurado(c, p->mapa[a].pad) || (t != SDL_SCANCODE_UNKNOWN && teclado[t]))
                b->segurados |= ACAO_BIT(a);
        }
        if (c->alavanca_cima) b->segurados |= ACAO_BIT(ACAO_STAR_POWER);

        /* "apertados" vem dos eventos: pega até o toque que começa e termina
           dentro do mesmo frame, que a comparação de estados perderia */
        b->apertados   = inp->bordas[j];
        inp->bordas[j] = 0;

        /* whammy: quanto o analógico está para o lado (qualquer lado), sem a
           zona morta do centro; a tecla vale o curso inteiro */
        float x = SDL_fabsf((float)c->eixo[EIXO_X[p->analogico]] / 32767.0f);
        x = (x - WHAMMY_ZONA_MORTA) / (1.0f - WHAMMY_ZONA_MORTA);
        b->whammy = x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x);
        if (b->segurados & ACAO_BIT(ACAO_WHAMMY)) b->whammy = 1.0f;
    }
}

/* ---------------------------------------------------------------------------
 * Ciclo de vida
 * ------------------------------------------------------------------------- */

bool Input_init(Input *inp)
{
    if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
        SDL_Log("[input] SDL_InitSubSystem(GAMEPAD) falhou: %s", SDL_GetError());
        return false;
    }

    inp->capturando = -1;
    inp->sair       = false;
    SDL_strlcpy(inp->ultimo, "-", sizeof(inp->ultimo));

    /* No console o script de launch aponta XDG_DATA_HOME para conf/, então
       isto vira <pasta do jogo>/conf/arkhero/. No PC, ~/.local/share/arkhero/. */
    char *pasta = SDL_GetPrefPath("", "arkhero");
    if (!pasta) SDL_Log("[input] sem pasta para salvar os controles: %s", SDL_GetError());

    /* mapeamentos extras (controles que a SDL não conhece), um por linha, no
       formato do SDL_GameControllerDB: <pasta de dados>/gamecontrollerdb.txt */
    if (pasta) {
        char db[600];
        SDL_snprintf(db, sizeof(db), "%sgamecontrollerdb.txt", pasta);
        const int n = SDL_AddGamepadMappingsFromFile(db);
        if (n > 0) SDL_Log("[input] %d mapeamento(s) de controle de %s", n, db);
    }
    if (pasta) SDL_strlcpy(inp->pasta, pasta, sizeof(inp->pasta));
    SDL_free(pasta);
    for (int j = 0; j < INPUT_MAX_JOGADORES; j++) le_perfil(inp, j, "");   /* o convidado de cada espaço */
    resolve_conflitos(inp);

    /* o mapeamento do jogador 1, de uma vez, para conferir no log.txt */
    char linha[512] = "";
    for (int a = 0; a < ACAO_N_REMAPEAVEIS; a++) {
        char item[48];
        SDL_snprintf(item, sizeof(item), "%s=%s/%s ", Acao_id((Acao)a), nome_pad(inp->perfil[0].mapa[a].pad),
                     nome_tecla(inp->perfil[0].mapa[a].tecla));
        SDL_strlcat(linha, item, sizeof(linha));
    }
    SDL_Log("[input] controles do jogador 1: %s analogico=%s", linha, Input_stick_label(inp, 0));

    const char *log_apertos = SDL_getenv("ARKHERO_LOG_INPUT");
    inp->log_apertos = log_apertos && SDL_strcmp(log_apertos, "1") == 0;
    return true;
}

void Input_quit(Input *inp)
{
    for (int j = 0; j < INPUT_MAX_JOGADORES; j++) {
        if (inp->ctl[j].gamepad) SDL_CloseGamepad(inp->ctl[j].gamepad);
        inp->ctl[j].gamepad = NULL;
    }
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
}

const char *Input_pad_label(const Input *inp, int j, Acao a)
{
    if (a < 0 || a >= ACAO_N) return "-";
    const Perfil *p = &inp->perfil[jog(j)];
    if (a == ACAO_WHAMMY && p->mapa[a].pad == INPUT_FISICO_NENHUM) return p->analogico ? N_("Analóg. D") : N_("Analóg. E");
    return rotulo_pad(p->mapa[a].pad);
}

const char *Input_key_label(const Input *inp, int j, Acao a)
{
    if (a < 0 || a >= ACAO_N) return "-";
    if (a >= ACAO_N_REMAPEAVEIS) return rotulo_tecla(TECLAS_MENU[a]);   /* as de menu são do teclado */
    return rotulo_tecla(inp->perfil[jog(j)].mapa[a].tecla);
}

const char *Input_stick_label(const Input *inp, int j)
{
    return inp->perfil[jog(j)].analogico ? N_("Direito") : N_("Esquerdo");
}

const char *Input_layout_label(const Input *inp, int j)
{
    j = jog(j);
    switch (inp->perfil[j].frontais) {
    case FRONTAIS_XBOX:     return "Xbox";
    case FRONTAIS_NINTENDO: return "Nintendo";
    default:                return inp->ctl[j].trocado ? "Auto (Nintendo)" : "Auto (Xbox)";
    }
}

const char *Input_gamepad_name(const Input *inp, int j)
{
    const Controle *c = &inp->ctl[jog(j)];
    if (!c->gamepad) return NULL;
    const char *nome = SDL_GetGamepadName(c->gamepad);
    return nome ? nome : "controle";
}
