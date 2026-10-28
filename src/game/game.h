#ifndef ARK_GAME_H
#define ARK_GAME_H

/* O estado do jogo e a lógica que o altera. Sem SDL: recebe ações e tempo,
 * produz estado e eventos. O draw lê este estado (const) e desenha. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game/acao.h"
#include "game/chart.h"
#include "game/chartfile.h"
#include "game/songini.h"

typedef enum {
    TELA_INICIO,      /* menu principal */
    TELA_MUSICAS,     /* escolher a música */
    TELA_DIFICULDADE, /* escolher a dificuldade */
    TELA_CARREGANDO,  /* decodificando o áudio da música escolhida */
    TELA_CONFIG,      /* configurações */
    TELA_CONTROLES,   /* remapear botões */
    TELA_AUDIO,       /* teste de áudio: carregar e tocar a música de teste */
    TELA_JOGO,        /* a pista */
    TELA_N
} Tela;

/* ---------------------------------------------------------------------------
 * Menus: listas de itens, definidas como dados em menu.c.
 * Todas as telas de menu navegam do mesmo jeito (game.c).
 * ------------------------------------------------------------------------- */
typedef enum {
    ITEM_BOTAO,   /* confirmar executa alguma coisa */
    ITEM_OPCAO    /* esquerda/direita muda um valor */
} TipoItem;

typedef enum {
    ID_JOGAR, ID_CONFIG, ID_SAIR,
    ID_CONTROLES, ID_MODO, ID_VELOCIDADE, ID_PRECISAO, ID_CANHOTO, ID_FPS, ID_VOLUME, ID_CALIBRAR,
    ID_PROCURAR,     /* lista de músicas: procurar de novo nas pastas */
    ID_VOLTAR, ID_RESTAURAR, ID_ANALOGICO,
    ID_AUDIO_TESTE, ID_AUDIO_CARREGAR, ID_AUDIO_TOCAR,
    ID_CONTINUAR, ID_REINICIAR, ID_SAIR_MENU,
    ID_DEMO,         /* a música de demonstração, sem áudio */

    /* faixas de ids: precisam vir DEPOIS de todos os ids fixos acima, porque o
       executa() decide pelo intervalo (id >= ID_MAPEAR, id >= ID_DIFICULDADE...) */
    ID_MUSICA,                              /* ID_MUSICA + i: a música i da lista */
    ID_DIFICULDADE = ID_MUSICA + 1000,      /* ID_DIFICULDADE + d */
    ID_MAPEAR      = ID_DIFICULDADE + 100   /* ID_MAPEAR + Acao: remapear aquela ação */
} IdItem;

typedef struct {
    const char *rotulo;    /* só ASCII (fonte de debug) */
    int         id;        /* IdItem, ou ID_MAPEAR + Acao */
    TipoItem    tipo;
    bool        em_breve;  /* aparece, mas ainda não faz nada */
} ItemMenu;

typedef struct {
    const char     *titulo;
    const ItemMenu *itens;
    int             n;
} Menu;

/* ---------------------------------------------------------------------------
 * Opções do jogador (ainda não são salvas em arquivo; isso vem com a Lua)
 * ------------------------------------------------------------------------- */
#define GAME_VELOCIDADE_MIN 1
#define GAME_VELOCIDADE_MAX 10

typedef struct {
    int  velocidade;    /* 1..10: quão rápido as notas descem */
    bool canhoto;       /* inverte a ordem das faixas */
    bool mostrar_fps;
    bool precisao;      /* modo precisão: janela de acerto mais curta */
    int  volume;        /* 0..100 */
} Opcoes;

/* ---------------------------------------------------------------------------
 * Regras do julgamento (depois vêm do config.lua)
 * ------------------------------------------------------------------------- */
#define GAME_JANELA_NORMAL   0.100   /* segundos para cada lado da nota */
#define GAME_JANELA_PRECISAO 0.070   /* modo precisão (era o padrão até a etapa 4) */
#define GAME_PERFEITO        0.40    /* "Perfeito" = dentro de 40% da janela */
#define GAME_SUSTAIN_FOLGA   0.100   /* soltar nos últimos 100 ms conta como completo */
#define GAME_SP_FRASE        0.25f   /* uma frase de star power completa enche 25% */
#define GAME_SP_MINIMO       0.50f   /* para ativar, o medidor precisa de 50% */
#define GAME_SP_DURACAO      16.0f   /* segundos para o medidor cheio esvaziar */

/* ---------------------------------------------------------------------------
 * Eventos: tudo o que aconteceu no frame. A pontuação (score.c, depois a
 * Lua) e o draw reagem a eles. A lista é zerada no começo de cada update.
 * ------------------------------------------------------------------------- */
typedef enum {
    EV_ACERTO,         /* nota = índice, valor = erro de tempo (s; negativo = adiantado) */
    EV_ERRO,           /* nota = índice */
    EV_SUSTAIN,        /* nota = índice, valor = segundos segurados neste frame */
    EV_FRASE_STAR,     /* frase de star power completa */
    EV_STAR_ATIVOU
} TipoEvento;

typedef struct {
    TipoEvento tipo;
    int        nota;
    double     valor;
} EventoJogo;

#define GAME_MAX_EVENTOS 64

/* Pedidos: coisas que o game decide mas não pode fazer sozinho, porque são
 * da plataforma (remapear um botão, fechar o programa). O main.c lê e atende
 * no mesmo frame. Valem só por um frame, como uma caixa de saída. */
typedef enum {
    PEDIDO_NENHUM,
    PEDIDO_MAPEAR,               /* pedido_acao = qual ação remapear */
    PEDIDO_RESTAURAR_CONTROLES,
    PEDIDO_TROCAR_ANALOGICO,
    PEDIDO_AUDIO_CARREGAR,       /* tela de áudio: decodificar a música de teste */
    PEDIDO_AUDIO_TOCAR,          /* tela de áudio: tocar / pausar */
    PEDIDO_ABRIR_MUSICA,         /* pedido_indice = música: o main.c lê o notes.chart e chama Game_open_chart */
    PEDIDO_CARREGAR_MUSICA,      /* pedido_indice = música: decodificar o áudio dela */
    PEDIDO_CANCELAR_CARGA,
    PEDIDO_PROCURAR_MUSICAS,     /* varrer as pastas de novo */
    PEDIDO_SAIR
} Pedido;

/* ---------------------------------------------------------------------------
 * A lista de músicas: o main.c acha as pastas e lê os song.ini; o game guarda
 * o que precisa mostrar e monta o menu.
 * ------------------------------------------------------------------------- */
typedef struct {
    char titulo[64];
    char artista[48];
    char charter[48];
    int  duracao_ms;
    int  atraso_ms;
} InfoMusica;

#define GAME_PAGINA 7   /* Esq/Dir na lista de músicas pula isso */

typedef struct {
    Tela    tela;
    int     sel[TELA_N];     /* item selecionado em cada tela (volta onde estava) */
    Opcoes  opcoes;

    /* tempo */
    double  t_total;         /* segundos desde o início: animações de menu */
    double  t;               /* posição na música (segundos) */

    /* Relógio da música. Com áudio, o main.c põe aqui a posição do áudio a cada
       frame (o relógio mestre) e o jogo segue ela; sem áudio (ou nos testes),
       o tempo anda com o dt. */
    bool    relogio_audio;
    double  t_audio;
    int     partida;         /* muda a cada partida começada: o main.c (re)inicia o som */

    /* partida */
    Chart    chart;
    int      proxima;          /* primeira nota ainda pendente: o juiz começa aqui */
    int      primeira_visivel; /* primeira nota que ainda aparece na pista */
    bool     pausado;
    bool     terminou;
    int      sel_pausa;        /* item selecionado na pausa / no fim */
    double   segura_t[2];      /* há quanto tempo Cima/Baixo estão segurados (repetição) */
    double   repete_t[2];
    uint32_t segurados;        /* ações seguradas neste frame */
    float    whammy;           /* 0..1 */

    /* placar */
    double   pontos;
    double   mult_mudou_t;     /* t_total em que o multiplicador mudou (animação) */
    int      multiplicador;    /* 1..4, x2 com star power */
    int      combo, maior_combo;
    int      acertos, erros;   /* em notas (um acorde conta uma) */

    /* star power */
    float    sp_medidor;       /* 0..1 */
    bool     sp_ativo;
    bool     sp_frase_falhou;  /* errou alguma nota da frase atual */

    /* para os efeitos do draw (ele anima a partir de "quando aconteceu") */
    double   acerto_t[5];      /* t do último acerto em cada faixa */

    /* estatística da partida, para o log (a média do erro de tempo ajuda a
       calibrar: média negativa = o jogador aperta adiantado) */
    double   soma_erro, soma_erro2;
    int      pausas;
    double   ultimo_julgamento_t;
    double   ultimo_erro_tempo;  /* erro de tempo do último acerto (s) */
    bool     ultimo_foi_erro;

    /* o que aconteceu neste frame */
    EventoJogo eventos[GAME_MAX_EVENTOS];
    int        n_eventos;

    /* aviso curto na tela ("Em breve") */
    const char *aviso;
    double      aviso_ate;   /* em t_total */

    /* músicas */
    InfoMusica  *musicas;                 /* cresce conforme o main.c acrescenta */
    int          n_musicas, cap_musicas;
    ItemMenu    *itens_musicas;           /* demonstração + músicas + procurar + voltar */
    int          cap_itens;
    Menu         menu_musicas;
    int          musica;                  /* a escolhida: índice, ou -1 = demonstração */
    ArquivoChart arquivo;                 /* o notes.chart dela, já lido */
    bool         arquivo_ok;
    ItemMenu     itens_dif[DIF_N + 1];
    char         rotulos_dif[DIF_N][32];
    Menu         menu_dif;
    Dificuldade  dificuldade;

    /* saída para o main.c */
    Pedido  pedido;
    Acao    pedido_acao;
    int     pedido_indice;
} Game;

void Game_init(Game *g);
void Game_quit(Game *g);
void Game_update(Game *g, const Botoes *b, double dt);
bool Game_load_chart(Game *g, const Nota *notas, int n);   /* troca a música (testes) */
void Game_play(Game *g);                                    /* começa a partida com o chart atual */

/* músicas (chamadas pelo main.c) */
void Game_clear_songs(Game *g);
void Game_add_song(Game *g, const SongIni *ini, const char *nome_pasta);
void Game_songs_done(Game *g);    /* depois de acrescentar: monta o menu */
bool Game_open_chart(Game *g, const char *texto, size_t tamanho);   /* vai para a dificuldade */
void Game_audio_ready(Game *g, bool ok, double duracao);            /* carga terminou: começa */

/* Para o draw (e para os testes) */
const Menu *Game_menu(const Game *g);            /* menu visível agora, ou NULL */
int         Game_menu_sel(const Game *g);        /* item selecionado nele */
double      Game_janela_visivel(const Game *g);  /* segundos de pista visíveis */
double      Game_janela_acerto(const Game *g);   /* segundos para cada lado da nota */
double      Game_janela_perfeito(const Game *g);

/* menu.c */
const Menu *Menu_da_tela(Tela t);   /* NULL para TELA_JOGO */
const Menu *Menu_pausa(void);
const Menu *Menu_fim(void);
void        Menu_valor(const Game *g, int id, char *buf, size_t n);   /* texto de um ITEM_OPCAO */

/* judge.c: o juiz do modo controle (uso interno do game, e dos testes) */
void Judge_toques(Game *g, uint32_t apertados, uint32_t segurados);
void Judge_sustains(Game *g, uint32_t segurados, double dt);
void Judge_varre_erros(Game *g);

/* score.c: pontos a partir dos eventos. O main.c chama depois do update
 * (quando a Lua entrar, ela pode assumir este papel). */
void Score_apply_events(Game *g);
int  Score_multiplicador(const Game *g);

#endif
