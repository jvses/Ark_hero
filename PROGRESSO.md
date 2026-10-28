# Progresso do Ark Hero (branch `claudio`)

Registro de cada etapa feita na branch `claudio`: o que foi pedido, o que foi
feito e o que falta conferir. O código desta branch é escrito pelo Claude sob
a direção do João. Cada etapa é testada no R36S antes do commit.

## Roteiro

| # | Etapa | Estado |
|---|---|---|
| 0 | Arrumar a casa: módulos, input com remapeamento, menus, pista | **feita, aguardando teste no R36S** |
| 1 | Visualizador de trastes (cada botão acende a faixa) | adiantada na etapa 0 (pista de teste) |
| 2 | Áudio + relógio mestre (metrônomo gerado por código) | |
| 3 | Parser de `.chart` + mapa de tempo + testes | |
| 4 | Pista com as notas de uma música de verdade | |
| 5 | Juiz + pontuação | |
| 6 | Lua: `config.lua` e `regras.lua`, com recarga a quente | |
| 7 | Calibração de áudio e vídeo | |
| 8 | Biblioteca de músicas + cache | |

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

1. **Azul = R1.** O pedido tinha L1 duas vezes (Verde e Azul) e nenhum R1.
   Se a intenção era outra, troque na tela de Controles ou na tabela
   `PADRAO` de `src/platform/input.c`.
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
