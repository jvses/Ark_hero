# Progresso do Ark Hero (branch `claudio`)

Registro de cada etapa feita na branch `claudio`: o que foi pedido, o que foi
feito e o que falta conferir. O código desta branch é escrito pelo Claude sob
a direção do João. Cada etapa é testada no R36S antes do commit.

## Roteiro

| # | Etapa | Estado |
|---|---|---|
| 0 | Arrumar a casa: módulos, input com remapeamento, menus, pista | feita e testada no R36S (commit `f0f7eb9`) |
| 1 | Jogabilidade com notas de demonstração: juiz (modo controle), sustains, whammy, star power, placar | feita, testada no PC; falta o R36S |
| 2 | Áudio: Opus, Vorbis e WAV, mistura das faixas, relógio mestre | feita, testada no PC (commit `31548dd`); falta o R36S |
| 3 | Parser de `.chart` + mapa de tempo + testes | feita e jogada no PC; falta o R36S |
| 4 | Pista com as notas de uma música de verdade, escolha de música e dificuldade | feita junto com a 3 |
| 4b | Ajustes: janela de acerto, modo precisão, logs, procura de músicas, navegação, placar | feita e jogada no PC (teclado) |
| 4c | Recordes, organizar por pastas/título/artista, opções salvas | feita, testada no PC e no R36S |
| 4d | `.mid` e MP3, HOPO na pista, penalidade, ajuste de tempo, botões frontais por posição | feita, testada no R36S |
| 4e | Carga em paralelo, acorde exato, licenças e pasta `songs/` no pacote | **feita; falta testar** |
| 5 | Lua: `config.lua` e `regras.lua`, com recarga a quente | |
| 6 | Calibração de áudio e vídeo | |
| 7 | Biblioteca de músicas + cache | |
| 8 | Modo guitarra (com palhetada e HOPO) | |

---

## Etapa 0: arrumar a casa (2026-10-27)

### Pedido

- Revisar o código, limpar comentários desnecessários e estruturar os módulos
  para se complementarem com a lógica do jogo.
- Melhorar o estilo dos menus, com botões para as opções que o jogo terá no
  futuro, mesmo que ainda não funcionem.
- Aproveitar e melhorar as funções existentes, como o desenho da pista (tab).
- Input reconhecendo os outros botões do R36S.
- Trastes no R36S: Verde L1, Vermelho R2, Amarelo L2, Azul L1 (ver abaixo),
  Laranja B. Preparar um meio de cadastrar/alterar os botões de cada cor.
- Manter este arquivo de progresso a cada etapa.

### O que foi feito

**Estrutura** (segue a ARQUITETURA.md: `game/` sem SDL, `draw/` só lê o jogo,
só o `main.c` conhece tudo):

| Arquivo | Conteúdo |
|---|---|
| `src/main.c` | o `App` e os 4 callbacks; frame em ordem fixa: tempo, entrada, lógica, pedidos, desenho |
| `src/platform/video.*` | janela, renderer `opengles2`, resolução lógica 640x480, vsync, FPS |
| `src/platform/input.*` | teclado + controle -> ações; remapeamento; `controles.cfg` |
| `src/game/acao.*` | as ações (`Acao`) e o `Botoes` (segurados + apertados): o contrato input <-> jogo |
| `src/game/game.*` | estado do jogo, telas, navegação, opções, pausa |
| `src/game/menu.c` | os menus como dados (acrescentar um item = uma linha) |
| `src/draw/draw.*` | texturas (init/quit), fundos, menus, HUD, sobreposições |
| `src/draw/formas.c` | elipse, retângulo arredondado, degradê, texto em escala, paleta |
| `src/draw/pista.c` | a pista em perspectiva |
| `tests/test_game.c` | 7 testes da lógica, compilados sem a SDL |

Saíram `app.h`, `game_state.*` e `desenhos.*`: o conteúdo foi para os
arquivos acima. A elipse continua sendo a mesma ideia (leque de triângulos),
agora generalizada para qualquer forma convexa.

**Input**
- Cada ação tem um botão do controle e uma tecla. O jogo recebe, por frame,
  `segurados` (estado) e `apertados` (borda false -> true); cada tela escolhe
  qual usar. Os `apertados` vêm dos eventos, então nem um toque mais curto
  que um frame se perde.
- L2/R2 chegam da SDL como **eixo** (gatilho), não como botão; o input
  converte para botão, com histerese.
- Reconhece todos os botões do controle (frontais, ombros, gatilhos, d-pad,
  Select, Start, cliques dos analógicos).
- **Remapeamento:** Configurações > Controles > escolher a ação > apertar o
  botão ou a tecla nova (5 s; Esc cancela no teclado). Se o botão já era de
  outra ação, as duas trocam. Fica salvo em `controles.cfg`, um texto que dá
  para editar à mão: no console em `ports/arkhero/conf/arkhero/`, no PC em
  `~/.local/share/arkhero/`.
- A tela de Controles mostra o **último botão apertado**, com o nome da SDL,
  para conferir o mapeamento no aparelho.
- Select + Start continua fechando o jogo em qualquer tela.

**Menus**
- Início: Jogar (pista de teste), Biblioteca de músicas (em breve),
  Configurações, Sair.
- Configurações: Controles, Velocidade da pista (1 a 10), Modo canhoto,
  Mostrar FPS, Volume (em breve), Calibrar atraso (em breve), Voltar.
- Controles: as 9 ações remapeáveis, Restaurar padrão, Voltar.
- Pausa (Start na pista): Continuar, Reiniciar, Sair para o menu.
- Visual: fundo por tela, painel translúcido, seleção pulsando, aviso
  "Em breve" e rodapé com os botões **do mapeamento atual**.

**Pista**
- Perspectiva de verdade (largura e altura encolhem juntas com a distância),
  em vez do trapézio fixo; desenhada em tiras (ARQUITETURA 7.2).
- Bordas, divisórias, linhas de batida descendo, linha de acerto que pisca
  na palhetada, alvos que acendem com o traste segurado.
- Notas de **demonstração** (padrão fixo a 120 BPM, com acordes), posicionadas
  como função do tempo. Somem quando o parser de `.chart` entrar.
- Velocidade e modo canhoto já funcionam nela.

**Build e editor**
- `CMakeLists.txt`: arquivos novos, alvo `test_game` (sem SDL; `ctest`),
  `compile_commands.json` exportado.
- `.clangd`: o editor passa a achar os includes (`game/...`, SDL).
- Conferido: build local sem avisos; testes passam no PC e no binário ARM
  (sob emulação); build do container (GCC 12) ok; pacote em `dist/`.

### Decisões e suposições: conferir

1. ~~Azul = R1.~~ Esclarecido depois do teste: a ordem certa é
   **R1, R2, L2, L1 e o botão de baixo**. Corrigido na etapa 1.
2. **Letras do R36S.** A SDL nomeia os botões pela posição (padrão Xbox). O
   jogo mostra as letras do R36S: baixo = B, direita = A, esquerda = Y,
   cima = X. Laranja = B (baixo, o X do PlayStation); Confirmar = A;
   Voltar = B. Se no aparelho o "último botão apertado" mostrar outra coisa,
   o PortMaster mapeia diferente e a tabela precisa de ajuste.
3. **Palheta = d-pad cima/baixo**, Star power = Select, Pausa = Start.
4. **Teclado (PC):** trastes A S D F G, palheta setas cima/baixo, star
   power Espaço, pausa Esc; menus: setas, Enter, Esc.
5. As opções (velocidade, canhoto, FPS) **ainda não são salvas**; isso vem
   com o `config.lua` (etapa 6). Os controles já são.
6. O tempo da pista ainda é a soma dos `dt`; na etapa 2 vem do áudio.

### Como testar no R36S

Copiar `dist/Ark Hero.sh` e `dist/arkhero/` para `ports/` e conferir:

- [ ] o jogo abre no menu, a ~60 FPS (canto superior direito)
- [ ] d-pad navega, A confirma, B volta
- [ ] Controles: apertar cada botão e ver o "último botão apertado" (L1, L2,
      R1, R2, B...). **Anotar se algum nome vier diferente.**
- [ ] remapear uma cor, sair do jogo, abrir de novo: o mapeamento continua
- [ ] Restaurar padrão volta tudo
- [ ] Jogar: cada traste acende a faixa certa; palhetada pisca a linha
- [ ] Start pausa; Continuar / Reiniciar / Sair para o menu funcionam
- [ ] Velocidade e Modo canhoto mudam a pista
- [ ] Select + Start fecha o jogo
- [ ] `ports/arkhero/log.txt`: `render em uso : opengles2` e o FPS

### Pendências

- `assets/vaporwave_guitar3.jpg` não é usada e vai junto no pacote (+1 MB).
- `src/images/` (originais em JPG, inclusive os 4k) ainda está no git.
- O README diz que o código do jogo é escrito só pelo João; nesta branch
  isso mudou.

---

## Etapa 1: jogabilidade com notas de demonstração (2026-10-28)

### Pedido

Depois do teste da etapa 0 no R36S (tudo funcionando):

1. O padrão é o **modo controle** do Clone Hero: sem palhetada, só o botão.
2. Detectar acerto e erro, com marcação e animação de acerto; trocar o
   brilho da faixa inteira (traste segurado) por uma mudança só no alvo,
   na base da pista.
3. Explicar como o mapeamento continua salvo depois de fechar o jogo.
4. Há uma música de teste em `assets/musics/`.
5. Começar as notas longas (sustain). Whammy no analógico (esquerdo por
   padrão, com opção do direito), virando para os lados; star power no
   mesmo analógico, com um empurrão forte para cima.
6. Trastes no R36S: R1, R2, L2, L1 e o botão de baixo.

### O que foi feito

**As notas viraram dados do jogo** (`game/chart.*`): a pista lê `g->chart`,
o mesmo formato que o parser de `.chart` vai preencher. A música de
demonstração (`Chart_demo`, 120 BPM, ~25 s) tem notas simples, sustains,
acordes, colcheias e duas frases de star power.

**Juiz do modo controle** (`game/judge.c`):
- apertar o botão no tempo da nota é tocar a nota (janela de ±70 ms);
- acorde: acerta quando todos os botões estão apertados, sendo o último
  dentro da janela;
- botão sem nota não é punido; deixar a nota passar é erro (quebra o combo);
- sustain: depois da cabeça, segurar até o fim. Soltar antes não quebra o
  combo, só para de somar (folga de 100 ms no fim).

**Placar** (`game/score.c`, a partir dos eventos do frame):
50 pontos por botão da nota, 50 por segundo de sustain, multiplicador
x1 a x4 (a cada 10 de combo) e x2 com o star power ativo. Os eventos
(`EV_ACERTO`, `EV_ERRO`, `EV_SUSTAIN`...) são o ponto onde a Lua vai entrar.

**Star power:** notas de frase em branco com anel ciano; frase inteira
acertada = +25% no medidor; com 50% ou mais, ativa (Select, Espaço ou o
analógico para cima); dura até 16 s com o medidor cheio. Whammy num sustain
de frase também enche o medidor.

**Analógico** (`platform/input.c`): para os lados = whammy (0 a 100%, com
zona morta no centro); empurrão forte para cima = star power. Esquerdo por
padrão; em Configurações > Controles > Analógico troca para o direito.
A escolha fica no `controles.cfg`. No PC, o whammy é a tecla W.

**Visual:**
- o alvo afunda e acende quando o botão está apertado; a faixa não brilha
  mais;
- acerto: anel que se abre e chama que sobe no alvo; nota acertada some;
- sustain: cauda na faixa, que ondula com o whammy e "queima" no alvo
  enquanto é segurada; solta cedo, o resto passa apagado;
- nota perdida passa cinza; "PERFEITO" (até 30 ms), "BOM" ou "ERROU" sobre
  a linha de acerto, com o erro em ms (negativo = adiantado);
- HUD: pontos, multiplicador (a cor muda com o valor), combo, medidor de
  star power com a marca de 50%, barra do whammy;
- pista azulada e bordas ciano com o star power ativo;
- tela de fim: pontos, notas acertadas (%), maior combo, "Jogar de novo".

**Menus:** Configurações ganhou "Modo de jogo: Controle" (o modo guitarra
avisa "em breve"); Controles ganhou Whammy e Analógico. As linhas de
palhetada saíram da tela até o modo guitarra existir (as ações continuam).

**Testes:** 16 testes no `test_game`. Os novos cobrem: acerto sem
palhetada, fora da janela, botão errado, acorde, sustain completo e solto
cedo, multiplicador, star power, frase com erro e fim da música. Passam no
PC e no binário ARM.

**Músicas de teste fora do git e do pacote:** `assets/musics/` foi para o
`.gitignore` e saiu do índice do git (`git rm --cached`: os arquivos
continuam no disco); o `package_port.sh` não as copia.

### Decisões: conferir

1. **Botão de baixo.** O padrão do laranja é o botão de baixo pela posição
   (o `a` / SOUTH da SDL). No R36S a tela mostrou "A" para ele, o que
   sugere que o PortMaster entrega esse botão como `b`. A linha `laranja|...`
   do `conf/arkhero/controles.cfg` do aparelho tira a dúvida.
2. O tempo de cada aperto ainda é o do frame (~16 ms de resolução); os
   timestamps dos eventos entram com o relógio do áudio.
3. Regras (janela, pontos, duração do star power) são constantes em C;
   vão para a Lua na etapa 5.

### Como testar no R36S

- [ ] Configurações > Controles > Restaurar padrão: R1, R2, L2, L1, baixo
- [ ] Jogar: as notas são acertadas só com o botão, sem palhetada
- [ ] acordes precisam de todos os botões
- [ ] explosão no alvo ao acertar; "ERROU" e combo zerado ao deixar passar
- [ ] sustains: segurar soma pontos e a cauda "queima"; o whammy ondula a cauda
- [ ] depois da segunda frase ciano, "PRONTO!": analógico para cima ativa
- [ ] Analógico: trocar para o direito e conferir que o whammy muda de lado
- [ ] tela de fim com o resumo; "Jogar de novo"
- [ ] FPS continua ~60 com tudo na tela (`log.txt`)

### Para a próxima etapa (áudio)

A música de teste tem o áudio em **Opus** (`guitar.opus`, `bass.opus`,
`drums.opus`), sem uma faixa `song`. Opus precisa da libopus (+ opusfile),
que teria de ser compilada no container e ir no pacote, como a SDL. Muitas
músicas da comunidade usam `.ogg` (Vorbis), que o `stb_vorbis` resolve sem
dependência. Decidir antes de começar.

---

## Etapa 2: áudio e relógio mestre (2026-10-28)

### Pedido

- Tirar do GitHub a música de teste, que foi enviada por engano, e
  continuar o versionamento.
- Implementar logo as bibliotecas para suportar Opus, de um jeito que já
  seja compatível com o que existe nas músicas da comunidade.
- Por enquanto, só dá para testar no PC.

### Git: a música saiu do histórico

A música só existia no commit da etapa 0 (`f0f7eb9`). Ele foi refeito
sem `assets/musics/`, com a mesma mensagem e as mesmas datas, e com a
linha nova do `.gitignore` (novo commit: `ba1dacd`). Os arquivos continuam
no disco, ignorados pelo git. O commit antigo ficou guardado localmente em
`refs/backup/claudio-antes-da-limpeza`.

Falta o João enviar com force push (o modo automático do Claude não deixa
fazer isso sozinho, por ser destrutivo):

    git push --force-with-lease=claudio:f0f7eb9 origin claudio

### O que foi feito

**Bibliotecas** (Xiph, as de referência desses formatos): libogg 1.3.6,
libvorbis 1.3.7, opus 1.6.1 e opusfile 0.12. Os tarballs estão em
`third_party/src/`, conferidos com o SHA256 publicado pela Xiph.
- PC: as do sistema (o Arch já tem as quatro), achadas pelo pkg-config.
- Console: o `container_build.sh` compila as quatro **estáticas** em
  `third_party/xiph-<arch>/` (uma vez; depois fica em cache). Elas vão
  dentro do binário: o pacote não ganha nenhum `.so` novo. O binário do
  R36S depende só de libSDL3, libm e libc, e exige no mínimo a glibc 2.34.

**Formatos** (`platform/carregador.c`): Opus (`.opus`, e `.ogg` que na
verdade é Opus, detectado sozinho), Ogg Vorbis (`.ogg`, qualquer taxa) e
WAV. MP3 é reconhecido e ignorado por enquanto (aparece na tela).

**Faixas** (`platform/musica.c`): numa pasta do Clone Hero, acha `song`,
`guitar`, `bass`, `rhythm`, `keys`, `vocals*`, `drums*` e `crowd`, sem
diferenciar maiúsculas; `preview.*` fica de fora. Desce até 3 níveis de
subpastas. As músicas de teste ficam em `songs/` (no console, dentro da
pasta do jogo) ou `assets/musics/` (no PC).

**Carga:** numa thread, decodifica cada faixa, converte para 48 kHz s16
estéreo e soma tudo num buffer só (estratégia "pré-mixar" da conversa
inicial: zero decodificação durante a partida). A tela mostra o progresso.

**Relógio mestre** (`platform/audio.c`): a posição vem dos frames que o
dispositivo já consumiu (enviados - na fila - buffer do dispositivo),
suavizada com o relógio do sistema entre os degraus. Na partida, o tempo
do jogo passa a ser o do áudio (`g->relogio_audio`), e nunca volta para
trás. Sem dispositivo de áudio, o jogo roda como antes, pelo `dt`.

**Na partida:** a música de demonstração toca um metrônomo gerado por
código nas linhas de batida (acento no primeiro tempo), então dá para
ouvir se a pista está no tempo do som. Pausa pausa o som; Reiniciar
recomeça.

**Menus:** Volume passou a funcionar. Em Configurações entrou
**Teste de audio**: carrega a música de teste (com barra de progresso e o
tempo de carga), toca e pausa, e mostra os dois relógios lado a lado.

**Testes:** 18 no `test_game` (novos: `song.ini` e o relógio externo).

### Medições no PC

- Carga da música de teste (255 s, 3 faixas Opus): **3,5 s** (221x o tempo real).
- Relógio: degraus de 21,3 ms (o buffer de 1024 frames); o suave fica a no
  máximo 11 ms do cru e nunca volta. A diferença para o relógio do sistema
  ficou **constante em ~74 ms** do 1º ao 20º segundo (o atraso de partida do
  PipeWire): não há deriva.
- Decodificação conferida também no binário ARM (emulado): resultado
  idêntico ao do PC.

### Como testar

No PC: Configurações > Teste de audio > Carregar > Tocar; e Jogar, para
ouvir o metrônomo junto com as linhas de batida.

No R36S: copiar uma pasta de música para `ports/arkhero/songs/` e usar o
Teste de audio. **Anotar o tempo de carga que aparece na tela.** No R36S
ele deve ser bem maior que no PC; se passar de uns 15 s, a próxima melhoria
é decodificar em paralelo (o RK3326 tem 4 núcleos).

### Pendências

- MP3 (dr_mp3, um arquivo só, se aparecer nas músicas que você usa).
- Volume por faixa e silenciar a guitarra quando erra (estratégia "dois
  streams" da conversa inicial).
- O `delay` do `song.ini` e o `Offset` do `.chart` entram com o parser.

---

## Etapas 3 e 4: parser de `.chart`, escolha de música e dificuldade (2026-10-28)

### Pedido

Depois do teste no PC (metrônomo funcionando, jogabilidade melhor; o star
power ainda não foi testado, por ser no teclado):
- implementar a leitura dos arquivos das músicas (`notes.chart` e cia.);
- se possível, já jogar a música de teste com escolha de dificuldade.

### O que foi feito

**Parser de `.chart`** (`game/chartfile.c`, sem SDL):
- `[Song]`: `Resolution` e `Offset`; `[SyncTrack]`: andamento (`B`) e
  compasso (`TS`, inclusive `6/8`); as quatro dificuldades da guitarra.
- **Mapa de tempo**: montado uma vez na leitura; tick -> segundos é uma
  busca binária (a música de teste tem 504 mudanças de andamento).
- Notas no mesmo tick viram um acorde; `N 5` (HOPO forçado) e `N 6` (tap)
  viram flags para o futuro modo guitarra; `N 7` é a nota aberta;
  `S 2` são as frases de star power, agora numeradas (duas frases coladas
  não se misturam).
- Sustain mais curto que uma semicolcheia vira nota simples.
- `delay` do `song.ini` e `Offset` do chart deslocam as notas.
- Linhas de batida a cada semínima, a primeira de cada compasso forte.
- Quando a primeira nota vem cedo, a partida começa antes da música
  (2,5 s de preparação, com a pista já andando e o áudio em silêncio).
- O `notes.chart` de teste é lido em **1,9 ms**.

**Fluxo de jogo**, como no Clone Hero:
**Jogar -> Músicas -> Dificuldade -> Carregando -> partida**.
- Músicas: todas as pastas encontradas em `songs/` e `assets/musics/`,
  com nome, artista, charter e duração (do `song.ini`), mais a
  "Demonstração (sem música)". A lista rola; nomes compridos são cortados;
  caracteres fora do ASCII viram `?` (a fonte de debug não tem acentos).
- Dificuldade: só as que existem, com o número de notas de cada uma.
- Carregando: barra de progresso da decodificação; Voltar cancela. Uma
  música já decodificada não é decodificada de novo.
- Na partida toca a **música de verdade**, e o relógio do jogo é o do
  áudio. Se o áudio falhar, toca o metrônomo e avisa.
- A partida acaba quando acabam as notas **e** a música. "Sair para o
  menu" volta para a lista de músicas.

**Nota aberta** (para outras músicas): barra roxa que atravessa a pista;
no modo controle, qualquer botão acerta.

**Bug encontrado pelos testes:** os ids da pausa ("Reiniciar", "Sair")
caíam na faixa numérica das dificuldades e executavam a ação errada. A
ordem do enum foi corrigida, com um comentário explicando a regra.

**Testes:** 21 no `test_game`. Os novos cobrem:
- o parser: tempos com a mudança de andamento, acorde, sustains curto e
  longo, aberta, frases coladas, preparação, compasso, `delay`;
- a nota aberta e as duas frases numa partida;
- o fluxo completo de escolha (música, dificuldade, cancelar a carga,
  áudio pronto).

Fora dos testes automáticos, um jogador automático tocou o chart real
inteiro nas quatro dificuldades: **full combo em todas** (Expert: 1240
notas, com notas a 37 ms uma da outra no solo), terminando em 255 s, com
a música.

### Como testar

No PC: Jogar > Cliffs Of Dover > Expert (ou outra dificuldade). A música
toca e as notas descem nela. No R36S: copiar a pasta da música para
`ports/arkhero/songs/`.

### Pendências

- Formato `.mid` (muitas músicas da comunidade usam só ele; a música de
  teste tem os dois) e `.sng`.
- Seções do `[Events]` (para o modo treino) e solos.
- A lista de músicas é lida a cada abertura do jogo; com bibliotecas
  grandes, vai precisar do cache (etapa 7).
- Calibração do atraso (etapa 6): no PC o PipeWire teve ~74 ms de atraso
  de partida, que já não desalinha nada, mas o atraso do fone/alto-falante
  e da tela ainda não é compensado.

---

## Etapa 4b: ajustes depois de jogar no PC (2026-10-28)

### Pedido

Depois de jogar a música de teste no PC (desempenho muito bom):
- janela de acerto mais folgada, com um "modo precisão" que guarde a atual;
- mais logs, para trazer do R36S;
- procurar músicas novas nas pastas e acrescentar na lista;
- navegação para pular várias opções de uma vez (listas grandes);
- pontuação e multiplicador mais para baixo na tela durante a partida.

### O que foi feito

**Janela de acerto:** o padrão passou de ±70 ms para **±100 ms**.
Configurações > **Modo precisão: Sim** volta para os ±70 ms. "Perfeito" é
40% da janela (40 ms no normal, 28 ms na precisão).

**Procura de músicas** (`platform/musica.c`):
- roda numa thread, ao abrir o jogo e pelo item "Procurar musicas novas"
  no fim da lista; a tela mostra "procurando... N encontradas";
- desce até 6 níveis de pastas (Artista/Álbum/Música...);
- lê os `song.ini` dentro da thread; o main só interpreta o texto;
- a lista sai **ordenada pelo título**; a música selecionada e a já
  decodificada continuam valendo depois de procurar de novo;
- a lista nova só é aplicada fora da partida, da escolha de dificuldade
  e de uma carga (os índices mudam);
- sem limite de 256 músicas: as listas crescem conforme precisam;
- **pastas extras:** o arquivo `pastas.txt`, ao lado do `controles.cfg`
  (no console, `ports/arkhero/conf/arkhero/pastas.txt`), tem uma pasta por
  linha. Ele é criado na primeira vez com um exemplo. As pastas `songs/` e
  `assets/musics/` são sempre procuradas;
- teste com uma biblioteca falsa de 402 músicas (aninhadas, uma só com
  `.mid`, uma sem áudio, `Notes.chart` com maiúscula, pasta extra do
  `pastas.txt`): todas achadas e contadas certo, em 6 ms no SSD do PC.

**Navegação:**
- segurar Cima/Baixo em qualquer menu: anda um item, espera 0,4 s e
  depois anda ~14 itens por segundo;
- na lista de músicas, Esq/Dir pulam 7 itens (uma página);
- a posição aparece no canto ("12/340").

**Placar:** pontos, multiplicador e combo saíram do canto de cima e foram
para uma caixa à direita, na altura do meio da pista (o mais baixo possível
sem ficar em cima dela, que alarga perto da base). O multiplicador cresce
um pouco quando muda.

**Logs** (no console, tudo vai para `ports/arkhero/log.txt`):
- `[arkhero]`: versão, pasta do executável, pasta dos dados;
- `[input]`: os controles em uso; ao conectar o controle, o nome, o tipo
  e o **mapeamento completo da SDL** (diz qual botão físico é o `a` e qual é
  o `b`); com `ARKHERO_LOG_INPUT=1`, cada aperto, com o tempo;
- `[audio]`: driver, buffer, latência;
- `[musica]`: pastas procuradas, quantas músicas, quantas só com `.mid`,
  quantas sem áudio, o tempo da procura; ao abrir uma música, o tempo de
  leitura do chart e as notas por dificuldade;
- `[carga]`: faixas, tempo de decodificação, velocidade, memória usada;
- `[partida]`: no começo, música, dificuldade, janela, velocidade e se o
  som é a música ou o metrônomo; no fim (ou ao abandonar, reiniciar,
  fechar o jogo), pontos, notas, combo e:
  - **erro médio de tempo dos acertos** e o desvio: é a medida para a
    calibração (negativo = adiantado, positivo = atrasado);
  - **frames lentos** (mais de 25 ms) e **saltos do relógio do áudio**:
    se aparecerem, há travada ou problema no dispositivo de áudio.

**Build:** os headers das bibliotecas da Xiph entram como `SYSTEM` no
container (os avisos de dentro deles não eram do jogo): o build do R36S
voltou a ter zero avisos.

**Testes:** 23 no `test_game` (novos: modo precisão, lista de 300
músicas, página e repetição ao segurar).

### O que trazer do R36S

Do `ports/arkhero/log.txt`, depois de jogar uma música inteira:
1. as linhas `[input] controle:` e `[input] mapeamento SDL:`;
2. as linhas `[audio]` e `[carga] ... em X s (Nx o tempo real)`;
3. as duas linhas `[partida]` do fim (erro médio, frames lentos, saltos);
4. as linhas `[video] fps`.

---

## Etapa 4c: recordes, organização da lista, opções salvas (2026-10-28)

### Pedido

- Salvar o recorde de pontuação de cada música (a maior, em qualquer
  dificuldade).
- Músicas em subpastas da pasta de músicas, para o jogador organizar por
  jogo de origem, estilo etc.; e sugestões de como organizar a lista.
- Uma revisão do que falta para completar o jogo, e o que a Lua poderia
  permitir como mods.

### O que foi feito

**Recordes** (`game/biblioteca.c` + `main.c`):
- no fim da partida, se os pontos passaram do recorde da música, ele é
  atualizado e gravado em `recordes.txt` (ao lado do `controles.cfg`);
- guarda os pontos, a dificuldade em que foram feitos, a porcentagem e se
  foi full combo; a demonstração não conta;
- a chave é **título/artista/charter** (do `song.ini`), e não a pasta:
  reorganizar as pastas não perde os recordes;
- aparece embaixo da lista de músicas (em amarelo) e na tela de fim
  ("NOVO RECORDE!", com o anterior).

**Organizar a lista**: o primeiro item da lista de músicas é
"Organizar por", com Esq/Dir:
- **Pastas** (o padrão): a lista segue as subpastas. `[Guitar Hero 3]  25`
  é uma pasta com 25 músicas dentro (contando as sub-subpastas); entrar
  mostra o que tem nela; Voltar sobe uma pasta e deixa selecionada a
  pasta de onde saiu;
- **Título**: todas as músicas, de A a Z;
- **Artista**: todas, por artista ("Artista - Título").

**Opções salvas**: velocidade, modo precisão, canhoto, FPS, volume e o
modo de organizar vão para `opcoes.cfg`, ao sair das Configurações (e ao
trocar o modo de organizar). A conversão está em `game/opcoes.c` (sem SDL,
com teste); valor estranho no arquivo é limitado à faixa válida.

**Bug latente corrigido**: com mais de 1000 músicas, os ids delas
invadiriam a faixa das dificuldades. As faixas foram reordenadas (a das
músicas, sem limite, é a última) e o código usa `Game_item_musica()`,
`Game_item_pasta()` e `Game_item_acao()` em vez de comparar ids à mão.

**Testes**: 26 no `test_game` (novos: pastas e subpastas, voltar para a
pasta certa, modo por artista, recorde novo e recorde mantido, opções em
texto). Zero avisos no PC e no container.

---

## O que falta (revisão de 2026-10-28)

**Antes de tudo: o R36S.** Desempenho (as notas são desenhadas com
geometria calculada na CPU a cada frame; num Cortex-A35 isso pode pesar
com muitas notas na tela), tempo de carga do Opus, atraso do áudio, e o
botão A/B. Os logs da etapa 4b foram feitos para isso.

**Para jogar qualquer biblioteca do Clone Hero:**
1. `notes.mid` (muitas músicas só têm ele) e depois `.sng`;
2. MP3 (`dr_mp3`, um arquivo só);
3. cache da biblioteca (com milhares de músicas num cartão SD, a procura e
   a leitura dos `song.ini` a cada abertura vão pesar);
4. nomes com acento/japonês (hoje viram `?`): fonte TrueType (SDL_ttf),
   pelo menos na lista de músicas.

**Para a partida ficar completa:**
5. calibração do atraso de áudio e de vídeo (etapa 6);
6. som de erro e silenciar a guitarra quando erra (precisa mixar a faixa
   da guitarra separada das outras);
7. estrelas por música (1 a 5, pela pontuação), como no Clone Hero;
8. barra de rock / falha (hoje não dá para perder a música);
9. modo treino: começar numa seção (as seções do `[Events]` já estão no
   chart) e tocar mais devagar;
10. prévia da música ao passar pela lista.

**Controles:** modo guitarra (palhetada, HOPO e tap: as flags já são
lidas), guitarras USB, navegação dos menus pelo analógico.

**Distribuição:** `port.json` completo, imagens e descrição para o
PortMaster, build x86_64, licenças das bibliotecas no pacote.

**Lua:** ainda não começou (ver a proposta de usos na conversa da etapa 4c).

---

## Etapa 4d: o que o teste no R36S mostrou (2026-10-28)

### Pedido (depois do teste no R36S: roda liso, sem queda de FPS)

1. Diferenciar as notas que precisam de palhetada das que não precisam:
   as comuns com o miolo preto e o centro branco; as especiais como estão.
2. Penalidade por apertar sem nota (zerar combo e multiplicador), mantendo
   o "segurar uma e apertar a outra" nos acordes repetidos.
3. As músicas em MP3 não apareciam (e a do Rascal Flatts, em Opus, também não).
4. Confirmar/voltar trocavam de lugar entre o controle do PC e o R36S
   (usar a linguagem dos pontos cardeais: sul, leste, oeste, norte).
5. "Perfeito / Bom / Errou" como opção (poluem a pista).
6. Não dava para trocar o analógico do whammy.

### O que os logs mostraram

- A causa do item 3 não era só o MP3: o log dizia "75 so com notes.mid".
  O pacote do GH3 é **.mid + MP3** e o Rascal Flatts é **.mid + Opus**.
- O `GO-Super Gamepad` do R36S tem `a:b1,b:b0` no mapeamento: o PortMaster
  nomeia os frontais pela letra impressa (estilo Nintendo). No controle de
  Xbox do PC é pela posição (`a:b0,b:b1`). Por isso o mesmo "b" da SDL era
  o botão de baixo num e o da direita no outro.
- Erro médio dos acertos: -48 ms no R36S, -22 a -27 ms no PC (o jogador
  aperta adiantado em relação ao relógio do jogo).
- Carga da música de teste no R36S: 14,8 s (52x o tempo real); no PC, 2,4 s.

### O que foi feito

**Leitor de `.mid`** (`game/midifile.c`, sem SDL): trilha `PART GUITAR`
(ou `T1 GEMS`), as quatro dificuldades, forçar HOPO / palhetada, star power
(116, ou o `multiplier_note` do song.ini), tap (104 e SysEx do Phase Shift),
abertas (`[ENHANCED_OPENS]` e SysEx), andamento e compasso. Lê com limite
(arquivo cortado não lê fora). Conferido nos 77 MIDI da pasta de teste:
número de notas do Expert idêntico ao de um leitor independente em Python
nos 77, todas as músicas com star power, as notas terminando antes do fim
do áudio; o pior arquivo levou 15,7 ms no PC. 40 arquivos estragados de
propósito (cortados, bytes trocados) passaram pelo AddressSanitizer sem
nenhum erro.

**MP3** (`third_party/dr_libs/dr_mp3.h`, domínio público, um arquivo só,
versão fixada por commit e SHA256): entra no carregador junto com Opus,
Vorbis e WAV. A música do GH3 (3 faixas MP3) deu 296,47 s, igual ao ffprobe.

**HOPO na pista:** o `Chart_from_file` calcula o HOPO como o Clone Hero
(nota simples, de cor diferente, até 65/192 de batida da anterior; acorde
nunca), com os marcadores do arquivo por cima (.chart "N 5" inverte; .mid
força HOPO ou palhetada). Notas de palhetada: miolo preto, centro branco.
HOPO e tap: como antes. 26% das notas do Expert do GH3 são HOPO.

**Penalidade** (opção "Penalidade (erro)", ligada): apertar um botão sem
nota para ele quebra o combo (e o multiplicador, que vem do combo). O
acorde apertado com os botões juntos não pune o segundo botão, nem o deixa
pegar a nota seguinte (bug que existia antes e foi corrigido junto);
segurar um botão e apertar o outro continua acertando acordes repetidos.

**Ajuste de tempo** (troca o "Calibrar atraso (em breve)"): -300 a +300 ms,
de 5 em 5. Desloca o relógio do áudio. A tela de fim mostra o erro médio e
sugere o valor (pelo log do R36S, algo como **+45 ms** lá; no PC, +25 ms).

**Botões frontais por posição:** o jogo guarda e mostra **Sul, Leste,
Oeste, Norte**. Ao conectar o controle, detecta se ele é do estilo Nintendo
(pela lista de nomes conhecidos, ou pelo `a:b1,b:b0` no mapeamento) e
desfaz a troca. Confirmar = Sul e voltar = Leste em qualquer controle.
Controles > "Botões frontais" força Xbox ou Nintendo se a detecção errar.
O `controles.cfg` antigo (com "a"/"b") é convertido ao conectar o controle
e regravado com os nomes novos.

**Whammy:** na captura do Whammy, mexer um analógico escolhe esse
analógico. Mexer um analógico também aparece no "último botão apertado".

**"Mostrar acertos"**: opção, desligada por padrão.

**Testes:** 30 no `test_game` (novos: penalidade, acorde apertado junto,
acordes repetidos, opção desligada, ajuste de tempo, HOPO no .chart, e um
MIDI montado byte a byte com mudança de andamento, HOPO natural, palhetada
forçada, sustain e star power). Limpos também com AddressSanitizer e UBSan
(que apontaram `qsort` com lista vazia: corrigido).

### Como testar

- [ ] a lista mostra as ~78 músicas (GH3, Rascal Flatts, Mac DeMarco, Cliffs)
- [ ] músicas do GH3 (MIDI + MP3) carregam e tocam no tempo
- [ ] notas de palhetada com o miolo preto; HOPOs coloridos
- [ ] apertar sem nota quebra o combo; acordes repetidos segurando um botão
- [ ] no R36S e no PC: confirmar no botão de baixo, voltar no da direita
- [ ] Ajuste de tempo: usar a sugestão da tela de fim e ver o erro médio cair
- [ ] Controles > Whammy: mexer o analógico direito; o whammy passa para ele
- [ ] tempo de carga de uma música do GH3 no R36S (log `[carga]`)

### Pendências

- A carga no R36S (14,8 s para 255 s com 3 faixas) pode cair à metade ou
  menos decodificando as faixas em paralelo (o RK3326 tem 4 núcleos).
- `.sng`, e as abertas/taps de charts de outras fontes (só o formato
  RB/CH/PS foi visto até agora).

---

## Etapa 4e: carga em paralelo e acorde exato (2026-10-28)

### Pedido (depois do teste no R36S: liso, mecânicas boas)

- A Rascal Flatts demorou mais para carregar: usar os outros núcleos.
- Acorde vermelho + azul aceitava vermelho + amarelo + azul apertados
  juntos: isso deve errar (e ter penalidade).
- Analisar os próximos passos: a guitarra (testada só no PC) e o que mais
  estiver ficando de lado.

### O log do R36S

- A carga cresce com o número de faixas: Pat Benatar (3 faixas MP3, 181 s)
  em 7,5 s; Rascal Flatts (**7 faixas** Opus, 254 s) em **28,2 s**, num
  núcleo só.
- Erro médio dos acertos: -37 a -44 ms (sugestão: Ajuste de tempo +40 ms).

### O que foi feito

**Carga em paralelo** (`platform/carregador.c`): uma thread por núcleo
(no máximo uma por faixa); cada uma pega a próxima faixa livre, decodifica
inteira e soma no mix. A soma passou a ser em 32 bits, com um corte só no
fim (antes cortava a cada faixa): o resultado é o mesmo em qualquer ordem
e o som não satura no meio da mistura. No PC, a Rascal Flatts:

| threads | tempo | |
|---|---|---|
| 1 | 3,96 s | |
| 4 (como o R36S) | 1,48 s | 2,7x mais rápido |
| 7 | 1,07 s | |

O áudio sai idêntico, byte a byte, com 1, 4 ou 7 threads (mesmo hash), e o
ThreadSanitizer não acusou nada. No R36S, a estimativa é cair de 28 s para
uns 10 s. `ARKHERO_THREADS=n` força o número de threads, para medir.

**Acorde exato** (`game/judge.c`): num acorde, os botões apertados têm de
ser exatamente os dele; um a mais (apertado agora ou já segurado) erra o
acorde e conta uma penalidade (uma por frame, não uma por botão). O botão
de um sustain ainda segurado não conta como "a mais". Nota simples continua
tolerante a um botão segurado de antes. Conferido com o jogador automático
no Expert das 78 músicas: 80.521 notas, full combo em todas, zero
penalidades.

**Pacote:** `licencas/` com os textos de licença da SDL, libogg, libvorbis,
opus, opusfile e dr_mp3 (as da Xiph, BSD, exigem isso na distribuição), e a
pasta `songs/` com um aviso de onde pôr as músicas.

**Testes:** 32 no `test_game` (novos: acorde com botão a mais, botão a mais
já segurado, acorde no meio de um sustain longo).

---

## Etapa 5: multijogador, instrumentos, som de erro, carga em aparelhos menores (2026-10-29)

### Pedido (depois de testar a 4e no PC)

- Conferir quantos núcleos o aparelho tem antes de dividir a carga: o que
  acontece com 1 ou 2 núcleos? Criar meios de contornar.
- Multijogador: até 4 jogadores, cada um com a sua configuração de
  controle, também no aarch64. A ideia é ser uma alternativa ao Clone Hero
  para PCs fracos.
- Implementar o baixo e as outras partes que o chart tiver.
- Opção para ligar o som de erro do instrumento.
- (O desenho das notas não precisa de otimização: FPS constante no R36S.)
- Testes futuros: EmuELEC (gamestick Y7, TV box) e ArkOS clássico (R36S,
  R35S).

### O que foi feito

**Instrumentos** (`game/chartfile.c`, `game/midifile.c`): o leitor guarda
as cinco partes de 5 trastes: guitarra, baixo, rítmica, guitarra coop e
teclado. No `.chart` são as seções `Single`, `DoubleBass`, `DoubleRhythm`,
`DoubleGuitar` e `Keyboard`; no `.mid`, as trilhas `PART GUITAR` (ou
`T1 GEMS`), `PART BASS`, `PART RHYTHM`, `PART GUITAR COOP` e `PART KEYS`.
Um chart sem guitarra (só baixo, por exemplo) agora também abre. Na
biblioteca de teste: guitarra em 78 músicas, baixo em 66 e rítmica em 9.

**Jogadores** (`game/game.h`): tudo o que é de uma pessoa (a pista, o
placar, o star power, as estatísticas) saiu do `Game` e foi para um
`Jogador`, e o `Game` tem quatro (`g->jog[0..3]`). O tempo, a pausa e a
música continuam no `Game`. O juiz e a pontuação recebem o jogador; cada
evento diz de quem é. O `Game_update` recebe os botões de cada um: nos
menus vale qualquer jogador, mas na preparação e na pista cada um só mexe
no que é seu. Qualquer jogador pausa. A partida acaba quando as notas de
todos acabam.

**Tela de preparação** (no lugar da escolha de dificuldade): uma coluna por
jogador. Funciona assim:
- o jogador 1 já está dentro; os outros entram com Confirmar no próprio
  controle;
- Cima/Baixo escolhe a linha (instrumento ou dificuldade); Esq/Dir troca o
  valor, só entre as partes que o chart tem;
- Confirmar marca "pronto"; Voltar desfaz o pronto e, apertado de novo,
  tira o jogador (o jogador 1 volta para a lista);
- quando todos os que entraram estão prontos, a partida começa. Se alguém
  entrou e largou o controle, o jogador 1 confirma de novo e começa sem ele;
- as escolhas e quem entrou continuam para a próxima música.

Sozinho, o número de apertos é o mesmo de antes.

**Controles** (`platform/input.c`): cada controle conectado vai para o
primeiro espaço livre (o primeiro é o jogador 1). Cada jogador tem um
perfil: os botões, o analógico do whammy e o estilo dos frontais. Os
perfis ficam em `controles.cfg` (J1, o mesmo arquivo de antes),
`controles2.cfg`, `controles3.cfg` e `controles4.cfg`. A tela de Controles
ganhou o item "Jogador" (J1..J4) e mostra o controle daquele jogador; o
remapeamento só aceita o botão do controle dele (ou o teclado). Teclado
para dois no PC: o J2 usa H J K L Ç nos trastes, N para star power, M para
whammy, Backspace para pausar e voltar, P para confirmar, I e , (vírgula)
para cima e baixo, U e O para os lados. J3 e J4 começam sem teclas.

**Pistas lado a lado** (`draw/pista.c`, `draw/draw.c`): com 2, 3 ou 4
jogadores, as pistas encolhem (base de 300, 205 e 155 px) e cada uma tem a
borda da cor do jogador (rosa, laranja, verde, azul). Em cima de cada pista
aparece quem é e o que toca; embaixo, pontos, multiplicador, combo e a
barra de star power. O resultado sai em colunas, e o recorde da música fica
com o melhor jogador (agora guardado também com o instrumento). Com um
jogador, o visual é o de antes.

**Som de erro** (Configurações > "Som de erro", desligado por padrão): ao
errar uma nota ou apertar sem nota, o instrumento "engasga": um ruído
abafado com um tom grave que cai, gerado por código (`Audio_make_miss`).
O do baixo é mais grave. Ele toca num segundo dispositivo lógico de áudio,
que a SDL mistura com a música; o relógio da música não muda. Conferido
pela saída do driver `disk` da SDL: o efeito entra na hora certa e os
cliques do metrônomo continuam no lugar.

**Carga em aparelhos menores** (`platform/carregador.c`):
- **1 núcleo:** uma thread só, como antes do paralelo: funciona, só demora
  mais (no PC, a Rascal Flatts levou 3,9 s em vez de 1,2 s, com o áudio
  idêntico byte a byte).
- **2 núcleos:** duas threads. O risco era a decodificação roubar o
  processador da tela e do som. Agora as threads da carga (e a da procura
  de músicas) rodam com **prioridade baixa**: o sistema dá a vez para as
  outras primeiro.
- **Pouca RAM:** a soma em 32 bits ocupa o dobro da música pronta. Se ela
  passar de 1/4 da RAM, a soma é feita direto em 16 bits ("modo
  econômico"). Na Rascal Flatts, 0,0016% das amostras saem diferentes, só
  onde a mistura estoura. `ARKHERO_ECONOMICO=1` força o modo, para testar.
  A música anterior agora sai da memória antes da carga da nova (antes, no
  pico, ficavam as duas).
- O log mostra, no começo, o número de núcleos e a RAM; cada carga diz
  quantas threads usou e se somou em 32 ou 16 bits.

**Testes:** 36 no `test_game`. Os novos cobrem dois jogadores (guitarra e
baixo) com placares e penalidades separados, a preparação sem os outros,
um chart só de baixo, as cinco partes do `.chart` e o baixo no `.mid`.
Passam no PC (com ASan e UBSan) e no aarch64. O jogador automático tocou
as 78 músicas com um jogador por instrumento existente: 153 partes,
148.875 notas, full combo em todas, zero penalidades, sem erro de memória.

### Como testar

- **Dois no PC:** um no teclado de sempre (A S D F G) e outro em H J K L Ç;
  ou dois controles. Escolha uma música; na preparação, o J2 entra com P
  (ou com o botão de baixo do controle dele).
- **Baixo:** numa música com baixo, Cima na preparação e Esq/Dir.
- **Som de erro:** Configurações > Som de erro > Sim.
- **No R36S:** a carga da Rascal Flatts (o log diz threads e núcleos), e se
  a tela de carga continua animada enquanto decodifica.

### Compatibilidade com sistemas mais antigos (análise)

O binário de hoje exige a **glibc 2.34**. Isso não vem do código do jogo:
vem do container, que é Debian 12 (glibc 2.36). A partir da 2.34, as
funções de thread e de `dlopen` mudaram de versão, e tudo o que é compilado
ali (a SDL3 também) passa a exigir a 2.34 ou mais nova.

- **dArkOS** (o seu R36S de hoje): Debian 12, glibc 2.36: funciona.
- **ArkOS clássico** (R36S, R35S): pelo que sei, é baseado no Ubuntu 19.10,
  com glibc 2.30 (confira com `ldd --version`): **não abriria** ("version
  GLIBC_2.34 not found" no log).
- **EmuELEC:** depende da versão; `ldd --version` no terminal do aparelho
  diz qual é.

O caminho: compilar num container mais antigo, com glibc 2.28 (Debian 10).
Um binário feito com uma glibc velha roda nas mais novas, então um build só
serviria para todos. Os cuidados são: o CMake do Debian 10 é velho demais
para a SDL3 (instalar um mais novo pelo pip), e a SDL3 só com os backends
de console (KMSDRM e ALSA). Fica para a próxima etapa, antes do teste
nesses aparelhos.

### Pendências

- O controle vai para o primeiro espaço livre na ordem em que conectou. Se
  um sistema expuser um "controle fantasma" (no R36S só aparece um, pelo
  log), o controle de verdade cairia no J2. A tela de preparação mostra o
  nome do controle de cada espaço, o que ajuda a perceber.
- Guitarra coop e teclado não têm nenhuma música na biblioteca de teste:
  só os testes com charts montados à mão os cobrem.
- O som de erro toca também na nota perdida (não só no aperto errado), já
  que não há como silenciar a faixa da guitarra: as faixas são misturadas
  numa só na carga.

---

## Etapa 5b: teclado dividido, teclas exclusivas, som de erro, idiomas (2026-10-29)

### Pedido (depois de jogar a etapa 5 no PC com um amigo)

- Liso, 60 FPS cravados no PC.
- A ideia era jogar em 4 (um no controle, três no teclado), mas só deu para
  2: sem controle, os jogadores 3 e 4 não tinham como entrar.
- Apertar o vermelho zerava o combo: a tecla que o J2 pegou continuava no
  J1, e os dois recebiam o mesmo aperto. As teclas devem ser exclusivas
  entre os jogadores.
- Com um teclado só, a preparação (instrumento, dificuldade) deve ser um
  jogador de cada vez, pelas setas e o Enter. Quem tem controle continua
  independente.
- O som de erro não tocou.
- Suporte a outros idiomas, de um jeito que outras pessoas possam alterar e
  acrescentar traduções.

### O que os arquivos mostraram

- `controles.cfg` (J1) tinha A S D F G; o `controles2.cfg` (J2), A S J K L:
  o S do amigo também era o vermelho do J1, que levava a penalidade. O W do
  J3 (vermelho) também era o whammy do J1.
- Os J3 e J4 tinham teclas nos trastes (Q W E R T e 4 5 6 7 8), mas as
  teclas de menu (Confirmar, setas) não são remapeáveis, e eles começam sem
  nenhuma: sem controle, nunca conseguiam entrar.
- `opcoes.cfg`: `som_erro = 0`. A opção estava desligada (as opções só são
  gravadas ao sair das Configurações, então ela nunca foi ligada).

### O que foi feito

**Teclas exclusivas** (`platform/input.c`): uma tecla de jogo é de um
jogador só. Ao mapear uma tecla que outro jogador usava, ela sai de lá (o
log diz de quem). Os arquivos antigos com tecla repetida são arrumados ao
abrir o jogo: fica com quem a escolheu de propósito (a tecla padrão do J1
perde para a escolhida pelo J2, J3 ou J4). Com os seus arquivos: A e S
ficam com o J2 e o W com o J3, e o J1 perde essas três no teclado (no
controle, nada muda).

**O teclado como um aparelho dividido**: as teclas de menu (setas, Enter,
Esc) deixaram de ser do J1 e passaram a ser do teclado, entregues à parte
(`Input_frame`). Um jogador sem controle e com teclas nos trastes é um
"jogador de teclado" (o main.c avisa o jogo a cada frame). Os J2 a J4
começam sem teclas: quem vai jogar no teclado escolhe as suas em Controles >
Jogador.

**Preparação com o teclado dividido** (`game/game.c`):
- quem vira jogador de teclado (mapeou teclas) já entra; depois continua
  dentro de uma música para a outra, como os de controle;
- o teclado atende um jogador de teclado por vez, na ordem (J1, J2...), com a
  etiqueta "vez do teclado" na coluna dele: as setas escolhem, **Enter** =
  pronto (a vez passa ao próximo), **Esc** = esse não joga esta música;
- sem ninguém na vez: Esc desfaz o último pronto, Enter traz de volta quem
  tinha saído;
- os jogadores de controle continuam cada um no seu, como antes;
- sem nenhum jogador de teclado, as teclas de menu valem como as do J1 (o
  jeito de sempre).

Exemplo com você no controle e três no teclado: mapeie as teclas do J2, J3 e
J4 em Controles; na preparação, o teclado configura o J2, Enter, o J3,
Enter, o J4, Enter; você confirma no controle e a partida começa com 4.

**Som de erro**: além de ligar a opção, ele foi refeito para aparecer por
cima da música. Antes era um tom grave (98 Hz) que alto-falantes pequenos
mal tocam; agora são dois dentes de serra a um semitom (a dissonância soa
"errado") com um chiado no ataque. Tem cerca de 6 dB a mais acima de 300 Hz.
Ao ligar a opção, ele toca uma vez, para você ouvir como é. O log de cada
partida diz se a opção estava ligada e quantas vezes o som tocou.

**Idiomas** (`game/texto.c`, `assets/lang/`):
- o texto em português é a chave: no código, `T("Jogar")` devolve a tradução
  do idioma escolhido, ou o próprio texto se faltar; textos em tabelas (os
  menus) são marcados com `N_()` e traduzidos na hora de desenhar;
- cada idioma é um arquivo `assets/lang/<codigo>.txt`, uma linha por texto
  (`Jogar = Play`); a opção Configurações > Idioma lista os arquivos que
  existem, e a escolha fica salva em `opcoes.cfg` (`idioma = en`);
- vieram `en.txt` (inglês) e `es.txt` (espanhol), completos (139 textos);
- segurança: se os `%d`/`%s` da tradução não baterem com os do original, a
  linha é ignorada (um `%s` no lugar de um `%d` derrubaria o jogo);
- acentos: a fonte ainda é só ASCII, então `é` vira `e`, `ñ` vira `n`, e o
  ¿ e o ¡ somem. Os nomes de música também ganharam isso (antes, acento
  virava `?`);
- `tools/textos.py` acha todos os textos do código e confere cada idioma (o
  que falta, o que sobra, os `%d/%s` errados); `tools/textos.py novo fr
  Francais` cria um idioma novo com todos os textos para traduzir; o
  `assets/lang/COMO_TRADUZIR.md` explica o formato.

**Testes:** 39 no `test_game` (novos: o teclado dividido com 4 jogadores, o
teclado sem jogadores de teclado e o J1 sem controle, e as traduções: busca,
texto que falta, `%d/%s` diferentes, acentos e a opção salva). As teclas
exclusivas foram conferidas num programa à parte com uma cópia dos seus
arquivos de controle.

### Como testar

- **4 jogadores, 1 controle:** Configurações > Controles > Jogador: J2 e
  mapeie os trastes dele; o mesmo para o J3 e o J4. Escolha uma música: o
  teclado atende o J2, o J3 e o J4 em ordem (Enter em cada um), e você
  confirma no controle.
- **Penalidade cruzada:** o amigo aperta o vermelho dele; o seu combo não
  pode mudar.
- **Som de erro:** Configurações > Som de erro > Sim: deve tocar na hora.
- **Idioma:** Configurações > Idioma > English / Español.

### Pendências

- Depois de jogar a demonstração (que é só do J1), os jogadores de teclado
  ficam fora: voltam com Enter na preparação.
- O som de erro toca também na nota perdida, e a música não silencia a
  guitarra (as faixas são misturadas numa só na carga). Separar a faixa da
  guitarra é um passo próprio.
- Alfabetos não latinos precisam de uma fonte TrueType.

---

## Etapa 5c: a fonte do jogo, com acentos (2026-10-29)

### Pedido

Depois de ver os menus e os idiomas da 5b: usar a fonte que você baixou
(`assets/terminal_16_unicode.zip`, "Terminal 16 Unicode") no lugar da fonte de
debug da SDL, no estilo do jogo, para os acentos e a pontuação aparecerem.

### A fonte

- É uma fonte de pixel (feita no Bits'n'Picas), monoespaçada, desenhada numa
  grade de **12x16** por caractere: 14 px acima da linha de base, 2 abaixo.
- Tem o ASCII, todo o latim com acentos (á, ç, ã, ñ, ¿, ¡...), boa parte do
  grego e do cirílico, e as setas ← ↑ → ↓ e ▲ ▼. Não tem ★, … nem —.
- Ela só fica nítida em múltiplos inteiros do tamanho dela: reduzida pela
  metade (para o 8x8 da fonte de antes) fica ilegível (testado).
- **Origem e licença:** "Terminal 16 Unicode", de artdzyk, baixada de
  https://www.dafont.com/pt/terminal-16-unicode.font, onde a licença está
  como **"100% Grátis"** (o filtro de fontes livres do dafont). O arquivo em
  si não traz texto de licença. O registro fica em `assets/fonts/FONTE.txt`,
  que o pacote copia para `licencas/`.

### O que foi feito

**Desenho** (`src/draw/fonte.c`, novo): o `stb_truetype` (um arquivo só,
domínio público ou MIT, em `third_party/stb/`, fixado num commit) lê a fonte
uma vez, no `Draw_init`, e desenha cada caractere numa célula de uma
textura. Cada letra na tela passa a ser a cópia de um pedaço dessa textura,
ampliada sem suavizar (`SDL_SCALEMODE_NEAREST`), sempre em escala inteira e
em posição inteira, para o pixel não "tremer". O stb usa a memória e a
matemática da SDL (nenhuma biblioteca a mais no pacote). O texto é UTF-8; o
que a fonte não tiver vira `?`. Sem o arquivo da fonte, o jogo usa a de
debug, esticada para a mesma célula, e o layout não quebra.

**As telas, refeitas para 12x16** (`src/draw/draw.c`): os textos pequenos
(8x8) e os de menu (16x16) agora usam a fonte no tamanho 1 (12x16); os
títulos, o tamanho 2; o "ARK HERO", o 3. O que mudou de lugar:
- a preparação virou uma grade 2x2 de cartões de 300 px (com 146 px, uma
  coluna só cabia 12 letras, pouco para "< Guitarra coop >");
- as dicas do rodapé ficaram mais curtas (cabem ~52 letras) e usam setas no
  lugar de "Esq/Dir" e "Cima/Baixo"; o que passar da largura é cortado com
  "..", em qualquer idioma;
- o placar de vários jogadores ganhou duas linhas embaixo de cada pista, e
  some na pausa e no fim (o resultado mostra os números);
- o painel do resultado, o do teste de áudio e o da captura de botão
  ficaram mais largos; textos longos (nomes de música, de controle, do
  áudio) são cortados sem partir um acento no meio.

**Português com acento**: os textos do jogo passaram a ser escritos certo
("Configurações", "Música", "Não", "Fácil"...). Como eles são as chaves das
traduções, o `en.txt` e o `es.txt` foram refeitos (148 textos cada); o
espanhol agora mostra ¿ e ¡. Os nomes de música também mantêm os acentos, e
`song.ini` antigos gravados em Latin-1 (um "é" num byte só) são convertidos.
O direcional aparece como setas (↑ ↓ ← →) e os frontais (Sul, Leste...)
são traduzidos.

**Ferramentas**: o `tools/textos.py` passou a ler os acentos do código
certo; o `package_port.sh` inclui a licença do stb_truetype e não leva
arquivos `.zip` de `assets/`.

**Testes**: 39 no `test_game`, no PC (ASan e UBSan) e no aarch64. As telas
foram conferidas em capturas, nos três idiomas.

### Pendências

- A página do autor diz que a fonte é "uma versão Unicode da fonte Terminal
  12x16 do MS-DOS" (a Terminal é da Microsoft). Para um jogo gratuito o
  risco é baixo, e o autor a liberou como 100% grátis; se um dia o jogo for
  vendido ou entrar numa loja, vale trocar por uma fonte de pixel com licença
  OFL ou domínio público explícito.
- O `assets/terminal_16_unicode.zip` continua na pasta (é o seu download);
  o jogo usa a cópia extraída em `assets/fonts/terminal16unicode.ttf`, e o
  pacote não leva o zip.
- No R36S, conferir o FPS com o texto novo (cada letra virou uma cópia de
  textura; na teoria é o mesmo custo da fonte de debug).

---

## Etapa 6: tela de jogadores, contas, controle que some, modo guitarra (2026-10-30)

### Pedido (depois de testar a 5c no R36S e no PC)

- No R36S: liso, a carga caiu muito (`tests/log1.txt` e `log2.txt`: 3 faixas
  em 5,6 s, 59,9 FPS o tempo todo, 2 jogadores sem engasgo), o som de erro
  tocou. Teste com 4 no PC e com uma guitarra (reconhecida como controle de
  Xbox).
- Escolher a quantidade de jogadores era pouco intuitivo: no PC começava com
  os 4 e era preciso Esc para tirar cada um.
- Se o controle do J1 desconecta, nada se reorganiza: só reconectando ele.
- Um par de controles sem fio chama L2/R2 de Select/Start: um acorde fechava
  o jogo. No R36S, esse par nem foi reconhecido.
- Pedidos: seleção de jogadores explícita antes de jogar (como no Clone
  Hero); contas com nome salvas no aparelho, com um "convidado" para vários;
  remanejo quando um controle some ou um jogador sai; modo guitarra (com
  palhetada); modo prática (velocidade, seções, loop A-B).

### O que foi feito

**Tela de jogadores** (`game/jogadores.c`, nova), entre "Jogar" e a lista de
músicas:
- um cartão por jogador. Quem tem controle: Confirmar entra, Esq/Dir troca a
  conta, Confirmar de novo = pronto; Voltar desfaz e, de novo, sai;
- quem joga no teclado: o teclado atende um cartão por vez (Cima/Baixo troca
  de cartão), com as mesmas teclas; ficar pronto passa a vez ao próximo;
- todos prontos: uma barra de 1,2 s e a lista de músicas (dá tempo de mais
  alguém entrar, o que cancela a contagem);
- **quem entrou aqui joga as próximas músicas**: a preparação de cada música
  não pergunta de novo quem joga, só o instrumento e a dificuldade. Lá, Esc
  ainda deixa alguém de fora só daquela música. Voltar da lista de músicas
  leva de volta para esta tela;
- o J1 já começa dentro, e os de teclado não entram mais sozinhos (era o que
  obrigava a apertar Esc em cada um).

**Contas**: "Convidado" (vários podem usar ao mesmo tempo) e contas com nome
(cada uma com um jogador por vez), em `conf/arkhero/jogadores.txt`, um nome
por linha. "+ novo jogador" abre um teclado na tela (letras, acentos do
português e do espanhol, números; maiúsculas/minúsculas, apagar, OK), que
cada um usa com o próprio controle. A última conta de cada jogador fica
salva nas opções. O nome aparece nos cartões, no placar, no resultado e no
recorde da música ("recorde: 104758 de João").

**Controle que some**: o jogo sabe, a cada frame, o aparelho de cada jogador.
- na tela de jogadores, quem perdeu o aparelho sai;
- na preparação, fica fora daquela música;
- na partida, ela pausa ("Um controle desconectou"); ligando outro, ele cai
  no primeiro espaço livre (o do jogador que perdeu) e a partida continua;
- o papel que era sempre do J1 (voltar para a lista, começar sem os
  outros) passou para o **líder**: o primeiro jogador que ainda tem aparelho.

**Controles problemáticos**: Select+Start não fecha mais o jogo quando um
desses dois é traste no perfil do jogador. O jogo também lê mapeamentos
extras de `conf/arkhero/gamecontrollerdb.txt`, o script do port aponta a
SDL para o banco de controles do PortMaster, e um joystick sem mapeamento
aparece no log com o nome e o GUID (para escrever a linha dele).

**Modo guitarra** (`judge.c`, `Judge_guitarra`), escolhido **por jogador** em
Controles > Modo de jogo (fica no perfil do controle, junto dos botões; a
palhetada voltou para a lista de Controles):
- palhetada com os trastes certos acerta. Nota simples: vale o traste mais
  alto segurado (pode ancorar nos de baixo). Acorde: os exatos. Aberta:
  nenhum traste;
- a palhetada pode vir até 50 ms antes dos trastes;
- HOPO: com o combo de pé, trocar de traste já toca. Tap: sempre. Uma
  palhetada até 100 ms depois de um HOPO tocado assim é ignorada;
- palhetada sem nota é erro (segue a opção "Penalidade");
- o traste de um sustain ainda tocando não atrapalha a nota de cima.

**Testes**: 43 no `test_game` (novos: tela de jogadores com teclado e conta
nova, controle desconectando, modo guitarra, folga e ancoragem), no PC com
ASan e UBSan. O jogador automático tocou as 78 músicas nos **dois** modos
(no da guitarra, palhetando as notas comuns e martelando os HOPO): 148.875
notas, full combo e zero penalidades em cada um.

### Como testar

- Jogar: a tela de jogadores. Sozinho, Confirmar duas vezes (entra já está;
  pronto) e a lista aparece. Com mais gente, cada um entra no seu controle.
- Contas: Esq/Dir até "+ novo jogador", Confirmar, escrever, OK.
- Desligar o controle do J1 na preparação: o J2 começa sozinho.
- Guitarra: Controles > Jogador (o da guitarra) > Modo de jogo: Guitarra, e
  mapear os trastes e a palheta (Palheta cima/baixo).
- Os controles do par sem fio: Select+Start (L2+R2) não deve mais fechar.
- No R36S: Ajuste de tempo **+40 ms** (os dois logs sugerem de novo).

---

## Etapa 6b: menus pela guitarra, jogadores presos ao aparelho, perfis por conta (2026-10-30)

### Pedido (depois de testar a 6 no PC com a guitarra)

- A guitarra funcionou bem, sem perda de desempenho. Antes do modo prática:
- **menus pela guitarra**: tirar o máximo de interações com Esq/Dir (a
  guitarra não tem). O verde numa opção entra num "modo de escolha" daquela
  opção, e aí se escolhe. Assim dá para usar o jogo todo sem teclado,
  mantendo o teclado funcionando;
- **controle que some na tela de jogadores**: o jogador continuava lá, e só
  saía selecionando com o teclado e apertando Esc (impossível num portátil
  ou game stick sem teclado). O jogador sem controle deve sumir sozinho, e
  quem vai dividir o teclado deve ser ajustado à mão, com um teclado, antes
  de valer;
- **perfil na conta**: o mapeamento e o modo (controle/guitarra) ficam na
  conta do jogador, não no número (J1..J4). O convidado usa os mapeamentos
  padrão de cada número;
- o modo controle/guitarra sai das Configurações e vai para a tela de
  jogadores.

### O que foi feito

**Modo de escolha** (todos os menus, `game.c`): um tipo novo de item,
`ITEM_VALOR` (velocidade, ajuste de tempo, volume, idioma e o Jogador da
tela de controles). Confirmar (o verde) entra no modo de escolha: a linha
fica ciano, o valor ganha ▲▼, Cima/Baixo (a palhetada) mudam, Confirmar
fica e Voltar (o vermelho) desfaz. As opções Sim/Não e as de poucos valores
(analógico, botões frontais, organizar as músicas) trocam direto no
Confirmar. Esq/Dir continuam mudando direto, para quem tem (teclado,
direcional). Segurar Cima/Baixo repete e, depois de 2 s, corre (a lista de
músicas inteira sem Esq/Dir); segurando, a lista para no fim, e um toque dá
a volta.

**Os cartões viraram listas** (tela de jogadores e preparação): Cima/Baixo
escolhe a linha, Confirmar entra no modo de escolha dela (ou executa),
Voltar desfaz, depois desfaz o pronto e, de novo, sai. O cursor começa no
"Pronto": com a mesma escolha de antes, um Confirmar basta.
- tela de jogadores: **conta** (modo de escolha), **modo** (controle /
  guitarra, troca direto), **Controles** (abre a tela de controles daquele
  jogador e volta para cá, sem tirar o pronto dos outros) e **Pronto**;
- preparação: **instrumento**, **dificuldade** (os dois com modo de
  escolha) e **Pronto**; embaixo, as notas e o modo.

**Cada jogador preso ao seu aparelho**:
- entra pelo controle do seu espaço, ou pelo teclado (`Jogador.no_teclado`);
- quem é de controle e perde o controle **sai sozinho** da tela de
  jogadores. Não vira mais jogador de teclado (era o que acontecia: o J1
  tinha teclas padrão);
- o teclado só entra num cartão **sem controle**, com Enter. Esq/Dir escolhe
  o cartão (a etiqueta "teclado" mostra qual), e ele atende um por vez;
- para ficar pronto, quem está no teclado precisa de teclas nos trastes e de
  nenhuma tecla repetida com outro jogador de teclado. Senão, a linha
  Controles ganha um "!", o cartão diz "sem teclas" ou "teclas repetidas",
  e o Confirmar avisa "Ajuste as teclas em Controles";
- as teclas de jogo só chegam a quem está no teclado (antes chegavam a todo
  perfil que tivesse a tecla, mesmo jogando no controle);
- um controle novo pula os espaços de quem está no teclado;
- quem apertou "Jogar" já entra (o controle dele, ou o teclado no primeiro
  cartão sem controle). Ninguém mais começa dentro sem ter apertado nada;
- sair não fecha a tela: com a sessão vazia, mais um Voltar vai para o menu.

**Perfis por conta** (`input.c`, `Input_use_account`): cada conta tem o seu
perfil (botões, teclas, analógico, frontais, modo) em
`conf/arkhero/contas/<código>.cfg` (o código vem do nome, porque nomes com
acento podem dar problema no cartão SD; a primeira linha do arquivo diz de
quem é). O convidado usa o perfil do número, como antes
(`controles.cfg`, `controles2.cfg`...), com as teclas padrão do J1. Uma conta
sem arquivo usa o perfil do convidado, e o arquivo dela nasce na primeira
mudança (mudar o modo, remapear). O main.c troca o perfil do espaço quando a
conta muda (inclusive enquanto se passa pelas contas, para mostrar o modo de
cada uma), e quem sai da sessão volta ao convidado. A tela de controles
mostra de quem é ("J2 Ana").

**Teclado da tela no modo guitarra**: sem Esq/Dir, a palhetada anda tecla por
tecla, na ordem de leitura (segurar acelera; para trás desde o A cai no OK).
Com setas, a grade de sempre.

**Outros**: o Esc do teclado pausa a partida (as teclas de jogo agora são só
de quem está no teclado); a demonstração usa o primeiro jogador da sessão;
na preparação sem ninguém no teclado, as teclas de menu ajudam o líder (no
PC, quem joga no controle usa o Enter); o rodapé só mostra as teclas
("Sul/Enter") depois que alguém usou o teclado (no R36S, nunca).

**Testes**: 47 no `test_game` (novos: modo de escolha nos menus, quem some
sai, teclado só em cartão livre, linhas de modo e controles, teclado da
tela na guitarra; atualizados: preparação em linhas, lista que acelera e
para no fim), no PC com ASan e UBSan e no aarch64. Um teste à parte do
`input.c` (eventos de teclado sintéticos): teclas só para quem está no
teclado, perfil da conta criado na primeira mudança e lido de volta, tecla
remapeada saindo de outro jogador de teclado, tecla repetida detectada. O
jogador automático: 78 músicas, 148.875 notas, zero problemas nos dois
modos. Traduções completas (166 textos em en e es).

### Como testar

- Tudo pela guitarra: palhetada para andar, verde para entrar/escolher,
  vermelho para voltar/desfazer. Configurações > Velocidade: verde, palheta,
  verde.
- Tela de jogadores: desligar um controle de quem entrou (ele some). No PC,
  Enter num cartão sem controle põe um jogador de teclado; ←/→ troca de
  cartão.
- Modo guitarra: na tela de jogadores, linha "modo". Ele fica na conta: com
  uma conta, trocar o modo ou remapear em "Controles" e conferir que volta
  igual na próxima vez (o arquivo aparece em `conf/arkhero/contas/`).
- Dois no teclado com as mesmas teclas: o segundo não fica pronto até mudar
  em Controles.

---

## Etapa 7 (planejada): modo prática

Pedido: um modo sem como perder, com a velocidade da música ajustável
durante a partida e a escolha de trechos com loop e pontos A-B.

- **Seções**: os charts já trazem as seções ("section Intro", "Solo 1"...):
  no `.chart`, em `[Events]`; no `.mid`, na trilha EVENTS. O leitor passa a
  guardá-las (nome e tempo).
- **Tela da prática**: escolher a seção inicial (A) e a final (B), ou uma
  só; a velocidade (50% a 100%, de 5 em 5); e, durante a partida, botões
  para mudar a velocidade e marcar A e B na hora.
- **Loop**: chegando em B, volta para A (o áudio volta para a posição de A,
  e as notas de A a B voltam a ficar pendentes), com uma contagem curta.
- **Velocidade**: o jeito simples é tocar o áudio mais devagar
  (`SDL_SetAudioStreamFrequencyRatio`), o que também deixa o som mais grave,
  como uma fita. Manter o tom exige esticar o áudio no tempo (WSOLA), feito
  na hora, enquanto toca, e com o relógio do jogo acompanhando a posição de
  verdade na música. Proposta: começar pelo simples e trocar pelo esticado
  se o som grave incomodar.
- **Sem perder**: hoje já não existe "perder" (não há barra de rock); o modo
  prática também não grava recorde.

---

## Próximos passos (análise de 2026-10-28)

### A guitarra (modo guitarra), para testar no PC

O que já existe: as ações de palhetada (cima/baixo) no input, as flags de
HOPO e tap em cada nota, o item "Modo de jogo" no menu, e o juiz isolado
em `judge.c`.

O que falta, em ordem:

1. **Reconhecer o controle.** A SDL3 identifica guitarras como
   `SDL_JOYSTICK_TYPE_GUITAR`. Para o tipo mais comum no PC (guitarra de
   Xbox 360 / Rock Band): trastes em A, B, Y, X, LB; palhetada no d-pad;
   whammy no eixo X do analógico direito; inclinação (star power) no eixo
   Y dele. Um perfil automático por tipo de controle.
2. **O juiz do modo guitarra**, ao lado do atual (o modo controle fica):
   - palhetada com os trastes certos acerta; palhetada sem nota é erro;
   - nota simples aceita trastes abaixo segurados ("ancorar"); acorde exato;
   - HOPO: acerta trocando de traste sem palhetar, se a nota anterior foi
     acertada; tap: sempre sem palhetar;
   - uma palhetada logo depois de um HOPO não pune (folga);
   - testes para cada regra: é onde moram os bugs desse gênero de jogo.
3. **Menus:** "Modo de jogo: Controle / Guitarra", palhetada de volta na
   tela de Controles, e o perfil da guitarra.

### O que está ficando de lado

**Para fazer agora / decidir:**
- **Ajuste de tempo no R36S:** colocar em +40 ms (pelo log).
- **Licença do próprio Ark Hero:** o repositório não tem um arquivo de
  licença; sem ele, legalmente ninguém pode usar o código. Escolher (MIT,
  GPL...) antes de divulgar.
- **A branch `main`:** decidir se a `claudio` entra na `main` ou se a
  `main` fica como a sua versão de estudo.
- **Documentos:** a linha "Estado" do README e a ARQUITETURA.md descrevem o
  jogo de semanas atrás.

**Para a biblioteca da comunidade:**
- ~~outros instrumentos~~ (feito na etapa 5);
- `.sng`; nomes com acento (fonte TrueType); capa do álbum (`album.jpg`
  pede um leitor de JPEG); prévia da música na lista.

**Para a partida:**
- estrelas por música; barra de rock (hoje não dá para perder); silenciar a
  guitarra quando erra (o som de erro foi feito na etapa 5); modo treino por seção (as seções do
  `[Events]` já estão nos arquivos).

**Distribuição:**
- testar em outro firmware (ROCKNIX, EmuELEC, ArkOS clássico: ver a análise
  da glibc na etapa 5); build x86_64; o `port.json` e as imagens do PortMaster.

**O objetivo original: Lua** (temas, regras, modificadores, carreira:
ver a etapa 4c).
