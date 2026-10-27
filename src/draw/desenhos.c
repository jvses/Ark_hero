#include "desenhos.h"
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <stdio.h>
// #include <cstdio>

void Clear_screen(Drawing_state *ds){//apaga tudo dentro do render
    // SDL_SetRenderDrawColor(ds->render, bg_color, bg_color, bg_color, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(ds->render);
    // SDL_RenderPresent(ds->render); // coloca tudo na tela
}

void Draw_memory_prepare_stuffs(Drawing_state *ds,Game_logic_State *gl){// essa função vai ajustar figura de bg do jogo de acordo com os estados e já deixar a textura na memória
        char caminho[40];
    switch (gl->game_menu_state) {
        case idle:
            snprintf(caminho, sizeof(caminho),"assets/images/vaporwave_guitar1.png");
            break;
        case gameplay:
            snprintf(caminho,sizeof(caminho), "assets/images/vaporwave_guitar2.png");
            break;
        case config:
            snprintf(caminho,sizeof(caminho), "assets/images/vaporwave_guitar3.png");
            break;
    }
    SDL_Surface *s = SDL_LoadPNG(caminho);
    if (!s) {
        SDL_Log("nao carreguei '%s': %s", caminho, SDL_GetError());
        // return SDL_APP_FAILURE;
    }
    ds->fundo = SDL_CreateTextureFromSurface(ds->render, s);
    SDL_DestroySurface(s);
    SDL_SetTextureBlendMode(ds->fundo, SDL_BLENDMODE_NONE);   /* fundo é opaco: o modo mais barato */
}

void Draw_bg_image(Drawing_state *ds,Game_logic_State *gl){
    SDL_RenderTexture(ds->render, ds->fundo, NULL, NULL);
}

void Draw_menu_and_bg(Drawing_state *ds,Game_logic_State *gl){

    Draw_bg_image(ds,gl);
    switch (gl->game_menu_state) {
        case idle:
            Draw_menu_idle(ds,gl);
            break;
        case gameplay:
            Draw_music_tab(ds);
            break;
        case config:
            Draw_Rectangle(ds, gl->cor_bg);
            break;
    }

}

void Draw_menu_idle(Drawing_state *ds,Game_logic_State *gl){
    SDL_Color cor = Draw_get_color(branco);//cor de fundo branca para os botões e depois para o texto;
    const int delta = 50;//espaço entre cada botão
    const int yinit = 100;

    for (int i=1; i<=3; i++) {
        SDL_FRect rec = {
            .x=50,
            .y=(delta+i*yinit),
            .w=(8*6),
            .h=16
        };
        cor = Draw_get_color(branco);
        SDL_SetRenderDrawColor(ds->render,cor.r,cor.g,cor.b,cor.a);
        SDL_RenderFillRect(ds->render, &rec);

        if(gl->game_menu_state_selected == (i-1)) {
            cor = Draw_get_color(preto);
        }
        else{
         cor = Draw_get_color(azul);
        }

        char btn_msg[20];
        switch (i-1) {
            case idle:
            snprintf(btn_msg, sizeof(btn_msg), "Idle");
                // btn_msg = "Idle";
                break;
            case gameplay:
            snprintf(btn_msg, sizeof(btn_msg), "Play");
                // btn_msg = "Jogar";
                break;
            case config:
                snprintf(btn_msg, sizeof(btn_msg), "Configure");
                // btn_msg = "Configurações";
                break;
        }

        SDL_SetRenderDrawColor(ds->render,cor.r,cor.g,cor.b,cor.a);
        SDL_RenderDebugTextFormat(ds->render,  delta,  (delta+i*yinit), "%s",btn_msg);
        // Draw_FillRectangle(ds, 50, (delta+), fcor);
    }
    
}

void Draw_menu_pause(Drawing_state *ds,Game_logic_State *gl);

void Draw_Rectangle(Drawing_state *ds, int color){
    SDL_FRect rec;
    rec.x = DEFAULT_WIDTH/4;
    rec.y = DEFAULT_HEIGTH/4;
    rec.w = DEFAULT_WIDTH/2;
    rec.h = DEFAULT_HEIGTH/2;
    SDL_SetRenderDrawColor(ds->render, color, color, color, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(ds->render, &rec);
    //SDL_RenderPresent(ds->render); // coloca tudo na tela
}

void Draw_print_fps(Drawing_state *ds,float val){
    //reajustando a cor para amarelo antes de escrever a mensagem com o FPS
    SDL_SetRenderDrawColor(ds->render, 255, 255, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderDebugTextFormat(ds->render,  0,  0, "FPS: %.2f", val);
}

void Draw_FillRectangle(Drawing_state *ds,float x, float y, SDL_Color cor){//desenha retângulo preenchido com cor e endereços definidos
    SDL_FRect rec;
    rec.x = x;
    rec.y = y;
    rec.w = 40;
    rec.h = 20;
    SDL_SetRenderDrawColor(ds->render,cor.r,cor.g,cor.b,cor.a);
    SDL_RenderFillRect(ds->render, &rec);
}
/*

*/



// void Draw_triangle(Drawing_state *ds){
//     SDL_Vertex vertices_triangulo = {.position.x = 165 , .color, .tex_coord };
// }

void Draw_music_tab(Drawing_state *ds){

    const SDL_FColor fcor = { 255.0f,255.0f, 255.0f, 255.0f };
    SDL_Vertex vertices_trapezio[4] = {
        { .position = {.x= TAB_XD, .y=TAB_YD }, .color = fcor, .tex_coord = {0,0} },
        { .position = {.x= (TAB_XD + TAB_XD_DELTA), .y=TAB_YD }, .color = fcor, .tex_coord = {0,0} },
        { .position = {.x= TAB_XU, .y=TAB_YU }, .color = fcor, .tex_coord = {0,0} },
        { .position = {.x= (TAB_XU + TAB_XU_DELTA), .y=TAB_YU }, .color = fcor, .tex_coord = {0,0} },
    };
    int indices[6]={0,1,2,1,2,3};
    
    SDL_RenderGeometry(ds->render, NULL, vertices_trapezio, 4, indices, 6);
}

#define ELIPSE_SEGMENTOS 48 //pontos na borda; de 32 a 64 já fica liso em 640x480

//desenha uma elipse preenchida com centro em (x,y), largura w e altura h (círculo: w == h)
//a elipse é um "leque" de triângulos: todos saem do centro e vão até dois pontos vizinhos da borda
//obs: sem SDL_SetRenderDrawBlendMode(BLEND) o alfa da cor é ignorado (sem textura, o RenderGeometry usa o blend de desenho)
void Draw_Elipse(Drawing_state *ds, float x, float y, float w, float h, SDL_Color cor){
    SDL_Vertex vertices[ELIPSE_SEGMENTOS + 1]; //0 = centro, 1..N = borda
    int indices[ELIPSE_SEGMENTOS * 3];         //3 vértices por triângulo, N triângulos

    const float a = w / 2.0f; //semi-eixo horizontal (raio em x)
    const float b = h / 2.0f; //semi-eixo vertical (raio em y)

    //o SDL_FColor vai de 0.0 a 1.0, não de 0 a 255
    const SDL_FColor fcor = { cor.r / 255.0f, cor.g / 255.0f, cor.b / 255.0f, cor.a / 255.0f };

    //vértice 0: o centro
    vertices[0].position = (SDL_FPoint){ x, y };
    vertices[0].color = fcor;
    vertices[0].tex_coord = (SDL_FPoint){ 0.0f, 0.0f }; //sem textura, não é usado

    for (int i = 0; i < ELIPSE_SEGMENTOS; i++) {
        //ângulo do i-ésimo ponto da borda: a volta inteira (2π) dividida em N partes
        const float teta = 2.0f * SDL_PI_F * (float)i / (float)ELIPSE_SEGMENTOS;

        //forma paramétrica da elipse: x = cx + a*cos(θ), y = cy + b*sin(θ)
        vertices[i + 1].position.x = x + a * SDL_cosf(teta);
        vertices[i + 1].position.y = y + b * SDL_sinf(teta);
        vertices[i + 1].color = fcor;
        vertices[i + 1].tex_coord = (SDL_FPoint){ 0.0f, 0.0f };

        //triângulo i: (centro, ponto i, ponto seguinte)
        //o "% ELIPSE_SEGMENTOS" faz o último triângulo voltar ao ponto 1 e fechar a volta
        indices[3 * i]     = 0;
        indices[3 * i + 1] = i + 1;
        indices[3 * i + 2] = (i + 1) % ELIPSE_SEGMENTOS + 1;
    }

    //tudo numa chamada só: a GPU preenche os N triângulos de uma vez
    SDL_RenderGeometry(ds->render, NULL, vertices, ELIPSE_SEGMENTOS + 1, indices, ELIPSE_SEGMENTOS * 3);
}

SDL_Color Draw_get_color(int cor){//função que edita aray com valores rgb para setar as cores
    SDL_Color rgba;
        switch (cor) {
        case branco:
            rgba.b=rgba.r=rgba.g=255;
            break;
        case vermelho:
            rgba.r=250;
            rgba.g=rgba.b=0;
            break;
        case verde:
            rgba.g=250;
            rgba.r=rgba.b=0;
            break;
        case azul:
            rgba.b=250;
            rgba.g=rgba.r=0;
            break;
        case amarelo:
            rgba.b=0;
            rgba.g=rgba.r=250;
            break;
        case laranja:
            rgba.b=0;
            rgba.g=165;
            rgba.r=250;
            break;
        case cinza:
            rgba.r=rgba.g=rgba.b=200;
            break;
        case cinza_escuro:
            rgba.r=rgba.g=rgba.b=150;
            break;
        case preto:
            rgba.r=rgba.g=rgba.b=0;
            break;  
    }  
    rgba.a=255;
    return rgba;
}