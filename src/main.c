// #include <SDL3/SDL_init.h>
#include "platform/input.h"
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#include <stdint.h>
#include <sys/types.h>
#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL_main.h>
#include "platform/video.h"
#include "app.h"


// static SDL_Time 

// App jogo;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    App *jogo = SDL_calloc(1, sizeof(App));
    if (!jogo) return SDL_APP_FAILURE;
    *appstate = jogo;            /* a partir daqui a SDL entrega o app(jogo) para os outros callbacks */

    SDL_SetAppMetadata("Ark Hero", "0.1", "ArkHero_Game");
    
    //parte responsável pelo vídeo/tela
    if(!Video_init(&jogo->video)) return SDL_APP_FAILURE;
    else SDL_Log("Vídeo initialized correctly");
    jogo->video.fps_inicio = jogo->video.frame_anterior = SDL_GetTicks();
    jogo->video.fps_frames=0;
    jogo->video.cor=20;
    jogo->video.razao=1;
    
    //parte responsável pelo input/gamepad
    if(!Input_init(&jogo->input)) return SDL_APP_FAILURE;
    else SDL_Log("Input/Gamepad initialized correctly");

    jogo->input.sair = false;

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    App *jogo=appstate;

    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }

    Input_Event_Handler(&jogo->input,event);

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    const Uint64 agora=SDL_GetTicks();//variavel captacao de tempo

    App *jogo=appstate;


    Clear_screen(&jogo->video, 50);
    Draw_Rectangle(&jogo->video, jogo->video.cor);
    Video_RenderAll(&jogo->video);
    // SDL_Delay(1);
    // SDL_Log("Log antes: cor= %d\nrazao(passo)=%d",jogo->video.cor,jogo->video.razao);
    if(jogo->video.cor>=254){
        jogo->video.razao = -1;
    } else if(jogo->video.cor<50){
        jogo->video.razao = 1;
    }
    jogo->video.cor+=jogo->video.razao;
    // SDL_Log("Log depois: i= %d\nrazao=%d",jogo->video.cor,jogo->video.razao);

    //processo para contagem de fps
    const Uint64 intervalo = agora - jogo->video.frame_anterior;
    jogo->video.frame_anterior = agora;
    jogo->video.fps_frames++;
    if (agora - jogo->video.fps_inicio >= 1000) {//se a diferença de tempo entre o primeiro frame e o agora for >= 1000 então passou 1 segundo
        jogo->video.fps =  jogo->video.fps_frames *1000.0f/ (float)(agora - jogo->video.fps_inicio);
        SDL_Log("FPS %.1f",jogo->video.fps);
        jogo->video.fps_inicio = agora;
        jogo->video.fps_frames=0;

    }

    if (jogo->input.sair) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    App *jogo = appstate;
    /* SDL will clean up the window/renderer for us. */
    Video_quit(&jogo->video);
    SDL_free(jogo);
    SDL_Quit();
}


// /*
//  * Ark Hero -- teste de portabilidade 00
//  *
//  * Uma bola quicando por 10 segundos, e entao o programa encerra.
//  * O objetivo NAO e o jogo: e provar a cadeia toolchain -> empacotamento ->
//  * execucao no handheld, e despejar no log tudo que precisamos saber sobre
//  * o ambiente de video/input do aparelho.
//  *
//  * O relogio aqui e SDL_GetTicks() so porque nao ha audio ainda. No jogo de
//  * verdade o relogio mestre sera derivado dos frames de PCM ja reproduzidos.
//  */
// #include <SDL3/SDL.h>
// #include <SDL3/SDL_main.h>

// #define LOGICAL_W 640
// #define LOGICAL_H 480
// #define BALL_R    32
// #define RUN_MS    10000

// /* Desenha um circulo com borda suave numa superficie e sobe pra GPU.
//    Serve tambem como primeiro teste do caminho de textura + alpha blend,
//    que e o que a highway e as notas vao usar depois. */
// static SDL_Texture *make_ball(SDL_Renderer *ren, int r)
// {
//     const int size = r * 2;
//     SDL_Surface *s = SDL_CreateSurface(size, size, SDL_PIXELFORMAT_RGBA32);
//     if (!s) {
//         SDL_Log("SDL_CreateSurface falhou: %s", SDL_GetError());
//         return NULL;
//     }

//     const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(s->format);
//     Uint32 *px = (Uint32 *)s->pixels;
//     const int stride = s->pitch / 4;

//     for (int y = 0; y < size; y++) {
//         for (int x = 0; x < size; x++) {
//             const float dx = (float)x - (float)r + 0.5f;
//             const float dy = (float)y - (float)r + 0.5f;
//             const float d  = SDL_sqrtf(dx * dx + dy * dy);
//             float a = ((float)r - d) / 1.5f;   /* antialias de ~1.5px na borda */
//             if (a > 1.0f) a = 1.0f;
//             if (a < 0.0f) a = 0.0f;
//             px[y * stride + x] = SDL_MapRGBA(fmt, NULL, 60, 220, 90, (Uint8)(a * 255.0f));
//         }
//     }

//     SDL_Texture *t = SDL_CreateTextureFromSurface(ren, s);
//     SDL_DestroySurface(s);
//     if (!t) {
//         SDL_Log("SDL_CreateTextureFromSurface falhou: %s", SDL_GetError());
//         return NULL;
//     }
//     SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
//     return t;
// }

/* Tudo isto e impresso ANTES de criar a janela, de proposito: se a criacao
   falhar, o log ja diz em que ambiente ela falhou. */
// static void log_drivers(void)
// {
//     const int v = SDL_GetVersion();
//     char buf[256];
//     int i;

//     SDL_Log("===== Ark Hero / teste de portabilidade =====");
//     SDL_Log("SDL compilado : %d.%d.%d",
//             SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);
//     SDL_Log("SDL runtime   : %d.%d.%d (%s)",
//             SDL_VERSIONNUM_MAJOR(v), SDL_VERSIONNUM_MINOR(v), SDL_VERSIONNUM_MICRO(v),
//             SDL_GetRevision());

//     buf[0] = '\0';
//     for (i = 0; i < SDL_GetNumVideoDrivers(); i++) {
//         SDL_strlcat(buf, SDL_GetVideoDriver(i), sizeof(buf));
//         SDL_strlcat(buf, " ", sizeof(buf));
//     }
//     SDL_Log("video disp.   : %s", buf);
//     SDL_Log("video em uso  : %s", SDL_GetCurrentVideoDriver());

//     buf[0] = '\0';
//     for (i = 0; i < SDL_GetNumRenderDrivers(); i++) {
//         SDL_strlcat(buf, SDL_GetRenderDriver(i), sizeof(buf));
//         SDL_strlcat(buf, " ", sizeof(buf));
//     }
//     SDL_Log("render disp.  : %s", buf);
// }
// \
// /* Tenta os backends em ordem. O fallback existe para o log conseguir
//    distinguir "a GPU nao coopera" de "o resto do programa esta quebrado":
//    se cair no software e a bola aparecer, o problema e so o backend. */
// static SDL_Renderer *create_renderer(SDL_Window *win)
// {
//     const char *tries[4];
//     int n = 0;

//     const char *forced = SDL_getenv("ARKHERO_RENDER_DRIVER");
//     if (forced && *forced) tries[n++] = forced;
//     tries[n++] = "opengles2";   /* o backend do handheld */
//     tries[n++] = NULL;          /* deixa a SDL escolher */
//     tries[n++] = "software";    /* ultimo recurso */

//     for (int i = 0; i < n; i++) {
//         SDL_Renderer *r = SDL_CreateRenderer(win, tries[i]);
//         if (r) {
//             SDL_Log("render em uso : %s", SDL_GetRendererName(r));
//             return r;
//         }
//         SDL_Log("renderer '%s' falhou: %s",
//                 tries[i] ? tries[i] : "(automatico)", SDL_GetError());
//     }
//     return NULL;
// }

// /* Abre o primeiro gamepad disponivel e lista todos no log.
//    Ainda nao usamos input pra nada: e so pra saber o que o aparelho expoe. */
// static SDL_Gamepad *open_first_gamepad(void)
// {
//     int n = 0;
//     SDL_JoystickID *ids = SDL_GetGamepads(&n);
//     SDL_Gamepad *pad = NULL;

//     SDL_Log("gamepads      : %d", n);
//     if (ids) {
//         for (int i = 0; i < n; i++) {
//             SDL_Log("  [%d] id=%u  %s", i, (unsigned)ids[i], SDL_GetGamepadNameForID(ids[i]));
//         }
//         if (n > 0) {
//             pad = SDL_OpenGamepad(ids[0]);
//             if (!pad) SDL_Log("SDL_OpenGamepad falhou: %s", SDL_GetError());
//         }
//         SDL_free(ids);
//     }
//     return pad;
// }

// int main(int argc, char *argv[])
// {
//     (void)argc;
//     (void)argv;

//     SDL_SetAppMetadata("Ark Hero", "0.0.1", "com.arkhero.game");

//     if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
//         SDL_Log("SDL_Init falhou: %s", SDL_GetError());
//         return 1;
//     }

//     log_drivers();

//     /* ---------------------------------------------------------------------
//        ESTA E A LINHA QUE FAZ O JOGO ABRIR NO HANDHELD.

//        Em KMS/DRM a SDL cria uma EGL surface junto com a janela, antes de
//        existir renderer. Para escolher o EGLConfig ela olha o perfil de GL
//        pedido: sem perfil definido, ela pede EGL_OPENGL_BIT (OpenGL desktop).
//        O Mali-G31 com Panfrost so expoe configs de OpenGL ES -- nenhum config
//        casa, e a janela morre com "Could not create EGL window surface".

//        No desktop isso passa despercebido porque o Mesa oferece os dois.
//        Pedindo ES explicitamente, os dois ambientes seguem o mesmo caminho.
//        --------------------------------------------------------------------- */
//     SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
//     SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
//     SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

//     /* No handheld o script de launch exporta ARKHERO_FULLSCREEN=1.
//        No desktop fica em janela, que e mais pratico pra desenvolver. */
//     SDL_WindowFlags flags = SDL_WINDOW_OPENGL;
//     const char *fs = SDL_getenv("ARKHERO_FULLSCREEN");
//     if (fs && SDL_strcmp(fs, "1") == 0) flags |= SDL_WINDOW_FULLSCREEN;

//     SDL_Window *win = SDL_CreateWindow("Ark Hero", LOGICAL_W, LOGICAL_H, flags);
//     if (!win) {
//         /* Alguns drivers de video nao tem GL nenhum (dummy, offscreen sem EGL,
//            algum firmware exotico). Vale tentar sem a flag: o renderer de
//            software ainda desenha, e o jogo abre em vez de morrer. */
//         SDL_Log("janela com OpenGL falhou (%s); tentando sem", SDL_GetError());
//         flags &= ~(SDL_WindowFlags)SDL_WINDOW_OPENGL;
//         win = SDL_CreateWindow("Ark Hero", LOGICAL_W, LOGICAL_H, flags);
//     }
//     if (!win) {
//         SDL_Log("SDL_CreateWindow falhou: %s", SDL_GetError());
//         SDL_Quit();
//         return 1;
//     }

//     SDL_Renderer *ren = create_renderer(win);
//     if (!ren) {
//         SDL_Log("nenhum renderer disponivel");
//         SDL_DestroyWindow(win);
//         SDL_Quit();
//         return 1;
//     }

//     SDL_SetRenderVSync(ren, 1);
//     /* Desenhamos sempre em 640x480 logico; a SDL escala pra tela real.
//        Isso ja resolve o R36S (640x480 nativo) e qualquer monitor de desktop. */
//     SDL_SetRenderLogicalPresentation(ren, LOGICAL_W, LOGICAL_H,
//                                      SDL_LOGICAL_PRESENTATION_LETTERBOX);

//     int pw = 0, ph = 0;
//     SDL_GetWindowSizeInPixels(win, &pw, &ph);
//     SDL_Log("janela        : %dx%d px (resolucao logica %dx%d)",
//             pw, ph, LOGICAL_W, LOGICAL_H);

//     SDL_Gamepad *pad = open_first_gamepad();

//     SDL_Texture *ball = make_ball(ren, BALL_R);
//     if (!ball) {
//         SDL_Quit();
//         return 1;
//     }

//     float bx = LOGICAL_W * 0.5f, by = LOGICAL_H * 0.35f;
//     float vx = 215.0f, vy = 160.0f;   /* pixels por segundo */

//     const Uint64 t0 = SDL_GetTicks();
//     Uint64 prev = t0;
//     Uint64 frames = 0;
//     bool running = true;

//     while (running) {
//         SDL_Event e;
//         while (SDL_PollEvent(&e)) {
//             switch (e.type) {
//             case SDL_EVENT_QUIT:
//                 running = false;
//                 break;
//             case SDL_EVENT_KEY_DOWN:
//                 if (e.key.key == SDLK_ESCAPE) running = false;
//                 break;
//             case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
//                 SDL_Log("botao: %d (%s)", e.gbutton.button,
//                         SDL_GetGamepadStringForButton((SDL_GamepadButton)e.gbutton.button));
//                 if (e.gbutton.button == SDL_GAMEPAD_BUTTON_START) running = false;
//                 break;
//             default:
//                 break;
//             }
//         }

//         const Uint64 now = SDL_GetTicks();
//         float dt = (float)(now - prev) / 1000.0f;
//         prev = now;
//         if (dt > 0.05f) dt = 0.05f;   /* clamp: evita a bola teleportar apos um stall */

//         bx += vx * dt;
//         by += vy * dt;
//         if (bx - BALL_R < 0.0f)              { bx = BALL_R;              vx = -vx; }
//         if (bx + BALL_R > (float)LOGICAL_W)  { bx = LOGICAL_W - BALL_R;  vx = -vx; }
//         if (by - BALL_R < 0.0f)              { by = BALL_R;              vy = -vy; }
//         if (by + BALL_R > (float)LOGICAL_H)  { by = LOGICAL_H - BALL_R;  vy = -vy; }

//         SDL_SetRenderDrawColor(ren, 18, 18, 24, 255);
//         SDL_RenderClear(ren);

//         SDL_SetRenderDrawColor(ren, 45, 45, 62, 255);
//         SDL_RenderLine(ren, 0.0f, LOGICAL_H - 1.0f, (float)LOGICAL_W, LOGICAL_H - 1.0f);

//         const SDL_FRect dst = { bx - BALL_R, by - BALL_R, BALL_R * 2.0f, BALL_R * 2.0f };
//         SDL_RenderTexture(ren, ball, NULL, &dst);

//         SDL_RenderPresent(ren);
//         frames++;

//         if (now - t0 >= RUN_MS) running = false;
//     }

//     const Uint64 elapsed = SDL_GetTicks() - t0;
//     SDL_Log("encerrando: %llu frames em %llu ms (~%.1f fps)",
//             (unsigned long long)frames, (unsigned long long)elapsed,
//             (double)frames * 1000.0 / (double)(elapsed ? elapsed : 1));

//     SDL_DestroyTexture(ball);
//     if (pad) SDL_CloseGamepad(pad);
//     SDL_DestroyRenderer(ren);
//     SDL_DestroyWindow(win);
//     SDL_Quit();
//     return 0;
// }
