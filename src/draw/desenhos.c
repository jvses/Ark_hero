#include "desenhos.h"
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>

void Clear_screen(Drawing_state *ds,int bg_color){
    SDL_SetRenderDrawColor(ds->render, bg_color, bg_color, bg_color, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(ds->render);
    // SDL_RenderPresent(ds->render); // coloca tudo na tela
}

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
quero fazer algumas funções para criar os recursos gráficos que vou usar por enquanto e preciso de ajuda para me esclarecer algumas coisas.
1) o mais difícil pra mim no momento é um meio para fazer as elipses, preciso fazer uma função que as desenhe que tenha como parâmetro de entrada o endereço do centro, largura, altura e cores RGB da elipse.
2) em seguida quero saber se o SDL3 já tem suporte a desenhar blocos(retângulos para botões) que consigam ajustar automaticamente o seu tamanho  de acordo com os textos que quero colocar dentro deles. 
3) Acredito ter visto que posso usar uma "virtualização" de dimensões no SDL para eu manter o desenvolvimento dos desenhos com dimensões 640x480 para situações onde o display é maior e a tela tem uma resolução maior que o que estou preparando no jogo. quero sabser como implementar isso
4) quero que me diga um método/estratégia para eu criar uma linha de cima para baixo com alguma inclinação(fazer a linha é fácil eu sei) mas de maneira que um círculo percorra essa linha indo de cima para baixo com sua origem seguindo essa linha.

*/

void Draw_menu_init(Drawing_state *ds){

SDL_Color cor;


}

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