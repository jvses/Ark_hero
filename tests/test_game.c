/* Testes da lógica do jogo: sem janela, sem controle, sem SDL.
 * Cada teste monta um Game, simula botões e confere o estado.
 * Rodar: ./build/local/test_game  (ou ctest --test-dir build/local) */

#include <math.h>
#include <stdio.h>

#include "game/game.h"
#include "game/songini.h"

#include <string.h>

static int falhas = 0;

/* não usa assert(): no build Release o NDEBUG desligaria todos os testes */
#define CONFERE(cond)                                                       \
    do {                                                                    \
        if (!(cond)) {                                                      \
            printf("  FALHOU %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
            falhas++;                                                       \
        }                                                                   \
    } while (0)

#define DT (1.0 / 120.0)   /* frames curtos: o tempo do aperto fica preciso */

/* um frame, com o que o main.c faz depois do update */
static void frame(Game *g, uint32_t segurados, uint32_t apertados)
{
    const Botoes b = { .segurados = segurados | apertados, .apertados = apertados };
    Game_update(g, &b, DT);
    Score_apply_events(g);
}

static void aperta(Game *g, Acao a)
{
    frame(g, 0, ACAO_BIT(a));
}

/* frames segurando "segurados" até g->t chegar em "t" (ou a música acabar:
   aí o tempo para de andar e o laço nunca terminaria) */
static void ate(Game *g, double t, uint32_t segurados)
{
    while (g->t < t - DT / 2 && !g->terminou) frame(g, segurados, 0);
}

static void espera(Game *g, double segundos)
{
    for (double t = 0; t < segundos; t += DT) frame(g, 0, 0);
}

#define VERDE    ACAO_BIT(ACAO_VERDE)
#define VERMELHO ACAO_BIT(ACAO_VERMELHO)
#define AMARELO  ACAO_BIT(ACAO_AMARELO)

/* entra na pista com um chart montado à mão */
static void partida(Game *g, const Nota *notas, int n)
{
    Game_init(g);
    Game_load_chart(g, notas, n);
    Game_play(g);
}

/* ===========================================================================
 * Menus
 * ========================================================================= */

static void teste_navegacao_da_volta(void)
{
    Game g;
    Game_init(&g);
    const int n = Game_menu(&g)->n;

    aperta(&g, ACAO_CIMA);                    /* do primeiro item, para cima... */
    CONFERE(Game_menu_sel(&g) == n - 1);      /* ...vai para o último, não para 255 */
    aperta(&g, ACAO_BAIXO);
    CONFERE(Game_menu_sel(&g) == 0);
    Game_quit(&g);
}

static void teste_segurar_nao_corre_o_menu(void)
{
    Game g;
    Game_init(&g);
    for (int i = 0; i < 30; i++) frame(&g, ACAO_BIT(ACAO_BAIXO), 0);   /* segurado, sem borda */
    CONFERE(Game_menu_sel(&g) == 0);
    Game_quit(&g);
}

static void teste_jogar_e_pausar(void)
{
    Game g;
    Game_init(&g);
    aperta(&g, ACAO_CONFIRMA);                /* Jogar */
    CONFERE(g.tela == TELA_MUSICAS);
    aperta(&g, ACAO_BAIXO);                   /* o item 0 é "Organizar por" */
    aperta(&g, ACAO_CONFIRMA);                /* Demonstracao */
    CONFERE(g.tela == TELA_JOGO);

    espera(&g, 1.0);
    CONFERE(g.t > 0.9 && g.t < 1.1);

    aperta(&g, ACAO_PAUSA);
    CONFERE(g.pausado);
    const double t_pausa = g.t;
    espera(&g, 1.0);
    CONFERE(g.t == t_pausa);                  /* pausado, o tempo não anda */

    aperta(&g, ACAO_BAIXO);                   /* Reiniciar */
    aperta(&g, ACAO_BAIXO);                   /* Sair para o menu */
    aperta(&g, ACAO_CONFIRMA);
    CONFERE(g.tela == TELA_MUSICAS);          /* volta para a lista de músicas */
    Game_quit(&g);
}

static void teste_em_breve_nao_faz_nada(void)
{
    Game g;
    Game_init(&g);
    aperta(&g, ACAO_BAIXO);                   /* Configuracoes */
    aperta(&g, ACAO_CONFIRMA);
    aperta(&g, ACAO_BAIXO);                   /* Modo de jogo: o guitarra ainda é "em breve" */
    aperta(&g, ACAO_DIREITA);
    CONFERE(g.tela == TELA_CONFIG);
    CONFERE(g.aviso != NULL);
    espera(&g, 2.0);
    CONFERE(g.aviso == NULL);
    Game_quit(&g);
}

static void teste_opcoes_e_voltar(void)
{
    Game g;
    Game_init(&g);
    aperta(&g, ACAO_BAIXO);                   /* Configuracoes */
    aperta(&g, ACAO_CONFIRMA);
    CONFERE(g.tela == TELA_CONFIG);

    aperta(&g, ACAO_BAIXO);                   /* Modo de jogo */
    aperta(&g, ACAO_BAIXO);                   /* Velocidade da pista */
    for (int i = 0; i < 20; i++) aperta(&g, ACAO_DIREITA);
    CONFERE(g.opcoes.velocidade == GAME_VELOCIDADE_MAX);   /* para no máximo */

    aperta(&g, ACAO_BAIXO);                   /* Modo precisao */
    CONFERE(Game_janela_acerto(&g) == GAME_JANELA_NORMAL);
    aperta(&g, ACAO_CONFIRMA);
    CONFERE(g.opcoes.precisao && Game_janela_acerto(&g) == GAME_JANELA_PRECISAO);

    for (int i = 0; i < 4; i++) aperta(&g, ACAO_BAIXO);   /* penalidade, acertos, ajuste, Modo canhoto */
    aperta(&g, ACAO_CONFIRMA);
    CONFERE(g.opcoes.canhoto);

    aperta(&g, ACAO_VOLTA);
    CONFERE(g.tela == TELA_INICIO);
    CONFERE(Game_menu_sel(&g) == 1);          /* voltou para onde estava */
    Game_quit(&g);
}

static void teste_pedido_de_remapear(void)
{
    Game g;
    Game_init(&g);
    aperta(&g, ACAO_BAIXO);
    aperta(&g, ACAO_CONFIRMA);                /* Configuracoes */
    aperta(&g, ACAO_CONFIRMA);                /* Controles */
    CONFERE(g.tela == TELA_CONTROLES);

    aperta(&g, ACAO_BAIXO);                   /* Vermelho */
    aperta(&g, ACAO_CONFIRMA);
    CONFERE(g.pedido == PEDIDO_MAPEAR);
    CONFERE(g.pedido_acao == ACAO_VERMELHO);

    espera(&g, 0.1);
    CONFERE(g.pedido == PEDIDO_NENHUM);       /* pedido vale só um frame */
    Game_quit(&g);
}

/* ===========================================================================
 * Juiz (modo controle)
 * ========================================================================= */

static void teste_acerto_sem_palhetada(void)
{
    const Nota notas[] = { { .t = 1.0, .mascara = 1 } };   /* uma nota verde */
    Game g;
    partida(&g, notas, 1);

    ate(&g, 0.98, 0);
    frame(&g, 0, VERDE);                      /* só o botão, 20 ms adiantado */
    CONFERE(g.acertos == 1 && g.combo == 1);
    CONFERE(g.chart.notas[0].estado == NOTA_ACERTADA);
    CONFERE(g.ultimo_erro_tempo < 0.0);       /* negativo = adiantado */
    CONFERE(g.pontos == 50.0);
    Game_quit(&g);
}

static void teste_fora_da_janela(void)
{
    const Nota notas[] = { { .t = 1.0, .mascara = 1 } };
    Game g;
    partida(&g, notas, 1);

    ate(&g, 0.85, 0);
    frame(&g, 0, VERDE);                      /* 150 ms antes: não conta */
    CONFERE(g.acertos == 0 && g.erros == 0);
    CONFERE(g.toques_errados == 1);           /* é um aperto sem nota: penalidade */

    ate(&g, 1.15, 0);                         /* passou da janela sem acertar */
    CONFERE(g.erros == 1 && g.combo == 0);
    CONFERE(g.chart.notas[0].estado == NOTA_PERDIDA);
    Game_quit(&g);
}

static void teste_botao_errado(void)
{
    const Nota notas[] = { { .t = 1.0, .mascara = 1 } };
    Game g;
    partida(&g, notas, 1);
    ate(&g, 1.0, 0);
    frame(&g, 0, VERMELHO);                   /* nota verde, botão vermelho */
    CONFERE(g.acertos == 0 && g.toques_errados == 1);
    Game_quit(&g);
}

static void teste_acorde(void)
{
    const Nota notas[] = { { .t = 1.0, .mascara = 1 | 2 },     /* verde + vermelho */
                           { .t = 2.0, .mascara = 1 | 2 } };
    Game g;
    partida(&g, notas, 2);

    ate(&g, 0.98, 0);
    frame(&g, 0, VERDE);                      /* só metade do acorde */
    CONFERE(g.acertos == 0);
    frame(&g, VERDE, VERMELHO);               /* completou, ainda dentro da janela */
    CONFERE(g.acertos == 1);
    CONFERE(g.pontos == 100.0);               /* 50 por botão */

    ate(&g, 2.0, 0);
    frame(&g, 0, VERDE);                      /* só metade, e nunca completa */
    ate(&g, 2.2, VERDE);
    CONFERE(g.acertos == 1 && g.erros == 1);
    Game_quit(&g);
}

static void teste_sustain_completo(void)
{
    const Nota notas[] = { { .t = 1.0, .duracao = 1.0, .mascara = 4 } };   /* amarelo, 1 s */
    Game g;
    partida(&g, notas, 1);

    ate(&g, 1.0, 0);
    frame(&g, 0, AMARELO);
    CONFERE(g.chart.notas[0].segurando);
    ate(&g, 2.2, AMARELO);                    /* segura até depois do fim */
    CONFERE(!g.chart.notas[0].segurando);
    CONFERE(fabs(g.chart.notas[0].solto - 2.0) < 1e-9);
    /* 50 da nota + ~50 do segundo segurado */
    CONFERE(g.pontos > 95.0 && g.pontos < 101.0);
    Game_quit(&g);
}

static void teste_sustain_solto_cedo(void)
{
    const Nota notas[] = { { .t = 1.0, .duracao = 1.0, .mascara = 4 } };
    Game g;
    partida(&g, notas, 1);

    ate(&g, 1.0, 0);
    frame(&g, 0, AMARELO);
    ate(&g, 1.5, AMARELO);
    frame(&g, 0, 0);                          /* soltou na metade */
    CONFERE(!g.chart.notas[0].segurando);
    CONFERE(g.chart.notas[0].solto < 1.6);
    CONFERE(g.combo == 1);                    /* soltar não quebra o combo */
    const double pontos = g.pontos;
    ate(&g, 2.2, 0);
    CONFERE(g.pontos == pontos);              /* e não soma mais nada */
    Game_quit(&g);
}

static void teste_multiplicador(void)
{
    Nota notas[12];
    for (int i = 0; i < 12; i++) notas[i] = (Nota){ .t = 1.0 + 0.5 * i, .mascara = 1 };
    Game g;
    partida(&g, notas, 12);

    for (int i = 0; i < 12; i++) {
        ate(&g, notas[i].t, 0);
        frame(&g, 0, VERDE);
    }
    CONFERE(g.combo == 12);
    CONFERE(g.multiplicador == 2);            /* passou de 10 de combo */
    /* 9 notas a x1, a partir da 10ª (combo 10) a x2: 9*50 + 3*100 */
    CONFERE(g.pontos == 750.0);
    Game_quit(&g);
}

static void teste_star_power(void)
{
    /* duas frases de star power (2 notas cada) e uma nota comum depois */
    const Nota notas[] = {
        { .t = 1.0, .mascara = 1, .flags = NOTA_STAR }, { .t = 1.5, .mascara = 2, .flags = NOTA_STAR },
        { .t = 2.0, .mascara = 1 },
        { .t = 3.0, .mascara = 1, .flags = NOTA_STAR }, { .t = 3.5, .mascara = 2, .flags = NOTA_STAR },
        { .t = 5.0, .mascara = 1 },
        { .t = 20.0, .mascara = 1 },   /* só para a música durar até o medidor esvaziar */
    };
    Game g;
    partida(&g, notas, 7);

    for (int i = 0; i < 5; i++) {
        ate(&g, notas[i].t, 0);
        frame(&g, 0, (uint32_t)notas[i].mascara);
    }
    CONFERE(fabs(g.sp_medidor - 0.5f) < 1e-6);   /* duas frases = 50% */

    frame(&g, 0, ACAO_BIT(ACAO_STAR_POWER));
    CONFERE(g.sp_ativo);
    ate(&g, 5.0, 0);
    frame(&g, 0, VERDE);
    CONFERE(g.multiplicador == 2);            /* x1 dobrado */
    ate(&g, 15.0, 0);                         /* 50% dura 8 s */
    CONFERE(!g.sp_ativo);
    Game_quit(&g);
}

static void teste_frase_com_erro_nao_conta(void)
{
    const Nota notas[] = { { .t = 1.0, .mascara = 1, .flags = NOTA_STAR },
                           { .t = 1.5, .mascara = 2, .flags = NOTA_STAR } };
    Game g;
    partida(&g, notas, 2);
    ate(&g, 1.3, 0);                          /* deixa a primeira passar */
    ate(&g, 1.5, 0);
    frame(&g, 0, VERMELHO);                   /* acerta a última */
    CONFERE(g.sp_medidor == 0.0f);
    Game_quit(&g);
}

static void teste_fim_da_musica(void)
{
    const Nota notas[] = { { .t = 1.0, .mascara = 1 } };
    Game g;
    partida(&g, notas, 1);
    espera(&g, 4.0);
    CONFERE(g.terminou);
    CONFERE(Game_menu(&g) == Menu_fim());
    aperta(&g, ACAO_CONFIRMA);                /* Jogar de novo */
    CONFERE(!g.terminou && g.t < 0.1 && g.erros == 0);
    CONFERE(g.chart.notas[0].estado == NOTA_PENDENTE);
    Game_quit(&g);
}

/* ===========================================================================
 * song.ini
 * ========================================================================= */

static void teste_song_ini(void)
{
    const char texto[] =
        "\xEF\xBB\xBF[song]\r\n"                              /* BOM e fim de linha do Windows */
        "name = Cliffs Of Dover (with Drums)\r\n"
        "artist = Eric Johnson\r\n"
        "charter = <color=orange>Neversoft</color>, 4EverfreeBrony\r\n"
        "song_length = 255000\r\n"
        "delay = -120\r\n"
        "[outra]\r\n"
        "name = nao e daqui\r\n";
    SongIni s;
    CONFERE(SongIni_parse(texto, sizeof(texto) - 1, &s));
    CONFERE(strcmp(s.nome, "Cliffs Of Dover (with Drums)") == 0);
    CONFERE(strcmp(s.artista, "Eric Johnson") == 0);
    CONFERE(strcmp(s.charter, "Neversoft, 4EverfreeBrony") == 0);   /* sem as tags */
    CONFERE(s.duracao_ms == 255000);
    CONFERE(s.atraso_ms == -120);

    const char maiuscula[] = "[Song]\nName=X\nARTIST =  Y  \n";
    CONFERE(SongIni_parse(maiuscula, sizeof(maiuscula) - 1, &s));
    CONFERE(strcmp(s.nome, "X") == 0 && strcmp(s.artista, "Y") == 0);

    CONFERE(!SongIni_parse("lixo", 4, &s));
}

static void teste_relogio_externo(void)
{
    const Nota notas[] = { { .t = 1.0, .mascara = 1 } };
    Game g;
    partida(&g, notas, 1);
    g.relogio_audio = true;

    g.t_audio = 0.98;                         /* o áudio diz onde a música está */
    frame(&g, 0, VERDE);
    CONFERE(g.t == 0.98 && g.acertos == 1);

    g.t_audio = 0.90;                         /* um tropeço para trás não volta o jogo */
    frame(&g, 0, 0);
    CONFERE(g.t == 0.98);
    Game_quit(&g);
}

/* ===========================================================================
 * Parser de .chart
 * ========================================================================= */

/* 192 ticks por semínima; 120 BPM até o tick 768 (2,0 s), depois 60 BPM */
static const char CHART[] =
    "[Song]\r\n{\r\n  Resolution = 192\r\n  Offset = 0\r\n}\r\n"
    "[SyncTrack]\r\n{\r\n  0 = TS 4\r\n  0 = B 120000\r\n  768 = B 60000\r\n}\r\n"
    "[Events]\r\n{\r\n  0 = E \"section Intro\"\r\n}\r\n"
    "[ExpertSingle]\r\n{\r\n"
    "  384 = N 0 0\r\n  384 = N 1 0\r\n"         /* acorde verde+vermelho em 1,0 s     */
    "  576 = N 2 30\r\n"                           /* amarela em 1,5 s; sustain curto: 0  */
    "  768 = N 3 192\r\n  768 = S 2 400\r\n"     /* azul em 2,0 s, 1 s de sustain; frase 1 */
    "  960 = N 7 0\r\n"                            /* aberta em 3,0 s                     */
    "  1152 = N 4 0\r\n"                           /* laranja em 4,0 s, ainda na frase 1  */
    "  1168 = S 2 100\r\n  1200 = N 0 0\r\n  1200 = N 5 0\r\n"   /* frase 2 colada; HOPO forçado */
    "}\r\n"
    "[EasySingle]\r\n{\r\n  384 = N 0 0\r\n}\r\n";

static bool perto(double a, double b)
{
    return fabs(a - b) < 1e-9;
}

static void teste_parser(void)
{
    ArquivoChart a;
    char erro[64];
    CONFERE(ChartFile_parse(&a, CHART, sizeof(CHART) - 1, erro, sizeof(erro)));
    CONFERE(a.resolucao == 192 && a.n_tempos == 2);
    CONFERE(perto(ChartFile_tempo(&a, 384), 1.0));
    CONFERE(perto(ChartFile_tempo(&a, 768), 2.0));
    CONFERE(perto(ChartFile_tempo(&a, 960), 3.0));    /* depois da mudança para 60 BPM */
    CONFERE(ChartFile_contar(&a, DIF_EXPERT) == 6);
    CONFERE(ChartFile_contar(&a, DIF_FACIL) == 1);
    CONFERE(ChartFile_contar(&a, DIF_DIFICIL) == 0);

    Chart c;
    CONFERE(Chart_from_file(&c, &a, DIF_EXPERT, 0.0));
    CONFERE(c.n == 6);
    CONFERE(perto(c.notas[0].t, 1.0) && c.notas[0].mascara == (1 | 2));
    CONFERE(c.notas[1].duracao == 0.0);                       /* sustain curto demais */
    CONFERE(perto(c.notas[2].duracao, 1.0));                  /* 192 ticks a 60 BPM */
    CONFERE(c.notas[3].flags & NOTA_ABERTA);
    CONFERE(c.notas[2].frase == 1 && c.notas[4].frase == 1);
    CONFERE(c.notas[5].frase == 2);                           /* frases coladas não se misturam */
    /* 1200 vem 48 ticks depois de outra cor: seria HOPO, mas o "N 5" inverte */
    CONFERE(!(c.notas[5].flags & NOTA_HOPO));
    CONFERE(!(c.notas[1].flags & NOTA_HOPO));                 /* longe da anterior: palhetada */
    CONFERE(!(c.notas[1].flags & NOTA_STAR));
    CONFERE(perto(c.inicio, -1.5));                           /* 2,5 s antes da primeira nota */
    CONFERE(c.batidas[0].forte && !c.batidas[1].forte && c.batidas[4].forte);   /* compasso 4/4 */
    Chart_free(&c);

    Chart d;
    CONFERE(Chart_from_file(&d, &a, DIF_EXPERT, 0.5));       /* delay do song.ini */
    CONFERE(perto(d.notas[0].t, 1.5));
    Chart_free(&d);
    ChartFile_free(&a);

    CONFERE(!ChartFile_parse(&a, "[Song]\n{\n}\n", 12, erro, sizeof(erro)));   /* sem notas */
}

static void teste_aberta_e_frases(void)
{
    ArquivoChart a;
    char erro[64];
    ChartFile_parse(&a, CHART, sizeof(CHART) - 1, erro, sizeof(erro));
    Game g;
    Game_init(&g);
    Chart_free(&g.chart);
    Chart_from_file(&g.chart, &a, DIF_EXPERT, 0.0);
    Game_play(&g);
    CONFERE(perto(g.t, -1.5));                                /* começa na preparação */

    const uint32_t botoes[6] = { VERDE | VERMELHO, AMARELO, ACAO_BIT(ACAO_AZUL),
                                 VERMELHO /* aberta: qualquer botão */, ACAO_BIT(ACAO_LARANJA), VERDE };
    for (int i = 0; i < 6; i++) {
        ate(&g, g.chart.notas[i].t, 0);
        frame(&g, 0, botoes[i]);
    }
    CONFERE(g.acertos == 6 && g.erros == 0);
    CONFERE(fabs(g.sp_medidor - 0.5f) < 1e-6);               /* duas frases, duas vezes 25% */
    Game_quit(&g);
    ChartFile_free(&a);
}

static void teste_escolher_musica(void)
{
    Game g;
    Game_init(&g);
    const SongIni ini = { .nome = "Musica Teste", .artista = "Banda", .atraso_ms = 0 };
    Game_add_song(&g, &ini, "pasta", "");
    Game_songs_done(&g);
    CONFERE(g.n_musicas == 1 && g.menu_musicas.n == 5);       /* organizar, demo, a música, procurar, voltar */

    aperta(&g, ACAO_CONFIRMA);                                /* Jogar */
    aperta(&g, ACAO_BAIXO);                                   /* demonstração */
    aperta(&g, ACAO_BAIXO);                                   /* a música */
    aperta(&g, ACAO_CONFIRMA);
    CONFERE(g.pedido == PEDIDO_ABRIR_MUSICA && g.pedido_indice == 0);

    CONFERE(Game_open_chart(&g, CHART, sizeof(CHART) - 1));  /* o main.c lê o arquivo e entrega */
    CONFERE(g.tela == TELA_DIFICULDADE);
    CONFERE(g.menu_dif.n == 3);                               /* Expert, Facil, Voltar */

    aperta(&g, ACAO_CONFIRMA);                                /* Expert */
    CONFERE(g.tela == TELA_CARREGANDO && g.pedido == PEDIDO_CARREGAR_MUSICA);
    CONFERE(g.dificuldade == DIF_EXPERT && g.chart.n == 6);

    aperta(&g, ACAO_VOLTA);                                   /* desistiu no meio da carga */
    CONFERE(g.tela == TELA_DIFICULDADE && g.pedido == PEDIDO_CANCELAR_CARGA);

    aperta(&g, ACAO_BAIXO);                                   /* Facil */
    aperta(&g, ACAO_CONFIRMA);
    CONFERE(g.chart.n == 1);
    Game_audio_ready(&g, true, 10.0);                         /* o áudio ficou pronto */
    CONFERE(g.tela == TELA_JOGO && g.chart.duracao_audio == 10.0);
    CONFERE(g.t < 0.0);                                       /* tempo de preparação */
    Game_quit(&g);
}

static void teste_modo_precisao(void)
{
    const Nota notas[] = { { .t = 1.0, .mascara = 1 } };
    Game g;
    partida(&g, notas, 1);
    ate(&g, 1.085, 0);
    frame(&g, 0, VERDE);                      /* 85 ms atrasado */
    CONFERE(g.acertos == 1);                  /* janela normal: +-100 ms */
    Game_quit(&g);

    partida(&g, notas, 1);
    g.opcoes.precisao = true;
    ate(&g, 1.085, 0);
    frame(&g, 0, VERDE);
    CONFERE(g.acertos == 0);                  /* precisão: +-70 ms */
    Game_quit(&g);
}

static void teste_lista_comprida(void)
{
    Game g;
    Game_init(&g);
    for (int i = 0; i < 300; i++) {           /* mais que o limite antigo de 256 */
        SongIni ini = {0};
        snprintf(ini.nome, sizeof(ini.nome), "Musica %03d", i);
        Game_add_song(&g, &ini, "pasta", "");
    }
    Game_songs_done(&g);
    CONFERE(g.n_musicas == 300 && g.menu_musicas.n == 304);
    CONFERE(strcmp(g.menu_musicas.itens[301].rotulo, "Musica 299") == 0);
    CONFERE(Game_item_musica(g.menu_musicas.itens[301].id) == 299);   /* sem colidir com outras faixas de id */

    aperta(&g, ACAO_CONFIRMA);                /* Jogar */
    aperta(&g, ACAO_BAIXO);                   /* sai do "Organizar por" */
    aperta(&g, ACAO_DIREITA);                 /* uma página para baixo */
    CONFERE(Game_menu_sel(&g) == 1 + GAME_PAGINA);
    aperta(&g, ACAO_ESQUERDA);
    aperta(&g, ACAO_ESQUERDA);                /* não passa do começo */
    CONFERE(Game_menu_sel(&g) == 0);

    aperta(&g, ACAO_BAIXO);                   /* um toque: um item */
    CONFERE(Game_menu_sel(&g) == 1);
    for (int i = 0; i < 120; i++) frame(&g, ACAO_BIT(ACAO_BAIXO), 0);   /* segurando 1 s */
    CONFERE(Game_menu_sel(&g) > 5 && Game_menu_sel(&g) < 15);         /* 0,4 s parado, depois ~14/s */
    Game_quit(&g);
}

/* ===========================================================================
 * Biblioteca: pastas, ordem, recordes, opções salvas
 * ========================================================================= */

static void musica(Game *g, const char *nome, const char *artista, const char *grupo)
{
    SongIni ini = {0};
    snprintf(ini.nome, sizeof(ini.nome), "%s", nome);
    snprintf(ini.artista, sizeof(ini.artista), "%s", artista);
    Game_add_song(g, &ini, nome, grupo);
}

static int acha_item(const Game *g, const char *rotulo)
{
    for (int i = 0; i < g->menu_musicas.n; i++)
        if (strcmp(g->menu_musicas.itens[i].rotulo, rotulo) == 0) return i;
    return -1;
}

static void teste_pastas(void)
{
    Game g;
    Game_init(&g);
    musica(&g, "Avulsa", "Z", "");
    musica(&g, "Cliffs", "Eric Johnson", "GH3/Tier 1");
    musica(&g, "Fire", "DragonForce", "GH3/Tier 2");
    musica(&g, "Outra", "Banda", "Rock");
    Game_songs_done(&g);

    /* raiz: organizar, demo, [GH3] 2, [Rock] 1, Avulsa, procurar, voltar */
    CONFERE(g.menu_musicas.n == 7);
    CONFERE(acha_item(&g, "[GH3]  2") == 2 && acha_item(&g, "[Rock]  1") == 3);
    CONFERE(acha_item(&g, "Avulsa") == 4);

    aperta(&g, ACAO_CONFIRMA);                       /* Jogar */
    g.sel[TELA_MUSICAS] = 2;
    aperta(&g, ACAO_CONFIRMA);                       /* entra em GH3 */
    CONFERE(strcmp(g.pasta_atual, "GH3") == 0);
    CONFERE(acha_item(&g, "[Tier 1]  1") >= 0 && acha_item(&g, "Avulsa") < 0);
    g.sel[TELA_MUSICAS] = acha_item(&g, "[Tier 2]  1");
    aperta(&g, ACAO_CONFIRMA);                       /* entra em GH3/Tier 2 */
    CONFERE(strcmp(g.pasta_atual, "GH3/Tier 2") == 0 && acha_item(&g, "Fire") >= 0);

    aperta(&g, ACAO_VOLTA);                          /* sobe para GH3, com Tier 2 selecionada */
    CONFERE(strcmp(g.pasta_atual, "GH3") == 0);
    CONFERE(Game_menu_sel(&g) == acha_item(&g, "[Tier 2]  1"));
    aperta(&g, ACAO_VOLTA);
    CONFERE(g.pasta_atual[0] == '\0' && g.tela == TELA_MUSICAS);
    aperta(&g, ACAO_VOLTA);                          /* na raiz, voltar sai da lista */
    CONFERE(g.tela == TELA_INICIO);

    /* por artista: todas juntas, ordenadas pelo artista */
    aperta(&g, ACAO_CONFIRMA);
    g.sel[TELA_MUSICAS] = 0;
    aperta(&g, ACAO_DIREITA);                        /* Pastas -> Titulo */
    aperta(&g, ACAO_DIREITA);                        /* Titulo -> Artista */
    CONFERE(g.opcoes.organizar == ORGANIZAR_ARTISTA && g.pedido == PEDIDO_SALVAR_OPCOES);
    CONFERE(strcmp(g.menu_musicas.itens[2].rotulo, "Banda - Outra") == 0);
    CONFERE(strcmp(g.menu_musicas.itens[5].rotulo, "Z - Avulsa") == 0);
    Game_quit(&g);
}

static void teste_recorde(void)
{
    Game g;
    Game_init(&g);
    musica(&g, "Musica", "Banda", "");
    Game_songs_done(&g);
    g.musica = 0;
    Game_open_chart(&g, CHART, sizeof(CHART) - 1);
    aperta(&g, ACAO_CONFIRMA);                       /* Expert */
    Game_audio_ready(&g, true, 0.0);

    ate(&g, 1.0, 0);
    frame(&g, 0, VERDE | VERMELHO);                  /* acerta só o acorde: 100 pontos */
    while (!g.terminou) frame(&g, 0, 0);
    CONFERE(g.novo_recorde && g.recorde_anterior == 0);
    CONFERE(g.pedido == PEDIDO_SALVAR_RECORDE && g.pedido_indice == 0);
    CONFERE(g.musicas[0].recorde == 100 && g.musicas[0].recorde_dif == DIF_EXPERT && !g.musicas[0].recorde_fc);

    aperta(&g, ACAO_CONFIRMA);                       /* Jogar de novo, sem acertar nada */
    while (!g.terminou) frame(&g, 0, 0);
    CONFERE(!g.novo_recorde && g.musicas[0].recorde == 100);   /* pior: o recorde fica */
    Game_quit(&g);
}

static void teste_opcoes_texto(void)
{
    Opcoes a = { .velocidade = 8, .precisao = true, .canhoto = true, .mostrar_fps = false,
                 .volume = 30, .organizar = ORGANIZAR_ARTISTA };
    char texto[256];
    Opcoes_write(&a, texto, sizeof(texto));

    Opcoes b = { .velocidade = 5, .volume = 80 };
    Opcoes_read(&b, texto, strlen(texto));
    CONFERE(b.velocidade == 8 && b.precisao && b.canhoto && !b.mostrar_fps);
    CONFERE(b.volume == 30 && b.organizar == ORGANIZAR_ARTISTA);

    const char estranho[] = "velocidade = 99\nvolume = -5\nlixo\n";
    Opcoes_read(&b, estranho, sizeof(estranho) - 1);
    CONFERE(b.velocidade == GAME_VELOCIDADE_MAX && b.volume == 0);   /* fora da faixa: limitado */
}

/* ===========================================================================
 * Penalidade, ajuste de tempo, HOPO e MIDI
 * ========================================================================= */

static void teste_penalidade(void)
{
    const Nota notas[] = { { .t = 1.0, .mascara = 1 | 2 }, { .t = 1.04, .mascara = 2 },
                           { .t = 2.0, .mascara = 1 } };
    Game g;
    partida(&g, notas, 3);
    ate(&g, 0.5, 0);
    aperta(&g, ACAO_VERDE);
    CONFERE(g.toques_errados == 1);           /* nada perto: quebra o combo */

    ate(&g, 1.0, 0);
    frame(&g, 0, VERDE | VERMELHO);           /* o acorde, os dois juntos */
    CONFERE(g.acertos == 1 && g.toques_errados == 1);   /* o vermelho não foi punido... */
    CONFERE(g.chart.notas[1].estado == NOTA_PENDENTE);   /* ...nem pegou a nota seguinte */
    ate(&g, 1.04, 0);
    frame(&g, 0, VERMELHO);
    CONFERE(g.acertos == 2 && g.combo == 2);
    Game_quit(&g);

    partida(&g, notas, 3);
    g.opcoes.penalidade = false;
    ate(&g, 0.5, 0);
    aperta(&g, ACAO_VERDE);
    CONFERE(g.toques_errados == 0);           /* desligada: sem punição */
    Game_quit(&g);
}

static void teste_acordes_repetidos(void)
{
    /* verde+vermelho três vezes: segura o verde e toca só o vermelho */
    const Nota notas[] = { { .t = 1.0, .mascara = 1 | 2 }, { .t = 1.5, .mascara = 1 | 2 },
                           { .t = 2.0, .mascara = 1 | 2 } };
    Game g;
    partida(&g, notas, 3);
    ate(&g, 1.0, 0);
    frame(&g, 0, VERDE | VERMELHO);
    for (int i = 1; i < 3; i++) {
        ate(&g, notas[i].t, VERDE);           /* o verde continua segurado... */
        frame(&g, VERDE, VERMELHO);           /* ...e só o vermelho é apertado */
    }
    CONFERE(g.acertos == 3 && g.toques_errados == 0 && g.combo == 3);
    Game_quit(&g);
}

static void teste_acorde_exato(void)
{
    /* vermelho + azul: apertar vermelho, amarelo e azul juntos erra o acorde */
    const Nota notas[] = { { .t = 1.0, .mascara = 2 | 8 }, { .t = 2.0, .mascara = 2 | 8 } };
    Game g;
    partida(&g, notas, 2);
    ate(&g, 1.0, 0);
    frame(&g, 0, VERMELHO | AMARELO | ACAO_BIT(ACAO_AZUL));
    CONFERE(g.acertos == 0 && g.toques_errados == 1);   /* uma penalidade só, não três */
    ate(&g, 1.2, 0);
    CONFERE(g.erros == 1);                              /* e o acorde passou */

    /* o amarelo já segurado de antes também conta como "a mais" */
    ate(&g, 2.0, AMARELO);
    frame(&g, AMARELO, VERMELHO | ACAO_BIT(ACAO_AZUL));
    CONFERE(g.acertos == 0);
    Game_quit(&g);
}

static void teste_acorde_com_sustain(void)
{
    /* um sustain verde longo, e um acorde vermelho + azul no meio dele */
    const Nota notas[] = { { .t = 1.0, .duracao = 2.0, .mascara = 1 }, { .t = 2.0, .mascara = 2 | 8 } };
    Game g;
    partida(&g, notas, 2);
    ate(&g, 1.0, 0);
    frame(&g, 0, VERDE);
    ate(&g, 2.0, VERDE);
    frame(&g, VERDE, VERMELHO | ACAO_BIT(ACAO_AZUL));   /* o verde do sustain não é "a mais" */
    CONFERE(g.acertos == 2 && g.toques_errados == 0);
    CONFERE(g.chart.notas[0].segurando);
    Game_quit(&g);
}

static void teste_ajuste_de_tempo(void)
{
    const Nota notas[] = { { .t = 1.0, .mascara = 1 } };
    Game g;
    partida(&g, notas, 1);
    g.relogio_audio = true;
    g.opcoes.ajuste_ms = 100;                 /* o som chega 100 ms depois: o jogo se adianta */
    g.t_audio = 0.9;
    frame(&g, 0, VERDE);
    CONFERE(perto(g.t, 1.0) && g.acertos == 1);
    CONFERE(fabs(g.ultimo_erro_tempo) < 1e-9);
    Game_quit(&g);
}

/* um MIDI pequeno, montado byte a byte */
typedef struct { unsigned char b[1024]; size_t n; uint32_t ultimo; } Midi;

static void m_byte(Midi *m, unsigned v) { m->b[m->n++] = (unsigned char)v; }

static void m_vlq(Midi *m, uint32_t v)
{
    unsigned char t[5]; int k = 0;
    do { t[k++] = v & 0x7F; v >>= 7; } while (v);
    while (k--) m_byte(m, t[k] | (k ? 0x80 : 0));
}

static void m_evento(Midi *m, uint32_t tick, unsigned a, unsigned b, unsigned c)   /* 3 bytes */
{
    m_vlq(m, tick - m->ultimo);
    m->ultimo = tick;
    m_byte(m, a); m_byte(m, b); m_byte(m, c);
}

static void m_nota(Midi *m, uint32_t ini, uint32_t fim, unsigned nota)
{
    m_evento(m, ini, 0x90, nota, 100);
    m_evento(m, fim, 0x80, nota, 0);
}

static void m_trilha(Midi *arq, const Midi *t)
{
    memcpy(arq->b + arq->n, "MTrk", 4); arq->n += 4;
    const uint32_t n = (uint32_t)t->n + 4;   /* + fim de trilha */
    m_byte(arq, n >> 24); m_byte(arq, (n >> 16) & 0xFF); m_byte(arq, (n >> 8) & 0xFF); m_byte(arq, n & 0xFF);
    memcpy(arq->b + arq->n, t->b, t->n); arq->n += t->n;
    m_byte(arq, 0); m_byte(arq, 0xFF); m_byte(arq, 0x2F); m_byte(arq, 0);
}

static void teste_midi(void)
{
    /* trilha 0: 120 BPM, e 60 BPM a partir do tick 1440 */
    Midi t0 = {0};
    m_vlq(&t0, 0); m_byte(&t0, 0xFF); m_byte(&t0, 0x51); m_byte(&t0, 3);
    m_byte(&t0, 0x07); m_byte(&t0, 0xA1); m_byte(&t0, 0x20);                 /* 500000 us */
    m_vlq(&t0, 1440); m_byte(&t0, 0xFF); m_byte(&t0, 0x51); m_byte(&t0, 3);
    m_byte(&t0, 0x0F); m_byte(&t0, 0x42); m_byte(&t0, 0x40);                 /* 1000000 us */

    /* trilha 1: a guitarra (480 ticks por semínima) */
    Midi t1 = {0};
    m_vlq(&t1, 0); m_byte(&t1, 0xFF); m_byte(&t1, 0x03); m_byte(&t1, 11);
    memcpy(t1.b + t1.n, "PART GUITAR", 11); t1.n += 11;
    m_nota(&t1, 480, 600, 96);                /* verde em 0,5 s (uma semínima a 120 BPM), curta: sem sustain */
    m_nota(&t1, 600, 720, 97);                /* vermelho 120 ticks depois: HOPO natural */
    /* os eventos precisam estar em ordem de tempo (o tempo é a diferença para o anterior) */
    m_evento(&t1, 960, 0x90, 98, 100);        /* amarelo, sustain de 0,5 s (até 1440) */
    m_evento(&t1, 1080, 0x90, 102, 100);      /* força palhetada sobre o azul */
    m_evento(&t1, 1080, 0x90, 99, 100);       /* azul: seria HOPO (perto e outra cor) */
    m_evento(&t1, 1100, 0x80, 102, 0);
    m_evento(&t1, 1200, 0x80, 99, 0);
    m_evento(&t1, 1440, 0x80, 98, 0);
    m_evento(&t1, 1900, 0x90, 116, 100);      /* star power */
    m_evento(&t1, 1920, 0x90, 96, 100);       /* acorde verde + vermelho em 2,5 s */
    m_evento(&t1, 1920, 0x90, 97, 100);
    m_evento(&t1, 1960, 0x80, 96, 0);
    m_evento(&t1, 1960, 0x80, 97, 0);
    m_evento(&t1, 2000, 0x80, 116, 0);
    m_nota(&t1, 2400, 2500, 60);              /* uma nota no Fácil */

    Midi arq = {0};
    memcpy(arq.b, "MThd", 4); arq.n = 4;
    const unsigned char cab[] = { 0, 0, 0, 6, 0, 1, 0, 2, 0x01, 0xE0 };
    memcpy(arq.b + arq.n, cab, sizeof(cab)); arq.n += sizeof(cab);
    m_trilha(&arq, &t0);
    m_trilha(&arq, &t1);

    ArquivoChart a;
    char erro[64];
    const bool leu = ChartFile_parse_mid(&a, arq.b, arq.n, 116, erro, sizeof(erro));
    CONFERE(leu);
    if (!leu) { printf("  erro do MIDI: %s\n", erro); return; }
    CONFERE(ChartFile_contar(&a, DIF_EXPERT) == 5 && ChartFile_contar(&a, DIF_FACIL) == 1);
    Chart c;
    CONFERE(Chart_from_file(&c, &a, DIF_EXPERT, 0.0));
    CONFERE(c.n == 5);
    CONFERE(perto(c.notas[0].t, 0.5) && c.notas[0].duracao == 0.0);
    CONFERE(!(c.notas[0].flags & NOTA_HOPO) && (c.notas[1].flags & NOTA_HOPO));
    CONFERE(perto(c.notas[2].duracao, 0.5));
    CONFERE(!(c.notas[3].flags & NOTA_HOPO));                 /* forçado a palhetada */
    CONFERE(perto(c.notas[4].t, 2.5) && c.notas[4].mascara == (1 | 2));   /* depois da troca para 60 BPM */
    CONFERE((c.notas[4].flags & NOTA_STAR) && !(c.notas[4].flags & NOTA_HOPO));
    Chart_free(&c);
    ChartFile_free(&a);

    CONFERE(!ChartFile_parse_mid(&a, arq.b, 30, 116, erro, sizeof(erro)));   /* cortado no meio */
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);   /* sem buffer: se um teste travar, o que veio antes aparece */
    teste_navegacao_da_volta();
    teste_segurar_nao_corre_o_menu();
    teste_jogar_e_pausar();
    teste_em_breve_nao_faz_nada();
    teste_opcoes_e_voltar();
    teste_pedido_de_remapear();

    teste_acerto_sem_palhetada();
    teste_fora_da_janela();
    teste_botao_errado();
    teste_acorde();
    teste_sustain_completo();
    teste_sustain_solto_cedo();
    teste_multiplicador();
    teste_star_power();
    teste_frase_com_erro_nao_conta();
    teste_fim_da_musica();
    teste_relogio_externo();

    teste_song_ini();
    teste_parser();
    teste_aberta_e_frases();
    teste_escolher_musica();
    teste_modo_precisao();
    teste_lista_comprida();
    teste_pastas();
    teste_recorde();
    teste_opcoes_texto();
    teste_penalidade();
    teste_acordes_repetidos();
    teste_acorde_exato();
    teste_acorde_com_sustain();
    teste_ajuste_de_tempo();
    teste_midi();

    if (falhas) {
        printf("%d verificacao(oes) falharam\n", falhas);
        return 1;
    }
    puts("test_game: todos os testes passaram");
    return 0;
}
