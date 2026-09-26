#ifndef VIDEO_LIB
#define VIDEO_LIB

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <stdbool.h>

#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGTH 480

typedef struct {
    SDL_Window* Window_Ark;
    SDL_Renderer* Render_Ark;
}Video;

bool Video_init(Video *vid);
void Video_quit(Video *vid);

#endif