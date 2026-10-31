#include "platform/pasta.h"

#include <SDL3/SDL.h>

char *Pasta_dados(void)
{
#ifdef SDL_PLATFORM_WINDOWS
    const char *base = SDL_GetBasePath();
    if (base) {
        char pasta[512];
        SDL_snprintf(pasta, sizeof(pasta), "%sconf\\arkhero\\", base);
        if (SDL_CreateDirectory(pasta)) return SDL_strdup(pasta);
    }
#endif
    return SDL_GetPrefPath("", "arkhero");
}
