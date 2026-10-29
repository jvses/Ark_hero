/* Ark Hero: o ponto de encontro dos módulos.
 *
 * Só este arquivo conhece o App inteiro. Cada módulo recebe só o pedaço que é
 * dele, e é aqui que a plataforma (video, input, audio, arquivos) e o jogo
 * (game, draw) se ligam.
 *
 * Um frame (SDL_AppIterate), sempre nesta ordem:
 *   1. tempo   2. entrada   3. som e relógio   4. lógica   5. pedidos   6. desenho */

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "draw/draw.h"
#include "game/game.h"
#include "game/songini.h"
#include "platform/audio.h"
#include "platform/carregador.h"
#include "platform/input.h"
#include "platform/musica.h"
#include "platform/video.h"

typedef enum { SOM_NADA, SOM_PARTIDA, SOM_TESTE } OQueToca;

/* um recorde salvo: a chave identifica a música mesmo se ela mudar de pasta */
typedef struct Recorde {
    char chave[200];
    long pontos;
    int  dif, pct;
    bool fc;
} Recorde;

typedef struct {
    Video  video;
    Input  input;
    Audio  audio;
    Draw   draw;
    Game   game;

    /* as músicas encontradas, na mesma ordem de game.musicas */
    Musica   *musicas;
    int       n_musicas;
    Varredura varredura;          /* procura as pastas numa thread */

    /* arquivos de dados, no PrefPath (no console: ports/arkhero/conf/arkhero/) */
    char      arquivo_opcoes[512];
    char      arquivo_recordes[512];
    struct Recorde *recordes;     /* o recordes.txt inteiro na memória */
    int       n_recordes, cap_recordes;

    /* o que o log registra de cada partida */
    bool   logando;
    int    partida_logada;
    int    frames_lentos;         /* frames com mais de 25 ms durante a partida */

    /* som */
    OQueToca   tocando;
    int        partida_tocando;   /* g->partida que está tocando (para saber quando recomeçar) */
    Pcm        cliques;           /* metrônomo, para a demonstração (ou se o áudio falhar) */
    Carregador carga;             /* decodifica uma música numa thread */
    int        carga_indice;      /* qual música está sendo decodificada */
    Pcm        pcm;               /* a última música decodificada */
    int        pcm_indice;        /* qual é (-1 = nenhuma) */

    /* textos da tela de teste de áudio (ela usa a primeira música) */
    char info_musica[160], info_faixas[256], info_carga[160];

    Uint64 ultimo_ns;
    bool   sair;
} App;

/* ---------------------------------------------------------------------------
 * Opções e recordes: arquivos de texto no PrefPath. O game converte as
 * opções (Opcoes_read/write); os recordes são uma linha por música:
 *     chave|pontos|dificuldade|porcentagem|full combo
 * ------------------------------------------------------------------------- */
static void carrega_opcoes(App *app)
{
    size_t tam = 0;
    char *texto = SDL_LoadFile(app->arquivo_opcoes, &tam);
    if (!texto) return;   /* primeira vez: ficam os padrões */
    Opcoes_read(&app->game.opcoes, texto, tam);
    SDL_free(texto);
    Game_songs_done(&app->game);   /* o modo de organizar pode ter mudado */
    SDL_Log("[arkhero] opcoes carregadas de %s", app->arquivo_opcoes);
}

static void salva_opcoes(App *app)
{
    char texto[512];
    const int n = Opcoes_write(&app->game.opcoes, texto, sizeof(texto));
    if (n > 0 && !SDL_SaveFile(app->arquivo_opcoes, texto, (size_t)n))
        SDL_Log("[arkhero] nao salvei as opcoes: %s", SDL_GetError());
}

static Recorde *acha_recorde(App *app, const char *chave)
{
    for (int i = 0; i < app->n_recordes; i++)
        if (SDL_strcmp(app->recordes[i].chave, chave) == 0) return &app->recordes[i];
    return NULL;
}

static Recorde *novo_recorde(App *app, const char *chave)
{
    if (app->n_recordes >= app->cap_recordes) {
        const int cap = app->cap_recordes ? app->cap_recordes * 2 : 64;
        Recorde *novo = SDL_realloc(app->recordes, sizeof(Recorde) * (size_t)cap);
        if (!novo) return NULL;
        app->recordes     = novo;
        app->cap_recordes = cap;
    }
    Recorde *r = &app->recordes[app->n_recordes++];
    SDL_zerop(r);
    SDL_strlcpy(r->chave, chave, sizeof(r->chave));
    return r;
}

static void carrega_recordes(App *app)
{
    size_t tam = 0;
    char *texto = SDL_LoadFile(app->arquivo_recordes, &tam);
    if (!texto) return;
    char *fim = NULL;
    for (char *linha = SDL_strtok_r(texto, "\r\n", &fim); linha; linha = SDL_strtok_r(NULL, "\r\n", &fim)) {
        if (linha[0] == '#') continue;
        char *campo = NULL;
        const char *chave = SDL_strtok_r(linha, "|", &campo);
        const char *pontos = SDL_strtok_r(NULL, "|", &campo);
        const char *dif = SDL_strtok_r(NULL, "|", &campo);
        const char *pct = SDL_strtok_r(NULL, "|", &campo);
        const char *fc = SDL_strtok_r(NULL, "|", &campo);
        if (!chave || !pontos || !dif || !pct || !fc) continue;
        Recorde *r = novo_recorde(app, chave);
        if (!r) break;
        r->pontos = SDL_strtol(pontos, NULL, 10);
        r->dif    = SDL_atoi(dif);
        r->pct    = SDL_atoi(pct);
        r->fc     = SDL_atoi(fc) != 0;
    }
    SDL_free(texto);
    SDL_Log("[arkhero] %d recorde(s) em %s", app->n_recordes, app->arquivo_recordes);
}

static void salva_recordes(App *app)
{
    size_t cap = 128 + (size_t)app->n_recordes * 240, n = 0;
    char *texto = SDL_malloc(cap);
    if (!texto) return;
    n += (size_t)SDL_snprintf(texto, cap, "# Ark Hero - recordes: chave|pontos|dificuldade|porcentagem|full combo\n");
    for (int i = 0; i < app->n_recordes && n < cap; i++) {
        const Recorde *r = &app->recordes[i];
        n += (size_t)SDL_snprintf(texto + n, cap - n, "%s|%ld|%d|%d|%d\n", r->chave, r->pontos, r->dif, r->pct, r->fc);
    }
    if (n > cap) n = cap;
    if (!SDL_SaveFile(app->arquivo_recordes, texto, n)) SDL_Log("[arkhero] nao salvei os recordes: %s", SDL_GetError());
    SDL_free(texto);
}

/* chave: título|artista|charter em minúsculas. Sobrevive a mudar a pasta de lugar */
static void monta_chave(char *chave, size_t n, const SongIni *ini)
{
    SDL_snprintf(chave, n, "%s/%s/%s", ini->nome, ini->artista, ini->charter);
    for (char *c = chave; *c; c++) {
        if (*c == '|') *c = '/';
        else if (*c >= 'A' && *c <= 'Z') *c = (char)(*c - 'A' + 'a');
    }
}

/* ---------------------------------------------------------------------------
 * Músicas. A thread da varredura acha as pastas e lê os song.ini; aqui o
 * texto é interpretado (SongIni_parse, do game), a lista é ordenada pelo
 * título e entregue ao game.
 * ------------------------------------------------------------------------- */
typedef struct {
    int     i;
    SongIni ini;
} Ordem;

static int compara_titulo(const void *x, const void *y)
{
    const Ordem *a = x, *b = y;
    const int c = SDL_strcasecmp(a->ini.nome, b->ini.nome);
    return c ? c : SDL_strcasecmp(a->ini.artista, b->ini.artista);
}

static void textos_do_teste(App *app)
{
    app->info_musica[0] = app->info_faixas[0] = '\0';
    if (app->n_musicas == 0) return;
    const InfoMusica *info = &app->game.musicas[0];
    if (info->artista[0]) SDL_snprintf(app->info_musica, sizeof(app->info_musica), "%s - %s", info->titulo, info->artista);
    else                  SDL_strlcpy(app->info_musica, info->titulo, sizeof(app->info_musica));
    for (int i = 0; i < app->musicas[0].n_faixas; i++) {
        SDL_strlcat(app->info_faixas, app->musicas[0].faixas[i], sizeof(app->info_faixas));
        SDL_strlcat(app->info_faixas, "  ", sizeof(app->info_faixas));
    }
}

static int acha_pasta(const Musica *lista, int n, const char *pasta)
{
    for (int i = 0; i < n; i++)
        if (SDL_strcmp(lista[i].pasta, pasta) == 0) return i;
    return -1;
}

/* a varredura terminou: troca a lista. Só quando é seguro (fora da partida,
 * da escolha de dificuldade e de uma carga), porque os índices mudam. */
static void aplica_varredura(App *app)
{
    Game *g = &app->game;
    if (Varredura_state(&app->varredura) != VARRE_PRONTO) return;
    if (g->tela == TELA_DIFICULDADE || g->tela == TELA_CARREGANDO || g->tela == TELA_JOGO) return;
    if (Carregador_state(&app->carga) == CARGA_ANDANDO) return;

    const Varredura *v = &app->varredura;
    const int pastas = v->pastas, sem_audio = v->sem_audio;
    const double seg = v->segundos;
    Musica *nova = NULL;
    int n = 0;
    Varredura_take(&app->varredura, &nova, &n);

    /* interpreta os song.ini e ordena pelo título */
    Ordem *ordem = SDL_malloc(sizeof(Ordem) * (size_t)(n > 0 ? n : 1));
    Musica *ordenada = SDL_malloc(sizeof(Musica) * (size_t)(n > 0 ? n : 1));
    if (!ordem || !ordenada) {
        SDL_free(ordem); SDL_free(ordenada);
        Musica_free_list(nova, n);
        SDL_Log("[musica] sem memoria para a lista de musicas");
        return;
    }
    for (int i = 0; i < n; i++) {
        ordem[i].i = i;
        if (!nova[i].ini_texto || !SongIni_parse(nova[i].ini_texto, nova[i].ini_tam, &ordem[i].ini))
            ordem[i].ini = (SongIni){0};
        if (!ordem[i].ini.nome[0]) SDL_strlcpy(ordem[i].ini.nome, nova[i].nome, sizeof(ordem[i].ini.nome));
        SDL_free(nova[i].ini_texto);   /* já foi interpretado: libera a memória */
        nova[i].ini_texto = NULL;
    }
    SDL_qsort(ordem, (size_t)n, sizeof(Ordem), compara_titulo);
    for (int i = 0; i < n; i++) ordenada[i] = nova[ordem[i].i];
    SDL_free(nova);

    /* a música decodificada e a selecionada continuam valendo, nas posições novas */
    char pcm_pasta[512] = "", sel_pasta[512] = "";
    if (app->pcm_indice >= 0 && app->pcm_indice < app->n_musicas)
        SDL_strlcpy(pcm_pasta, app->musicas[app->pcm_indice].pasta, sizeof(pcm_pasta));
    const int sel = g->sel[TELA_MUSICAS] - 1;   /* o item 0 é a demonstração */
    if (sel >= 0 && sel < app->n_musicas) SDL_strlcpy(sel_pasta, app->musicas[sel].pasta, sizeof(sel_pasta));

    Musica_free_list(app->musicas, app->n_musicas);
    app->musicas    = ordenada;
    app->n_musicas  = n;
    app->pcm_indice = pcm_pasta[0] ? acha_pasta(ordenada, n, pcm_pasta) : -1;
    if (app->pcm_indice < 0) Pcm_free(&app->pcm);

    Game_clear_songs(g);
    for (int i = 0; i < n; i++) {
        Game_add_song(g, &ordem[i].ini, ordenada[i].nome, ordenada[i].grupo);
        monta_chave(ordenada[i].chave, sizeof(ordenada[i].chave), &ordem[i].ini);
        const Recorde *r = acha_recorde(app, ordenada[i].chave);
        if (r) Game_set_record(g, i, r->pontos, r->dif, r->pct, r->fc);
    }
    Game_songs_done(g);
    g->musica = -1;
    if (sel_pasta[0]) {
        const int novo = acha_pasta(ordenada, n, sel_pasta);
        if (novo >= 0) g->sel[TELA_MUSICAS] = novo + 1;
    }
    SDL_free(ordem);
    textos_do_teste(app);

    int mid = 0;
    for (int i = 0; i < n; i++) mid += SDL_strcasecmp(ordenada[i].chart, "notes.mid") == 0;
    SDL_Log("[musica] %d musica(s) em %.2f s (%d com .chart, %d com .mid; %d pastas olhadas; "
            "%d com chart mas sem audio suportado)", n, seg, n - mid, mid, pastas, sem_audio);
}

/* o jogo escolheu uma música: ler o notes.chart e entregar o texto a ele */
static void abre_musica(App *app, int i)
{
    if (i < 0 || i >= app->n_musicas) return;
    char caminho[600];
    Musica_path(&app->musicas[i], app->musicas[i].chart, caminho, sizeof(caminho));
    size_t tam = 0;
    char *texto = SDL_LoadFile(caminho, &tam);
    if (!texto) {
        SDL_Log("[musica] nao li %s: %s", caminho, SDL_GetError());
        return;
    }
    const Uint64 inicio = SDL_GetTicksNS();
    const bool ok = Game_open_chart(&app->game, texto, tam);
    SDL_free(texto);
    if (!ok) {
        SDL_Log("[musica] %s: chart invalido", app->musicas[i].nome);
        return;
    }
    const ArquivoChart *a = &app->game.arquivo;
    SDL_Log("[musica] %s: chart lido em %.1f ms (resolucao %d, %d mudancas de andamento, offset %.3f s) "
            "notas: facil %d, medio %d, dificil %d, expert %d", app->musicas[i].nome,
            (double)(SDL_GetTicksNS() - inicio) / 1e6, a->resolucao, a->n_tempos, a->offset,
            ChartFile_contar(a, DIF_FACIL), ChartFile_contar(a, DIF_MEDIO),
            ChartFile_contar(a, DIF_DIFICIL), ChartFile_contar(a, DIF_EXPERT));
}

static void comeca_carga(App *app, int i)
{
    if (i < 0 || i >= app->n_musicas) return;
    Carregador_quit(&app->carga);   /* se outra estava carregando, cancela */
    app->carga_indice  = i;
    app->info_carga[0] = '\0';

    const Musica *m = &app->musicas[i];
    char arquivos[CARREGADOR_MAX_FAIXAS][512];
    for (int k = 0; k < m->n_faixas; k++) Musica_path(m, m->faixas[k], arquivos[k], sizeof(arquivos[k]));
    SDL_Log("[carga] %s: %d faixa(s)%s", m->nome, m->n_faixas, m->ignoradas ? " (alguma faixa em formato nao suportado)" : "");
    Carregador_start(&app->carga, (const char (*)[512])arquivos, m->n_faixas);
}

/* ---------------------------------------------------------------------------
 * Som
 * ------------------------------------------------------------------------- */
static void para_som(App *app)
{
    Audio_stop(&app->audio);
    app->tocando = SOM_NADA;
}

/* metrônomo nas batidas do chart */
static void gera_cliques(App *app)
{
    const Chart *c = &app->game.chart;
    Pcm_free(&app->cliques);
    const int n = c->n_batidas > 0 ? c->n_batidas : 1;
    double *tempo = SDL_malloc(sizeof(double) * (size_t)n);
    bool   *forte = SDL_malloc(sizeof(bool) * (size_t)n);
    if (tempo && forte) {
        for (int i = 0; i < c->n_batidas; i++) {
            tempo[i] = c->batidas[i].t;
            forte[i] = c->batidas[i].forte;
        }
        Audio_make_clicks(&app->cliques, tempo, forte, c->n_batidas, c->fim + 3.0);
    }
    SDL_free(tempo);
    SDL_free(forte);
}

/* começo da partida: a música, se ela foi decodificada; senão, o metrônomo.
 * O silêncio antes (-chart.inicio) é o tempo de preparação: a pista já anda. */
static void comeca_som_partida(App *app)
{
    const Game *g = &app->game;
    Audio_stop(&app->audio);
    const double preparacao = -g->chart.inicio;
    if (g->musica >= 0 && g->musica == app->pcm_indice && g->chart.duracao_audio > 0.0) {
        Audio_play(&app->audio, &app->pcm, preparacao);
    } else {
        gera_cliques(app);
        Audio_play(&app->audio, &app->cliques, preparacao);
    }
    app->tocando         = SOM_PARTIDA;
    app->partida_tocando = g->partida;
}

/* a carga em andamento terminou? */
static void confere_carga(App *app)
{
    const EstadoCarga e = Carregador_state(&app->carga);
    if (e == CARGA_PRONTO) {
        if (app->tocando == SOM_TESTE) para_som(app);   /* o Pcm antigo vai ser liberado */
        Pcm_free(&app->pcm);
        const double seg = app->carga.segundos, audio = app->carga.segundos_audio;
        Carregador_take(&app->carga, &app->pcm);
        app->pcm_indice = app->carga_indice;
        SDL_snprintf(app->info_carga, sizeof(app->info_carga),
                     "Carregada: %.1f s de musica em %.2f s (%.0fx o tempo real)",
                     (double)app->pcm.frames / AUDIO_TAXA, seg, seg > 0 ? audio / seg : 0.0);
        SDL_Log("[carga] pronta: %.1f s de musica, %.1f MB na memoria", (double)app->pcm.frames / AUDIO_TAXA,
                (double)app->pcm.frames * AUDIO_CANAIS * sizeof(Sint16) / (1024.0 * 1024.0));
        Game_audio_ready(&app->game, true, (double)app->pcm.frames / AUDIO_TAXA);
    } else if (e == CARGA_ERRO) {
        SDL_snprintf(app->info_carga, sizeof(app->info_carga), "Erro: %s", app->carga.erro);
        Game_audio_ready(&app->game, false, 0.0);   /* só faz algo se estiver esperando */
    }
}

/* O que deve estar tocando sai do estado do jogo (e não de eventos soltos):
 * na pista, o som da partida; fora dela, nada; na tela de teste, o teste. */
static void sincroniza_som(App *app)
{
    const Game *g = &app->game;
    Audio_set_volume(&app->audio, g->opcoes.volume / 100.0f);
    confere_carga(app);

    const bool na_partida = g->tela == TELA_JOGO && !g->terminou;
    if (na_partida) {
        if (app->tocando != SOM_PARTIDA || app->partida_tocando != g->partida) comeca_som_partida(app);
        Audio_pause(&app->audio, g->pausado);
    } else if (app->tocando == SOM_PARTIDA) {
        para_som(app);
    }
    if (app->tocando == SOM_TESTE && g->tela != TELA_AUDIO) para_som(app);

    Audio_update(&app->audio);
}

/* ---------------------------------------------------------------------------
 * Pedidos do jogo: o game pede, a plataforma faz
 * ------------------------------------------------------------------------- */
static void atende_pedido(App *app)
{
    const int i = app->game.pedido_indice;
    switch (app->game.pedido) {
    case PEDIDO_MAPEAR:              Input_capture_start(&app->input, app->game.pedido_acao); break;
    case PEDIDO_RESTAURAR_CONTROLES: Input_reset_defaults(&app->input);                       break;
    case PEDIDO_TROCAR_ANALOGICO:    Input_toggle_stick(&app->input);                         break;
    case PEDIDO_TROCAR_FRONTAIS:     Input_cycle_layout(&app->input);                         break;
    case PEDIDO_SAIR:                app->sair = true;                                        break;
    case PEDIDO_ABRIR_MUSICA:        abre_musica(app, i);                                     break;

    case PEDIDO_CARREGAR_MUSICA:
        if (app->pcm_indice == i) Game_audio_ready(&app->game, true, (double)app->pcm.frames / AUDIO_TAXA);
        else                      comeca_carga(app, i);   /* já decodificada: não decodifica de novo */
        break;

    case PEDIDO_CANCELAR_CARGA:
        Carregador_quit(&app->carga);
        break;

    case PEDIDO_PROCURAR_MUSICAS:
        Varredura_start(&app->varredura);   /* já procurando: ignora */
        break;

    case PEDIDO_SALVAR_OPCOES:
        salva_opcoes(app);
        break;

    case PEDIDO_SALVAR_RECORDE:
        if (i >= 0 && i < app->n_musicas) {
            const InfoMusica *m = &app->game.musicas[i];
            Recorde *r = acha_recorde(app, app->musicas[i].chave);
            if (!r) r = novo_recorde(app, app->musicas[i].chave);
            if (r) {
                r->pontos = m->recorde;
                r->dif    = m->recorde_dif;
                r->pct    = m->recorde_pct;
                r->fc     = m->recorde_fc;
                salva_recordes(app);
            }
            SDL_Log("[partida] NOVO RECORDE em %s: %ld pontos (antes %ld)", m->titulo, m->recorde,
                    app->game.recorde_anterior);
        }
        break;

    case PEDIDO_AUDIO_CARREGAR:   /* tela de teste: a primeira música */
        if (app->n_musicas > 0 && Carregador_state(&app->carga) != CARGA_ANDANDO) comeca_carga(app, 0);
        break;

    case PEDIDO_AUDIO_TOCAR:
        if (app->pcm_indice != 0) break;   /* ainda não carregou */
        if (app->tocando == SOM_TESTE) {
            Audio_pause(&app->audio, !app->audio.pausado);
        } else {
            Audio_play(&app->audio, &app->pcm, 0.0);
            app->tocando = SOM_TESTE;
        }
        break;

    case PEDIDO_NENHUM:
        break;
    }
}

/* ---------------------------------------------------------------------------
 * Log de cada partida: o que estava valendo no começo e um resumo no fim.
 * O erro médio de tempo é a pista para a calibração: negativo = o jogador
 * (ou o som/tela) está adiantado; positivo = atrasado.
 * ------------------------------------------------------------------------- */
static void log_inicio(App *app)
{
    const Game *g = &app->game;
    const bool com_musica = g->musica >= 0 && g->musica == app->pcm_indice && g->chart.duracao_audio > 0.0;
    SDL_Log("[partida] inicio: %s | %d notas | janela +-%.0f ms%s | velocidade %d | som: %s | preparacao %.1f s",
            g->chart.titulo, g->chart.n, Game_janela_acerto(g) * 1000.0, g->opcoes.precisao ? " (precisao)" : "",
            g->opcoes.velocidade, !Audio_ok(&app->audio) ? "sem dispositivo" : (com_musica ? "musica" : "metronomo"),
            g->chart.inicio < 0.0 ? -g->chart.inicio : 0.0);
    SDL_Log("[partida] penalidade %s | ajuste de tempo %+d ms", g->opcoes.penalidade ? "sim" : "nao",
            g->opcoes.ajuste_ms);
    app->logando        = true;
    app->partida_logada = g->partida;
    app->frames_lentos  = 0;
}

static void log_fim(App *app, const char *como)
{
    const Game *g = &app->game;
    const int julgadas = g->acertos + g->erros;
    const double media  = g->acertos ? g->soma_erro / g->acertos : 0.0;
    const double var    = g->acertos ? g->soma_erro2 / g->acertos - media * media : 0.0;
    SDL_Log("[partida] %s em %.1f s: %ld pontos, %d/%d notas (%d%%), maior combo %d, star power %.0f%%",
            como, g->t, (long)g->pontos, g->acertos, julgadas,
            julgadas ? (100 * g->acertos + julgadas / 2) / julgadas : 0, g->maior_combo, 100.0f * g->sp_medidor);
    SDL_Log("[partida] tempo dos acertos: erro medio %+.1f ms, desvio %.1f ms | toques sem nota %d | pausas %d | "
            "frames lentos %d | saltos do relogio %d", media * 1000.0, var > 0 ? SDL_sqrt(var) * 1000.0 : 0.0,
            g->toques_errados, g->pausas, app->frames_lentos, app->audio.saltos);
    if (g->acertos >= 20 && SDL_fabs(media) >= 0.010)
        SDL_Log("[partida] sugestao: Ajuste de tempo %+d ms", g->opcoes.ajuste_ms - (int)SDL_lround(media * 1000.0));
    app->logando = false;
}

static void acompanha_partida(App *app, double dt)
{
    const Game *g = &app->game;
    const bool jogando = g->tela == TELA_JOGO && !g->terminou;
    if (jogando && (!app->logando || app->partida_logada != g->partida)) {
        if (app->logando) log_fim(app, "reiniciada");
        log_inicio(app);
    } else if (!jogando && app->logando) {
        log_fim(app, g->terminou ? "terminou" : "abandonada");
    }
    if (jogando && !g->pausado && dt > 0.025) app->frames_lentos++;
}

/* ---------------------------------------------------------------------------
 * O que a tela mostra e não é do jogo
 * ------------------------------------------------------------------------- */
static void monta_hud(App *app, Hud *h, Uint64 agora_ns)
{
    h->fps           = app->video.fps;
    h->capturando    = app->input.capturando;
    h->captura_resta = Input_capture_left(&app->input);
    h->ultimo        = app->input.ultimo;
    h->analogico     = Input_stick_label(&app->input);
    h->frontais      = Input_layout_label(&app->input);
    for (int a = 0; a < ACAO_N; a++) {
        h->pad[a]   = Input_pad_label(&app->input, (Acao)a);
        h->tecla[a] = Input_key_label(&app->input, (Acao)a);
    }

    h->procurando = Varredura_state(&app->varredura) != VARRE_PARADO;
    h->achadas    = Varredura_found(&app->varredura);

    HudAudio *ha      = &h->audio;
    ha->ok            = Audio_ok(&app->audio);
    ha->driver        = app->audio.driver;
    ha->buffer_frames = app->audio.frames_buffer;
    ha->musica        = app->n_musicas > 0 ? app->info_musica : NULL;
    ha->pasta         = app->varredura.raizes[0];
    ha->faixas        = app->info_faixas;
    ha->ignoradas     = app->n_musicas > 0 ? app->musicas[0].ignoradas : 0;
    ha->carga         = (int)Carregador_state(&app->carga);
    ha->progresso     = Carregador_progress(&app->carga);
    ha->carga_info    = app->info_carga[0] ? app->info_carga : NULL;
    ha->tocando       = app->tocando == SOM_TESTE;
    ha->pausado       = app->audio.pausado;
    ha->duracao       = (double)app->pcm.frames / AUDIO_TAXA;
    if (ha->tocando) {
        ha->t_cru   = Audio_raw_time(&app->audio);
        ha->t_suave = Audio_song_time(&app->audio, agora_ns);
    }
}

/* ---------------------------------------------------------------------------
 * Callbacks da SDL
 * ------------------------------------------------------------------------- */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    SDL_SetAppMetadata("Ark Hero", "0.1", "com.arkhero.game");

    App *app = SDL_calloc(1, sizeof(App));   /* tudo zerado: o Quit aguenta init parcial */
    if (!app) return SDL_APP_FAILURE;
    *appstate = app;
    app->pcm_indice = -1;

    if (!Video_init(&app->video)) return SDL_APP_FAILURE;
    if (!Input_init(&app->input)) return SDL_APP_FAILURE;
    Audio_init(&app->audio);   /* sem som o jogo roda do mesmo jeito (relógio pelo dt) */

    /* imagens: sempre relativas ao executável. No PC o CMake põe um link
       assets/ ao lado do binário; no console o pacote tem a pasta de verdade. */
    char pasta[512];
    SDL_snprintf(pasta, sizeof(pasta), "%sassets/images/", SDL_GetBasePath());
    if (!Draw_init(&app->draw, app->video.ren, pasta)) return SDL_APP_FAILURE;

    Game_init(&app->game);

    char *pref = SDL_GetPrefPath("", "arkhero");
    SDL_Log("[arkhero] versao 0.1 | executavel em %s | dados em %s", SDL_GetBasePath(), pref ? pref : "?");
    if (pref) {
        SDL_snprintf(app->arquivo_opcoes, sizeof(app->arquivo_opcoes), "%sopcoes.cfg", pref);
        SDL_snprintf(app->arquivo_recordes, sizeof(app->arquivo_recordes), "%srecordes.txt", pref);
        carrega_opcoes(app);
        carrega_recordes(app);
    }
    SDL_free(pref);
    Varredura_start(&app->varredura);   /* a lista aparece quando terminar */

    app->ultimo_ns = SDL_GetTicksNS();
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    App *app = appstate;
    if (event->type == SDL_EVENT_QUIT) return SDL_APP_SUCCESS;

    Input_event(&app->input, event);   /* só anota; quem age é o Iterate */
    return app->input.sair ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    App *app = appstate;

    /* 1. TEMPO do sistema (para animações e para suavizar o relógio do áudio) */
    const Uint64 agora_ns = SDL_GetTicksNS();
    double dt = (double)(agora_ns - app->ultimo_ns) / 1e9;
    app->ultimo_ns = agora_ns;
    if (dt > 0.05) dt = 0.05;   /* uma travada não faz a pista pular */

    /* 2. ENTRADA */
    const Botoes botoes = Input_frame(&app->input);

    /* 3. SOM E RELÓGIO: na partida, o tempo da música é o do áudio */
    sincroniza_som(app);
    app->game.relogio_audio = app->tocando == SOM_PARTIDA && Audio_ok(&app->audio);
    if (app->game.relogio_audio) app->game.t_audio = Audio_song_time(&app->audio, agora_ns);

    /* 4. LÓGICA: o jogo julga e produz eventos; a pontuação reage a eles */
    Game_update(&app->game, &botoes, dt);
    Score_apply_events(&app->game);

    /* 5. PEDIDOS do jogo para a plataforma, e o que chegou das threads */
    atende_pedido(app);
    aplica_varredura(app);
    acompanha_partida(app, dt);

    /* 6. DESENHO */
    Hud hud = {0};
    monta_hud(app, &hud, agora_ns);
    Video_begin_frame(&app->video);
    Draw_frame(&app->draw, app->video.ren, &app->game, &hud);
    Video_end_frame(&app->video);   /* o vsync espera aqui */

    return (app->sair || app->input.sair) ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    (void)result;
    App *app = appstate;
    if (!app) return;

    /* ordem inversa do Init: a thread de carga para antes de tudo, o som para
       antes de liberar o que ele toca, as texturas morrem antes do renderer */
    if (app->logando) log_fim(app, "abandonada (jogo fechado)");
    Varredura_quit(&app->varredura);
    Carregador_quit(&app->carga);
    Musica_free_list(app->musicas, app->n_musicas);
    SDL_free(app->recordes);
    Game_quit(&app->game);
    Draw_quit(&app->draw);
    Audio_quit(&app->audio);
    Pcm_free(&app->cliques);
    Pcm_free(&app->pcm);
    Input_quit(&app->input);
    Video_quit(&app->video);
    SDL_free(app);
    /* sem SDL_Quit(): a SDL chama sozinha depois daqui */
}
