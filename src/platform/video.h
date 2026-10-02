#ifndef VIDEO_LIB
#define VIDEO_LIB

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>

#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGTH 480

typedef struct {
    SDL_Window* Window_Ark;
    SDL_Renderer* Render_Ark;
    int16_t cor,razao;
    Uint64 fps_inicio;//instante em ms começo
    Uint64 frame_anterior;//instante do frame anterior(ms)
    int fps_frames;//frames condados no segundo atual
    float fps;//valor mostrado na tela
}Video;

bool Video_init(Video *vid);
void Video_quit(Video *vid);
void log_drivers(void);
static SDL_Renderer *create_renderer(SDL_Window *win);
void Clear_screen(Video *vid,int color);
void Draw_Rectangle(Video *vid, int color);
void Video_print_fps(Video *vid,float val);
void Video_RenderAll(Video *vid);

#endif
