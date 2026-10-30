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
    Game_update(g, &b, 1, NULL, DT);
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

/* do menu principal até a lista de músicas: Jogar (quem aperta já entra, no
   "pronto"), pronto, e a contagem passa */
static void jogar(Game *g)
{
    aperta(g, ACAO_CONFIRMA);
    aperta(g, ACAO_CONFIRMA);
    espera(g, 1.3);
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
    jogar(&g);
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

static void teste_aviso_some(void)
{
    Game g;
    Game_init(&g);
    aperta(&g, ACAO_BAIXO);                   /* Configurações */
    aperta(&g, ACAO_CONFIRMA);
    aperta(&g, ACAO_CONFIRMA);                /* Controles */
    for (int i = 0; i < 13; i++) aperta(&g, ACAO_BAIXO);   /* até "Restaurar padrão" */
    aperta(&g, ACAO_CONFIRMA);
    CONFERE(g.tela == TELA_CONTROLES && g.pedido == PEDIDO_RESTAURAR_CONTROLES);
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

    aperta(&g, ACAO_BAIXO);                   /* Velocidade da pista */
    for (int i = 0; i < 20; i++) aperta(&g, ACAO_DIREITA);  /* quem tem Esq/Dir muda direto */
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

/* O modo de escolha: tudo com Cima/Baixo, Confirmar e Voltar (uma guitarra) */
static void teste_modo_de_escolha(void)
{
    Game g;
    Game_init(&g);
    Game_add_language(&g, "en", "English");
    aperta(&g, ACAO_BAIXO);
    aperta(&g, ACAO_CONFIRMA);                /* Configurações */
    aperta(&g, ACAO_BAIXO);                   /* Velocidade da pista */
    aperta(&g, ACAO_CIMA);
    CONFERE(Game_menu_sel(&g) == 0);          /* fora do modo de escolha, Cima/Baixo andam na lista */
    aperta(&g, ACAO_BAIXO);
    aperta(&g, ACAO_CONFIRMA);
    CONFERE(g.escolhendo && g.opcoes.velocidade == 5);
    aperta(&g, ACAO_CIMA);
    aperta(&g, ACAO_CIMA);
    CONFERE(g.opcoes.velocidade == 7 && Game_menu_sel(&g) == 1);   /* no modo de escolha, mudam o valor */
    aperta(&g, ACAO_VOLTA);                   /* desfaz, e continua nas configurações */
    CONFERE(!g.escolhendo && g.opcoes.velocidade == 5 && g.tela == TELA_CONFIG);
    aperta(&g, ACAO_CONFIRMA);
    aperta(&g, ACAO_BAIXO);
    aperta(&g, ACAO_CONFIRMA);                /* fica */
    CONFERE(!g.escolhendo && g.opcoes.velocidade == 4);

    aperta(&g, ACAO_BAIXO);                   /* Modo precisão: Sim/Não troca direto */
    aperta(&g, ACAO_CONFIRMA);
    CONFERE(g.opcoes.precisao && !g.escolhendo);

    while (Menu_da_tela(TELA_CONFIG)->itens[Game_menu_sel(&g)].id != ID_IDIOMA) aperta(&g, ACAO_BAIXO);
    aperta(&g, ACAO_CONFIRMA);
    aperta(&g, ACAO_CIMA);                    /* o idioma muda já (para ver como fica)... */
    CONFERE(strcmp(g.opcoes.idioma, "en") == 0 && g.pedido == PEDIDO_TROCAR_IDIOMA);
    aperta(&g, ACAO_VOLTA);                   /* ...e Voltar traz o de antes */
    CONFERE(strcmp(g.opcoes.idioma, "pt") == 0 && g.pedido == PEDIDO_TROCAR_IDIOMA);
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

    aperta(&g, ACAO_BAIXO);                   /* sai do "Jogador": Verde */
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
    CONFERE(g.jog[0].acertos == 1 && g.jog[0].combo == 1);
    CONFERE(g.jog[0].chart.notas[0].estado == NOTA_ACERTADA);
    CONFERE(g.jog[0].ultimo_erro_tempo < 0.0);       /* negativo = adiantado */
    CONFERE(g.jog[0].pontos == 50.0);
    Game_quit(&g);
}

static void teste_fora_da_janela(void)
{
    const Nota notas[] = { { .t = 1.0, .mascara = 1 } };
    Game g;
    partida(&g, notas, 1);

    ate(&g, 0.85, 0);
    frame(&g, 0, VERDE);                      /* 150 ms antes: não conta */
    CONFERE(g.jog[0].acertos == 0 && g.jog[0].erros == 0);
    CONFERE(g.jog[0].toques_errados == 1);           /* é um aperto sem nota: penalidade */

    ate(&g, 1.15, 0);                         /* passou da janela sem acertar */
    CONFERE(g.jog[0].erros == 1 && g.jog[0].combo == 0);
    CONFERE(g.jog[0].chart.notas[0].estado == NOTA_PERDIDA);
    Game_quit(&g);
}

static void teste_botao_errado(void)
{
    const Nota notas[] = { { .t = 1.0, .mascara = 1 } };
    Game g;
    partida(&g, notas, 1);
    ate(&g, 1.0, 0);
    frame(&g, 0, VERMELHO);                   /* nota verde, botão vermelho */
    CONFERE(g.jog[0].acertos == 0 && g.jog[0].toques_errados == 1);
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
    CONFERE(g.jog[0].acertos == 0);
    frame(&g, VERDE, VERMELHO);               /* completou, ainda dentro da janela */
    CONFERE(g.jog[0].acertos == 1);
    CONFERE(g.jog[0].pontos == 100.0);               /* 50 por botão */

    ate(&g, 2.0, 0);
    frame(&g, 0, VERDE);                      /* só metade, e nunca completa */
    ate(&g, 2.2, VERDE);
    CONFERE(g.jog[0].acertos == 1 && g.jog[0].erros == 1);
    Game_quit(&g);
}

static void teste_sustain_completo(void)
{
    const Nota notas[] = { { .t = 1.0, .duracao = 1.0, .mascara = 4 } };   /* amarelo, 1 s */
    Game g;
    partida(&g, notas, 1);

    ate(&g, 1.0, 0);
    frame(&g, 0, AMARELO);
    CONFERE(g.jog[0].chart.notas[0].segurando);
    ate(&g, 2.2, AMARELO);                    /* segura até depois do fim */
    CONFERE(!g.jog[0].chart.notas[0].segurando);
    CONFERE(fabs(g.jog[0].chart.notas[0].solto - 2.0) < 1e-9);
    /* 50 da nota + ~50 do segundo segurado */
    CONFERE(g.jog[0].pontos > 95.0 && g.jog[0].pontos < 101.0);
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
    CONFERE(!g.jog[0].chart.notas[0].segurando);
    CONFERE(g.jog[0].chart.notas[0].solto < 1.6);
    CONFERE(g.jog[0].combo == 1);                    /* soltar não quebra o combo */
    const double pontos = g.jog[0].pontos;
    ate(&g, 2.2, 0);
    CONFERE(g.jog[0].pontos == pontos);              /* e não soma mais nada */
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
    CONFERE(g.jog[0].combo == 12);
    CONFERE(g.jog[0].multiplicador == 2);            /* passou de 10 de combo */
    /* 9 notas a x1, a partir da 10ª (combo 10) a x2: 9*50 + 3*100 */
    CONFERE(g.jog[0].pontos == 750.0);
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
    CONFERE(fabs(g.jog[0].sp_medidor - 0.5f) < 1e-6);   /* duas frases = 50% */

    frame(&g, 0, ACAO_BIT(ACAO_STAR_POWER));
    CONFERE(g.jog[0].sp_ativo);
    ate(&g, 5.0, 0);
    frame(&g, 0, VERDE);
    CONFERE(g.jog[0].multiplicador == 2);            /* x1 dobrado */
    ate(&g, 15.0, 0);                         /* 50% dura 8 s */
    CONFERE(!g.jog[0].sp_ativo);
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
    CONFERE(g.jog[0].sp_medidor == 0.0f);
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
    CONFERE(!g.terminou && g.t < 0.1 && g.jog[0].erros == 0);
    CONFERE(g.jog[0].chart.notas[0].estado == NOTA_PENDENTE);
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
    CONFERE(g.t == 0.98 && g.jog[0].acertos == 1);

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
    CONFERE(ChartFile_contar(&a, INSTR_GUITARRA, DIF_EXPERT) == 6);
    CONFERE(ChartFile_contar(&a, INSTR_GUITARRA, DIF_FACIL) == 1);
    CONFERE(ChartFile_contar(&a, INSTR_GUITARRA, DIF_DIFICIL) == 0);
    CONFERE(ChartFile_tem(&a, INSTR_GUITARRA) && !ChartFile_tem(&a, INSTR_BAIXO));

    Chart c;
    CONFERE(Chart_from_file(&c, &a, INSTR_GUITARRA, DIF_EXPERT, 0.0));
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
    CONFERE(Chart_from_file(&d, &a, INSTR_GUITARRA, DIF_EXPERT, 0.5));   /* delay do song.ini */
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
    Chart_free(&g.jog[0].chart);
    Chart_from_file(&g.jog[0].chart, &a, INSTR_GUITARRA, DIF_EXPERT, 0.0);
    Game_play(&g);
    CONFERE(perto(g.t, -1.5));                                /* começa na preparação */

    const uint32_t botoes[6] = { VERDE | VERMELHO, AMARELO, ACAO_BIT(ACAO_AZUL),
                                 VERMELHO /* aberta: qualquer botão */, ACAO_BIT(ACAO_LARANJA), VERDE };
    for (int i = 0; i < 6; i++) {
        ate(&g, g.jog[0].chart.notas[i].t, 0);
        frame(&g, 0, botoes[i]);
    }
    CONFERE(g.jog[0].acertos == 6 && g.jog[0].erros == 0);
    CONFERE(fabs(g.jog[0].sp_medidor - 0.5f) < 1e-6);               /* duas frases, duas vezes 25% */
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

    jogar(&g);
    aperta(&g, ACAO_BAIXO);                                   /* demonstração */
    aperta(&g, ACAO_BAIXO);                                   /* a música */
    aperta(&g, ACAO_CONFIRMA);
    CONFERE(g.pedido == PEDIDO_ABRIR_MUSICA && g.pedido_indice == 0);

    CONFERE(Game_open_chart(&g, CHART, sizeof(CHART) - 1));  /* o main.c lê o arquivo e entrega */
    CONFERE(g.tela == TELA_PREPARAR);
    CONFERE(g.notas[INSTR_GUITARRA][DIF_EXPERT] == 6 && g.notas[INSTR_GUITARRA][DIF_FACIL] == 1);
    CONFERE(g.jog[0].ativo && !g.jog[1].ativo && g.jog[0].dificuldade == DIF_EXPERT);

    aperta(&g, ACAO_CONFIRMA);                                /* pronto, no Expert */
    CONFERE(g.tela == TELA_CARREGANDO && g.pedido == PEDIDO_CARREGAR_MUSICA);
    CONFERE(g.jog[0].dificuldade == DIF_EXPERT && g.jog[0].chart.n == 6);

    aperta(&g, ACAO_VOLTA);                                   /* desistiu no meio da carga */
    CONFERE(g.tela == TELA_PREPARAR && g.pedido == PEDIDO_CANCELAR_CARGA);

    CONFERE(g.jog[0].linha_prep == PREP_PRONTO);
    aperta(&g, ACAO_CIMA);                                    /* a linha da dificuldade */
    aperta(&g, ACAO_CONFIRMA);                                /* modo de escolha */
    CONFERE(g.jog[0].escolhendo);
    aperta(&g, ACAO_BAIXO);                                   /* mais fácil: pula Dificil e Medio, que não existem */
    CONFERE(g.jog[0].dificuldade == DIF_FACIL);
    aperta(&g, ACAO_BAIXO);                                   /* já é a mais fácil: fica */
    CONFERE(g.jog[0].dificuldade == DIF_FACIL);
    aperta(&g, ACAO_VOLTA);                                   /* Voltar desfaz: Expert de novo */
    CONFERE(!g.jog[0].escolhendo && g.jog[0].dificuldade == DIF_EXPERT && g.tela == TELA_PREPARAR);
    aperta(&g, ACAO_CONFIRMA);
    aperta(&g, ACAO_BAIXO);
    aperta(&g, ACAO_CONFIRMA);                                /* fica no Fácil */
    CONFERE(!g.jog[0].escolhendo && g.jog[0].dificuldade == DIF_FACIL && !g.jog[0].pronto);
    aperta(&g, ACAO_BAIXO);                                   /* a linha do pronto */
    aperta(&g, ACAO_CONFIRMA);
    CONFERE(g.jog[0].chart.n == 1);
    Game_audio_ready(&g, true, 10.0);                         /* o áudio ficou pronto */
    CONFERE(g.tela == TELA_JOGO && g.duracao_audio == 10.0);
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
    CONFERE(g.jog[0].acertos == 1);                  /* janela normal: +-100 ms */
    Game_quit(&g);

    partida(&g, notas, 1);
    g.opcoes.precisao = true;
    ate(&g, 1.085, 0);
    frame(&g, 0, VERDE);
    CONFERE(g.jog[0].acertos == 0);                  /* precisão: +-70 ms */
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

    jogar(&g);
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
    for (int i = 0; i < 360; i++) frame(&g, ACAO_BIT(ACAO_BAIXO), 0);   /* mais 3 s: acelera */
    CONFERE(Game_menu_sel(&g) > 60);
    for (int i = 0; i < 1200; i++) frame(&g, ACAO_BIT(ACAO_BAIXO), 0);  /* segurando, para no fim (não dá a volta) */
    CONFERE(Game_menu_sel(&g) == g.menu_musicas.n - 1);
    aperta(&g, ACAO_BAIXO);                                           /* um toque, sim */
    CONFERE(Game_menu_sel(&g) == 0);
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

    jogar(&g);
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
    aperta(&g, ACAO_VOLTA);                          /* na raiz, voltar sai da lista: a tela de jogadores */
    CONFERE(g.tela == TELA_JOGADORES);

    /* por artista: todas juntas, ordenadas pelo artista */
    aperta(&g, ACAO_CONFIRMA);                       /* pronto: de volta às músicas */
    espera(&g, 1.3);
    CONFERE(g.tela == TELA_MUSICAS);
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
    CONFERE(g.jog[0].novo_recorde && g.jog[0].recorde_anterior == 0);
    CONFERE(g.pedido == PEDIDO_SALVAR_RECORDE && g.pedido_indice == 0);
    CONFERE(g.musicas[0].recorde == 100 && g.musicas[0].recorde_dif == DIF_EXPERT && !g.musicas[0].recorde_fc);

    aperta(&g, ACAO_CONFIRMA);                       /* Jogar de novo, sem acertar nada */
    while (!g.terminou) frame(&g, 0, 0);
    CONFERE(!g.jog[0].novo_recorde && g.musicas[0].recorde == 100);   /* pior: o recorde fica */
    Game_quit(&g);
}

static void teste_opcoes_texto(void)
{
    Opcoes a = { .velocidade = 8, .precisao = true, .canhoto = true, .mostrar_fps = false,
                 .volume = 30, .organizar = ORGANIZAR_ARTISTA, .som_erro = true };
    char texto[512];
    Opcoes_write(&a, texto, sizeof(texto));

    Opcoes b = { .velocidade = 5, .volume = 80 };
    Opcoes_read(&b, texto, strlen(texto));
    CONFERE(b.velocidade == 8 && b.precisao && b.canhoto && !b.mostrar_fps);
    CONFERE(b.volume == 30 && b.organizar == ORGANIZAR_ARTISTA && b.som_erro);

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
    CONFERE(g.jog[0].toques_errados == 1);           /* nada perto: quebra o combo */

    ate(&g, 1.0, 0);
    frame(&g, 0, VERDE | VERMELHO);           /* o acorde, os dois juntos */
    CONFERE(g.jog[0].acertos == 1 && g.jog[0].toques_errados == 1);   /* o vermelho não foi punido... */
    CONFERE(g.jog[0].chart.notas[1].estado == NOTA_PENDENTE);   /* ...nem pegou a nota seguinte */
    ate(&g, 1.04, 0);
    frame(&g, 0, VERMELHO);
    CONFERE(g.jog[0].acertos == 2 && g.jog[0].combo == 2);
    Game_quit(&g);

    partida(&g, notas, 3);
    g.opcoes.penalidade = false;
    ate(&g, 0.5, 0);
    aperta(&g, ACAO_VERDE);
    CONFERE(g.jog[0].toques_errados == 0);           /* desligada: sem punição */
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
    CONFERE(g.jog[0].acertos == 3 && g.jog[0].toques_errados == 0 && g.jog[0].combo == 3);
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
    CONFERE(g.jog[0].acertos == 0 && g.jog[0].toques_errados == 1);   /* uma penalidade só, não três */
    ate(&g, 1.2, 0);
    CONFERE(g.jog[0].erros == 1);                              /* e o acorde passou */

    /* o amarelo já segurado de antes também conta como "a mais" */
    ate(&g, 2.0, AMARELO);
    frame(&g, AMARELO, VERMELHO | ACAO_BIT(ACAO_AZUL));
    CONFERE(g.jog[0].acertos == 0);
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
    CONFERE(g.jog[0].acertos == 2 && g.jog[0].toques_errados == 0);
    CONFERE(g.jog[0].chart.notas[0].segurando);
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
    CONFERE(perto(g.t, 1.0) && g.jog[0].acertos == 1);
    CONFERE(fabs(g.jog[0].ultimo_erro_tempo) < 1e-9);
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

    /* trilha 2: o baixo, com duas notas no Expert */
    Midi t2 = {0};
    m_vlq(&t2, 0); m_byte(&t2, 0xFF); m_byte(&t2, 0x03); m_byte(&t2, 9);
    memcpy(t2.b + t2.n, "PART BASS", 9); t2.n += 9;
    m_nota(&t2, 480, 500, 96);
    m_nota(&t2, 960, 980, 100);

    Midi arq = {0};
    memcpy(arq.b, "MThd", 4); arq.n = 4;
    const unsigned char cab[] = { 0, 0, 0, 6, 0, 1, 0, 3, 0x01, 0xE0 };
    memcpy(arq.b + arq.n, cab, sizeof(cab)); arq.n += sizeof(cab);
    m_trilha(&arq, &t0);
    m_trilha(&arq, &t1);
    m_trilha(&arq, &t2);

    ArquivoChart a;
    char erro[64];
    const bool leu = ChartFile_parse_mid(&a, arq.b, arq.n, 116, erro, sizeof(erro));
    CONFERE(leu);
    if (!leu) { printf("  erro do MIDI: %s\n", erro); return; }
    CONFERE(ChartFile_contar(&a, INSTR_GUITARRA, DIF_EXPERT) == 5 && ChartFile_contar(&a, INSTR_GUITARRA, DIF_FACIL) == 1);
    CONFERE(ChartFile_contar(&a, INSTR_BAIXO, DIF_EXPERT) == 2 && ChartFile_contar(&a, INSTR_BAIXO, DIF_FACIL) == 0);
    CONFERE(!ChartFile_tem(&a, INSTR_RITMICA));
    Chart c;
    CONFERE(Chart_from_file(&c, &a, INSTR_GUITARRA, DIF_EXPERT, 0.0));
    CONFERE(c.n == 5);
    CONFERE(perto(c.notas[0].t, 0.5) && c.notas[0].duracao == 0.0);
    CONFERE(!(c.notas[0].flags & NOTA_HOPO) && (c.notas[1].flags & NOTA_HOPO));
    CONFERE(perto(c.notas[2].duracao, 0.5));
    CONFERE(!(c.notas[3].flags & NOTA_HOPO));                 /* forçado a palhetada */
    CONFERE(perto(c.notas[4].t, 2.5) && c.notas[4].mascara == (1 | 2));   /* depois da troca para 60 BPM */
    CONFERE((c.notas[4].flags & NOTA_STAR) && !(c.notas[4].flags & NOTA_HOPO));
    Chart_free(&c);

    CONFERE(Chart_from_file(&c, &a, INSTR_BAIXO, DIF_EXPERT, 0.0));
    CONFERE(c.n == 2 && perto(c.notas[0].t, 0.5) && c.notas[1].mascara == 16);   /* laranja em 1,0 s */
    Chart_free(&c);
    ChartFile_free(&a);

    CONFERE(!ChartFile_parse_mid(&a, arq.b, 30, 116, erro, sizeof(erro)));   /* cortado no meio */
}

/* ===========================================================================
 * Vários jogadores e instrumentos
 * ========================================================================= */

/* um frame com os botões de dois jogadores (o que foi apertado também está segurado) */
static void frame2(Game *g, uint32_t j1, uint32_t j2)
{
    const Botoes b[2] = { { .segurados = j1, .apertados = j1 }, { .segurados = j2, .apertados = j2 } };
    Game_update(g, b, 2, NULL, DT);
    Score_apply_events(g);
}

/* 120 BPM: o tick 384 é 1,0 s */
static const char CHART_DOIS[] =
    "[Song]\n{\n  Resolution = 192\n}\n"
    "[SyncTrack]\n{\n  0 = B 120000\n}\n"
    "[ExpertSingle]\n{\n  384 = N 0 0\n  768 = N 1 0\n}\n"                  /* verde 1 s, vermelho 2 s */
    "[ExpertDoubleBass]\n{\n  384 = N 2 0\n  576 = N 2 0\n  768 = N 3 0\n}\n"   /* amarelo, amarelo, azul */
    "[HardDoubleBass]\n{\n  384 = N 2 0\n}\n";

static void teste_multijogador(void)
{
    Game g;
    Game_init(&g);
    musica(&g, "Musica", "Banda", "");
    Game_songs_done(&g);

    /* a tela de jogadores: quem apertou Jogar (o J1) já está dentro; o J2 entra com o controle dele */
    frame2(&g, ACAO_BIT(ACAO_CONFIRMA), 0);          /* Jogar */
    CONFERE(g.tela == TELA_JOGADORES && g.jog[0].na_sessao && !g.jog[1].na_sessao);
    frame2(&g, 0, ACAO_BIT(ACAO_CONFIRMA));
    CONFERE(g.jog[1].na_sessao && !g.jog[1].pronto);
    frame2(&g, ACAO_BIT(ACAO_CONFIRMA), 0);          /* o 1 está pronto... */
    for (int i = 0; i < 200; i++) frame2(&g, 0, 0);
    CONFERE(g.tela == TELA_JOGADORES);               /* ...mas falta o 2: nada de contagem */
    frame2(&g, 0, ACAO_BIT(ACAO_CONFIRMA));          /* o 2 pronto: contagem, e as músicas */
    for (int i = 0; i < 160; i++) frame2(&g, 0, 0);
    CONFERE(g.tela == TELA_MUSICAS);

    g.musica = 0;
    CONFERE(Game_open_chart(&g, CHART_DOIS, sizeof(CHART_DOIS) - 1));
    CONFERE(g.tela == TELA_PREPARAR && Game_n_jogando(&g) == 2);   /* a sessão vale para esta música */
    frame2(&g, 0, ACAO_BIT(ACAO_CIMA));              /* do pronto para a dificuldade... */
    frame2(&g, 0, ACAO_BIT(ACAO_CIMA));              /* ...e o instrumento */
    frame2(&g, 0, ACAO_BIT(ACAO_CONFIRMA));          /* modo de escolha: só com a palhetada */
    frame2(&g, 0, ACAO_BIT(ACAO_CIMA));
    CONFERE(g.jog[1].instrumento == INSTR_BAIXO && g.jog[1].dificuldade == DIF_EXPERT);
    CONFERE(g.jog[0].instrumento == INSTR_GUITARRA); /* os botões do 2 não mexem no 1 */
    frame2(&g, 0, ACAO_BIT(ACAO_CIMA));              /* não há outro: dá a volta para a guitarra */
    CONFERE(g.jog[1].instrumento == INSTR_GUITARRA);
    frame2(&g, 0, ACAO_BIT(ACAO_BAIXO));
    CONFERE(g.jog[1].instrumento == INSTR_BAIXO);
    frame2(&g, 0, ACAO_BIT(ACAO_CONFIRMA));          /* fica no baixo */
    frame2(&g, 0, ACAO_BIT(ACAO_DIREITA));           /* com Esq/Dir, muda direto: guitarra */
    frame2(&g, 0, ACAO_BIT(ACAO_ESQUERDA));          /* e baixo de novo */
    CONFERE(g.jog[1].instrumento == INSTR_BAIXO && !g.jog[1].escolhendo);
    frame2(&g, 0, ACAO_BIT(ACAO_BAIXO));
    frame2(&g, 0, ACAO_BIT(ACAO_BAIXO));             /* a linha do pronto */

    frame2(&g, ACAO_BIT(ACAO_CONFIRMA), 0);          /* o 1 está pronto: espera o 2 */
    CONFERE(g.tela == TELA_PREPARAR && g.jog[0].pronto);
    frame2(&g, 0, ACAO_BIT(ACAO_CONFIRMA));          /* os dois prontos: carrega */
    CONFERE(g.tela == TELA_CARREGANDO && g.pedido == PEDIDO_CARREGAR_MUSICA);
    CONFERE(g.jog[0].chart.n == 2 && g.jog[1].chart.n == 3);
    Game_audio_ready(&g, true, 0.0);
    CONFERE(g.tela == TELA_JOGO);

    frame2(&g, 0, ACAO_BIT(ACAO_PAUSA));             /* qualquer um pausa... */
    CONFERE(g.pausado);
    frame2(&g, ACAO_BIT(ACAO_PAUSA), 0);             /* ...e qualquer um continua */
    CONFERE(!g.pausado);

    /* em 1 s: o 1 acerta o verde; o 2 aperta o verde, mas o baixo pede amarelo */
    while (g.t < 1.0 - DT / 2) frame2(&g, 0, 0);
    frame2(&g, VERDE, VERDE);
    CONFERE(g.jog[0].acertos == 1 && g.jog[1].acertos == 0 && g.jog[1].toques_errados == 1);
    while (g.t < 2.0 - DT / 2) frame2(&g, 0, 0);
    frame2(&g, VERMELHO, ACAO_BIT(ACAO_AZUL));
    while (!g.terminou) frame2(&g, 0, 0);
    CONFERE(g.jog[0].acertos == 2 && g.jog[0].erros == 0 && g.jog[0].pontos == 100.0);
    CONFERE(g.jog[1].acertos == 1 && g.jog[1].erros == 2 && g.jog[1].pontos == 50.0);

    /* o recorde é da música: fica o do melhor */
    CONFERE(g.jog[0].novo_recorde && !g.jog[1].novo_recorde);
    CONFERE(g.musicas[0].recorde == 100 && g.musicas[0].recorde_instr == INSTR_GUITARRA);
    Game_quit(&g);
}

static void teste_preparar_sem_os_outros(void)
{
    Game g;
    Game_init(&g);
    musica(&g, "Musica", "Banda", "");
    Game_songs_done(&g);
    g.jog[0].na_sessao = g.jog[1].na_sessao = true;  /* os dois saíram juntos da tela de jogadores */
    g.musica = 0;
    Game_open_chart(&g, CHART_DOIS, sizeof(CHART_DOIS) - 1);
    CONFERE(Game_n_jogando(&g) == 2);

    frame2(&g, ACAO_BIT(ACAO_CONFIRMA), 0);          /* o 1 fica pronto, e o 2 some */
    CONFERE(g.tela == TELA_PREPARAR);
    frame2(&g, ACAO_BIT(ACAO_CONFIRMA), 0);          /* o líder confirma de novo: começa sem ele */
    CONFERE(g.tela == TELA_CARREGANDO && !g.jog[1].ativo && Game_n_jogando(&g) == 1);
    CONFERE(g.jog[1].na_sessao);                     /* fora desta música, mas ainda na sessão */

    aperta(&g, ACAO_VOLTA);                          /* cancela a carga */
    CONFERE(g.tela == TELA_PREPARAR && !g.jog[0].pronto);
    frame2(&g, 0, ACAO_BIT(ACAO_CONFIRMA));          /* o 2 volta para esta música */
    CONFERE(g.jog[1].ativo && !g.jog[1].pronto);
    frame2(&g, 0, ACAO_BIT(ACAO_CONFIRMA));          /* pronto */
    frame2(&g, 0, ACAO_BIT(ACAO_VOLTA));             /* desfaz o pronto */
    CONFERE(g.jog[1].ativo && !g.jog[1].pronto);
    frame2(&g, 0, ACAO_BIT(ACAO_VOLTA));             /* fica fora desta */
    CONFERE(!g.jog[1].ativo && g.jog[1].na_sessao);
    aperta(&g, ACAO_VOLTA);                          /* o líder volta para a lista */
    CONFERE(g.tela == TELA_MUSICAS);

    g.musica = 0;                                    /* a próxima música: o 2 joga de novo */
    Game_open_chart(&g, CHART_DOIS, sizeof(CHART_DOIS) - 1);
    CONFERE(g.jog[1].ativo && Game_n_jogando(&g) == 2);
    Game_quit(&g);
}

/* o controle do J1 acaba a bateria: o J2 assume, e na partida ela pausa */
static void teste_controle_desconectado(void)
{
    Game g;
    Game_init(&g);
    musica(&g, "Musica", "Banda", "");
    Game_songs_done(&g);
    g.jog[0].na_sessao = g.jog[1].na_sessao = true;
    bool con[4] = { true, true, false, false }, gui[4] = { false };
    int  tec[4] = { TECLAS_OK, TECLAS_OK, TECLAS_OK, TECLAS_OK };
    Game_set_devices(&g, con, tec, gui);
    g.musica = 0;
    Game_open_chart(&g, CHART_DOIS, sizeof(CHART_DOIS) - 1);
    CONFERE(Game_leader(&g) == 0 && Game_n_jogando(&g) == 2);

    con[0] = false;
    Game_set_devices(&g, con, tec, gui);
    frame2(&g, 0, 0);
    CONFERE(!g.jog[0].ativo && Game_leader(&g) == 1);    /* o J1 fica fora; o J2 manda */
    frame2(&g, 0, ACAO_BIT(ACAO_CONFIRMA));               /* e começa sozinho */
    CONFERE(g.tela == TELA_CARREGANDO && Game_n_jogando(&g) == 1);
    Game_audio_ready(&g, true, 0.0);
    frame2(&g, 0, 0);
    CONFERE(g.tela == TELA_JOGO && !g.pausado);

    con[1] = false;                                       /* agora o do J2, no meio da música */
    Game_set_devices(&g, con, tec, gui);
    frame2(&g, 0, 0);
    CONFERE(g.pausado && g.aviso != NULL);

    con[1] = true;                                        /* ligou outro: segue */
    Game_set_devices(&g, con, tec, gui);
    frame2(&g, 0, ACAO_BIT(ACAO_PAUSA));
    CONFERE(!g.pausado);

    Game_vai_para(&g, TELA_JOGADORES);                    /* na tela de jogadores: quem não tem aparelho sai */
    frame2(&g, 0, 0);
    CONFERE(!g.jog[0].na_sessao && g.jog[1].na_sessao);
    Game_quit(&g);
}

static void teste_chart_so_de_baixo(void)
{
    static const char SO_BAIXO[] = "[Song]\n{\n  Resolution = 192\n}\n[SyncTrack]\n{\n  0 = B 120000\n}\n"
                                   "[MediumDoubleBass]\n{\n  384 = N 0 0\n}\n";
    Game g;
    Game_init(&g);
    CONFERE(Game_open_chart(&g, SO_BAIXO, sizeof(SO_BAIXO) - 1));   /* sem guitarra: vale o que tiver */
    CONFERE(g.jog[0].instrumento == INSTR_BAIXO && g.jog[0].dificuldade == DIF_MEDIO);
    aperta(&g, ACAO_CONFIRMA);
    CONFERE(g.tela == TELA_CARREGANDO && g.jog[0].chart.n == 1);
    Game_quit(&g);
}

/* as cinco partes de 5 trastes de um .chart, cada uma no seu instrumento */
static void teste_partes_do_chart(void)
{
    static const char TODAS[] = "[Song]\n{\n  Resolution = 192\n}\n[SyncTrack]\n{\n  0 = B 120000\n}\n"
                                "[ExpertSingle]\n{\n  384 = N 0 0\n}\n"
                                "[HardDoubleBass]\n{\n  384 = N 1 0\n  480 = N 1 0\n}\n"
                                "[MediumDoubleRhythm]\n{\n  384 = N 2 0\n}\n"
                                "[EasyDoubleGuitar]\n{\n  384 = N 3 0\n}\n"
                                "[ExpertKeyboard]\n{\n  384 = N 4 0\n  576 = N 4 0\n  768 = N 4 0\n}\n"
                                "[ExpertDrums]\n{\n  384 = N 0 0\n}\n";   /* bateria: fica de fora */
    ArquivoChart a;
    char erro[64];
    CONFERE(ChartFile_parse(&a, TODAS, sizeof(TODAS) - 1, erro, sizeof(erro)));
    CONFERE(ChartFile_contar(&a, INSTR_GUITARRA, DIF_EXPERT) == 1);
    CONFERE(ChartFile_contar(&a, INSTR_BAIXO, DIF_DIFICIL) == 2);
    CONFERE(ChartFile_contar(&a, INSTR_RITMICA, DIF_MEDIO) == 1);
    CONFERE(ChartFile_contar(&a, INSTR_COOP, DIF_FACIL) == 1);
    CONFERE(ChartFile_contar(&a, INSTR_TECLADO, DIF_EXPERT) == 3);
    Chart c;
    CONFERE(Chart_from_file(&c, &a, INSTR_TECLADO, DIF_EXPERT, 0.0) && c.n == 3 && c.notas[0].mascara == 16);
    Chart_free(&c);
    CONFERE(!Chart_from_file(&c, &a, INSTR_COOP, DIF_EXPERT, 0.0));   /* parte que não existe */
    Chart_free(&c);
    ChartFile_free(&a);
}

/* um frame com os botões do jogador 1 (controle) e as teclas de menu do teclado */
static void frame_teclado(Game *g, uint32_t j1, uint32_t tecla)
{
    const Botoes b[1] = { { .segurados = j1, .apertados = j1 } };
    const Botoes t = { .segurados = tecla, .apertados = tecla };
    Game_update(g, b, 1, &t, DT);
    Score_apply_events(g);
}

#define TECLA(a) ACAO_BIT(a)

/* J1 no controle, J2, J3 e J4 dividindo o teclado */
static void teste_teclado_dividido(void)
{
    Game g;
    Game_init(&g);
    musica(&g, "Musica", "Banda", "");
    Game_songs_done(&g);
    for (int k = 0; k < 4; k++) g.jog[k].na_sessao = true;   /* entraram na tela de jogadores... */
    g.jog[1].no_teclado = g.jog[2].no_teclado = g.jog[3].no_teclado = true;   /* ...três pelo teclado */
    g.musica = 0;
    Game_open_chart(&g, CHART_DOIS, sizeof(CHART_DOIS) - 1);
    CONFERE(g.tela == TELA_PREPARAR && Game_n_jogando(&g) == 4 && Game_vez_teclado(&g) == 1);

    frame_teclado(&g, 0, TECLA(ACAO_CIMA));             /* o teclado mexe no J2... */
    frame_teclado(&g, 0, TECLA(ACAO_CIMA));
    frame_teclado(&g, 0, TECLA(ACAO_DIREITA));
    CONFERE(g.jog[1].instrumento == INSTR_BAIXO && g.jog[2].instrumento == INSTR_GUITARRA);
    CONFERE(!g.jog[0].pronto);                           /* ...e não no J1 */
    frame_teclado(&g, 0, TECLA(ACAO_BAIXO));
    frame_teclado(&g, 0, TECLA(ACAO_BAIXO));
    frame_teclado(&g, 0, TECLA(ACAO_CONFIRMA));          /* J2 pronto: a vez passa ao J3 */
    CONFERE(g.jog[1].pronto && Game_vez_teclado(&g) == 2);
    frame_teclado(&g, 0, TECLA(ACAO_CONFIRMA));          /* J3 pronto */
    frame_teclado(&g, 0, TECLA(ACAO_VOLTA));             /* J4 não joga esta */
    CONFERE(!g.jog[3].ativo && g.jog[3].na_sessao && Game_vez_teclado(&g) == -1);
    frame_teclado(&g, 0, TECLA(ACAO_CONFIRMA));          /* sem vez: Enter traz o J4 de volta */
    CONFERE(g.jog[3].ativo && Game_vez_teclado(&g) == 3);
    frame_teclado(&g, 0, TECLA(ACAO_VOLTA));
    CONFERE(!g.jog[3].ativo);
    frame_teclado(&g, 0, TECLA(ACAO_VOLTA));             /* sem vez: Esc desfaz o último pronto (J3) */
    CONFERE(!g.jog[2].pronto && Game_vez_teclado(&g) == 2);
    frame_teclado(&g, 0, TECLA(ACAO_CONFIRMA));
    CONFERE(g.tela == TELA_PREPARAR);                    /* falta o J1 */

    frame_teclado(&g, ACAO_BIT(ACAO_CONFIRMA), 0);       /* o J1, no controle */
    CONFERE(g.tela == TELA_CARREGANDO && Game_n_jogando(&g) == 3);
    CONFERE(g.jog[1].chart.n == 3 && g.jog[2].chart.n == 2);   /* baixo e guitarra */
    Game_quit(&g);
}

/* um frame com os botões de dois controles e as teclas de menu do teclado */
static void frame_tudo(Game *g, uint32_t j1, uint32_t j2, uint32_t tecla)
{
    const Botoes b[2] = { { .segurados = j1, .apertados = j1 }, { .segurados = j2, .apertados = j2 } };
    const Botoes t = { .segurados = tecla, .apertados = tecla };
    Game_update(g, b, 2, &t, DT);
    Score_apply_events(g);
}

/* o que o main.c informa: quem tem controle, e as teclas de cada um */
static void aparelhos(Game *g, bool c0, bool c1, bool c2, bool c3, int teclas)
{
    const bool con[4] = { c0, c1, c2, c3 }, gui[4] = { false };
    const int  tec[4] = { teclas, teclas, teclas, teclas };
    Game_set_devices(g, con, tec, gui);
}

/* A tela de jogadores com um controle (J1) e dois no teclado, e uma conta
   nova pelo teclado da tela */
static void teste_tela_de_jogadores(void)
{
    Game g;
    Game_init(&g);
    Game_add_account(&g, "Ana");
    aparelhos(&g, true, false, false, false, TECLAS_OK);
    frame_tudo(&g, ACAO_BIT(ACAO_CONFIRMA), 0, 0);              /* Jogar, no controle do J1 */
    CONFERE(g.tela == TELA_JOGADORES && g.jog[0].na_sessao && !g.jog[0].no_teclado);
    CONFERE(!g.jog[1].na_sessao && g.jog[0].linha_sessao == SESSAO_PRONTO);

    frame_tudo(&g, 0, 0, TECLA(ACAO_CONFIRMA));                 /* Enter: o teclado entra no J2 (sem controle) */
    CONFERE(g.jog[1].na_sessao && g.jog[1].no_teclado && g.cursor_teclado == 1 && g.jog[1].conta == 0);
    frame_tudo(&g, 0, 0, TECLA(ACAO_CIMA));
    frame_tudo(&g, 0, 0, TECLA(ACAO_CIMA));
    frame_tudo(&g, 0, 0, TECLA(ACAO_CIMA));                     /* do pronto até a conta */
    CONFERE(g.jog[1].linha_sessao == SESSAO_CONTA);
    frame_tudo(&g, 0, 0, TECLA(ACAO_CONFIRMA));                 /* modo de escolha da conta */
    frame_tudo(&g, 0, 0, TECLA(ACAO_CIMA));                     /* Ana */
    CONFERE(g.jog[1].escolhendo && Game_player_name(&g, 1) && strcmp(Game_player_name(&g, 1), "Ana") == 0);
    frame_tudo(&g, 0, 0, TECLA(ACAO_CONFIRMA));
    for (int i = 0; i < 3; i++) frame_tudo(&g, 0, 0, TECLA(ACAO_BAIXO));   /* até o pronto */
    frame_tudo(&g, 0, 0, TECLA(ACAO_CONFIRMA));                 /* pronto */
    CONFERE(g.jog[1].pronto && g.cursor_teclado == 1);

    frame_tudo(&g, 0, 0, TECLA(ACAO_DIREITA));                  /* o teclado vai para o J3 */
    CONFERE(g.cursor_teclado == 2);
    frame_tudo(&g, 0, 0, TECLA(ACAO_CONFIRMA));                 /* e entra nele */
    aparelhos(&g, true, false, false, false, TECLAS_REPETIDAS); /* as teclas do J3 batem com as do J2 */
    frame_tudo(&g, 0, 0, TECLA(ACAO_CONFIRMA));
    CONFERE(g.jog[2].na_sessao && !g.jog[2].pronto && g.aviso != NULL);   /* não fica pronto: Controles */
    aparelhos(&g, true, false, false, false, TECLAS_OK);

    frame_tudo(&g, 0, 0, TECLA(ACAO_CIMA));
    frame_tudo(&g, 0, 0, TECLA(ACAO_CIMA));
    frame_tudo(&g, 0, 0, TECLA(ACAO_CIMA));                     /* a conta */
    frame_tudo(&g, 0, 0, TECLA(ACAO_CONFIRMA));
    frame_tudo(&g, 0, 0, TECLA(ACAO_CIMA));                     /* a Ana está com o J2: pula para "+ novo" */
    CONFERE(g.jog[2].conta == g.n_contas + 1);
    frame_tudo(&g, 0, 0, TECLA(ACAO_CONFIRMA));                 /* o teclado da tela */
    CONFERE(g.nome_jogador == 2);

    /* "BIA": B, I e A, e o OK na linha de baixo */
    frame_tudo(&g, 0, 0, TECLA(ACAO_DIREITA));
    frame_tudo(&g, 0, 0, TECLA(ACAO_CONFIRMA));
    for (int i = 0; i < 7; i++) frame_tudo(&g, 0, 0, TECLA(ACAO_DIREITA));
    frame_tudo(&g, 0, 0, TECLA(ACAO_CONFIRMA));
    for (int i = 0; i < 8; i++) frame_tudo(&g, 0, 0, TECLA(ACAO_ESQUERDA));
    frame_tudo(&g, ACAO_BIT(ACAO_CONFIRMA), 0, 0);              /* o controle do J1 não escreve aqui */
    frame_tudo(&g, 0, 0, TECLA(ACAO_CONFIRMA));
    CONFERE(strcmp(g.nome_novo, "BIA") == 0);
    for (int i = 0; i < 4; i++) frame_tudo(&g, 0, 0, TECLA(ACAO_BAIXO));
    for (int i = 0; i < 3; i++) frame_tudo(&g, 0, 0, TECLA(ACAO_DIREITA));
    CONFERE(g.nome_cursor == TECLA_OK);
    frame_tudo(&g, 0, 0, TECLA(ACAO_CONFIRMA));
    CONFERE(g.nome_jogador == -1 && g.n_contas == 2 && g.pedido == PEDIDO_SALVAR_CONTAS);
    CONFERE(Game_player_name(&g, 2) && strcmp(Game_player_name(&g, 2), "BIA") == 0);
    CONFERE(!g.jog[0].pronto);

    for (int i = 0; i < 3; i++) frame_tudo(&g, 0, 0, TECLA(ACAO_BAIXO));   /* da conta ao pronto */
    frame_tudo(&g, 0, 0, TECLA(ACAO_CONFIRMA));                 /* J3 pronto */
    frame_tudo(&g, ACAO_BIT(ACAO_CONFIRMA), 0, 0);              /* J1 pronto: todos prontos */
    CONFERE(g.tela == TELA_JOGADORES && g.contagem > 0.0);
    espera(&g, 1.3);
    CONFERE(g.tela == TELA_MUSICAS);
    CONFERE(strcmp(g.opcoes.conta[1], "Ana") == 0 && strcmp(g.opcoes.conta[2], "BIA") == 0);   /* lembradas */
    Game_quit(&g);
}

/* Quem perde o controle sai sozinho; quem está no teclado nunca some, e o
   teclado só entra num cartão sem controle */
static void teste_quem_some_sai(void)
{
    Game g;
    Game_init(&g);
    aparelhos(&g, true, true, false, false, TECLAS_FALTAM);
    frame_tudo(&g, ACAO_BIT(ACAO_CONFIRMA), 0, 0);              /* Jogar (o J1 entra) */
    frame_tudo(&g, 0, ACAO_BIT(ACAO_CONFIRMA), 0);              /* o J2 entra pelo controle */
    frame_tudo(&g, 0, 0, TECLA(ACAO_CONFIRMA));                 /* o teclado: o J3 */
    CONFERE(g.jog[0].na_sessao && g.jog[1].na_sessao && g.jog[2].na_sessao && g.jog[2].no_teclado);
    frame_tudo(&g, 0, 0, TECLA(ACAO_CONFIRMA));                 /* sem teclas: não fica pronto */
    CONFERE(!g.jog[2].pronto && g.aviso != NULL);

    aparelhos(&g, false, true, false, false, TECLAS_FALTAM);    /* o controle do J1 desliga */
    frame_tudo(&g, 0, 0, 0);
    CONFERE(!g.jog[0].na_sessao && g.jog[1].na_sessao && g.jog[2].na_sessao);   /* sai sozinho, sem teclado */
    frame_tudo(&g, 0, 0, TECLA(ACAO_ESQUERDA));                 /* o cartão do J1 ficou livre para o teclado... */
    CONFERE(g.cursor_teclado == 0);
    frame_tudo(&g, 0, 0, TECLA(ACAO_ESQUERDA));                 /* ...o do J2 não (tem controle): volta ao J4 */
    CONFERE(g.cursor_teclado == 3);
    frame_tudo(&g, 0, 0, TECLA(ACAO_VOLTA));                    /* Esc num cartão livre, com gente: nada */
    CONFERE(g.tela == TELA_JOGADORES);

    frame_tudo(&g, 0, ACAO_BIT(ACAO_VOLTA), 0);                 /* o J2 sai; o J3 (teclado) fica */
    CONFERE(!g.jog[1].na_sessao && g.jog[2].na_sessao);
    frame_tudo(&g, 0, 0, TECLA(ACAO_ESQUERDA));                 /* do J4 de volta ao J3 */
    frame_tudo(&g, 0, 0, TECLA(ACAO_VOLTA));                    /* o J3 sai: a sessão fica vazia, a tela fica */
    CONFERE(!g.jog[2].na_sessao && !g.jog[2].no_teclado && g.tela == TELA_JOGADORES);
    frame_tudo(&g, 0, ACAO_BIT(ACAO_VOLTA), 0);                 /* Voltar sem ninguém: menu principal */
    CONFERE(g.tela == TELA_INICIO);

    /* voltando pelo teclado: quem apertou Jogar entra, no primeiro cartão sem controle */
    frame_tudo(&g, 0, 0, TECLA(ACAO_CONFIRMA));
    CONFERE(g.tela == TELA_JOGADORES && g.jog[0].na_sessao && g.jog[0].no_teclado && g.cursor_teclado == 0);
    Game_quit(&g);
}

/* As outras linhas do cartão: o modo (o main.c troca no perfil) e os
   Controles daquele jogador, que voltam para esta tela */
static void teste_cartao_modo_e_controles(void)
{
    Game g;
    Game_init(&g);
    frame2(&g, ACAO_BIT(ACAO_CONFIRMA), 0);                     /* Jogar */
    frame2(&g, 0, ACAO_BIT(ACAO_CONFIRMA));                     /* o J2 entra */
    frame2(&g, ACAO_BIT(ACAO_CONFIRMA), 0);                     /* o J1 fica pronto */
    frame2(&g, 0, ACAO_BIT(ACAO_CIMA));
    frame2(&g, 0, ACAO_BIT(ACAO_CIMA));                         /* a linha do modo */
    frame2(&g, 0, ACAO_BIT(ACAO_CONFIRMA));
    CONFERE(g.trocar_modo == 2u);                               /* o modo do J2 */
    frame2(&g, 0, ACAO_BIT(ACAO_BAIXO));                        /* Controles */
    frame2(&g, 0, ACAO_BIT(ACAO_CONFIRMA));
    CONFERE(g.tela == TELA_CONTROLES && g.controles_jogador == 1);
    frame2(&g, 0, ACAO_BIT(ACAO_CONFIRMA));                     /* Jogador: modo de escolha (J1..J4) */
    frame2(&g, 0, ACAO_BIT(ACAO_CIMA));
    CONFERE(g.escolhendo && g.controles_jogador == 2);
    frame2(&g, 0, ACAO_BIT(ACAO_VOLTA));                        /* desfaz: de volta ao J2 */
    CONFERE(!g.escolhendo && g.controles_jogador == 1);
    frame2(&g, 0, ACAO_BIT(ACAO_VOLTA));                        /* sai dos controles: a tela de jogadores */
    CONFERE(g.tela == TELA_JOGADORES && g.jog[0].pronto && g.jog[1].na_sessao);   /* quem estava pronto continua */
    Game_quit(&g);
}

/* O teclado da tela no modo guitarra: a palhetada anda tecla por tecla */
static void teste_nome_na_guitarra(void)
{
    Game g;
    Game_init(&g);
    g.guitarra[0] = true;
    jogar(&g);
    aperta(&g, ACAO_VOLTA);                                     /* das músicas de volta aos jogadores */
    CONFERE(g.tela == TELA_JOGADORES && g.jog[0].na_sessao && !g.jog[0].pronto);
    for (int i = 0; i < 3; i++) aperta(&g, ACAO_CIMA);          /* a conta */
    aperta(&g, ACAO_CONFIRMA);
    aperta(&g, ACAO_BAIXO);                                     /* sem contas: "+ novo jogador" */
    aperta(&g, ACAO_CONFIRMA);
    CONFERE(g.nome_jogador == 0 && g.nome_cursor == 0);
    aperta(&g, ACAO_BAIXO);                                     /* B: a próxima tecla, não a linha de baixo */
    CONFERE(g.nome_cursor == 1);
    aperta(&g, ACAO_CONFIRMA);
    aperta(&g, ACAO_CIMA);
    aperta(&g, ACAO_CIMA);                                      /* do A para trás: OK, a última */
    CONFERE(g.nome_cursor == TECLA_OK);
    for (int i = 0; i < 120; i++) frame(&g, ACAO_BIT(ACAO_BAIXO), 0);  /* segurando 1 s: anda sozinho, e dá a volta */
    CONFERE(g.nome_cursor > 1 && g.nome_cursor < 10);
    aperta(&g, ACAO_VOLTA);                                     /* apaga o B */
    aperta(&g, ACAO_VOLTA);                                     /* vazio: desiste */
    CONFERE(g.nome_jogador == -1 && g.jog[0].conta == 0 && g.n_contas == 0);
    Game_quit(&g);
}

#define PALHETA ACAO_BIT(ACAO_PALHETA_BAIXO)

static void teste_modo_guitarra(void)
{
    const Nota notas[] = {
        { .t = 1.00, .mascara = 1 },                         /* verde, de palhetada */
        { .t = 1.25, .mascara = 2, .flags = NOTA_HOPO },     /* vermelho, HOPO */
        { .t = 1.50, .mascara = 1 | 4 },                     /* acorde verde + amarelo */
        { .t = 2.00, .mascara = 8, .flags = NOTA_TAP },      /* azul, tap */
        { .t = 2.50, .mascara = 0, .flags = NOTA_ABERTA },   /* aberta */
    };
    Game g;
    partida(&g, notas, 5);
    g.guitarra[0] = true;

    ate(&g, 1.0, VERDE);
    CONFERE(g.jog[0].acertos == 0);                     /* só o traste não toca */
    frame(&g, VERDE, PALHETA);
    CONFERE(g.jog[0].acertos == 1);
    ate(&g, 1.25, VERDE);
    frame(&g, VERDE | VERMELHO, 0);                     /* HOPO: martela o vermelho (o verde ancorado) */
    CONFERE(g.jog[0].acertos == 2);
    frame(&g, VERDE | VERMELHO, PALHETA);               /* palhetada logo depois do HOPO: ignorada */
    CONFERE(g.jog[0].toques_errados == 0);
    ate(&g, 1.5, VERDE | VERMELHO);
    frame(&g, VERDE | AMARELO, PALHETA);                /* acorde: os trastes exatos */
    CONFERE(g.jog[0].acertos == 3);
    ate(&g, 2.0, 0);
    frame(&g, ACAO_BIT(ACAO_AZUL), 0);                  /* tap: sem palhetar */
    CONFERE(g.jog[0].acertos == 4);
    ate(&g, 2.5, 0);
    frame(&g, 0, PALHETA);                              /* aberta: palheta sem traste */
    CONFERE(g.jog[0].acertos == 5 && g.jog[0].erros == 0 && g.jog[0].toques_errados == 0);
    espera(&g, 0.5);
    frame(&g, 0, PALHETA);                              /* palhetada sem nota: erro */
    CONFERE(g.jog[0].toques_errados == 1 && g.jog[0].combo == 0);
    Game_quit(&g);
}

static void teste_guitarra_folga_e_ancora(void)
{
    const Nota notas[] = {
        { .t = 1.0, .mascara = 2 },     /* vermelho */
        { .t = 2.0, .mascara = 4 },     /* amarelo */
        { .t = 3.0, .mascara = 4 },     /* amarelo, com o verde ancorado */
        { .t = 4.0, .mascara = 2 },     /* vermelho, mas com o amarelo (acima) segurado */
    };
    Game g;
    partida(&g, notas, 4);
    g.guitarra[0] = true;

    ate(&g, 0.98, 0);
    frame(&g, 0, PALHETA);                              /* palhetou um pouco antes do traste... */
    frame(&g, VERMELHO, 0);                             /* ...8 ms depois: ainda vale */
    CONFERE(g.jog[0].acertos == 1 && g.jog[0].toques_errados == 0);
    ate(&g, 2.0, 0);
    frame(&g, VERDE, PALHETA);                          /* traste errado, e não corrigiu a tempo */
    espera(&g, 0.1);
    CONFERE(g.jog[0].toques_errados == 1);
    ate(&g, 3.0, 0);
    frame(&g, VERDE | AMARELO, PALHETA);                /* nota simples: os trastes de baixo podem ficar */
    CONFERE(g.jog[0].acertos == 2);
    ate(&g, 4.0, 0);
    frame(&g, VERMELHO | AMARELO, PALHETA);             /* um acima segurado: não é o vermelho */
    CONFERE(g.jog[0].acertos == 2);
    Game_quit(&g);
}

/* na preparação, sem ninguém no teclado, as teclas de menu valem como as do
   líder (no PC, quem joga no controle usa o Enter) */
static void teste_teclado_do_jogador_1(void)
{
    Game g;
    Game_init(&g);
    Game_open_chart(&g, CHART_DOIS, sizeof(CHART_DOIS) - 1);
    frame_teclado(&g, 0, TECLA(ACAO_CIMA));
    frame_teclado(&g, 0, TECLA(ACAO_CIMA));
    frame_teclado(&g, 0, TECLA(ACAO_DIREITA));           /* as setas chegam ao J1: guitarra -> baixo */
    CONFERE(g.jog[0].instrumento == INSTR_BAIXO);
    frame_teclado(&g, 0, TECLA(ACAO_BAIXO));
    frame_teclado(&g, 0, TECLA(ACAO_BAIXO));
    frame_teclado(&g, 0, TECLA(ACAO_CONFIRMA));
    CONFERE(g.tela == TELA_CARREGANDO);
    Game_quit(&g);

    /* o J1 no teclado: é ele que o teclado atende */
    Game_init(&g);
    g.jog[0].na_sessao = g.jog[0].no_teclado = true;
    Game_open_chart(&g, CHART_DOIS, sizeof(CHART_DOIS) - 1);
    CONFERE(Game_vez_teclado(&g) == 0);
    frame_teclado(&g, 0, TECLA(ACAO_VOLTA));             /* Esc na vez do J1: volta para a lista */
    CONFERE(g.tela == TELA_MUSICAS);
    Game_quit(&g);
}

static void teste_traducao(void)
{
    static const char EN[] =
        "# comentario\n"
        "idioma = English\n"
        "Jogar = Play\n"
        "Configuracoes = Settings\r\n"                  /* fim de linha do Windows */
        "%d musica(s) = %d song(s)\n"
        "%ld pontos = %s points\n"                       /* tipo errado: ignorada */
        "Voltar = Atr\xc3\xa1s\n"
        "linha sem igual\n";
    char nome[32];
    int ignoradas = 0;
    CONFERE(Texto_nome(EN, sizeof(EN) - 1, nome, sizeof(nome)) && strcmp(nome, "English") == 0);
    CONFERE(Texto_carrega(EN, sizeof(EN) - 1, nome, sizeof(nome), &ignoradas) == 4);
    CONFERE(ignoradas == 1);
    CONFERE(strcmp(T("Jogar"), "Play") == 0 && strcmp(T("Configuracoes"), "Settings") == 0);
    CONFERE(strcmp(T("%d musica(s)"), "%d song(s)") == 0);
    CONFERE(strcmp(T("%ld pontos"), "%ld pontos") == 0);     /* a ignorada fica em português */
    CONFERE(strcmp(T("Voltar"), "Atr\xc3\xa1s") == 0);                         /* "Atrás", com o acento */
    CONFERE(strcmp(T("Sair"), "Sair") == 0);                 /* sem tradução: o original */
    Texto_limpa();
    CONFERE(strcmp(T("Jogar"), "Jogar") == 0);

    char s[32];
    Texto_copia(s, sizeof(s), "Caf\xc3\xa9 \xc2\xbfS\xc3\xad? \xe6\x97\xa5");   /* UTF-8 passa como está */
    CONFERE(strcmp(s, "Caf\xc3\xa9 \xc2\xbfS\xc3\xad? \xe6\x97\xa5") == 0);
    Texto_copia(s, sizeof(s), "Caf\xe9");                                           /* Latin-1: vira UTF-8 */
    CONFERE(strcmp(s, "Caf\xc3\xa9") == 0);
    Texto_copia(s, 5, "Caf\xc3\xa9!");                                             /* não parte o "é" */
    CONFERE(strcmp(s, "Caf") == 0);

    Opcoes o = { .velocidade = 5 };
    snprintf(o.idioma, sizeof(o.idioma), "%s", "en");
    char texto[512];
    Opcoes_write(&o, texto, sizeof(texto));
    Opcoes p = { .velocidade = 5 };
    Opcoes_read(&p, texto, strlen(texto));
    CONFERE(strcmp(p.idioma, "en") == 0);
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);   /* sem buffer: se um teste travar, o que veio antes aparece */
    teste_navegacao_da_volta();
    teste_segurar_nao_corre_o_menu();
    teste_jogar_e_pausar();
    teste_aviso_some();
    teste_opcoes_e_voltar();
    teste_modo_de_escolha();
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

    teste_multijogador();
    teste_preparar_sem_os_outros();
    teste_chart_so_de_baixo();
    teste_partes_do_chart();
    teste_teclado_dividido();
    teste_controle_desconectado();
    teste_tela_de_jogadores();
    teste_quem_some_sai();
    teste_cartao_modo_e_controles();
    teste_nome_na_guitarra();
    teste_modo_guitarra();
    teste_guitarra_folga_e_ancora();
    teste_teclado_do_jogador_1();
    teste_traducao();

    if (falhas) {
        printf("%d verificacao(oes) falharam\n", falhas);
        return 1;
    }
    puts("test_game: todos os testes passaram");
    return 0;
}
