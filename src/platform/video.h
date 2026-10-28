#ifndef ARK_VIDEO_H
#define ARK_VIDEO_H

/* Janela, renderer e apresentação. Não sabe nada do jogo nem do que é desenhado. */

#include <SDL3/SDL.h>

#define VIDEO_LARGURA 640   /* resolução lógica: o jogo inteiro desenha neste espaço */
#define VIDEO_ALTURA  480

typedef struct {
    SDL_Window   *janela;
    SDL_Renderer *ren;

    /* medição do ciclo de apresentação */
    Uint64 fps_inicio;
    Uint64 frame_anterior;
    int    fps_frames;
    float  fps;
    Uint64 pior_frame_ms;
} Video;

bool Video_init(Video *vid);
void Video_quit(Video *vid);
void Video_begin_frame(Video *vid);   /* limpa a tela */
void Video_end_frame(Video *vid);     /* apresenta (o vsync espera aqui) e conta o FPS */

#endif
