#include "platform/video.h"

#define FPS_LOG_MS 5000   /* de quanto em quanto tempo o FPS vai para o log */

static void log_drivers(void)
{
    const int v = SDL_GetVersion();
    char buf[256];

    SDL_Log("===== Ark Hero =====");
    SDL_Log("SDL compilado : %d.%d.%d", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);
    SDL_Log("SDL runtime   : %d.%d.%d (%s)", SDL_VERSIONNUM_MAJOR(v), SDL_VERSIONNUM_MINOR(v),
            SDL_VERSIONNUM_MICRO(v), SDL_GetRevision());

    buf[0] = '\0';
    for (int i = 0; i < SDL_GetNumVideoDrivers(); i++) {
        SDL_strlcat(buf, SDL_GetVideoDriver(i), sizeof(buf));
        SDL_strlcat(buf, " ", sizeof(buf));
    }
    SDL_Log("video disp.   : %s", buf);
    SDL_Log("video em uso  : %s", SDL_GetCurrentVideoDriver());

    buf[0] = '\0';
    for (int i = 0; i < SDL_GetNumRenderDrivers(); i++) {
        SDL_strlcat(buf, SDL_GetRenderDriver(i), sizeof(buf));
        SDL_strlcat(buf, " ", sizeof(buf));
    }
    SDL_Log("render disp.  : %s", buf);
}

/* opengles2 primeiro, para o PC testar o mesmo caminho do console. No
   Windows, o que a SDL escolher (Direct3D): lá o GLES depende do driver. */
static SDL_Renderer *cria_renderer(SDL_Window *janela)
{
    const char *tentativas[3];
    int n = 0;

    const char *forcado = SDL_getenv("ARKHERO_RENDER_DRIVER");
    if (forcado && *forcado) tentativas[n++] = forcado;
#ifndef SDL_PLATFORM_WINDOWS
    tentativas[n++] = "opengles2";
#endif
    tentativas[n++] = NULL;   /* deixa a SDL escolher */

    for (int i = 0; i < n; i++) {
        SDL_Renderer *r = SDL_CreateRenderer(janela, tentativas[i]);
        if (r) {
            SDL_Log("render em uso : %s", SDL_GetRendererName(r));
            return r;
        }
        SDL_Log("[video] renderer '%s' falhou: %s",
                tentativas[i] ? tentativas[i] : "(automatico)", SDL_GetError());
    }
    return NULL;
}

bool Video_init(Video *vid)
{
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        SDL_Log("[video] SDL_InitSubSystem(VIDEO) falhou: %s", SDL_GetError());
        return false;
    }
    log_drivers();

    /* Em KMS/DRM a SDL cria a superfície EGL junto com a janela. Sem pedir
       GLES aqui, ela pede OpenGL desktop, que o Mali do R36S não tem: a janela
       não nasce (BUILD.md, "Could not create EGL window surface"). */
    if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES) ||
        !SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2) ||
        !SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0)) {
        SDL_Log("[video] nao consegui pedir o perfil GLES: %s", SDL_GetError());
    }

    /* No console o script de launch exporta ARKHERO_FULLSCREEN=1.
       No PC a janela pode ser redimensionada: a resolução lógica cuida da escala. */
    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
#ifndef SDL_PLATFORM_WINDOWS
    flags |= SDL_WINDOW_OPENGL;
#endif
    const char *fs = SDL_getenv("ARKHERO_FULLSCREEN");
    if (fs && SDL_strcmp(fs, "1") == 0) flags |= SDL_WINDOW_FULLSCREEN;

    vid->janela = SDL_CreateWindow("Ark Hero", VIDEO_LARGURA, VIDEO_ALTURA, flags);
    if (!vid->janela) {
        SDL_Log("[video] SDL_CreateWindow falhou: %s", SDL_GetError());
        return false;
    }

    vid->ren = cria_renderer(vid->janela);
    if (!vid->ren) return false;

    if (!SDL_SetRenderVSync(vid->ren, 1) &&
        !SDL_SetRenderVSync(vid->ren, SDL_RENDERER_VSYNC_ADAPTIVE)) {
        SDL_Log("[video] sem vsync: %s", SDL_GetError());
    }

    /* Daqui pra frente tudo é desenhado em 640x480, qualquer que seja a tela.
       No R36S a escala é 1:1; num monitor maior aparecem faixas pretas. */
    SDL_SetRenderLogicalPresentation(vid->ren, VIDEO_LARGURA, VIDEO_ALTURA,
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);

    vid->fps_inicio = vid->frame_anterior = SDL_GetTicks();
    return true;
}

void Video_quit(Video *vid)
{
    if (vid->ren)    SDL_DestroyRenderer(vid->ren);
    if (vid->janela) SDL_DestroyWindow(vid->janela);
    vid->ren    = NULL;
    vid->janela = NULL;
}

void Video_begin_frame(Video *vid)
{
    SDL_SetRenderDrawColor(vid->ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(vid->ren);
}

void Video_end_frame(Video *vid)
{
    SDL_RenderPresent(vid->ren);   /* com vsync, espera a tela aqui */

    const Uint64 agora     = SDL_GetTicks();
    const Uint64 intervalo = agora - vid->frame_anterior;
    vid->frame_anterior = agora;
    if (intervalo > vid->pior_frame_ms) vid->pior_frame_ms = intervalo;

    vid->fps_frames++;
    const Uint64 passou = agora - vid->fps_inicio;
    if (passou >= FPS_LOG_MS) {
        vid->fps = (float)vid->fps_frames * 1000.0f / (float)passou;
        SDL_Log("[video] fps %.1f  pior frame %llu ms", vid->fps, (unsigned long long)vid->pior_frame_ms);
        vid->fps_inicio    = agora;
        vid->fps_frames    = 0;
        vid->pior_frame_ms = 0;
    }
}
