#include "platform/audio.h"

#define BYTES_POR_FRAME   (AUDIO_CANAIS * (int)sizeof(Sint16))
#define ALVO_NA_FILA      (AUDIO_TAXA / 5)   /* mantém ~200 ms prontos no stream */
#define BLOCO             2048               /* frames por SDL_PutAudioStreamData */
#define SUAVE_SALTO       0.050              /* erro maior que isso: o relógio pula */
#define SUAVE_CORRECAO    0.10               /* fração do erro corrigida por frame */

static const Sint16 SILENCIO[BLOCO * AUDIO_CANAIS];   /* zeros */

void Pcm_free(Pcm *p)
{
    SDL_free(p->amostras);
    p->amostras = NULL;
    p->frames   = 0;
}

/* ---------------------------------------------------------------------------
 * Ciclo de vida
 * ------------------------------------------------------------------------- */
bool Audio_init(Audio *a)
{
    a->volume = 1.0f;

    /* buffer pequeno = menos latência; grande demais = estalos (underrun).
       1024 frames a 48 kHz = 21 ms. Dá para trocar sem recompilar. */
    const char *frames = SDL_getenv("ARKHERO_AUDIO_FRAMES");
    SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, (frames && *frames) ? frames : "1024");

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SDL_Log("[audio] SDL_InitSubSystem(AUDIO) falhou: %s (o jogo segue sem som)", SDL_GetError());
        return false;
    }

    const SDL_AudioSpec spec = { SDL_AUDIO_S16, AUDIO_CANAIS, AUDIO_TAXA };
    a->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!a->stream) {
        SDL_Log("[audio] nao abri o dispositivo: %s (o jogo segue sem som)", SDL_GetError());
        return false;
    }

    SDL_AudioSpec disp;
    SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(a->stream), &disp, &a->frames_buffer);
    SDL_strlcpy(a->driver, SDL_GetCurrentAudioDriver() ? SDL_GetCurrentAudioDriver() : "?", sizeof(a->driver));
    SDL_Log("[audio] driver %s, dispositivo %d Hz %d canais, buffer %d frames (%.1f ms)",
            a->driver, disp.freq, disp.channels, a->frames_buffer,
            1000.0 * a->frames_buffer / (disp.freq > 0 ? disp.freq : AUDIO_TAXA));

    /* Os efeitos (o som de erro) vão num segundo dispositivo LÓGICO: cada
       SDL_OpenAudioDeviceStream abre um, e a SDL mistura todos no mesmo
       dispositivo físico. (Ligar um segundo stream ao primeiro não dá: a SDL
       não deixa mexer nas ligações de um dispositivo aberto assim.) Sem ele
       o jogo segue, só sem o som de erro. */
    a->efeito = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (a->efeito) SDL_ResumeAudioStreamDevice(a->efeito);
    else           SDL_Log("[audio] sem stream de efeitos: %s (o jogo segue sem o som de erro)", SDL_GetError());

    /* o stream nasce pausado; ele toca silêncio enquanto não há música */
    SDL_ResumeAudioStreamDevice(a->stream);
    return true;
}

void Audio_quit(Audio *a)
{
    if (a->efeito) SDL_DestroyAudioStream(a->efeito);
    a->efeito = NULL;
    if (a->stream) SDL_DestroyAudioStream(a->stream);   /* fecha o dispositivo junto */
    a->stream = NULL;
    a->pcm    = NULL;
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

bool Audio_ok(const Audio *a)
{
    return a->stream != NULL;
}

/* ---------------------------------------------------------------------------
 * Reprodução
 * ------------------------------------------------------------------------- */
void Audio_play(Audio *a, const Pcm *pcm, double atraso_s)
{
    if (!a->stream) return;
    SDL_ClearAudioStream(a->stream);
    a->pcm      = pcm;
    a->enviados = 0;
    a->silencio = atraso_s > 0.0 ? (Sint64)(atraso_s * AUDIO_TAXA) : 0;
    a->pausado  = false;
    a->suave_ok = false;
    a->saltos   = 0;
    SDL_ResumeAudioStreamDevice(a->stream);
    Audio_update(a);
}

void Audio_stop(Audio *a)
{
    if (!a->stream) return;
    SDL_ClearAudioStream(a->stream);
    a->pcm     = NULL;
    a->pausado = false;
    SDL_ResumeAudioStreamDevice(a->stream);
}

void Audio_pause(Audio *a, bool pausar)
{
    if (!a->stream || !a->pcm || a->pausado == pausar) return;
    a->pausado  = pausar;
    a->suave_ok = false;   /* na volta, o relógio suave recomeça do cru */
    if (pausar) SDL_PauseAudioStreamDevice(a->stream);
    else        SDL_ResumeAudioStreamDevice(a->stream);
}

bool Audio_playing(const Audio *a)
{
    return a->pcm != NULL;
}

void Audio_set_volume(Audio *a, float volume)
{
    if (!a->stream || volume == a->volume) return;
    a->volume = volume;
    SDL_SetAudioStreamGain(a->stream, volume);
    if (a->efeito) SDL_SetAudioStreamGain(a->efeito, volume);
}

void Audio_effect(Audio *a, const Pcm *som)
{
    if (!a->efeito || !som->amostras) return;
    SDL_ClearAudioStream(a->efeito);
    SDL_PutAudioStreamData(a->efeito, som->amostras, (int)(som->frames * BYTES_POR_FRAME));
}

/* Mantém sempre ~200 ms na fila do stream. Primeiro o silêncio inicial, depois
 * a música, depois silêncio de novo: o relógio continua andando depois do fim
 * (o jogo precisa disso para chegar à tela de resultado). */
void Audio_update(Audio *a)
{
    if (!a->stream || !a->pcm || a->pausado) return;

    while (SDL_GetAudioStreamQueued(a->stream) / BYTES_POR_FRAME < ALVO_NA_FILA) {
        const Sint64 na_musica = a->enviados - a->silencio;   /* frame da música que vai agora */
        const Sint16 *dados = SILENCIO;
        Sint64 n = BLOCO;

        if (na_musica < 0) {                                  /* ainda no silêncio inicial */
            if (-na_musica < n) n = -na_musica;
        } else if (na_musica < a->pcm->frames) {              /* a música */
            dados = a->pcm->amostras + na_musica * AUDIO_CANAIS;
            if (a->pcm->frames - na_musica < n) n = a->pcm->frames - na_musica;
        }
        if (!SDL_PutAudioStreamData(a->stream, dados, (int)(n * BYTES_POR_FRAME))) break;
        a->enviados += n;
    }
}

/* ---------------------------------------------------------------------------
 * O relógio
 *
 * Cru: frames enviados - frames ainda na fila - buffer do dispositivo. Ele
 * anda em degraus (o dispositivo puxa um buffer inteiro de cada vez, ~21 ms).
 * Suave: anda com o relógio do sistema entre os degraus e é puxado aos
 * poucos para o cru, que é quem manda a longo prazo (ARQUITETURA 5.3).
 * ------------------------------------------------------------------------- */
double Audio_raw_time(const Audio *a)
{
    if (!a->stream || !a->pcm) return 0.0;
    const Sint64 na_fila = SDL_GetAudioStreamQueued(a->stream) / BYTES_POR_FRAME;
    const Sint64 tocados = a->enviados - na_fila - a->frames_buffer;
    return (double)(tocados - a->silencio) / AUDIO_TAXA;
}

double Audio_song_time(Audio *a, Uint64 agora_ns)
{
    const double cru = Audio_raw_time(a);
    if (!a->suave_ok || a->pausado) {
        a->t_suave   = cru;
        a->ultimo_ns = agora_ns;
        a->suave_ok  = !a->pausado;
        return cru;
    }

    a->t_suave  += (double)(agora_ns - a->ultimo_ns) / 1e9;
    a->ultimo_ns = agora_ns;

    const double erro = cru - a->t_suave;
    if (SDL_fabs(erro) > SUAVE_SALTO) {                      /* longe demais: pula */
        a->t_suave = cru;
        a->saltos++;
    } else {
        a->t_suave += erro * SUAVE_CORRECAO;
    }
    return a->t_suave;
}

/* ---------------------------------------------------------------------------
 * Metrônomo: senoide curta com decaimento; acento mais agudo e mais forte.
 * ------------------------------------------------------------------------- */
#define CLIQUE_FRAMES (AUDIO_TAXA / 40)   /* 25 ms */

bool Audio_make_clicks(Pcm *saida, const double *tempo, const bool *forte, int n, double duracao)
{
    saida->frames   = (Sint64)(duracao * AUDIO_TAXA);
    saida->amostras = SDL_calloc((size_t)saida->frames * AUDIO_CANAIS, sizeof(Sint16));
    if (!saida->amostras) { saida->frames = 0; return false; }

    for (int i = 0; i < n; i++) {
        const Sint64 inicio = (Sint64)(tempo[i] * AUDIO_TAXA);
        const float  freq   = forte[i] ? 1500.0f : 1000.0f;
        const float  amp    = forte[i] ? 0.55f : 0.35f;
        for (Sint64 k = 0; k < CLIQUE_FRAMES && inicio + k < saida->frames; k++) {
            if (inicio + k < 0) continue;
            const float tt  = (float)k / AUDIO_TAXA;
            const float env = 1.0f - (float)k / CLIQUE_FRAMES;          /* decai até zero */
            const Sint16 v  = (Sint16)(32767.0f * amp * env * env * SDL_sinf(2.0f * SDL_PI_F * freq * tt));
            saida->amostras[(inicio + k) * 2]     = v;
            saida->amostras[(inicio + k) * 2 + 1] = v;
        }
    }
    return true;
}

/* ---------------------------------------------------------------------------
 * Som de erro: a "nota errada". Dois dentes de serra (todos os harmônicos:
 * soa áspero, como corda) a um semitom de distância (a dissonância é que
 * diz "errou"), caindo um pouco de altura, mais um chiado agudo no ataque (a
 * palheta raspando). O conteúdo fica entre 200 Hz e alguns kHz, onde o ouvido
 * e os alto-falantes pequenos (o do R36S) escutam por cima da música.
 * ------------------------------------------------------------------------- */
#define ERRO_SEGUNDOS 0.22f

bool Audio_make_miss(Pcm *saida, float grave)
{
    saida->frames   = (Sint64)(ERRO_SEGUNDOS * AUDIO_TAXA);
    saida->amostras = SDL_calloc((size_t)saida->frames * AUDIO_CANAIS, sizeof(Sint16));
    if (!saida->amostras) { saida->frames = 0; return false; }

    const float f0 = 196.0f * grave;   /* sol (G3) na guitarra; o baixo, uma oitava abaixo */
    Uint32 semente = 12345;
    float  fase1 = 0.0f, fase2 = 0.0f, anterior = 0.0f;
    for (Sint64 k = 0; k < saida->frames; k++) {
        const float t    = (float)k / AUDIO_TAXA;
        const float env  = SDL_min(1.0f, t / 0.002f) * SDL_expf(-t * 14.0f);   /* ataque de 2 ms */
        const float cai  = 1.0f - 0.25f * t / ERRO_SEGUNDOS;                    /* o tom cai um pouco */
        fase1 += f0 * cai / AUDIO_TAXA;
        fase2 += f0 * 1.06f * cai / AUDIO_TAXA;                                 /* um semitom acima */
        fase1 -= SDL_floorf(fase1);
        fase2 -= SDL_floorf(fase2);
        const float serra = (2.0f * fase1 - 1.0f) + (2.0f * fase2 - 1.0f);

        semente = semente * 1664525u + 1013904223u;                           /* ruído branco barato */
        const float branco = (float)(semente >> 8) / 8388608.0f - 1.0f;       /* -1..1 */
        const float agudo  = branco - anterior;                               /* passa-alta: só o chiado */
        anterior = branco;

        const float v = env * 0.40f * serra + 0.30f * agudo * SDL_expf(-t * 60.0f);
        const Sint16 amostra = (Sint16)(32767.0f * SDL_clamp(v, -1.0f, 1.0f));
        saida->amostras[k * 2]     = amostra;
        saida->amostras[k * 2 + 1] = amostra;
    }
    return true;
}
