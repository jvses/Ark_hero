#ifndef ARK_AUDIO_H
#define ARK_AUDIO_H

/* Saída de áudio e o RELÓGIO MESTRE do jogo.
 *
 * A posição na música vem de quantos frames de áudio o dispositivo já
 * consumiu, nunca do relógio do sistema: os dois derivam entre si, e depois
 * de alguns minutos as notas "escorregariam" do som (CONVERSA_INICIAL 1).
 *
 * Toda música já chega decodificada e misturada (Pcm: s16, estéreo, 48 kHz):
 * durante a partida não há decodificação nenhuma, só cópia. */

#include <SDL3/SDL.h>

#define AUDIO_TAXA   48000   /* frames por segundo */
#define AUDIO_CANAIS 2

typedef struct {
    Sint16 *amostras;   /* intercalado: E D E D ... */
    Sint64  frames;
} Pcm;

void Pcm_free(Pcm *p);

typedef struct {
    SDL_AudioStream *stream;          /* ligado ao dispositivo (SDL_OpenAudioDeviceStream) */
    int              frames_buffer;   /* tamanho do buffer do dispositivo: a latência básica */
    char             driver[32];

    const Pcm *pcm;        /* o que está tocando (o Audio não é dono) */
    Sint64     enviados;   /* frames já colocados no stream, contando o silêncio inicial */
    Sint64     silencio;   /* frames de silêncio antes da música (t negativo) */
    bool       pausado;

    /* relógio suavizado */
    double     t_suave;
    Uint64     ultimo_ns;
    bool       suave_ok;
    float      volume;
} Audio;

bool   Audio_init(Audio *a);          /* falhar não impede o jogo: ele roda sem som */
void   Audio_quit(Audio *a);
bool   Audio_ok(const Audio *a);

void   Audio_play(Audio *a, const Pcm *pcm, double atraso_s);   /* do começo; atraso = silêncio antes */
void   Audio_stop(Audio *a);
void   Audio_pause(Audio *a, bool pausar);
void   Audio_update(Audio *a);         /* alimenta o dispositivo: uma vez por frame */
void   Audio_set_volume(Audio *a, float volume);   /* 0..1 */
bool   Audio_playing(const Audio *a);  /* tocando ou pausado */

double Audio_raw_time(const Audio *a);                /* posição crua, em degraus (diagnóstico) */
double Audio_song_time(Audio *a, Uint64 agora_ns);    /* o relógio mestre, suavizado */

/* Metrônomo gerado por código: um clique em cada tempo[i] (forte[i] = acento). */
bool   Audio_make_clicks(Pcm *saida, const double *tempo, const bool *forte, int n, double duracao);

#endif
