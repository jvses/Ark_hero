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
#include "game/texto.h"

typedef enum {
    TELA_INICIO,      /* menu principal */
    TELA_JOGADORES,   /* quem vai jogar, e com que conta */
    TELA_MUSICAS,     /* escolher a música */
    TELA_PREPARAR,    /* cada jogador escolhe instrumento e dificuldade */
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
/* Tudo funciona só com Cima/Baixo, Confirmar e Voltar (a palhetada e os
 * trastes verde e vermelho de uma guitarra). Esquerda/Direita, de quem tem,
 * mudam o valor de uma opção direto. */
typedef enum {
    ITEM_BOTAO,   /* confirmar executa alguma coisa */
    ITEM_OPCAO,   /* confirmar troca o valor (Sim/Não, ou poucos valores, em roda) */
    ITEM_VALOR    /* confirmar entra no modo de escolha: Cima/Baixo mudam, Confirmar fica, Voltar desfaz */
} TipoItem;

typedef enum {
    ID_JOGAR, ID_CONFIG, ID_SAIR,
    ID_CONTROLES, ID_VELOCIDADE, ID_PRECISAO, ID_PENALIDADE, ID_JULGAMENTO, ID_AJUSTE,
    ID_CANHOTO, ID_FPS, ID_VOLUME, ID_SOM_ERRO, ID_IDIOMA,
    ID_JOGADOR,      /* tela de controles: de qual jogador é a configuração */
    ID_PROCURAR,     /* lista de músicas: procurar de novo nas pastas */
    ID_ORGANIZAR,    /* lista de músicas: por pasta, título ou artista */
    ID_VOLTAR, ID_RESTAURAR, ID_ANALOGICO, ID_FRONTAIS,
    ID_AUDIO_TESTE, ID_AUDIO_CARREGAR, ID_AUDIO_TOCAR,
    ID_CONTINUAR, ID_REINICIAR, ID_SAIR_MENU,
    ID_DEMO,         /* a música de demonstração, sem áudio */

    /* Faixas de ids, todas acima dos ids fixos. O executa() decide pelo
       intervalo, do maior para o menor; a das músicas é a última porque não
       tem limite (uma biblioteca pode ter milhares). */
    ID_MAPEAR      = 1000,                  /* ID_MAPEAR + Acao: remapear aquela ação */
    ID_PASTA       = ID_MAPEAR + 100,       /* ID_PASTA + k: a subpasta k da pasta atual */
    ID_MUSICA      = ID_PASTA + 100000      /* ID_MUSICA + i: a música i */
} IdItem;

/* o que um id de item representa (-1 = não é desse tipo) */
int Game_item_acao(int id);
int Game_item_musica(int id);
int Game_item_pasta(int id);

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
 * Opções (o main.c grava em opcoes.cfg; valem para todos os jogadores)
 * ------------------------------------------------------------------------- */
#define GAME_VELOCIDADE_MIN 1
#define GAME_VELOCIDADE_MAX 10
#define GAME_AJUSTE_MAX     300   /* ms, para cada lado */
#define GAME_AJUSTE_PASSO   5

typedef struct {
    int  velocidade;    /* 1..10: quão rápido as notas descem */
    bool canhoto;       /* inverte a ordem das faixas */
    bool mostrar_fps;
    bool precisao;      /* modo precisão: janela de acerto mais curta */
    bool penalidade;    /* apertar sem nota quebra o combo */
    bool mostrar_julgamento;   /* "Perfeito / Bom / Errou" na pista */
    int  ajuste_ms;     /* ajuste de tempo: + = as notas chegam antes (compensa o atraso do som) */
    int  organizar;     /* lista de músicas: ORGANIZAR_PASTAS, _TITULO, _ARTISTA */
    int  volume;        /* 0..100 */
    bool som_erro;      /* o instrumento "engasga" quando o jogador erra */
    char idioma[16];    /* código do arquivo assets/lang/<codigo>.txt; "pt" = o embutido */
    char conta[4][24];  /* a última conta de cada jogador ("" = convidado) */
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
    EV_TOQUE_ERRADO,   /* apertou um botão sem nota (com a penalidade ligada) */
    EV_FRASE_STAR,     /* frase de star power completa */
    EV_STAR_ATIVOU
} TipoEvento;

typedef struct {
    TipoEvento tipo;
    int        jogador;   /* índice em Game.jog */
    int        nota;
    double     valor;
} EventoJogo;

#define GAME_MAX_EVENTOS 64

/* Pedidos: coisas que o game decide mas não pode fazer sozinho, porque são
 * da plataforma (remapear um botão, fechar o programa). O main.c lê e atende
 * no mesmo frame. Valem só por um frame, como uma caixa de saída. */
typedef enum {
    PEDIDO_NENHUM,
    PEDIDO_MAPEAR,               /* pedido_acao = qual ação remapear (do jogador controles_jogador) */
    PEDIDO_RESTAURAR_CONTROLES,
    PEDIDO_TROCAR_ANALOGICO,
    PEDIDO_TROCAR_FRONTAIS,
    PEDIDO_AUDIO_CARREGAR,       /* tela de áudio: decodificar a música de teste */
    PEDIDO_AUDIO_TOCAR,          /* tela de áudio: tocar / pausar */
    PEDIDO_ABRIR_MUSICA,         /* pedido_indice = música: o main.c lê o notes.chart e chama Game_open_chart */
    PEDIDO_CARREGAR_MUSICA,      /* pedido_indice = música: decodificar o áudio dela */
    PEDIDO_CANCELAR_CARGA,
    PEDIDO_PROCURAR_MUSICAS,     /* varrer as pastas de novo */
    PEDIDO_SALVAR_RECORDE,       /* pedido_indice = música com recorde novo */
    PEDIDO_SALVAR_OPCOES,
    PEDIDO_TROCAR_IDIOMA,        /* opcoes.idioma mudou: o main.c carrega o arquivo dele */
    PEDIDO_TOCAR_SOM_ERRO,       /* o som de erro foi ligado: uma amostra dele */
    PEDIDO_SALVAR_CONTAS,        /* uma conta nova: o main.c grava jogadores.txt */
    PEDIDO_SAIR
} Pedido;

/* ---------------------------------------------------------------------------
 * A lista de músicas: o main.c acha as pastas e lê os song.ini; o game guarda
 * o que precisa mostrar e monta o menu.
 * ------------------------------------------------------------------------- */
enum { ORGANIZAR_PASTAS, ORGANIZAR_TITULO, ORGANIZAR_ARTISTA, ORGANIZAR_N };

typedef struct {
    char titulo[64];
    char artista[48];
    char charter[48];
    char grupo[128];          /* subpasta dentro da pasta de músicas: "Guitar Hero 3/Tier 1" */
    char rotulo_artista[120]; /* "Artista - Titulo", para a lista por artista */
    int  duracao_ms;
    int  atraso_ms;
    int  nota_sp;             /* nota MIDI do star power (song.ini "multiplier_note") */

    /* recorde: a maior pontuação em qualquer dificuldade */
    long recorde;
    char recorde_nome[24];    /* de quem é ("" = convidado) */
    int  recorde_instr;       /* Instrumento em que foi feito */
    int  recorde_dif;         /* Dificuldade */
    int  recorde_pct;         /* porcentagem de notas */
    bool recorde_fc;          /* full combo */
} InfoMusica;

#define GAME_PAGINA 7   /* Esq/Dir na lista de músicas pula isso (na guitarra: segurar a palhetada acelera) */

/* ---------------------------------------------------------------------------
 * Jogadores: até 4 na mesma partida, cada um com o seu controle, o seu
 * instrumento e a sua dificuldade. Tudo o que é de uma pessoa (a pista, o
 * placar, o star power) fica aqui; o que é da música (o tempo, a pausa) fica
 * no Game. Com um jogador só, o jogo é o mesmo de antes.
 * ------------------------------------------------------------------------- */
#define GAME_MAX_JOGADORES 4
#define GAME_CONTAGEM      1.2   /* tela de jogadores: todos prontos, espera isso até as músicas */
#define GAME_MAX_IDIOMAS   16
#define GAME_MAX_CONTAS    32
#define GAME_NOME          24   /* bytes de um nome de conta (até 12 letras, UTF-8) */
#define GAME_NOME_LETRAS   12

/* As linhas do cartão de cada jogador: Cima/Baixo escolhe a linha, Confirmar
 * entra no modo de escolha dela (ou executa), Voltar desfaz. */
enum { SESSAO_CONTA, SESSAO_MODO, SESSAO_CONTROLES, SESSAO_PRONTO, SESSAO_N };   /* tela de jogadores */
enum { PREP_INSTRUMENTO, PREP_DIFICULDADE, PREP_PRONTO, PREP_N };              /* preparação */

/* as teclas de um jogador de teclado (o main.c informa, Game_set_devices) */
enum { TECLAS_OK, TECLAS_FALTAM, TECLAS_REPETIDAS };

typedef struct {
    /* quem joga e o quê */
    bool        na_sessao;         /* entrou na tela de jogadores: joga as próximas músicas */
    bool        no_teclado;        /* entrou pelo teclado (senão, pelo controle do espaço dele) */
    int         conta;             /* 0 = convidado, 1..n = Game.contas[conta - 1] */
    bool        ativo;             /* joga esta música (quem está na sessão, menos quem ficou de fora) */
    Instrumento instrumento;
    Dificuldade dificuldade;
    bool        pronto;

    /* o cartão dele (tela de jogadores e preparação) */
    int         linha_sessao;      /* SESSAO_* */
    int         linha_prep;        /* PREP_* */
    bool        escolhendo;        /* no modo de escolha da linha */
    int         antes[2];          /* o valor de quando entrou nele (Voltar desfaz) */

    /* a pista dele */
    Chart    chart;
    int      proxima;          /* primeira nota ainda pendente: o juiz começa aqui */
    int      primeira_visivel; /* primeira nota que ainda aparece na pista */
    uint32_t segurados;        /* ações seguradas neste frame */
    float    whammy;           /* 0..1 */

    /* placar */
    double   pontos;
    double   mult_mudou_t;     /* t_total em que o multiplicador mudou (animação) */
    int      multiplicador;    /* 1..4, x2 com star power */
    int      combo, maior_combo;
    int      acertos, erros;   /* em notas (um acorde conta uma) */
    int      toques_errados;

    /* star power */
    float    sp_medidor;       /* 0..1 */
    bool     sp_ativo;
    bool     sp_frase_falhou;  /* errou alguma nota da frase atual */

    /* para os efeitos do draw (ele anima a partir de "quando aconteceu") */
    double   acerto_t[5];      /* t do último acerto em cada faixa */
    double   ultimo_julgamento_t;
    double   ultimo_erro_tempo;  /* erro de tempo do último acerto (s) */
    bool     ultimo_foi_erro;

    /* estatística da partida, para o log (a média do erro de tempo ajuda a
       calibrar: média negativa = o jogador aperta adiantado) */
    double   soma_erro, soma_erro2;

    /* recorde da partida que acabou */
    bool     novo_recorde;
    long     recorde_anterior;

    /* modo guitarra (judge.c): a palhetada que ainda espera os trastes certos */
    double   palhetada_t;      /* quando (-1 = nenhuma) */
    double   hopo_t;           /* último acerto sem palhetada (HOPO/tap): perdoa a palhetada logo depois */
    uint32_t trastes_antes;    /* trastes segurados no frame anterior */
} Jogador;

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

    /* partida: o que é de todos */
    Jogador  jog[GAME_MAX_JOGADORES];

    /* Os aparelhos de cada jogador: o main.c atualiza a cada frame
       (Game_set_devices). Cada espaço tem o seu controle; o teclado é um
       aparelho só, dividido entre quem entrou por ele (Jogador.no_teclado):
       na tela de jogadores e na preparação ele atende um de cada vez. */
    bool     aparelhos;                           /* false = ninguém informou (testes): todos valem */
    bool     conectado[GAME_MAX_JOGADORES];       /* tem controle */
    bool     presente_antes[GAME_MAX_JOGADORES];
    int      teclas[GAME_MAX_JOGADORES];          /* TECLAS_*: as do perfil dele dão para jogar no teclado? */
    bool     guitarra[GAME_MAX_JOGADORES];        /* o perfil dele está no modo guitarra */

    /* tela de jogadores */
    int      cursor_teclado;   /* o cartão que o teclado está atendendo (-1 = nenhum) */
    double   contagem;         /* todos prontos: segundos até ir para as músicas */
    int      origem;           /* quem apertou Confirmar neste frame: 0..3, GAME_TECLADO, ou -1 */
    bool     teclado_usado;    /* alguém já apertou uma tecla de menu (no console, nunca): o draw mostra as dicas do teclado */

    /* contas: os nomes salvos no aparelho (o convidado não está aqui) */
    char     contas[GAME_MAX_CONTAS][GAME_NOME];
    int      n_contas;
    int      nome_jogador;     /* quem está escrevendo um nome novo (-1 = ninguém) */
    char     nome_novo[GAME_NOME];
    int      nome_cursor;      /* a tecla selecionada no teclado da tela */
    bool     nome_minusculas;
    char     titulo[96];       /* nome da música */
    double   inicio;           /* onde a partida começa: negativo = preparação antes da música */
    double   fim;              /* fim da última nota, de todos os jogadores */
    double   duracao_audio;    /* 0 = sem áudio */
    bool     pausado;
    bool     terminou;
    int      sel_pausa;        /* item selecionado na pausa / no fim */
    int      pausas;
    double   segura_t[2];      /* há quanto tempo Cima/Baixo estão segurados (repetição) */
    double   repete_t[2];

    /* menus: o modo de escolha de um ITEM_VALOR (o item é o selecionado) */
    bool     escolhendo;
    Opcoes   opcoes_antes;     /* para o Voltar desfazer */
    int      jogador_antes;

    /* o que aconteceu neste frame */
    EventoJogo eventos[GAME_MAX_EVENTOS];
    int        n_eventos;

    /* aviso curto na tela ("Em breve") */
    const char *aviso;
    double      aviso_ate;   /* em t_total */

    /* músicas */
    InfoMusica  *musicas;                 /* cresce conforme o main.c acrescenta */
    int          n_musicas, cap_musicas;
    ItemMenu    *itens_musicas;           /* organizar + demonstração + pastas + músicas + procurar + voltar */
    int          cap_itens;
    char         pasta_atual[128];        /* modo pastas: onde estamos ("" = a raiz) */
    char       (*pastas_nome)[96];        /* subpastas da pasta atual */
    char       (*pastas_rotulo)[112];
    int          n_pastas, cap_pastas;
    const InfoMusica **ordem;             /* rascunho para ordenar por artista */
    int          cap_ordem;
    Menu         menu_musicas;
    int          musica;                  /* a escolhida: índice, ou -1 = demonstração */
    ArquivoChart arquivo;                 /* o chart dela, já lido */
    bool         arquivo_ok;
    int          notas[INSTR_N][DIF_N];   /* quantas notas cada parte do chart tem (0 = não existe) */

    int          controles_jogador;       /* tela de controles: de quem é a configuração mostrada */
    Tela         controles_volta;         /* para onde o Voltar dela leva (configurações ou jogadores) */

    /* os idiomas que o main.c encontrou (o 0 é o português, embutido) */
    char         idiomas[GAME_MAX_IDIOMAS][16];
    char         nomes_idioma[GAME_MAX_IDIOMAS][32];
    int          n_idiomas;

    /* saída para o main.c */
    Pedido   pedido;
    Acao     pedido_acao;
    int      pedido_indice;
    unsigned trocar_modo;     /* bit k: trocar o modo (controle / guitarra) do perfil do jogador k */
} Game;

#define GAME_TECLADO GAME_MAX_JOGADORES   /* Game.origem: foi o teclado */

void Game_init(Game *g);
void Game_quit(Game *g);
/* b[0..n-1]: os botões de cada jogador (b[0] = jogador 1); teclado: as teclas
   de menu do teclado (NULL = sem teclado). Nos menus vale qualquer um; nos
   cartões dos jogadores e na pista, cada um mexe só no que é seu. */
void Game_update(Game *g, const Botoes *b, int n, const Botoes *teclado, double dt);
bool Game_load_chart(Game *g, const Nota *notas, int n);   /* só o jogador 1, com estas notas (testes) */
void Game_play(Game *g);                                    /* começa a partida com os charts atuais */

/* aparelhos de cada jogador, a cada frame (main.c): tem controle, as teclas
   do perfil (TECLAS_*), modo guitarra */
void Game_set_devices(Game *g, const bool *conectado, const int *teclas, const bool *guitarra);
bool Game_present(const Game *g, int k);   /* o aparelho dele está aí (o teclado está sempre) */

/* contas (chamadas pelo main.c) */
void        Game_add_account(Game *g, const char *nome);
const char *Game_player_name(const Game *g, int k);   /* o nome da conta, ou NULL = convidado */
int         Game_leader(const Game *g);               /* quem manda nos menus da partida (-1 = ninguém) */

/* teclado na tela, para escrever um nome: as teclas (UTF-8) e as especiais */
#define GAME_TECLAS_LINHA 13
#define GAME_TECLAS_N     (4 * GAME_TECLAS_LINHA + 4)
enum { TECLA_ESPACO = 4 * GAME_TECLAS_LINHA, TECLA_CAIXA, TECLA_APAGA, TECLA_OK };
const char *Game_key_text(const Game *g, int i);   /* o que a tecla i escreve (as especiais: o rótulo) */

/* idiomas (chamadas pelo main.c) */
void Game_add_language(Game *g, const char *codigo, const char *nome);
int  Game_language_index(const Game *g);   /* o escolhido, na lista */

/* músicas (chamadas pelo main.c) */
void Game_clear_songs(Game *g);
void Game_add_song(Game *g, const SongIni *ini, const char *nome_pasta, const char *grupo);
void Game_set_record(Game *g, int musica, long pontos, const char *nome, int instr, int dif, int pct, bool fc);
void Game_songs_done(Game *g);    /* depois de acrescentar: monta o menu */
bool Game_open_chart(Game *g, const char *texto, size_t tamanho);   /* vai para a preparação */
void Game_audio_ready(Game *g, bool ok, double duracao);            /* carga terminou: começa */

/* Para o draw (e para os testes) */
const Menu *Game_menu(const Game *g);            /* menu visível agora, ou NULL */
int         Game_menu_sel(const Game *g);        /* item selecionado nele */
double      Game_janela_visivel(const Game *g);  /* segundos de pista visíveis */
double      Game_janela_acerto(const Game *g);   /* segundos para cada lado da nota */
double      Game_janela_perfeito(const Game *g);
int         Game_n_jogando(const Game *g);       /* jogadores ativos */
int         Game_vez_teclado(const Game *g);     /* preparação: de quem é a vez no teclado (-1 = ninguém) */
bool        Game_teclado_livre(const Game *g, int k);   /* tela de jogadores: o teclado pode entrar no cartão k */

/* game.c, para os outros arquivos do game: troca de tela (com o que cada tela faz ao chegar) */
void Game_vai_para(Game *g, Tela t);

/* jogadores.c (uso interno do game) */
void Jogadores_entra(Game *g);                                    /* chegando na tela de jogadores */
void Jogadores_update(Game *g, const Botoes *b, int n, const Botoes *teclado, double dt);
void Jogadores_nome_update(Game *g, const Botoes *b, double dt);  /* o teclado da tela, para um nome */
void Jogadores_sai(Game *g, int k);                               /* sai da sessão */

/* game.c, para os outros arquivos do game: Cima/Baixo segurados repetem
   (devolve quantos passos andar: 0, 1 ou mais) */
int  Game_repete(Game *g, const Botoes *b, Acao a, double dt);

/* biblioteca.c (uso interno do game) */
void        Biblioteca_monta_menu(Game *g);
void        Biblioteca_entra(Game *g, int k);
bool        Biblioteca_sobe(Game *g);
void        Biblioteca_fim_partida(Game *g);
void        Biblioteca_free(Game *g);
const char *Biblioteca_nome_organizar(int modo);

/* opcoes.c: as opções em texto (o main.c lê e grava o arquivo) */
int  Opcoes_write(const Opcoes *o, char *buf, size_t n);
void Opcoes_read(Opcoes *o, const char *texto, size_t tamanho);

/* menu.c */
const Menu *Menu_da_tela(Tela t);   /* NULL para TELA_JOGO */
const Menu *Menu_pausa(void);
const Menu *Menu_fim(void);
void        Menu_valor(const Game *g, int id, char *buf, size_t n);   /* texto de um ITEM_OPCAO / ITEM_VALOR */

/* judge.c: o juiz, um jogador por vez (uso interno do game, e dos testes).
   Judge_toques é o modo controle; Judge_guitarra, o modo guitarra. */
void Judge_toques(Game *g, Jogador *j, uint32_t apertados, uint32_t segurados);
void Judge_guitarra(Game *g, Jogador *j, uint32_t apertados, uint32_t segurados);
void Judge_sustains(Game *g, Jogador *j, uint32_t segurados, double dt);
void Judge_varre_erros(Game *g, Jogador *j);

/* score.c: pontos a partir dos eventos. O main.c chama depois do update
 * (quando a Lua entrar, ela pode assumir este papel). */
void Score_apply_events(Game *g);
int  Score_multiplicador(const Jogador *j);

#endif
