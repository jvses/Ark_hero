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
| 4b | Ajustes: janela de acerto, modo precisão, logs, procura de músicas, navegação, placar | **feita; falta testar** |
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
