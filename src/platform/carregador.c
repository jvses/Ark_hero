#include "platform/carregador.h"

#include <opusfile.h>
#include <vorbis/vorbisfile.h>

#define LEITURA    4096                  /* frames lidos por vez de cada arquivo */
#define SAIDA_MAX  (LEITURA * 4)         /* frames convertidos por vez (folga para reamostrar) */
#define MARGEM     (AUDIO_TAXA / 10)     /* 100 ms a mais no fim, por segurança */

/* ---------------------------------------------------------------------------
 * Uma faixa aberta: entrega áudio no formato dela, e um SDL_AudioStream
 * converte para o nosso (s16, estéreo, 48 kHz). Vorbis costuma vir a
 * 44,1 kHz; o Opus é sempre 48 kHz.
 * ------------------------------------------------------------------------- */
typedef enum { FONTE_OPUS, FONTE_VORBIS, FONTE_WAV } TipoFonte;

typedef struct {
    TipoFonte      tipo;
    OggOpusFile   *opus;
    OggVorbis_File vorbis;
    Uint8         *wav;
    Uint32         wav_bytes, wav_pos;
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

/* soma com saturação: as faixas foram feitas para tocar juntas, mas somadas
   podem passar do limite do s16 */
static void mistura(Sint16 *destino, const Sint16 *faixa, Sint64 amostras)
{
    for (Sint64 i = 0; i < amostras; i++) {
        const int v = destino[i] + faixa[i];
        destino[i] = (Sint16)(v > 32767 ? 32767 : (v < -32768 ? -32768 : v));
    }
}

/* ---------------------------------------------------------------------------
 * A thread
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
    const SDL_AudioSpec destino = { SDL_AUDIO_S16, AUDIO_CANAIS, AUDIO_TAXA };

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

    const Sint64 total = maior + MARGEM;
    Sint16 *mix  = SDL_calloc((size_t)total * AUDIO_CANAIS, sizeof(Sint16));
    float  *ler  = SDL_malloc(LEITURA * 2 * sizeof(float));
    Sint16 *conv = SDL_malloc(SAIDA_MAX * AUDIO_CANAIS * sizeof(Sint16));
    if (!mix || !ler || !conv) {
        SDL_free(mix); SDL_free(ler); SDL_free(conv);
        for (int i = 0; i < abertas; i++) fecha(&fontes[i]);
        falha(c, "sem memoria para %s", "a musica");
        return 0;
    }

    /* 2. cada faixa: decodifica, converte para 48 kHz s16 e soma no mix */
    Sint64 feitos = 0, fim = 0;
    bool   cancelado = false;
    for (int i = 0; i < abertas && !cancelado; i++) {
        Fonte *f = &fontes[i];
        SDL_AudioStream *s = SDL_CreateAudioStream(&f->spec, &destino);
        Sint64 pos = 0;
        bool acabou = false;

        while (s && !acabou) {
            if (SDL_GetAtomicInt(&c->cancelar)) { cancelado = true; break; }

            const int bytes = le(f, ler);
            if (bytes > 0) SDL_PutAudioStreamData(s, ler, bytes);
            else           { SDL_FlushAudioStream(s); acabou = true; }   /* fim (ou erro): esvazia */

            int obtidos;
            while ((obtidos = SDL_GetAudioStreamData(s, conv, SAIDA_MAX * AUDIO_CANAIS * (int)sizeof(Sint16))) > 0) {
                Sint64 n = obtidos / (AUDIO_CANAIS * (Sint64)sizeof(Sint16));
                if (pos + n > total) n = total - pos;
                if (n <= 0) break;
                mistura(mix + pos * AUDIO_CANAIS, conv, n * AUDIO_CANAIS);
                pos += n;
            }
            SDL_SetAtomicInt(&c->progresso, (int)(1000 * (feitos + pos) / (soma > 0 ? soma : 1)));
        }
        if (pos > fim) fim = pos;
        feitos += pos;
        SDL_DestroyAudioStream(s);
        fecha(f);
    }
    for (int i = 0; i < abertas; i++) fecha(&fontes[i]);   /* as que sobraram, se cancelou */
    SDL_free(ler);
    SDL_free(conv);

    if (cancelado) {
        SDL_free(mix);
        falha(c, "%s", "carga cancelada");
        return 0;
    }

    c->pcm.amostras = mix;
    c->pcm.frames   = fim;
    c->segundos     = (double)(SDL_GetTicksNS() - inicio) / 1e9;
    c->segundos_audio = (double)soma / AUDIO_TAXA;
    SDL_Log("[carga] %d faixa(s), %.1f s de audio em %.2f s (%.0fx o tempo real)", abertas,
            (double)fim / AUDIO_TAXA, c->segundos,
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
