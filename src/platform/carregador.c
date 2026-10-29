#include "platform/carregador.h"

#include <opusfile.h>
#include <vorbis/vorbisfile.h>

/* MP3: o dr_mp3 é um arquivo só; a implementação entra neste .c.
   Sem as funções de stdio próprias dele: a leitura é pela SDL (mesmo caminho
   no PC e no console) e a memória também. */
#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO
#define DRMP3_MALLOC(n)     SDL_malloc(n)
#define DRMP3_REALLOC(p, n) SDL_realloc(p, n)
#define DRMP3_FREE(p)       SDL_free(p)
#include "dr_mp3.h"

#define LEITURA    4096                  /* frames lidos por vez de cada arquivo */
#define SAIDA_MAX  (LEITURA * 4)         /* frames convertidos por vez (folga para reamostrar) */
#define MARGEM     (AUDIO_TAXA / 10)     /* 100 ms a mais no fim, por segurança */

/* ---------------------------------------------------------------------------
 * Uma faixa aberta: entrega áudio no formato dela, e um SDL_AudioStream
 * converte para o nosso (s16, estéreo, 48 kHz). Vorbis costuma vir a
 * 44,1 kHz; o Opus é sempre 48 kHz.
 * ------------------------------------------------------------------------- */
typedef enum { FONTE_OPUS, FONTE_VORBIS, FONTE_WAV, FONTE_MP3 } TipoFonte;

typedef struct {
    TipoFonte      tipo;
    OggOpusFile   *opus;
    OggVorbis_File vorbis;
    Uint8         *wav;
    Uint32         wav_bytes, wav_pos;
    drmp3          mp3;
    void          *mp3_dados;   /* o arquivo inteiro na memória (o dr_mp3 lê dele) */
    bool           mp3_aberto;
    SDL_AudioSpec  spec;     /* formato que a faixa entrega */
    Sint64         frames;   /* duração, em frames da própria faixa */
} Fonte;

static bool termina_com(const char *s, const char *fim)
{
    const size_t a = SDL_strlen(s), b = SDL_strlen(fim);
    return a >= b && SDL_strcasecmp(s + a - b, fim) == 0;
}

static bool abre(Fonte *f, const char *caminho)
{
    *f = (Fonte){0};

    if (termina_com(caminho, ".mp3")) {
        size_t tam = 0;
        f->mp3_dados = SDL_LoadFile(caminho, &tam);
        if (!f->mp3_dados) return false;
        if (!drmp3_init_memory(&f->mp3, f->mp3_dados, tam, NULL)) {
            SDL_free(f->mp3_dados);
            f->mp3_dados = NULL;
            SDL_SetError("MP3 invalido");
            return false;
        }
        f->mp3_aberto = true;
        f->tipo   = FONTE_MP3;
        f->spec   = (SDL_AudioSpec){ SDL_AUDIO_F32, (int)f->mp3.channels, (int)f->mp3.sampleRate };
        f->frames = (Sint64)drmp3_get_pcm_frame_count(&f->mp3);   /* percorre o arquivo: na carga, tudo bem */
        drmp3_seek_to_pcm_frame(&f->mp3, 0);
        return true;
    }

    if (termina_com(caminho, ".wav")) {
        if (!SDL_LoadWAV(caminho, &f->spec, &f->wav, &f->wav_bytes)) return false;
        f->tipo   = FONTE_WAV;
        f->frames = f->wav_bytes / SDL_AUDIO_FRAMESIZE(f->spec);
        return true;
    }

    /* ov_fopen fecha o arquivo sozinho se falhar: não precisa de ov_clear */
    if (termina_com(caminho, ".ogg") && ov_fopen(caminho, &f->vorbis) == 0) {
        const vorbis_info *vi = ov_info(&f->vorbis, -1);
        f->tipo   = FONTE_VORBIS;
        f->spec   = (SDL_AudioSpec){ SDL_AUDIO_F32, 2, (int)vi->rate };
        f->frames = ov_pcm_total(&f->vorbis, -1);
        return true;
    }

    /* .opus, ou um .ogg que não é Vorbis: muitos são Opus dentro de Ogg */
    int erro = 0;
    f->opus = op_open_file(caminho, &erro);
    if (f->opus) {
        f->tipo   = FONTE_OPUS;
        f->spec   = (SDL_AudioSpec){ SDL_AUDIO_F32, 2, 48000 };   /* opusfile sempre entrega 48 kHz */
        f->frames = op_pcm_total(f->opus, -1);
        return true;
    }
    SDL_SetError("formato nao suportado (ou arquivo corrompido)");
    return false;
}

static void fecha(Fonte *f)
{
    if (f->opus) op_free(f->opus);
    if (f->tipo == FONTE_VORBIS) ov_clear(&f->vorbis);
    if (f->mp3_aberto) drmp3_uninit(&f->mp3);
    SDL_free(f->mp3_dados);
    SDL_free(f->wav);
    *f = (Fonte){0};
}

/* Lê o próximo pedaço no formato da faixa. Devolve bytes; 0 = fim; -1 = erro. */
static int le(Fonte *f, float *buf)
{
    for (;;) {
        switch (f->tipo) {
        case FONTE_OPUS: {
            const int r = op_read_float_stereo(f->opus, buf, LEITURA * 2);
            if (r == OP_HOLE) continue;             /* falha no meio do arquivo: segue */
            return r < 0 ? -1 : r * 2 * (int)sizeof(float);
        }
        case FONTE_VORBIS: {
            float **canais;
            int secao;
            const long r = ov_read_float(&f->vorbis, &canais, LEITURA, &secao);
            if (r == OV_HOLE) continue;
            if (r <= 0) return r < 0 ? -1 : 0;
            const int n = ov_info(&f->vorbis, secao)->channels;
            for (long i = 0; i < r; i++) {          /* planar -> intercalado; mono vira estéreo */
                buf[2 * i]     = canais[0][i];
                buf[2 * i + 1] = canais[n > 1 ? 1 : 0][i];
            }
            return (int)r * 2 * (int)sizeof(float);
        }
        case FONTE_MP3: {
            /* entrega no número de canais do arquivo (mono ou estéreo): o
               SDL_AudioStream converte para estéreo junto com a taxa */
            const int canais = (int)f->mp3.channels > 0 ? (int)f->mp3.channels : 1;
            const drmp3_uint64 r = drmp3_read_pcm_frames_f32(&f->mp3, (drmp3_uint64)(LEITURA * 2 / canais), buf);
            return (int)r * canais * (int)sizeof(float);
        }
        case FONTE_WAV: {
            Uint32 n = f->wav_bytes - f->wav_pos;
            const Uint32 max = LEITURA * 2 * sizeof(float);
            if (n > max) n = max;
            SDL_memcpy(buf, f->wav + f->wav_pos, n);
            f->wav_pos += n;
            return (int)n;
        }
        }
        return -1;
    }
}

/* ---------------------------------------------------------------------------
 * A carga, em paralelo: cada thread pega a próxima faixa livre, decodifica
 * inteira e soma no mix. O RK3326 tem 4 núcleos; uma música do Rock Band tem
 * 7 faixas. A soma é em 32 bits (a ordem entre as threads varia, e assim o
 * resultado é sempre o mesmo, sem cortar no meio); no fim, um corte só para
 * 16 bits.
 * ------------------------------------------------------------------------- */
typedef struct {
    Carregador   *c;
    Fonte        *fontes;
    int           n;
    SDL_AtomicInt proxima;       /* próxima faixa sem dono */
    SDL_AtomicInt feitos;        /* frames de saída prontos, somando todas as faixas */
    Sint64        soma;          /* quantos frames ao todo (para o progresso) */
    Sint32       *mix;
    Sint64        total;
    SDL_Mutex    *trava;         /* protege o mix e o fim */
    Sint64        fim;           /* a faixa mais longa decodificada */
    SDL_AtomicInt sem_memoria;
} Trabalho;

static void decodifica(Trabalho *t, Fonte *f, float *ler, Sint16 *conv)
{
    const SDL_AudioSpec destino = { SDL_AUDIO_S16, AUDIO_CANAIS, AUDIO_TAXA };
    SDL_AudioStream *s = SDL_CreateAudioStream(&f->spec, &destino);
    Sint64 pos = 0;
    bool acabou = false;

    while (s && !acabou && !SDL_GetAtomicInt(&t->c->cancelar)) {
        const int bytes = le(f, ler);
        if (bytes > 0) SDL_PutAudioStreamData(s, ler, bytes);
        else           { SDL_FlushAudioStream(s); acabou = true; }   /* fim (ou erro): esvazia */

        int obtidos;
        while ((obtidos = SDL_GetAudioStreamData(s, conv, SAIDA_MAX * AUDIO_CANAIS * (int)sizeof(Sint16))) > 0) {
            Sint64 n = obtidos / (AUDIO_CANAIS * (Sint64)sizeof(Sint16));
            if (pos + n > t->total) n = t->total - pos;
            if (n <= 0) break;
            /* somar é rápido perto de decodificar: uma trava só basta */
            SDL_LockMutex(t->trava);
            Sint32 *d = t->mix + pos * AUDIO_CANAIS;
            for (Sint64 k = 0; k < n * AUDIO_CANAIS; k++) d[k] += conv[k];
            SDL_UnlockMutex(t->trava);
            pos += n;
            const int feitos = SDL_AddAtomicInt(&t->feitos, (int)n) + (int)n;
            SDL_SetAtomicInt(&t->c->progresso, (int)(1000 * (Sint64)feitos / (t->soma > 0 ? t->soma : 1)));
        }
    }
    SDL_DestroyAudioStream(s);

    SDL_LockMutex(t->trava);
    if (pos > t->fim) t->fim = pos;
    SDL_UnlockMutex(t->trava);
}

static int SDLCALL operario(void *dado)
{
    Trabalho *t = dado;
    float  *ler  = SDL_malloc(LEITURA * 2 * sizeof(float));
    Sint16 *conv = SDL_malloc(SAIDA_MAX * AUDIO_CANAIS * sizeof(Sint16));
    if (!ler || !conv) {
        SDL_SetAtomicInt(&t->sem_memoria, 1);
    } else {
        for (;;) {
            const int i = SDL_AddAtomicInt(&t->proxima, 1);   /* devolve o valor de antes: a faixa é minha */
            if (i >= t->n || SDL_GetAtomicInt(&t->c->cancelar)) break;
            decodifica(t, &t->fontes[i], ler, conv);
        }
    }
    SDL_free(ler);
    SDL_free(conv);
    return 0;
}

/* quantas threads: uma por núcleo, sem passar do número de faixas
   (ARKHERO_THREADS força um número, para medir) */
static int quantas_threads(int faixas)
{
    int n = SDL_GetNumLogicalCPUCores();
    const char *forcado = SDL_getenv("ARKHERO_THREADS");
    if (forcado && *forcado) n = SDL_atoi(forcado);
    if (n > faixas) n = faixas;
    if (n > CARREGADOR_MAX_FAIXAS) n = CARREGADOR_MAX_FAIXAS;
    return n < 1 ? 1 : n;
}

/* ---------------------------------------------------------------------------
 * A thread da carga
 * ------------------------------------------------------------------------- */
static void falha(Carregador *c, const char *fmt, const char *detalhe)
{
    SDL_snprintf(c->erro, sizeof(c->erro), fmt, detalhe);
    SDL_Log("[carga] %s", c->erro);
    SDL_SetAtomicInt(&c->estado, CARGA_ERRO);
}

static int SDLCALL trabalha(void *dado)
{
    Carregador *c = dado;
    const Uint64 inicio = SDL_GetTicksNS();

    /* 1. abre todas as faixas para saber o tamanho final */
    Fonte  fontes[CARREGADOR_MAX_FAIXAS];
    int    abertas = 0;
    Sint64 maior = 0, soma = 0;   /* em frames de saída (48 kHz) */
    for (int i = 0; i < c->n; i++) {
        if (!abre(&fontes[abertas], c->arquivos[i])) {
            SDL_Log("[carga] pulando %s: %s", c->arquivos[i], SDL_GetError());
            continue;
        }
        const Fonte *f = &fontes[abertas++];
        const Sint64 saida = f->frames * AUDIO_TAXA / (f->spec.freq > 0 ? f->spec.freq : AUDIO_TAXA);
        if (saida > maior) maior = saida;
        soma += saida;
    }
    if (abertas == 0) { falha(c, "nenhuma faixa de audio abriu (%s)", c->n ? c->arquivos[0] : "-"); return 0; }

    /* 2. decodifica em paralelo, somando em 32 bits */
    Trabalho t;
    SDL_zero(t);
    t.c      = c;
    t.fontes = fontes;
    t.n      = abertas;
    t.soma   = soma;
    t.total  = maior + MARGEM;
    t.mix    = SDL_calloc((size_t)t.total * AUDIO_CANAIS, sizeof(Sint32));
    t.trava  = SDL_CreateMutex();
    if (!t.mix || !t.trava) {
        SDL_free(t.mix);
        SDL_DestroyMutex(t.trava);
        for (int i = 0; i < abertas; i++) fecha(&fontes[i]);
        falha(c, "sem memoria para %s", "a musica");
        return 0;
    }

    const int n_threads = quantas_threads(abertas);
    SDL_Thread *extras[CARREGADOR_MAX_FAIXAS];
    int criadas = 0;
    for (int i = 1; i < n_threads; i++) {   /* esta thread também trabalha: as outras são extras */
        extras[criadas] = SDL_CreateThread(operario, "arkhero-faixa", &t);
        if (extras[criadas]) criadas++;
    }
    operario(&t);
    for (int i = 0; i < criadas; i++) SDL_WaitThread(extras[i], NULL);
    SDL_DestroyMutex(t.trava);
    for (int i = 0; i < abertas; i++) fecha(&fontes[i]);

    if (SDL_GetAtomicInt(&c->cancelar) || SDL_GetAtomicInt(&t.sem_memoria)) {
        SDL_free(t.mix);
        falha(c, "%s", SDL_GetAtomicInt(&c->cancelar) ? "carga cancelada" : "sem memoria na carga");
        return 0;
    }

    /* 3. de 32 para 16 bits, no mesmo buffer: a amostra i (16 bits) é gravada
       antes do ponto em que as próximas (32 bits) ainda vão ser lidas */
    Sint16 *saida = (Sint16 *)t.mix;
    for (Sint64 i = 0; i < t.fim * AUDIO_CANAIS; i++) {
        const Sint32 v = t.mix[i];
        saida[i] = (Sint16)(v > 32767 ? 32767 : (v < -32768 ? -32768 : v));
    }
    Sint16 *menor = SDL_realloc(saida, (size_t)(t.fim > 0 ? t.fim : 1) * AUDIO_CANAIS * sizeof(Sint16));

    c->pcm.amostras   = menor ? menor : saida;
    c->pcm.frames     = t.fim;
    c->segundos       = (double)(SDL_GetTicksNS() - inicio) / 1e9;
    c->segundos_audio = (double)soma / AUDIO_TAXA;
    SDL_Log("[carga] %d faixa(s) em %d thread(s), %.1f s de audio em %.2f s (%.0fx o tempo real)", abertas,
            criadas + 1, (double)t.fim / AUDIO_TAXA, c->segundos,
            c->segundos > 0 ? (double)soma / AUDIO_TAXA / c->segundos : 0.0);
    SDL_SetAtomicInt(&c->progresso, 1000);
    SDL_SetAtomicInt(&c->estado, CARGA_PRONTO);   /* por último: a partir daqui o main lê os campos */
    return 0;
}

/* ---------------------------------------------------------------------------
 * Interface
 * ------------------------------------------------------------------------- */
static void espera_thread(Carregador *c)
{
    if (c->thread) SDL_WaitThread(c->thread, NULL);
    c->thread = NULL;
}

bool Carregador_start(Carregador *c, const char arquivos[][512], int n)
{
    if (Carregador_state(c) == CARGA_ANDANDO) return false;
    espera_thread(c);
    Pcm_free(&c->pcm);

    c->n = n < CARREGADOR_MAX_FAIXAS ? n : CARREGADOR_MAX_FAIXAS;
    for (int i = 0; i < c->n; i++) SDL_strlcpy(c->arquivos[i], arquivos[i], sizeof(c->arquivos[i]));
    c->erro[0]  = '\0';
    c->segundos = 0.0;
    SDL_SetAtomicInt(&c->progresso, 0);
    SDL_SetAtomicInt(&c->cancelar, 0);
    SDL_SetAtomicInt(&c->estado, CARGA_ANDANDO);

    c->thread = SDL_CreateThread(trabalha, "arkhero-carga", c);
    if (!c->thread) {
        falha(c, "nao criei a thread de carga: %s", SDL_GetError());
        return false;
    }
    return true;
}

EstadoCarga Carregador_state(const Carregador *c)
{
    return (EstadoCarga)SDL_GetAtomicInt((SDL_AtomicInt *)&c->estado);
}

float Carregador_progress(const Carregador *c)
{
    return SDL_GetAtomicInt((SDL_AtomicInt *)&c->progresso) / 1000.0f;
}

bool Carregador_take(Carregador *c, Pcm *saida)
{
    if (Carregador_state(c) != CARGA_PRONTO) return false;
    espera_thread(c);
    *saida = c->pcm;
    c->pcm = (Pcm){0};
    SDL_SetAtomicInt(&c->estado, CARGA_PARADO);
    return true;
}

void Carregador_quit(Carregador *c)
{
    SDL_SetAtomicInt(&c->cancelar, 1);
    espera_thread(c);
    Pcm_free(&c->pcm);
    SDL_SetAtomicInt(&c->estado, CARGA_PARADO);
}
