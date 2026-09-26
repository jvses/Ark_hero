# Apostila de SDL3

### Aprendendo a biblioteca na ordem em que o *Ark Hero* precisa dela

---

## Como usar esta apostila

Esta apostila assume que você sabe C: ponteiros, structs, alocação, compilação
separada, `const`. Não assume nada de SDL, de OpenGL ou de programação gráfica.

**Por que ela existe.** Cursos em vídeo de SDL costumam seguir a ordem do
*manual*: primeiro janela, depois eventos, depois imagens, depois som, depois
"tópicos avançados". Essa ordem é boa para quem quer conhecer a biblioteca
inteira. É ruim para quem quer construir uma coisa específica, porque dilui
o que é crítico no meio do que é acessório, e deixa para o fim justamente os
assuntos que determinam se o seu projeto funciona.

No seu caso, dois assuntos decidem o projeto — **sincronia de tempo** e
**compatibilidade de backend** — e os dois costumam aparecer no último terço
dos cursos, se aparecerem. Aqui eles vêm cedo.

**A ordem escolhida:**

| Parte | Capítulos | Por que aqui |
|---|---|---|
| I — Fundamentos | 1–3 | O modelo mental, e compatibilidade **antes** de escrever muito código |
| II — Desenho | 4–8 | A tela de 640×480, e onde o esforço cai: CPU ou GPU |
| III — Tempo, som, input | 9–12 | O coração de um jogo de ritmo |
| IV — Juntando | 13–15 | Arquitetura, depuração, e a porta para Lua |

**Como estudar.** Cada capítulo termina com um exercício que produz uma peça
real do jogo. Ao fim da apostila você terá escrito, em ordem, quase todos os
componentes do *Ark Hero* — separados, testáveis, sem ter tentado montar o
jogo inteiro de uma vez. Não pule os exercícios: ler SDL é fácil, e é
justamente isso que engana.

**Notação:**

> 📐 **Conceito** — a ideia, independente de SDL.
> 🔌 **Na SDL3** — como a biblioteca expressa a ideia.
> 🎮 **No R36S** — o que muda no hardware alvo.
> ⚠️ **Armadilha** — erro que você cometeria.
> 🔧 **Exercício** — mão na massa.

---

# Parte I — Fundamentos

---

## Capítulo 1 — O que a SDL3 é (e o que ela não é)

### 1.1 A definição que importa

A SDL é uma **camada de portabilidade**, não uma engine.

Ela existe para responder a uma pergunta: *"como eu abro uma janela, leio um
controle e toco um som sem escrever código diferente para cada sistema
operacional?"* Tudo o que ela faz deriva disso.

O que ela te dá:

- janela e contexto gráfico (ou um renderizador 2D pronto)
- eventos de teclado, mouse, toque, gamepad
- áudio: abrir um dispositivo, empurrar amostras
- tempo: relógios monotônicos, espera
- abstrações de sistema: arquivos, threads, memória, bibliotecas dinâmicas

O que ela **não** te dá, e é bom encarar isso agora:

- nenhum conceito de "sprite", "cena", "objeto de jogo", "animação"
- nenhuma física, colisão ou tween
- nenhuma interface de usuário
- nenhum carregador de PNG/JPG (isso é a SDL_image, biblioteca separada)
- nenhum sistema de fontes (isso é a SDL_ttf, separada — com uma exceção
  que veremos no Capítulo 12)
- nenhum mixer de áudio de alto nível (isso é a SDL_mixer, separada)

> 📐 **Conceito.** A SDL é o *chão*, não a *casa*. A arquitetura do seu jogo
> — como você organiza notas, tempo, estado, telas — é inteiramente sua. Isso
> é mais trabalho que usar uma engine, e é exatamente o que você quer: é onde
> o aprendizado acontece, e é o que permite caber em 1 GB de RAM.

### 1.2 Subsistemas

A SDL é dividida em subsistemas que você liga conforme precisa:

```c
if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
    SDL_Log("SDL_Init falhou: %s", SDL_GetError());
    return 1;
}
```

Ligar só o que você usa não é frescura de otimização: cada subsistema abre
dispositivos, cria threads e registra callbacks. No handheld, `SDL_INIT_AUDIO`
abre o dispositivo ALSA; se você não precisa dele naquele momento, não pague
por ele.

`SDL_Quit()` no fim desliga tudo.

### 1.3 As convenções da API SDL3

Aprenda estas cinco regras agora e você lerá qualquer função da SDL3 sem
consultar a documentação.

**1. Funções que podem falhar retornam `bool`.**

```c
if (!SDL_SetRenderVSync(ren, 1)) {
    SDL_Log("vsync indisponível: %s", SDL_GetError());
}
```

Esta é uma mudança importante da SDL2, onde elas retornavam `int` (0 = sucesso,
negativo = erro). Se você achar um tutorial escrevendo `if (SDL_Init(...) < 0)`,
esse tutorial é de SDL2. O código compila na SDL3 e faz a coisa errada em
silêncio — `true` é 1, então `< 0` nunca é verdade, e o erro passa batido.

⚠️ **Armadilha.** Esse é o erro de porte SDL2→SDL3 mais comum e mais
traiçoeiro. Como você tem cursos gravados, é bem provável que sejam de SDL2.
Ao assistir, traduza mentalmente: *retorno `int` vira `bool` invertido*.

**2. Funções que retornam ponteiro retornam `NULL` em erro.**

```c
SDL_Texture *t = SDL_CreateTexture(...);
if (!t) { /* SDL_GetError() */ }
```

**3. `SDL_GetError()` só é válido logo após uma falha.** Ela retorna a última
mensagem de erro daquela thread. Chamar outra função da SDL no meio pode
sobrescrevê-la. Leia o erro imediatamente, ou copie a string.

**4. Quem cria, destrói.** Todo `SDL_CreateXxx` tem um `SDL_DestroyXxx`
correspondente, e a ordem de destruição é a inversa da criação: textura,
renderer, janela, `SDL_Quit`.

**5. Memória devolvida pela SDL se libera com `SDL_free`**, não com `free`.

```c
int n;
SDL_JoystickID *ids = SDL_GetGamepads(&n);
/* ... usa ... */
SDL_free(ids);
```

A SDL pode ter seu próprio alocador. Misturar `malloc`/`SDL_free` ou
`SDL_malloc`/`free` é corrupção de heap esperando acontecer.

### 1.4 A forma canônica de um programa SDL

Todo programa SDL tem esta forma, e o *Ark Hero* não vai ser diferente:

```c
SDL_Init(...);                    /* 1. inicializa      */
/* cria janela, renderer, carrega recursos */
                                  /* 2. loop principal: */
while (rodando) {
    while (SDL_PollEvent(&e)) { } /*    2a. drena eventos */
    /* atualiza o estado do jogo */ /*  2b. simula        */
    /* desenha */                  /*    2c. apresenta   */
}
/* destrói recursos */
SDL_Quit();                       /* 3. finaliza        */
```

Os passos 2a, 2b e 2c serem coisas distintas não é detalhe de estilo: é o que
vai permitir, mais tarde, rodar a simulação com um passo de tempo fixo
derivado do áudio enquanto o desenho acontece na taxa da tela. Guarde isso.

### 🔧 Exercício 1

Pegue o [src/main.c](src/main.c) do repositório e leia-o inteiro. Identifique:

1. onde estão os cinco pontos de convenção da seção 1.3;
2. quais subsistemas são inicializados e por quê;
3. a ordem exata de destruição no fim, e por que ela é essa.

Depois, modifique-o para inicializar também `SDL_INIT_AUDIO` e imprimir o
driver de áudio em uso (`SDL_GetCurrentAudioDriver()`). Rode no desktop e no
R36S e compare a saída. No handheld você deve ver `alsa` ou `pipewire`.

---

## Capítulo 2 — Janela, renderer e textura: os três objetos

### 2.1 Os quatro tipos que você precisa distinguir

Boa parte da confusão inicial com SDL vem de embaralhar estes quatro:

| Tipo | O que é | Onde vive |
|---|---|---|
| `SDL_Window` | a área onde o jogo aparece | sistema de janelas |
| `SDL_Renderer` | o "pincel": sabe desenhar naquela janela | abstrai GL/GLES/Vulkan/software |
| `SDL_Texture` | pixels prontos para desenhar | **memória da GPU** |
| `SDL_Surface` | pixels manipuláveis um a um | **memória da CPU** |

> 📐 **Conceito — a distinção que mais importa.** `Surface` e `Texture`
> parecem a mesma coisa e não são. Uma `Surface` é um array de pixels na RAM:
> você pode ler e escrever cada um, mas a GPU não desenha a partir dela. Uma
> `Texture` está na memória da GPU: desenhar com ela é praticamente de graça,
> mas mexer nos pixels é caro, porque exige transferir dados de volta pelo
> barramento.
>
> A regra prática: **`Surface` é para construir, `Texture` é para desenhar.**
> Você gera pixels numa Surface uma vez, converte para Texture, e daí em
> diante só desenha. É exatamente o que a função `make_ball()` faz.

### 2.2 O ciclo do frame

```c
SDL_SetRenderDrawColor(ren, 18, 18, 24, 255);  /* cor de fundo */
SDL_RenderClear(ren);                          /* limpa tudo   */

/* ... todos os desenhos do frame ... */

SDL_RenderPresent(ren);                        /* mostra       */
```

Três coisas para entender sobre esse ciclo:

**`SDL_RenderClear` não é opcional.** Sem ele, você desenha por cima do frame
anterior. Às vezes isso até parece funcionar; depois vira rastro de imagem.

**`SDL_RenderPresent` não "desenha na tela".** Ele troca os buffers: o que
você desenhou no buffer de trás passa a ser o da frente. Isso se chama *double
buffering*, e existe para você nunca ver um frame pela metade.

**Com vsync ligado, `SDL_RenderPresent` bloqueia.** Ele espera o monitor
terminar de varrer a tela. É por isso que o seu teste da bola marcou 60,1 FPS:
não é o programa sendo rápido, é ele sendo *segurado* 60 vezes por segundo.

```c
SDL_SetRenderVSync(ren, 1);   /* 1 = sincroniza com a tela */
```

> 🎮 **No R36S.** O painel é de 60 Hz. Com vsync, seu loop roda a 60 FPS e
> cada frame tem 16,6 ms de orçamento. Se a lógica + desenho passarem disso,
> você perde o vsync e cai direto para 30 FPS — não há meio-termo. Esse é o
> número que você deve ter na cabeça o tempo todo: **16,6 ms**.

### 2.3 Criando os três

```c
SDL_Window *win = SDL_CreateWindow("Ark Hero", 640, 480, flags);
SDL_Renderer *ren = SDL_CreateRenderer(win, NULL);  /* NULL = SDL escolhe */
```

`SDL_CreateWindowAndRenderer()` faz os dois de uma vez. É conveniente, mas
**prefira separar**: quando algo falha, você quer saber qual dos dois falhou.
Foi exatamente essa separação que tornou legível o erro do R36S.

### 🔧 Exercício 2

Escreva um programa que desenha um quadrado parado no centro da tela. Depois:

1. comente o `SDL_RenderClear` e mova o quadrado — observe o rastro;
2. desligue o vsync (`SDL_SetRenderVSync(ren, 0)`) e meça os FPS;
3. rode a versão sem vsync no R36S e compare com o desktop.

A diferença nos dois números é o seu primeiro dado real de desempenho do
hardware.

---

## Capítulo 3 — Compatibilidade: o capítulo que normalmente vem por último

Este assunto quase sempre é o último capítulo dos cursos, ou um apêndice. Aqui
ele é o terceiro, por um motivo simples: **você já bateu nele**. O primeiro
teste do *Ark Hero* no R36S morreu com tela preta, e o que causou isso é o
tema desta seção. Entender agora custa uma hora; entender depois de ter 5000
linhas de código custa uma semana.

### 3.1 As duas camadas de backend

Quando você cria uma janela e um renderer, duas escolhas independentes
acontecem:

```
 seu código
     │
     ├── driver de VÍDEO  → como falar com a tela e o input
     │      x11 · wayland · kmsdrm · offscreen · dummy
     │
     └── driver de RENDER → como desenhar
            opengl · opengles2 · vulkan · gpu · software
```

São camadas separadas, e confundi-las é fonte de diagnóstico errado. No seu
log do R36S:

```
video em uso  : kmsdrm      ← camada 1
render em uso : opengles2   ← camada 2
```

Listar o que existe no aparelho:

```c
for (int i = 0; i < SDL_GetNumVideoDrivers(); i++)
    SDL_Log("%s", SDL_GetVideoDriver(i));

for (int i = 0; i < SDL_GetNumRenderDrivers(); i++)
    SDL_Log("%s", SDL_GetRenderDriver(i));
```

E forçar um, sem recompilar, pelas variáveis de ambiente:

```bash
SDL_VIDEODRIVER=kmsdrm  ./arkhero.aarch64
SDL_RENDER_DRIVER=software ./arkhero.aarch64
```

> 📐 **Conceito.** Poder trocar backend por variável de ambiente é a
> ferramenta de diagnóstico mais barata que existe. Se o jogo funciona com
> `software` e falha com `opengles2`, você isolou o problema na GPU em dez
> segundos, sem recompilar, sem depurador.

### 3.2 A pilha gráfica de um handheld Linux

No desktop, entre o seu jogo e a tela existe um servidor gráfico (X11 ou
Wayland) que gerencia janelas. No R36S não existe nada disso: o jogo fala
direto com o hardware. A pilha é esta, de baixo para cima:

```
   hardware: Mali-G31 MP2 + controlador de display
        ↑
   KMS/DRM   — o driver do kernel. Define modo de vídeo (640x480@60),
               entrega framebuffers para a tela varrer.
        ↑
   GBM       — "Generic Buffer Management". Aloca buffers de imagem
               que tanto a GPU quanto o display conseguem usar.
        ↑
   EGL       — a ponte entre o sistema de janelas (aqui, GBM) e a API
               de desenho (aqui, OpenGL ES). Cria contextos e superfícies.
        ↑
   OpenGL ES — a API com que se desenha de fato.
        ↑
   SDL (driver de vídeo kmsdrm + driver de render opengles2)
        ↑
   Ark Hero
```

Quando o seu log disse `Could not create EGL window surface`, a falha foi na
quarta camada. As três de baixo tinham funcionado.

### 3.3 Caso de estudo: a tela preta do R36S

Vale reconstruir o episódio inteiro, porque ele ensina um padrão que você vai
reencontrar.

**O sintoma.** Tela preta por um instante, programa encerra, volta para o
EmulationStation. No `log.txt`:

```
SDL_CreateWindowAndRenderer falhou: Could not create EGL window surface
```

**Passo 1 — localizar a mensagem.** Em vez de adivinhar, procurar a string
literal no fonte da SDL:

```bash
grep -rn "Could not create EGL window surface" src/
# src/video/kmsdrm/SDL_kmsdrmvideo.c:1824
```

> 📐 **Método.** Toda mensagem de erro da SDL existe numa linha de código.
> Você tem o fonte. Procurar a string é sempre mais rápido e mais confiável
> que procurar no fórum. Faça isso *antes* de pesquisar na internet.

**Passo 2 — ler o entorno.** A função é `KMSDRM_CreateSurfaces`, e a ordem
das operações conta a história:

```c
uint32_t surface_fmt = GBM_FORMAT_ARGB8888;

windata->gs = KMSDRM_gbm_surface_create(...);   /* ← passou */
if (!windata->gs)
    return SDL_SetError("Could not create GBM surface: %s", strerror(errno));

SDL_EGL_SetRequiredVisualId(_this, surface_fmt);
windata->egl_surface = SDL_EGL_CreateSurface(...);  /* ← falhou aqui */
if (windata->egl_surface == EGL_NO_SURFACE) {
    result = SDL_SetError("Could not create EGL window surface");
```

Isso já elimina metade das hipóteses: o **GBM funcionou**. O buffer foi
alocado, o KMS estava acessível, o modo de vídeo foi definido. Se o problema
fosse permissão em `/dev/dri/card0` ou EmulationStation segurando a tela, a
falha teria sido antes, com outra mensagem.

**Passo 3 — descer mais um nível.** `SDL_EGL_CreateSurface` começa assim:

```c
if (!SDL_EGL_ChooseConfig(_this)) {
    return EGL_NO_SURFACE;
}
```

Então o suspeito é a escolha do **EGLConfig**.

> 📐 **Conceito — o que é um EGLConfig.** Antes de desenhar, é preciso
> escolher o *formato* da superfície: quantos bits por canal de cor, se há
> canal alfa, quanto de buffer de profundidade, e — crucialmente — **para
> qual API de desenho** ela serve. O driver publica uma lista de combinações
> que o hardware suporta, e o programa pede a que quer. Se nenhuma bate com o
> pedido, não há superfície.

**Passo 4 — achar o critério.** Em `src/video/SDL_egl.c`, por volta da linha
823, está o pedido que a SDL monta:

```c
attribs[i++] = EGL_RENDERABLE_TYPE;
if (_this->gl_config.profile_mask == SDL_GL_CONTEXT_PROFILE_ES) {
    ...
    attribs[i++] = EGL_OPENGL_ES2_BIT;
} else {
    attribs[i++] = EGL_OPENGL_BIT;       /* ← OpenGL desktop */
}
```

Aqui está tudo. O critério é `gl_config.profile_mask`. Como o código não
tinha declarado perfil nenhum, `profile_mask` valia 0, e a SDL pediu
`EGL_OPENGL_BIT`: **OpenGL de desktop**.

O Mali-G31 com driver Panfrost só implementa OpenGL **ES**. Não existe nenhum
EGLConfig com `EGL_OPENGL_BIT` naquele aparelho. Nenhum config bate, a escolha
falha, e a superfície não nasce.

**Passo 5 — por que só no handheld?** Porque no seu Arch, o Mesa oferece
configs para OpenGL desktop *e* para OpenGL ES. O mesmo pedido encontra
resposta, e o programa funciona. O bug estava lá o tempo todo; o desktop
apenas o escondia.

**A causa raiz.** Em KMS/DRM, a SDL cria a superfície EGL **junto com a
janela**, antes de o renderer existir. O comentário no próprio fonte é
explícito:

```c
/* We can't get the EGL context yet because SDL_CreateRenderer has not been
   called, but we need an EGL surface NOW, or GL won't be able to render into
   any surface and we won't see the first frame. */
```

Ou seja: no instante da criação da janela, a SDL **precisa** saber qual API
você vai usar, mas você só diria isso depois, ao criar o renderer. Essa
inversão é a armadilha.

**A correção** — declarar a intenção antes de criar a janela:

```c
SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

SDL_Window *win = SDL_CreateWindow("Ark Hero", 640, 480,
                                   SDL_WINDOW_OPENGL | flags);
```

> 📐 **A lição geral, que vale muito além deste caso: declare sua intenção
> antes de criar recursos.** APIs gráficas negociam formatos e capacidades no
> momento da criação, não do uso. Tudo que você quiser — perfil de GL,
> profundidade de cor, buffer de profundidade, multisampling — precisa estar
> declarado *antes* do `SDL_CreateWindow`. Depois é tarde: o recurso já
> nasceu com um formato.

### 3.4 Por que uma lib compilada no Debian roda no dArkOS

Vale entender, porque contradiz a intuição.

A `libSDL3.so` do pacote foi compilada num Debian 12 com um Mesa específico.
No R36S há outro Mesa, outra versão do ALSA, outro kernel. Por que funciona?

Porque a SDL **não linka** com essas bibliotecas. Ela as carrega em tempo de
execução, por nome, com `dlopen()`. É o que o resumo do build informa:

```
Video drivers: kmsdrm(dynamic) wayland(dynamic) x11(dynamic)
Audio drivers: alsa(dynamic) pipewire(dynamic) pulseaudio(dynamic)
```

O `(dynamic)` quer dizer: "sei falar com isso, e vou procurar a biblioteca no
sistema quando precisar". Se existir, usa; se não, tenta o próximo backend.

> ⚠️ **Armadilha.** É por isso que **não** se deve linkar a SDL
> estaticamente para estes aparelhos. Estático congelaria essas dependências,
> e você perderia exatamente a propriedade que faz uma build servir a vários
> firmwares.

### 3.5 Checklist de compatibilidade

Grave esta lista; ela cobre quase todos os problemas deste tipo de aparelho:

1. **Declare o perfil GLES antes de criar a janela.** (Capítulo 3.3.)
2. **Nunca assuma um backend.** Sempre registre o que foi realmente escolhido.
3. **Registre o ambiente antes de criar qualquer coisa**, para que a falha
   tenha contexto no log.
4. **Tenha um caminho de recuo.** Se `opengles2` falhar, tente `software`:
   feio e lento, mas distingue "GPU não coopera" de "programa quebrado".
5. **Permita forçar o backend por variável de ambiente**, para depurar sem
   recompilar.
6. **Teste no hardware cedo e com frequência.** Esse bug era invisível no
   desktop. Não existe substituto para rodar no aparelho.
7. **Não instale nada no sistema do console.** Tudo que o jogo precisa viaja
   dentro da pasta do port.

### 🔧 Exercício 3

1. Rode o `arkhero.aarch64` no R36S forçando `SDL_RENDER_DRIVER=software` e
   compare os FPS com `opengles2`. A diferença é a sua primeira medida do
   ganho real da GPU naquele aparelho.
2. Force `SDL_VIDEODRIVER=dummy` e observe o caminho de recuo entrando em ação.
3. Abra `src/video/SDL_egl.c` do fonte em `build/sdl3-src-aarch64/` e leia a
   função `SDL_EGL_ChooseConfig` inteira. Responda: o que ela faz quando
   encontra vários configs compatíveis? Qual critério de desempate usa?

---

# Parte II — Desenho

---

## Capítulo 4 — Coordenadas e resolução lógica

### 4.1 Coordenadas em ponto flutuante

Na SDL3, as funções de desenho trabalham com `float`, não `int`:

```c
SDL_FRect r = { 100.5f, 200.0f, 64.0f, 64.0f };   /* x, y, w, h */
SDL_RenderTexture(ren, tex, NULL, &r);
```

A origem é o **canto superior esquerdo**, com Y crescendo para baixo. Isso é
convenção de computação gráfica 2D, e vai contra a intuição de quem vem da
matemática.

> 📐 **Por que float importa para você.** Numa highway de jogo de ritmo, as
> notas descem continuamente. Se a posição fosse inteira, uma nota a 200
> pixels/segundo avançaria 3,33 px por frame e seria arredondada para 3, 3, 4,
> 3, 3, 4 — movimento com trepidação visível. Com float, o movimento é suave
> e o arredondamento acontece só na rasterização. **Guarde posições em float
> sempre.**

### 4.2 Resolução lógica

O R36S tem 640×480. O seu monitor tem outra coisa. Você não quer dois códigos
de layout.

```c
SDL_SetRenderLogicalPresentation(ren, 640, 480,
                                 SDL_LOGICAL_PRESENTATION_LETTERBOX);
```

A partir daí, **você desenha sempre num mundo de 640×480** e a SDL cuida de
escalar para a tela real. Os modos disponíveis:

| Modo | Comportamento |
|---|---|
| `DISABLED` | sem escala; coordenadas são pixels reais |
| `STRETCH` | estica para preencher, distorcendo a proporção |
| `LETTERBOX` | preserva a proporção, com barras pretas |
| `OVERSCAN` | preserva a proporção, cortando o excedente |
| `INTEGER_SCALE` | só múltiplos inteiros (2×, 3×…), pixels nítidos |

Para o *Ark Hero*: **`LETTERBOX`**. No R36S a escala é 1:1 e não há barra
nenhuma — a proporção é exatamente 4:3. No seu monitor widescreen, aparecem
barras laterais e o jogo fica idêntico ao do aparelho. É isso que você quer:
desenvolver vendo o mesmo enquadramento do alvo.

> ⚠️ **Armadilha.** Com resolução lógica ativa, as coordenadas dos eventos
> de mouse/toque também são convertidas. Mas `SDL_GetWindowSizeInPixels()`
> continua devolvendo o tamanho **real**. Não use esse valor para posicionar
> nada; use sempre suas constantes lógicas.

### 🔧 Exercício 4

Desenhe uma moldura de 1 px na borda exata da área lógica e um X ligando os
cantos. Rode no desktop redimensionando a janela, e depois no R36S. A moldura
deve permanecer colada nas bordas nos dois casos. Experimente os cinco modos
de apresentação e descreva a diferença de cada um.

---

## Capítulo 5 — Desenhando: primitivas e texturas

### 5.1 Primitivas

```c
SDL_SetRenderDrawColor(ren, 255, 60, 60, 255);      /* R, G, B, A */

SDL_RenderPoint(ren, x, y);
SDL_RenderLine(ren, x1, y1, x2, y2);
SDL_RenderRect(ren, &frect);        /* contorno  */
SDL_RenderFillRect(ren, &frect);    /* preenchido */
```

Há versões em lote — `SDL_RenderLines`, `SDL_RenderFillRects` — que recebem
um array. **Prefira sempre as versões em lote**: uma chamada com 50 retângulos
custa muito menos que 50 chamadas. Voltaremos a isso no Capítulo 8.

Note o que **não** existe: círculo, elipse, polígono, linha com espessura,
curva. A SDL desenha ponto, linha, retângulo e triângulo. Nada mais. Círculos
você constrói (Capítulo 6) ou compõe com triângulos (Capítulo 7).

### 5.2 Texturas: os três tipos de acesso

```c
SDL_Texture *t = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32,
                                   acesso, largura, altura);
```

O parâmetro `acesso` define o que você poderá fazer com ela, e escolher errado
custa desempenho:

| Acesso | Significado | Quando usar |
|---|---|---|
| `STATIC` | carregada raramente, desenhada muito | **quase tudo**: notas, fundo, HUD |
| `STREAMING` | os pixels mudam com frequência | vídeo, efeito calculado na CPU |
| `TARGET` | pode receber desenhos | compor cenas, pré-renderizar |

> ⚠️ **Armadilha.** `STREAMING` parece a escolha flexível e é uma armadilha
> de desempenho. Ela implica transferir pixels da CPU para a GPU a cada
> atualização. Num A35, atualizar uma textura de tela cheia por frame consome
> uma fatia enorme do seu orçamento de 16,6 ms. Use `STATIC` a menos que você
> tenha uma razão medida para o contrário.

O caminho mais comum de criar uma textura é a partir de uma Surface:

```c
SDL_Surface *s = SDL_CreateSurface(64, 64, SDL_PIXELFORMAT_RGBA32);
/* ... escreve os pixels ... */
SDL_Texture *t = SDL_CreateTextureFromSurface(ren, s);
SDL_DestroySurface(s);          /* a Surface já cumpriu seu papel */
```

Repare que a Surface é destruída logo em seguida: os pixels já foram copiados
para a GPU. Manter as duas é desperdiçar RAM — coisa que você não tem sobrando.

### 5.3 Blend, modulação e filtragem

**Blend mode** define como os pixels novos se combinam com os que já estão lá:

```c
SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);  /* alfa normal */
```

| Modo | Efeito |
|---|---|
| `NONE` | sobrescreve, ignora alfa — o mais rápido |
| `BLEND` | transparência normal |
| `ADD` | soma luz: brilhos, star power, partículas |
| `MOD` | multiplica: sombras, escurecimento |

**Modulação** multiplica cor e alfa da textura na hora de desenhar, sem tocar
nos pixels originais:

```c
SDL_SetTextureColorMod(t, 255, 200, 0);   /* tinge de amarelo */
SDL_SetTextureAlphaMod(t, 128);           /* 50% transparente */
```

> 📐 **Isto vale ouro para o seu jogo.** Você **não** precisa de cinco
> texturas de nota, uma por cor. Gere **uma** nota branca e use
> `SDL_SetTextureColorMod` para produzir verde, vermelho, amarelo, azul e
> laranja. Uma textura, cinco cores, memória de uma só — e, como veremos no
> Capítulo 8, sem quebrar o lote de desenho.

**Filtragem** define o que acontece quando a textura é ampliada ou reduzida:

```c
SDL_SetTextureScaleMode(t, SDL_SCALEMODE_NEAREST);  /* pixel duro   */
SDL_SetTextureScaleMode(t, SDL_SCALEMODE_LINEAR);   /* interpolado  */
```

Em 640×480, use `LINEAR` para as notas (que encolhem conforme sobem a highway
e ficariam serrilhadas com `NEAREST`) e `NEAREST` para texto de bitmap e
elementos de HUD desenhados em escala 1:1, que devem ficar nítidos.

### 🔧 Exercício 5

Gere **uma** textura circular branca de 64×64. Desenhe cinco cópias lado a
lado, nas cinco cores dos trastes do Guitar Hero, usando apenas
`SDL_SetTextureColorMod`. Meça o uso de memória e compare mentalmente com a
alternativa de cinco texturas. Em seguida, faça as cinco pulsarem de tamanho
em fases diferentes.

---

## Capítulo 6 — Procedural ou sprites? A pergunta certa

Você perguntou se desenhar as notas proceduralmente sairia mais barato que
usar sprites. A resposta honesta começa por desfazer a pergunta: **"procedural"
significa quatro coisas diferentes**, com custos que diferem em ordens de
grandeza.

### 6.1 As quatro coisas que "procedural" pode significar

**(a) Calcular os pixels na CPU a cada frame.**
Escrever num buffer, mandar para uma textura `STREAMING`, desenhar.

**(b) Calcular os pixels na CPU uma vez, na carga.**
Gerar a imagem no início, converter para textura, e daí em diante só desenhar.

**(c) Montar a forma com geometria colorida.**
Triângulos com cor por vértice, sem textura (`SDL_RenderGeometry`).

**(d) Calcular no shader, na GPU.**
O jeito moderno — e indisponível pelo `SDL_Renderer`, que não expõe shaders.
Exigiria escrever GLES2 cru.

### 6.2 O custo de cada uma

Para uma nota de 64×64 desenhada 200 vezes por frame, a 60 FPS:

| | CPU por frame | GPU por frame | Memória | Veredito |
|---|---|---|---|---|
| (a) pixels na CPU | **altíssimo** | baixo | baixa | ❌ nunca |
| (b) gerado na carga | ~zero | baixo | 16 KB | ✅ **a escolha** |
| (c) geometria colorida | baixo | médio | zero | ⚠️ casos específicos |
| (d) shader | ~zero | baixo | zero | fora do SDL_Renderer |

Vamos aos números que sustentam a tabela.

**Opção (a) é fora de questão.** Uma textura de 64×64 RGBA tem 16.384 bytes.
Regenerá-la a cada frame significa calcular 4.096 pixels — cada um com uma
raiz quadrada, se houver antialiasing — e depois transferi-la para a GPU. Para
a tela inteira (640×480 RGBA = 1,2 MB por frame, 73 MB/s a 60 FPS) isso sozinho
estoura o orçamento de um Cortex-A35. Você teria queimado o frame antes de
desenhar qualquer coisa.

**Opção (b) é indistinguível de um sprite em tempo de execução.** E essa é a
chave da resposta:

> 📐 **A percepção central.** Depois que os pixels estão numa `SDL_Texture`,
> a GPU não sabe nem se importa de onde vieram. Desenhar uma textura gerada
> por código e desenhar uma textura carregada de um PNG têm **exatamente o
> mesmo custo**. "Procedural" e "sprite" não são alternativas concorrentes no
> loop de jogo: são duas origens diferentes para o mesmo objeto.

Ou seja, a pergunta real não é sobre desempenho. É sobre **de onde vêm os
pixels**, e aí procedural ganha por outros motivos:

- não depende de artista nem de arquivos de imagem para começar
- não precisa da SDL_image no pacote (menos uma dependência no container e no
  port — e você já viu o que custa uma dependência a mais)
- os parâmetros ficam em código: mudar o raio, a espessura da borda ou o
  degradê é editar um número, não reabrir um editor de imagem
- resolução é livre: gere 64×64 no handheld e 128×128 num alvo maior, do mesmo
  código
- o repositório fica minúsculo

E perde em: formas orgânicas, ilustração, qualquer coisa com estilo artístico
de verdade. Um círculo com borda e brilho você gera em 30 linhas. Uma chama
estilizada, não.

**Opção (c) tem um nicho real.** Geometria colorida não gasta memória nenhuma
e é ótima para formas grandes e chapadas — a própria highway, as faixas, os
degradês de fundo, o medidor de star power. Mas para as notas ela é pior:
um círculo decente pede uns 20 triângulos, contra 2 de um quad texturizado, e
você perde o antialiasing suave da borda (que na textura sai de graça, no
canal alfa).

**Conclusão prática para o *Ark Hero*:**

> **Gere proceduralmente na carga, para dentro de um atlas, e desenhe como
> sprite.** Use geometria colorida para a highway e superfícies grandes. Nunca
> calcule pixels no loop.

### 6.3 Dimensionando a memória

Vale confirmar que o custo é irrelevante mesmo. Um atlas de 512×512 em RGBA:

```
512 × 512 × 4 bytes = 1.048.576 bytes ≈ 1 MB
```

Um MB. Nele cabem, com folga: notas em várias escalas, sustentação, star
power, brilho de acerto, marcadores de faixa, uma fonte bitmap completa. Com
896 MB de RAM no aparelho, isso é ruído estatístico. **Memória de textura não
é o seu problema** — o áudio pré-mixado, esse sim (dezenas de MB por música).

### 6.4 Como se gera uma forma decente

O padrão é sempre o mesmo: para cada pixel, calcular uma **distância** até a
forma ideal e converter essa distância em alfa.

```c
const float d = SDL_sqrtf(dx * dx + dy * dy);   /* distância ao centro */
float a = (raio - d) / 1.5f;                    /* 1,5 px de transição */
if (a > 1.0f) a = 1.0f;
if (a < 0.0f) a = 0.0f;
```

Dentro do raio, `a` satura em 1 (opaco). Fora, em 0 (transparente). Na faixa
de 1,5 px da borda ele varia suavemente — e isso *é* antialiasing. Sem essa
transição, o círculo fica com a borda serrilhada.

A partir dessa ideia você constrói tudo o que precisa:

| Forma | Como |
|---|---|
| anel | `fabsf(d - raio) < espessura` |
| brilho | alfa proporcional a `1 - d/raio`, com `BLENDMODE_ADD` |
| retângulo arredondado | distância ao retângulo interno, não ao centro |
| degradê | alfa ou cor em função de `y` |

> 🔧 Este padrão tem nome — *signed distance field* — e é a mesma matemática
> que os shaders modernos usam. Você está aprendendo uma técnica que se
> transfere inteira para GLES2 se um dia você migrar.

### 🔧 Exercício 6

Escreva um módulo `art.c` com uma função por forma, todas devolvendo
`SDL_Texture *` e todas chamadas **uma vez** na carga:

1. `art_note_circle(ren, raio)` — círculo com borda suave;
2. `art_note_ring(ren, raio, espessura)` — anel;
3. `art_glow(ren, raio)` — brilho radial para usar com `BLENDMODE_ADD`;
4. `art_lane_marker(ren, w, h)` — retângulo arredondado.

Desenhe as quatro em fileira no R36S e registre o FPS. Depois, como
experimento, mova a chamada da (1) para dentro do loop, regenerando a textura
a cada frame, e registre o FPS de novo. **Anote os dois números.** Essa
diferença é a lição inteira deste capítulo, medida no seu hardware.

---

## Capítulo 7 — Geometria: triângulos e a highway

### 7.1 SDL_RenderGeometry

Quando retângulos não bastam — e uma estrada em perspectiva é o caso —, você
desce para triângulos:

```c
bool SDL_RenderGeometry(SDL_Renderer *renderer,
                        SDL_Texture *texture,          /* pode ser NULL */
                        const SDL_Vertex *vertices, int num_vertices,
                        const int *indices, int num_indices);
```

```c
typedef struct SDL_Vertex {
    SDL_FPoint position;    /* em coordenadas do renderer */
    SDL_FColor color;       /* FLOAT 0.0–1.0, não bytes! */
    SDL_FPoint tex_coord;   /* 0.0–1.0, normalizado */
} SDL_Vertex;
```

⚠️ **Armadilha.** `SDL_FColor` usa `float` de 0,0 a 1,0. Na SDL2 era
`SDL_Color`, com bytes de 0 a 255. Escrever `255.0f` aqui satura tudo em
branco. Tutoriais de SDL2 vão te induzir a esse erro.

Com `texture = NULL`, os triângulos são preenchidos pela cor dos vértices,
interpolada. É a opção (c) do capítulo anterior.

### 7.2 O problema da perspectiva

Este ponto é específico o bastante para valer um aviso destacado.

> ⚠️ **`SDL_RenderGeometry` interpola de forma afim, não corrigida em
> perspectiva.** Se você desenhar a highway como um trapézio feito de dois
> triângulos grandes, a textura vai **entortar visivelmente na diagonal** que
> divide os dois. Não é bug seu: é a limitação do renderizador 2D, que não
> carrega a coordenada W de profundidade.

Duas saídas:

**Subdividir** — corte a highway em 20 a 40 faixas horizontais, cada uma um
trapézio pequeno. O erro de interpolação é proporcional ao tamanho do trecho:
com faixas finas, ele vira invisível. Custo: 40 quads por frame, que é nada.
Continua tudo dentro do `SDL_Renderer`, portável.

**GLES2 cru** — matriz de projeção de verdade, correção exata. Mais controle,
muito mais código, e você abre mão da portabilidade do `SDL_Renderer`.

**Comece subdividindo.** Se mais tarde quiser efeitos que o `SDL_Renderer` não
dá — brilho no star power, desfoque de fundo, distorção da highway —, aí a
migração para GLES2 se justifica. Não antes.

### 7.3 Índices

O parâmetro `indices` evita repetir vértices. Um quad tem 4 cantos, mas 2
triângulos exigiriam 6 vértices — com 2 repetidos. Com índices:

```c
SDL_Vertex v[4] = { /* os 4 cantos */ };
int idx[6] = { 0, 1, 2,   2, 1, 3 };   /* dois triângulos */
SDL_RenderGeometry(ren, tex, v, 4, idx, 6);
```

Com 40 faixas de highway isso economiza bastante trabalho de CPU montando
vértices — e é CPU que você não tem sobrando.

### 🔧 Exercício 7

Desenhe uma highway estática em perspectiva: um trapézio que vai de estreito
no topo (horizonte) a largo embaixo (o jogador), com cinco faixas verticais
de cores diferentes.

1. Primeiro com **dois triângulos**, usando uma textura quadriculada. Observe
   a distorção na diagonal.
2. Depois com **32 faixas horizontais**. Compare.
3. Meça o FPS das duas versões no R36S — a diferença deve ser desprezível, e
   esse é o ponto: a solução correta é praticamente de graça.

---

## Capítulo 8 — CPU ou GPU? Onde o trabalho realmente acontece

### 8.1 O que acontece quando você chama SDL_RenderTexture

Não é um desenho imediato. A SDL **acumula** comandos e os envia à GPU em
lotes (*batches*). Um lote vira uma chamada de desenho ao driver.

> 📐 **Conceito.** O custo não está em quantos pixels a GPU pinta — ela é
> boa nisso. Está em **quantas vezes a CPU interrompe a GPU** para trocar o
> estado. Cada troca é uma conversa com o driver, e é a CPU que paga.

### 8.2 O que quebra um lote

Enquanto o estado não muda, a SDL continua acumulando. Estas mudanças forçam
o fechamento do lote e a abertura de outro:

| Quebra o lote | Não quebra |
|---|---|
| trocar de textura | mudar posição/tamanho |
| trocar o blend mode | trocar cor/alfa de modulação |
| mudar o clip rect | rotacionar, espelhar |
| trocar o render target | — |
| chamar `SDL_RenderGeometry` com outra textura | — |

Repare na segunda coluna: **`SDL_SetTextureColorMod` não quebra o lote.** É
o que permite a economia do Capítulo 5 — cinco cores de nota com uma textura
só, tudo num lote, sem custo adicional.

### 8.3 Atlas

Daí vem a técnica mais importante do capítulo. Em vez de uma textura por
elemento, coloque tudo numa só e desenhe recortes:

```c
/* Todas as notas num atlas de 256x256 */
SDL_FRect src = { 0, 0, 64, 64 };      /* recorte dentro do atlas */
SDL_FRect dst = { x, y, 64, 64 };      /* onde vai na tela        */
SDL_RenderTexture(ren, atlas, &src, &dst);
```

Com todas as notas no mesmo atlas, 200 notas na tela viram **um** lote. Com
uma textura por tipo de nota, viram cinco ou mais — e você paga a troca de
estado a cada uma.

> 🎮 **No R36S isso importa mais que no desktop.** O Mali-G31 MP2 é uma GPU
> pequena e a CPU que a alimenta é fraca. É exatamente a configuração em que o
> custo por chamada de desenho domina. Num PC você não notaria a diferença
> entre 1 e 20 lotes; ali você nota.

**Regra de organização:** desenhe agrupando por textura, não por objeto.
Primeiro todo o fundo, depois toda a highway, depois todas as notas, depois
todo o HUD. Se você intercalar (nota, HUD, nota, HUD), quebra o lote a cada
alternância.

### 8.4 Renderização em tiles e o custo do alfa

O Mali-G31 é uma GPU *tile-based*: ela divide a tela em blocos pequenos e
processa um de cada vez em memória interna, rápida. Isso a torna muito
eficiente em consumo de energia — e sensível a uma coisa específica.

> ⚠️ **Overdraw com transparência é o que dói.** Cada camada translúcida
> empilhada sobre o mesmo pixel é trabalho somado, porque a GPU não pode
> descartar o que está atrás: precisa misturar. Uma tela cheia de brilho
> translúcido sobre a highway sobre o fundo é três vezes a tela.

Na prática:

- use `SDL_BLENDMODE_NONE` para o fundo (é o mais barato — sobrescreve);
- reserve `BLEND` e `ADD` para o que realmente precisa;
- desconfie de efeitos de tela cheia. Um brilho de star power cobrindo tudo
  pode custar mais que todas as notas juntas;
- prefira o efeito concentrado onde ele se vê.

### 8.5 Render target: pré-renderizar o que não muda

Se um conjunto de elementos é caro de montar e muda pouco, desenhe-o uma vez
numa textura e reuse:

```c
SDL_Texture *hud = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32,
                                     SDL_TEXTUREACCESS_TARGET, 640, 480);
SDL_SetRenderTarget(ren, hud);
/* ... desenha o HUD ... */
SDL_SetRenderTarget(ren, NULL);      /* volta para a tela */

/* nos frames seguintes: um desenho só */
SDL_RenderTexture(ren, hud, NULL, NULL);
```

Bom para: fundo estático, moldura da highway, texto que muda uma vez por
segundo. Ruim para: qualquer coisa que muda todo frame — aí você paga a troca
de render target (que quebra o lote) sem ganhar nada.

### 8.6 A lista do que não fazer no loop

Estes cinco erros custam caro e são fáceis de cometer:

1. **Criar ou destruir texturas** dentro do loop. Aloque tudo na carga.
2. **`SDL_LockTexture` / `SDL_UpdateTexture`** por frame. É transferência
   CPU→GPU no caminho crítico.
3. **Carregar arquivos.** I/O de microSD trava por dezenas de milissegundos.
4. **Alocar memória** por frame. `malloc` numa lista fragmentada é
   imprevisível. Pré-aloque arrays.
5. **Processar objetos invisíveis.** Com as notas ordenadas por tempo, mantenha
   dois índices deslizantes — primeira e última visível — e percorra só a
   janela. Custo O(1) por frame, não O(n).

### 8.7 Meça antes de otimizar

```c
const Uint64 t0 = SDL_GetPerformanceCounter();
/* ... trecho suspeito ... */
const Uint64 t1 = SDL_GetPerformanceCounter();
const double ms = (double)(t1 - t0) * 1000.0 / SDL_GetPerformanceFrequency();
```

> 🎮 Meça **no R36S**, sempre. A proporção entre CPU e GPU lá não se parece
> nada com a do seu notebook. Otimizar com base nos números do desktop é
> otimizar o hardware errado.

### 🔧 Exercício 8

Desenhe 200 notas em movimento e meça o FPS no R36S em quatro versões:

1. cinco texturas separadas, uma por cor;
2. uma textura, cinco cores por `SDL_SetTextureColorMod`;
3. igual à (2), mas alternando nota e elemento de HUD a cada desenho;
4. igual à (2), mas agrupando tudo por textura.

Monte a tabela com os quatro números. Ela é o seu manual de desempenho
particular, medido no hardware que importa — vale mais que qualquer conselho
genérico, inclusive os deste capítulo.

---

# Parte III — Tempo, som e input

---

## Capítulo 9 — Tempo

Chegamos ao assunto que decide o *Ark Hero*. Um jogo de ritmo é, no fundo, um
programa que responde a uma única pergunta, sessenta vezes por segundo: **"em
que instante da música estamos agora?"** Se a resposta erra por 30 ms, o jogo
está quebrado, por mais bonito que esteja.

### 9.1 Os relógios da SDL

```c
Uint64 SDL_GetTicks(void);                  /* milissegundos desde SDL_Init */
Uint64 SDL_GetTicksNS(void);                /* nanossegundos                */
Uint64 SDL_GetPerformanceCounter(void);     /* unidades de alta resolução   */
Uint64 SDL_GetPerformanceFrequency(void);   /* unidades por segundo         */
```

Os três são **monotônicos**: nunca andam para trás, não são afetados por
mudança de fuso ou de relógio do sistema. Nunca use `time()` ou `clock()` para
lógica de jogo.

Para medir um intervalo com precisão:

```c
const Uint64 t0 = SDL_GetPerformanceCounter();
/* ... */
const double segundos = (double)(SDL_GetPerformanceCounter() - t0)
                      / SDL_GetPerformanceFrequency();
```

### 9.2 Delta time

O tempo entre frames varia. Se você mover objetos por frame, a velocidade
muda com o FPS — e o jogo fica mais rápido no desktop que no handheld. A
correção é medir o intervalo e multiplicar:

```c
const Uint64 agora = SDL_GetTicks();
float dt = (float)(agora - anterior) / 1000.0f;
anterior = agora;

if (dt > 0.05f) dt = 0.05f;      /* trava de segurança */

posicao += velocidade * dt;      /* pixels/segundo × segundos */
```

⚠️ **A trava não é opcional.** Se o sistema travar por meio segundo — leitura
de microSD, troca de contexto, o que for —, sem ela `dt` vale 0,5 e todo
objeto salta meio segundo de deslocamento de uma vez, atravessando paredes e
zonas de detecção. Limitar a 50 ms transforma um teleporte em uma leve
desaceleração.

### 9.3 Timestep fixo

Delta variável é simples, mas tem um defeito: a simulação produz resultados
levemente diferentes conforme o FPS, porque a integração numérica depende do
tamanho do passo. Para lógica que precisa ser reproduzível, o padrão é fixar
o passo e acumular o tempo:

```c
static double acumulador = 0.0;
const double PASSO = 1.0 / 120.0;     /* 120 Hz de simulação */

acumulador += dt;
while (acumulador >= PASSO) {
    simular(PASSO);                   /* sempre o mesmo passo */
    acumulador -= PASSO;
}
desenhar();                           /* na taxa da tela */
```

Para o *Ark Hero*, a detecção de acerto se beneficia disso: janelas de tempo
avaliadas em passos constantes são mais fáceis de raciocinar e de testar.

### 9.4 E por que nada disso serve como relógio mestre

Agora o ponto central do capítulo — e possivelmente da apostila inteira.

> ⚠️ **`SDL_GetTicks()` não pode ser a fonte da verdade sobre a posição na
> música.**

O motivo é físico. O relógio do sistema e o relógio do chip de áudio são
osciladores de cristal **independentes**. Dois cristais nunca têm a mesma
frequência exata: há sempre um desvio de algumas partes por milhão. Numa
música de quatro minutos, um desvio de 50 ppm — perfeitamente normal — dá:

```
240 segundos × 50/1.000.000 = 12 milissegundos
```

Doze milissegundos de deriva acumulada. Isso é audível, e num jogo de ritmo é
a diferença entre "Perfect" e "Good" em todas as notas do fim da música. E o
desvio é **cumulativo**: quanto mais longa a música, pior. O jogador sente
que o jogo "descola" do som com o tempo, e nenhuma calibração conserta,
porque não é um deslocamento constante — é um *drift*.

> 📐 **A regra.** O relógio mestre de um jogo de ritmo é o **áudio**: a
> posição é derivada de quantas amostras o hardware **já reproduziu**. Só
> isso mede o mesmo cristal que está produzindo o som que o jogador ouve.
> `SDL_GetTicks()` serve para animação, efeitos e medição de desempenho.
> Nunca para posicionar notas.

O próximo capítulo mostra como construir esse relógio.

### 🔧 Exercício 9

Escreva um programa que mostra na tela, lado a lado:

1. `SDL_GetTicks() / 1000.0` — segundos pelo relógio do sistema;
2. um contador de frames dividido por 60 — "tempo" pela contagem de frames.

Deixe rodando por cinco minutos no R36S e anote a diferença final. Você vai
ver a segunda métrica derivar da primeira. Agora imagine que uma delas é a
sua música e a outra são as suas notas: é exatamente esse o problema.

---

## Capítulo 10 — Áudio e o relógio mestre

### 10.1 O modelo da SDL3

A SDL3 reformulou o áudio em torno de um objeto: o `SDL_AudioStream`. Ele é,
ao mesmo tempo, fila e conversor — aceita dados num formato e entrega noutro,
reamostrando se preciso.

```c
SDL_AudioSpec spec = {
    .format   = SDL_AUDIO_S16,   /* 16 bits com sinal */
    .channels = 2,
    .freq     = 48000
};

SDL_AudioStream *stream = SDL_OpenAudioDeviceStream(
    SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);

SDL_ResumeAudioStreamDevice(stream);    /* começa pausado! */
```

⚠️ **O dispositivo abre pausado.** Esquecer o `SDL_ResumeAudioStreamDevice` é
o primeiro erro de todo mundo: nenhum som, nenhum erro, nenhuma pista.

Passando `NULL` como callback, você mesmo empurra os dados:

```c
SDL_PutAudioStreamData(stream, buffer, tamanho_em_bytes);
```

Passando uma função, ela é chamada quando o dispositivo precisa de mais dados.

> 📐 **A conversão é automática e vale muito.** Se o arquivo é 44.100 Hz e o
> dispositivo quer 48.000 Hz, o `SDL_AudioStream` reamostra sozinho. Mas
> reamostragem custa CPU. No *Ark Hero*, como você vai pré-mixar a música na
> carga, faça a conversão **uma vez ali** e entregue ao dispositivo o formato
> nativo dele. Descubra qual é com:
>
> ```c
> SDL_AudioSpec dev;
> SDL_GetAudioDeviceFormat(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &dev, NULL);
> ```

### 10.2 Construindo o relógio mestre

Esta é a função mais importante do jogo inteiro.

A ideia: você sabe quantos frames de áudio **enviou** ao stream. Você pode
perguntar quantos ainda estão **na fila**, não consumidos. A diferença é
quanto o hardware já reproduziu.

```c
int SDL_GetAudioStreamQueued(SDL_AudioStream *stream);
```

A documentação do cabeçalho é precisa em dois pontos que importam: o valor
está **nos bytes do formato de entrada** (o que você colocou), e a função é
**segura de chamar de qualquer thread**. As duas coisas são exatamente o que
você precisa.

```c
/* bytes por frame de áudio: canais × tamanho da amostra */
const int bpf = spec.channels * SDL_AUDIO_BYTESIZE(spec.format);

double ark_audio_clock(void)
{
    const int    pendentes_bytes  = SDL_GetAudioStreamQueued(stream);
    const Sint64 pendentes_frames = pendentes_bytes / bpf;
    const Sint64 reproduzidos     = frames_enviados - pendentes_frames;

    return (double)reproduzidos / (double)spec.freq
         - latencia_dispositivo      /* medida por calibração */
         - offset_do_chart           /* vem do .chart/song.ini */
         - offset_do_usuario;        /* ajuste manual */
}
```

Onde `frames_enviados` é um contador que você incrementa a cada
`SDL_PutAudioStreamData`.

> 📐 **Por que isto resolve a deriva.** O numerador conta amostras que o
> *chip de áudio* consumiu, no ritmo do *cristal do chip de áudio*. É o mesmo
> cristal que está gerando o som nos ouvidos do jogador. Não há dois relógios
> para divergir — só há um.

### 10.3 Os três offsets

Note os três termos subtraídos. Eles são fenômenos distintos e devem ficar
separados:

| Offset | O que é | Como se obtém |
|---|---|---|
| latência do dispositivo | buffer do driver + conversão digital-analógica | calibração com metrônomo |
| offset do chart | o autor da música gravou um deslocamento | lido do `.chart` / `song.ini` |
| offset do usuário | gosto pessoal, atraso da tela | menu de ajuste |

⚠️ **Confundir os três é o bug clássico do gênero.** Se você somar tudo numa
constante só, um dia vai trocar de música e não entender por que desregulou.
Mantenha três variáveis separadas, e escreva testes que verifiquem o sinal de
cada uma — errar um sinal desloca tudo para o lado errado, e o sintoma parece
idêntico ao de errar a magnitude.

### 10.4 Latência e tamanho de buffer

```
512 frames a 48 kHz  = 10,7 ms
1024 frames          = 21,3 ms
```

Comece com 512. Se houver estalos (*underrun* — o dispositivo pediu dados e
não havia), suba para 1024.

> 📐 **Latência constante não é problema; latência variável é.** 21 ms de
> atraso fixo se compensa inteiramente pela calibração — o jogador nem
> percebe. Já um atraso que oscila entre 10 e 30 ms é impossível de compensar
> e torna o jogo impreciso. Prefira o buffer maior e estável ao menor e
> instável.

### 10.5 Regras do callback de áudio

Se você usar callback, ele roda numa **thread de tempo real**. Atrasar ali
produz estalo audível imediato. Dentro dele, nunca:

- alocar ou liberar memória
- abrir, ler ou escrever arquivos
- pegar um mutex que outra thread possa segurar por muito tempo
- chamar Lua (é a regra do Capítulo 15, e aqui ela é absoluta)
- decodificar áudio comprimido

O callback deve fazer uma coisa só: **copiar bytes já prontos**. Todo o
trabalho pesado acontece antes, na carga ou numa thread de decodificação com
buffer circular.

### 🔧 Exercício 10

Este é o exercício mais importante da apostila.

1. Carregue um `.wav` com `SDL_LoadWAV` e toque-o.
2. Mostre na tela, em tempo real, o valor de `ark_audio_clock()`.
3. Mostre ao lado `SDL_GetTicks() / 1000.0`, zerado no início da música.
4. Desenhe um marcador visual que pulsa a cada segundo **do relógio de áudio**.
5. Deixe tocando uma música de **quatro minutos inteiros** no R36S e anote a
   diferença entre os dois números no fim.

Essa diferença é a deriva de que fala o Capítulo 9, medida no seu hardware.
Quando o marcador ainda estiver alinhado com o som aos quatro minutos, você
terá resolvido o problema central do projeto — e o resto vira trabalho, não
risco.

---

## Capítulo 11 — Input

### 11.1 Eventos ou estado?

A SDL oferece as duas formas, e em jogos de ritmo a distinção é essencial.

**Eventos** são a fila do que aconteceu:

```c
SDL_Event e;
while (SDL_PollEvent(&e)) {
    if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) { /* ... */ }
}
```

**Estado** é uma foto do agora:

```c
bool verde = SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_SOUTH);
```

> 📐 **A mecânica do gênero exige as duas.** Quando a palheta dispara (um
> *evento*), você precisa saber quais trastes estão segurados **naquele
> instante** (o *estado*). Não é "quais trastes receberam evento de pressão
> neste frame" — o jogador segura os trastes antes e mantém.
>
> Portanto: palheta por evento, trastes por estado, consultados no momento em
> que o evento chega.

### 11.2 Timestamps: 8 ms de graça

Todo evento da SDL3 carrega o instante em que ocorreu:

```c
typedef struct SDL_CommonEvent {
    Uint32 type;
    Uint32 reserved;
    Uint64 timestamp;   /* nanossegundos, via SDL_GetTicksNS() */
} SDL_CommonEvent;
```

Isso é mais valioso do que parece. Você processa eventos uma vez por frame; um
botão pressionado logo depois do processamento anterior esperou até 16,6 ms
para ser visto. Se você julgar o acerto pelo instante do *frame*, introduz um
erro de até um frame inteiro — em média 8 ms, numa janela de acerto de 70 ms.
É mais de 10% da janela, jogado fora por nada.

> 📐 **Use `e.gbutton.timestamp`, não o instante do frame**, para converter
> em posição na música e julgar o acerto. São alguns milissegundos de precisão
> ganhos de graça, e num jogo de ritmo isso se sente.

### 11.3 Gamepad, não joystick

A SDL tem duas APIs para controles:

- **`SDL_Joystick`** — cru. Botão 7, eixo 3. Cada controle é diferente.
- **`SDL_Gamepad`** — mapeado. `SDL_GAMEPAD_BUTTON_SOUTH` é sempre o botão de
  baixo, em qualquer controle reconhecido.

**Use `SDL_Gamepad`.** O mapeamento vem de uma base de dados interna, e o
PortMaster injeta a configuração do aparelho pela variável
`SDL_GAMECONTROLLERCONFIG` — que o script de launch do *Ark Hero* já exporta.
É por isso que os botões do R36S "simplesmente funcionam".

Os nomes seguem a posição física, não o rótulo:

```
SDL_GAMEPAD_BUTTON_SOUTH   /* embaixo  — "A" no Xbox, "B" no Nintendo */
SDL_GAMEPAD_BUTTON_EAST    /* direita  */
SDL_GAMEPAD_BUTTON_WEST    /* esquerda */
SDL_GAMEPAD_BUTTON_NORTH   /* em cima  */
```

⚠️ Não presuma que `SOUTH` se chama "A" no aparelho do jogador. Para exibir
na tela, pergunte à SDL o nome real com `SDL_GetGamepadStringForButton()`.

### 11.4 Conexão a quente

Controles entram e saem durante a partida:

```c
case SDL_EVENT_GAMEPAD_ADDED:
    pad = SDL_OpenGamepad(e.gdevice.which);
    break;
case SDL_EVENT_GAMEPAD_REMOVED:
    SDL_CloseGamepad(pad);
    pad = NULL;
    break;
```

No R36S o controle é fixo e isso parece desnecessário. Você vai querer quando
ligar um controle USB — e é o primeiro passo do suporte a outros controles que
está no seu plano.

### 🔧 Exercício 11

Escreva um visualizador de input que mostra, na tela do R36S:

1. o nome do controle detectado;
2. um retângulo por botão, aceso enquanto **segurado** (estado);
3. um registro rolante dos últimos 10 eventos com o `timestamp` em ms;
4. a diferença, em ms, entre o `timestamp` do evento e o instante em que o seu
   loop o processou.

O número (4) é a precisão que você ganha ao usar o timestamp do evento. Anote
o valor típico e o pior caso.

---

## Capítulo 12 — Texto

### 12.1 A SDL3 não tem sistema de fontes… com uma exceção

Renderizar texto é surpreendentemente complexo: glifos, kerning, Unicode,
escrita bidirecional. A SDL deixa isso para a SDL_ttf.

Mas a SDL3 ganhou uma função embutida que resolve muito caso prático:

```c
bool SDL_RenderDebugText(SDL_Renderer *renderer, float x, float y,
                         const char *str);
```

Uma fonte de bitmap 8×8 embutida na biblioteca, sem dependência nenhuma.
O nome diz "debug", e para depuração ela é perfeita — mas ela também é, com
honestidade, suficiente para um HUD funcional: contador de combo, pontuação,
porcentagem, FPS.

> 🎮 **Recomendação para o *Ark Hero*.** Use `SDL_RenderDebugText` para tudo
> no começo. Ela custa zero em dependências, zero no container, zero no
> pacote. Você adia inteiramente a decisão sobre fontes — e adiar decisões que
> não são urgentes é como este projeto vai chegar ao fim.

### 12.2 As três opções, quando chegar a hora

| Opção | Prós | Contras |
|---|---|---|
| `SDL_RenderDebugText` | zero dependência, já funciona | 8×8, só ASCII, sem estilo |
| fonte bitmap própria | controle total, rápida, atlas | você desenha cada glifo |
| SDL_ttf | qualquer `.ttf`, Unicode, qualidade | +1 dependência no container e no pacote |

A **fonte bitmap própria** é o meio-termo natural: você gera um atlas de
glifos na carga — exatamente a técnica do Capítulo 6 — e desenha cada
caractere como um recorte. Rápido, bonito, sem dependência, e batendo com o
estilo visual do resto que você gerou proceduralmente.

### 12.3 O problema que vai aparecer: nomes de música

Aqui está o motivo real pelo qual texto vai te dar trabalho, e não é o HUD.

A biblioteca do Clone Hero tem músicas com títulos em japonês, coreano,
russo, com acentos, com emoji. O navegador de músicas precisa exibir isso.

- `SDL_RenderDebugText` cobre ASCII. Um título japonês vira lixo.
- Uma fonte bitmap própria com cobertura Unicode ampla é inviável de desenhar
  à mão — são milhares de glifos.
- SDL_ttf com uma fonte de boa cobertura resolve, e passa a ser justificável.

> 📐 **Decisão de escopo sugerida.** HUD e menus com
> `SDL_RenderDebugText`, agora. Quando o navegador de músicas entrar
> (marco 9 do roadmap), avalie a SDL_ttf **só para o navegador**. Até lá, para
> caracteres fora do ASCII, substitua por `?` e siga em frente — isso não
> impede ninguém de jogar.

### 🔧 Exercício 12

1. Desenhe um HUD com `SDL_RenderDebugText`: FPS, tempo do relógio de áudio,
   combo fictício e pontuação.
2. Meça o custo de desenhar 50 linhas de texto por frame no R36S.
3. Gere proceduralmente um atlas de fonte com os dígitos 0–9 em tamanho
   grande, e desenhe a pontuação com ele. Compare o resultado visual com o da
   fonte de debug.

---

# Parte IV — Juntando as peças

---

## Capítulo 13 — Anatomia de um jogo SDL3

### 13.1 As três camadas

Conforme o projeto cresce, um `main.c` único vira um nó. A separação que
funciona — e que você deve adotar desde cedo — é esta:

```
┌─────────────────────────────────────────┐
│  PLATAFORMA                             │  janela, input, áudio, tempo
│  conhece a SDL                          │  arquivos, log
├─────────────────────────────────────────┤
│  SIMULAÇÃO                              │  notas, acerto, combo, estado
│  NÃO conhece a SDL                      │  pontuação, star power
├─────────────────────────────────────────┤
│  APRESENTAÇÃO                           │  highway, sprites, HUD
│  conhece a SDL e a simulação            │  efeitos
└─────────────────────────────────────────┘
```

> 📐 **O critério que vale a pena defender: a camada de simulação não inclui
> `SDL.h`.** Ela recebe o tempo atual e o estado do input como parâmetros, e
> devolve o que aconteceu. Nada mais.

Três coisas decorrem disso, e todas são grandes:

1. **Você consegue testar a lógica sem abrir janela.** Alimente uma sequência
   de notas e instantes de palheta, verifique os acertos. Um teste que roda em
   milissegundos, no desktop, sem hardware — para a parte do jogo em que os
   bugs são mais difíceis de ver a olho nu.
2. **Você pode trocar o backend gráfico** (`SDL_Renderer` hoje, GLES2 amanhã)
   sem tocar na lógica.
3. **É o que torna a integração com Lua limpa depois.** A fronteira já existe.

### 13.2 O loop, completo

Juntando tudo o que a apostila cobriu:

```c
while (rodando) {
    /* 1. TEMPO — a posição vem do áudio, não do sistema */
    const double musica_t = ark_audio_clock();

    /* 2. INPUT — eventos com seus timestamps */
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_EVENT_QUIT:
            rodando = false;
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            /* converte o timestamp do evento para posição na música */
            sim_input(&jogo, &e, musica_t);
            break;
        }
    }

    /* 3. SIMULAÇÃO — sem SDL aqui dentro */
    sim_update(&jogo, musica_t);

    /* 4. APRESENTAÇÃO */
    SDL_RenderClear(ren);
    draw_highway(ren, &jogo);        /* geometria subdividida  */
    draw_notes(ren, &jogo, atlas);   /* um lote, um atlas      */
    draw_hud(ren, &jogo);            /* por último, agrupado   */
    SDL_RenderPresent(ren);          /* bloqueia até o vsync   */
}
```

Cada linha aqui corresponde a um capítulo. Se algum passo não estiver claro,
volte ao capítulo correspondente antes de escrever código.

### 13.3 Recursos: carregue tudo na carga

```c
typedef struct {
    SDL_Texture *atlas;
    SDL_Texture *fundo;
    SDL_AudioStream *musica;
    /* ... */
} ArkAssets;
```

Carregue no início da música, libere no fim, **nunca no meio**. A tela de
detalhes da música existe justamente para dar cobertura a esse tempo de carga
— aproveite-a.

### 🔧 Exercício 13

Reorganize o seu código atual em três arquivos — `platform.c`, `sim.c`,
`draw.c` — e adicione ao `CMakeLists.txt`. A regra a respeitar: `sim.c` não
pode incluir `SDL3/SDL.h`. Se você precisar de algo da SDL ali dentro, é sinal
de que a fronteira está no lugar errado — mova o que for de plataforma para
`platform.c` e passe o resultado como parâmetro.

---

## Capítulo 14 — Erros, log e depuração no aparelho

### 14.1 O log é o seu único instrumento

No R36S não há depurador, nem terminal visível, nem janela de console. Se o
jogo fechar, tudo o que existe é o `log.txt`. Trate-o como o instrumento
principal, não como um detalhe.

```c
SDL_Log("mensagem comum");
SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "algo falhou: %s", SDL_GetError());
SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);   /* liga tudo */
```

### 14.2 Três regras aprendidas na prática

Estas vieram diretamente do episódio do Capítulo 3.

**1. Registre o ambiente antes de criar qualquer coisa.** A primeira versão do
`main.c` imprimia o driver de vídeo *depois* de criar a janela. Quando a
criação falhou, o log não dizia sequer em que backend a falha aconteceu.
Inverter essa ordem foi o que tornou o problema diagnosticável.

**2. Separe os passos que podem falhar.** `SDL_CreateWindowAndRenderer` é
conveniente e esconde qual dos dois falhou. Duas chamadas separadas custam
três linhas e entregam um log que aponta o culpado.

**3. Tenha um caminho de recuo que distingue as hipóteses.** Se `opengles2`
falhar e `software` funcionar, o log sozinho já provou que o problema é o
backend gráfico, e não o programa. Sem isso você ficaria olhando para uma tela
preta sem saber por onde começar.

### 14.3 Depuração remota

Com SSH no aparelho, dá para rodar fora do EmulationStation e ver a saída
ao vivo:

```bash
cd /roms/ports/arkhero
LD_LIBRARY_PATH=./libs.aarch64 ARKHERO_FULLSCREEN=1 ./arkhero.aarch64
```

E testar hipóteses sem recompilar:

```bash
SDL_RENDER_DRIVER=software  ./arkhero.aarch64   # é a GPU?
SDL_VIDEODRIVER=kmsdrm      ./arkhero.aarch64   # é o backend de vídeo?
SDL_LOGGING=app=verbose     ./arkhero.aarch64   # mais detalhe
```

> 📐 **Construa os seus próprios interruptores.** Toda hipótese que você
> puder testar por variável de ambiente é um ciclo de recompilar-e-copiar que
> você não paga. O `ARKHERO_RENDER_DRIVER` do código atual existe por isso.

### 14.4 Quando a mensagem não bastar, leia o fonte

Foi assim que o bug do EGL caiu:

```bash
grep -rn "a mensagem exata" build/sdl3-src-aarch64/src/
```

Você tem o fonte completo da SDL3 no repositório. Toda mensagem de erro dela
existe numa linha de código, com o contexto ao redor explicando as condições.
Isso é mais rápido e mais confiável que procurar no fórum — e ensina muito
mais.

### 🔧 Exercício 14

Escreva um módulo de log com um cabeçalho de diagnóstico padrão: versão da
SDL, driver de vídeo, driver de render, driver de áudio, formato e tamanho do
buffer de áudio, controles detectados, memória livre. Chame-o no início de
todo boot. Quando alguém reportar um problema, esse cabeçalho vai responder
metade das perguntas antes de você perguntar.

---

## Capítulo 15 — Onde a Lua vai entrar

Um capítulo curto, para fechar a ponte com a outra metade dos seus estudos.

### 15.1 A fronteira

| C | Lua |
|---|---|
| callback e mixagem de áudio | regras de pontuação e multiplicador |
| relógio mestre | lógica do star power |
| parsing de `.chart` e mapa de tempo | definição de tema e cores |
| detecção de acerto e janelas | fluxo de menus, filtros da lista |
| renderização e batching | mapeamento de controles |
| leitura de input | modificadores e condições de falha |

O critério é único: **C para o que roda por frame ou por amostra; Lua para o
que roda por evento ou é configuração.**

### 15.2 A linha vermelha

> ⚠️ **Nunca chame Lua por nota por frame.** Atravessar a fronteira
> C↔Lua custa centenas de nanossegundos num A35. Com 200 notas visíveis a 60
> FPS, isso seriam milhões de travessias por minuto — e você teria trocado
> todo o desempenho que ganhou com atlas e batching por conveniência de
> script.
>
> Chame Lua em **eventos discretos**: `on_note_hit`, `on_note_miss`,
> `on_combo_change`, `on_star_power_activate`, `on_song_end`. São dezenas de
> chamadas por segundo. O custo desaparece.

E dentro do callback de áudio, a regra é absoluta: **Lua nunca entra ali.**
O coletor de lixo pode disparar a qualquer momento, e uma pausa na thread de
áudio é um estalo audível.

### 15.3 Sobre a versão 5.5

Você decidiu usar Lua 5.5 em vez de 5.4, pelos ganhos de gerenciamento de
memória. É uma decisão razoável e o custo dela é conhecido: menos material de
consulta, e diferenças na API C em relação ao que a maior parte dos tutoriais
mostra.

Há uma precaução barata que reduz bastante esse custo, e vale adotá-la desde a
primeira linha: **concentre toda chamada à API C da Lua num único arquivo**
— um `script.c` com funções como `script_init()`, `script_on_note_hit()`,
`script_reload()`. O resto do jogo conversa só com essas funções, e nunca vê
um `lua_State *`.

Isso te dá três coisas: se a API mudar, você corrige um arquivo; se um
tutorial de 5.4 mostrar um padrão que não existe mais na 5.5, a adaptação fica
contida; e se um dia a 5.5 se mostrar um obstáculo, trocar de versão é uma
tarde de trabalho, não uma reescrita.

### 15.4 Recarga a quente

Implemente cedo: um botão que relê os scripts sem reiniciar a música. Ajustar
a janela de acerto, a velocidade da highway ou a curva do multiplicador
enquanto a música toca transforma o balanceamento de "compilar, copiar,
testar, repetir" em "apertar um botão". É o maior ganho isolado de ter Lua no
projeto, e é o motivo pelo qual vale a pena tê-la.

---

# Apêndices

---

## Apêndice A — Roteiro de exercícios

Os exercícios em ordem. Cada um produz uma peça reutilizável do jogo.

| # | Exercício | Entrega | Capítulo |
|---|---|---|---|
| 1 | Leitura do `main.c` + driver de áudio | familiaridade com a API | 1 |
| 2 | Quadrado, rastro, vsync | o ciclo do frame | 2 |
| 3 | Trocar backends, ler `SDL_EGL_ChooseConfig` | diagnóstico | 3 |
| 4 | Moldura na área lógica | resolução lógica resolvida | 4 |
| 5 | Cinco cores, uma textura | economia por modulação | 5 |
| 6 | `art.c` com quatro geradores | **arte do jogo** | 6 |
| 7 | Highway em perspectiva subdividida | **a estrada** | 7 |
| 8 | Quatro medições de batching | manual de desempenho | 8 |
| 9 | Deriva entre dois relógios | entender o problema | 9 |
| 10 | **Relógio de áudio na tela** | **o coração do jogo** | 10 |
| 11 | Visualizador de input | input resolvido | 11 |
| 12 | HUD com texto | HUD funcional | 12 |
| 13 | Separar em três camadas | arquitetura | 13 |
| 14 | Cabeçalho de diagnóstico | depurabilidade | 14 |

**O exercício 10 é o divisor de águas.** Se o marcador continuar alinhado com
o som depois de quatro minutos de música, o risco técnico do projeto acabou.

## Apêndice B — Referência rápida

Todas verificadas contra a SDL 3.4.16 que está em `third_party/src/`.

**Ciclo de vida**
```c
bool  SDL_Init(SDL_InitFlags flags);
void  SDL_Quit(void);
const char *SDL_GetError(void);
```

**Janela e renderer**
```c
SDL_Window   *SDL_CreateWindow(const char *title, int w, int h, SDL_WindowFlags flags);
SDL_Renderer *SDL_CreateRenderer(SDL_Window *window, const char *name);
bool SDL_SetRenderVSync(SDL_Renderer *renderer, int vsync);
bool SDL_SetRenderLogicalPresentation(SDL_Renderer *renderer, int w, int h,
                                      SDL_RendererLogicalPresentation mode);
const char *SDL_GetRendererName(SDL_Renderer *renderer);
```

**Desenho**
```c
bool SDL_SetRenderDrawColor(SDL_Renderer *r, Uint8 red, Uint8 g, Uint8 b, Uint8 a);
bool SDL_RenderClear(SDL_Renderer *renderer);
bool SDL_RenderLine(SDL_Renderer *r, float x1, float y1, float x2, float y2);
bool SDL_RenderFillRect(SDL_Renderer *r, const SDL_FRect *rect);
bool SDL_RenderTexture(SDL_Renderer *r, SDL_Texture *t,
                       const SDL_FRect *srcrect, const SDL_FRect *dstrect);
bool SDL_RenderGeometry(SDL_Renderer *r, SDL_Texture *texture,
                        const SDL_Vertex *vertices, int num_vertices,
                        const int *indices, int num_indices);
bool SDL_RenderDebugText(SDL_Renderer *r, float x, float y, const char *str);
bool SDL_RenderPresent(SDL_Renderer *renderer);
```

**Texturas**
```c
SDL_Texture *SDL_CreateTexture(SDL_Renderer *r, SDL_PixelFormat format,
                               SDL_TextureAccess access, int w, int h);
SDL_Texture *SDL_CreateTextureFromSurface(SDL_Renderer *r, SDL_Surface *surface);
bool SDL_SetTextureBlendMode(SDL_Texture *t, SDL_BlendMode blendMode);
bool SDL_SetTextureColorMod(SDL_Texture *t, Uint8 r, Uint8 g, Uint8 b);
bool SDL_SetTextureAlphaMod(SDL_Texture *t, Uint8 alpha);
bool SDL_SetTextureScaleMode(SDL_Texture *t, SDL_ScaleMode scaleMode);
bool SDL_SetRenderTarget(SDL_Renderer *r, SDL_Texture *texture);
```

**Superfícies**
```c
SDL_Surface *SDL_CreateSurface(int width, int height, SDL_PixelFormat format);
const SDL_PixelFormatDetails *SDL_GetPixelFormatDetails(SDL_PixelFormat format);
Uint32 SDL_MapRGBA(const SDL_PixelFormatDetails *format, const SDL_Palette *palette,
                   Uint8 r, Uint8 g, Uint8 b, Uint8 a);
void SDL_DestroySurface(SDL_Surface *surface);
```

**Tempo**
```c
Uint64 SDL_GetTicks(void);
Uint64 SDL_GetTicksNS(void);
Uint64 SDL_GetPerformanceCounter(void);
Uint64 SDL_GetPerformanceFrequency(void);
```

**Áudio**
```c
SDL_AudioStream *SDL_OpenAudioDeviceStream(SDL_AudioDeviceID devid,
                                           const SDL_AudioSpec *spec,
                                           SDL_AudioStreamCallback callback,
                                           void *userdata);
bool SDL_ResumeAudioStreamDevice(SDL_AudioStream *stream);
bool SDL_PutAudioStreamData(SDL_AudioStream *stream, const void *buf, int len);
int  SDL_GetAudioStreamQueued(SDL_AudioStream *stream);
bool SDL_GetAudioDeviceFormat(SDL_AudioDeviceID devid, SDL_AudioSpec *spec,
                              int *sample_frames);
bool SDL_LoadWAV(const char *path, SDL_AudioSpec *spec,
                 Uint8 **audio_buf, Uint32 *audio_len);
```

**Input**
```c
bool SDL_PollEvent(SDL_Event *event);
SDL_JoystickID *SDL_GetGamepads(int *count);
SDL_Gamepad *SDL_OpenGamepad(SDL_JoystickID instance_id);
bool SDL_GetGamepadButton(SDL_Gamepad *gamepad, SDL_GamepadButton button);
Sint16 SDL_GetGamepadAxis(SDL_Gamepad *gamepad, SDL_GamepadAxis axis);
const char *SDL_GetGamepadStringForButton(SDL_GamepadButton button);
```

## Apêndice C — Erros comuns e o que significam

| Mensagem / sintoma | Causa provável |
|---|---|
| `Could not create EGL window surface` | perfil GLES não declarado antes da janela (Cap. 3) |
| `Could not create GBM surface` | problema mais embaixo: KMS, permissão em `/dev/dri` |
| tela preta, sem erro | faltou `SDL_RenderPresent`, ou desenho fora da área lógica |
| rastro de imagem | faltou `SDL_RenderClear` |
| áudio mudo, sem erro | faltou `SDL_ResumeAudioStreamDevice` |
| estalos no áudio | buffer pequeno, ou trabalho pesado no callback |
| notas descolam ao longo da música | relógio mestre não é o áudio (Cap. 9) |
| jogo rápido no PC, lento no handheld | movimento por frame, sem delta time |
| objetos atravessam paredes após travada | faltou a trava do `dt` |
| tudo branco no `SDL_RenderGeometry` | `SDL_FColor` recebeu 0–255 em vez de 0,0–1,0 |
| FPS cai de 60 para 30 | perdeu o vsync: o frame passou de 16,6 ms |
| `if (SDL_Init(...) < 0)` nunca detecta erro | código de SDL2: na SDL3 o retorno é `bool` |

---

## Uma palavra final

A ordem desta apostila não é a ordem da documentação da SDL, e é de propósito.
Ela segue o risco: o que pode inviabilizar o *Ark Hero* vem primeiro, o que é
trabalho vem depois.

Por isso compatibilidade é o Capítulo 3 e não um apêndice — você já perdeu uma
tarde com isso. Por isso tempo e áudio ocupam o centro. Por isso texto,
que costuma abrir os cursos, é o penúltimo: é o assunto que menos ameaça o
projeto.

Se você fizer os exercícios na ordem, ao chegar no 10 terá nas mãos a resposta
para a única pergunta que ainda pode matar o projeto: *as notas continuam
alinhadas com o som depois de quatro minutos?* Com um "sim", tudo o que resta
é construir — e construir é a parte fácil.
