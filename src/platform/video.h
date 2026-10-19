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
#include "../draw/desenhos.h"



typedef struct {
    SDL_Window* Window_Ark;
    // SDL_Renderer* Render_Ark;
    Drawing_state ds_render;
}Video;

bool Video_init(Video *vid);
void Video_quit(Video *vid);
void log_drivers(void);
// static SDL_Renderer *create_renderer(SDL_Window *win);
void Video_RenderAll(Video *vid);

#endif
