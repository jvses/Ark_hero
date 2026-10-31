# Manual do Ark Hero

*English version: [MANUAL.en.md](MANUAL.en.md).*

O Ark Hero é um jogo de ritmo de guitarra que toca as músicas (charts) feitas
pela comunidade do Clone Hero. Nenhuma música acompanha o jogo: você traz a
sua biblioteca.

- [Instalação](#instalação)
- [As músicas](#as-músicas)
- [Uma partida, passo a passo](#uma-partida-passo-a-passo)
- [Como se joga](#como-se-joga)
- [Pausa](#pausa)
- [Modo prática](#modo-prática)
- [Configurações](#configurações)
- [Contas e controles](#contas-e-controles)
- [Onde ficam os arquivos](#onde-ficam-os-arquivos)
- [Problemas comuns](#problemas-comuns)

## Instalação

**PortMaster (R36S e outros handhelds Linux):** extraia o `arkhero.zip` na
pasta `ports/` da partição de ROMS. Ficam `ports/Ark Hero.sh` e
`ports/arkhero/`. A instalação por zip do próprio PortMaster também funciona.
O jogo aparece na lista de Ports.

**Windows 10 ou mais novo (64 bits):** extraia o
`ArkHero-<versão>-windows-x86_64.zip` em qualquer pasta e rode o
`arkhero.exe`. F11 alterna entre janela e tela cheia.

**Linux no PC:** compile pelo código (veja o [README](../README.md)).

## As músicas

Cada música é uma pasta com:

- o chart: `notes.chart` ou `notes.mid`;
- o `song.ini` (nome, artista, charter, atraso);
- o áudio: `song`, `guitar`, `bass`, `rhythm`, `keys`, `vocals`, `drums`... em
  `.opus`, `.ogg`, `.mp3` ou `.wav`. As faixas separadas são misturadas.

Onde colocar:

- no console: `ports/arkhero/songs/`;
- no Windows: a pasta `songs\` ao lado do `arkhero.exe`.

Subpastas valem ("Guitar Hero 3/Tier 1/Música"), e a lista de músicas segue
elas. Para usar uma biblioteca em outro lugar (outro cartão, a pasta do Clone
Hero no PC), escreva o caminho dela em `pastas.txt`, uma pasta por linha (veja
[Onde ficam os arquivos](#onde-ficam-os-arquivos)). Depois de acrescentar
músicas com o jogo aberto, use "Procurar músicas novas", no fim da lista.

Ainda não são lidos: arquivos `.sng`, bateria, voz e capas de álbum.

Sem nenhuma música, a entrada "Demonstração (sem música)" toca notas de exemplo com um
metrônomo.

## Uma partida, passo a passo

1. **Jogar**, no menu inicial.
2. **Jogadores:** cada jogador entra apertando Confirmar no próprio controle.
   No teclado, Enter. Cada um tem um cartão com quatro linhas:
   - **Conta:** um nome salvo, ou convidado;
   - **Modo:** controle ou guitarra (veja [Como se joga](#como-se-joga));
   - **Controles:** os botões daquela conta;
   - **Pronto:** quando todos estão prontos, o jogo vai para as músicas.
3. **Músicas:** escolha a música. Esquerda/Direita pulam uma página. O primeiro
   item organiza a lista por pasta, título ou artista.
4. **Preparação:** cada jogador escolhe o instrumento (guitarra, baixo,
   rítmica, guitarra coop, teclado) e a dificuldade, entre as que o chart
   tem, e fica pronto. Se alguém demorar, quem manda (o primeiro jogador)
   aperta Confirmar de novo e a música começa sem ele.
5. A música carrega e a partida começa. No fim aparece o resultado de cada
   um: pontos, porcentagem de notas, maior combo e, com 20 acertos ou mais,
   uma sugestão de [Ajuste de tempo](#configurações).

Os menus funcionam só com uma guitarra: a palhetada anda (cima/baixo), o
traste verde confirma e o vermelho volta.

## Como se joga

As notas descem pela pista. Cada cor é um traste: verde, vermelho, amarelo,
azul e laranja. Cada jogador escolhe um destes modos:

- **Modo controle** (o padrão): apertar o botão no tempo da nota já toca a
  nota. Não existe palhetada. Num acorde, os botões apertados precisam ser
  exatamente os do acorde.
- **Modo guitarra:** segure os trastes e palhete. Numa nota simples valem os
  trastes de baixo segurados junto. HOPO (nota com o miolo colorido) e tap
  tocam sem palhetar, trocando de traste.

O resto vale para os dois modos:

- **Sustain:** depois de acertar a cabeça, segure até o fim da cauda. Soltar
  antes não quebra o combo, só para de somar pontos.
- **Combo e multiplicador:** cada 10 acertos seguidos sobem o multiplicador,
  até 4x.
- **Star power:** as notas com anel ciano formam frases. Uma frase inteira
  acertada enche 25% do medidor, e o whammy num sustain dessas frases enche
  mais. Com o medidor em 50% ou mais, aperte o botão de star power: o
  multiplicador dobra enquanto o medidor esvazia (16 s, cheio).
- **Penalidade:** apertar (ou palhetar) sem nota quebra o combo. Pode ser
  desligada.

## Pausa

Start (ou Esc, no teclado) pausa a partida. Pela pausa dá para continuar,
reiniciar ou sair para a lista de músicas.

Ao continuar, a música **volta 2 segundos**, com uma contagem na tela, para
você retomar o ritmo. Até a música chegar de novo onde parou:

- as notas que você já tinha acertado aparecem apagadas ("fantasmas"), para
  tocar junto;
- apertar fora de nota não pune;
- o sustain que estava segurado e o star power ficam como estavam.

Se o controle de alguém desconecta no meio da música, quem ficou continua
jogando e as notas de quem caiu só passam. Se não sobra ninguém, a partida
pausa.

## Modo prática

Para treinar um trecho difícil, em loop e mais devagar, sem contar recorde.

1. **Prática**, no menu inicial. Depois, jogadores, música e preparação, como
   numa partida.
2. **A tela da prática:**
   - **Começo (A)** e **Fim (B):** as partes da música ("Intro", "Verse 1",
     "Solo"...), as mesmas do Clone Hero. Se o chart não tiver partes, o jogo
     cria uma a cada 8 compassos ("Compasso 1", "Compasso 9"...). Embaixo
     aparecem o tempo e quantas notas o trecho tem;
   - **Velocidade da música:** de 50% a 100%, de 5 em 5. A velocidade menor
     também deixa o som mais grave, como uma fita mais lenta;
   - **Começar.** O áudio já vai carregando enquanto você escolhe.
3. **O trecho** começa 2 segundos antes de A, com uma contagem. Ao chegar em
   B, volta para A sozinho. No alto aparecem a velocidade, o trecho, a volta e
   a porcentagem de notas da volta anterior.
4. **A pausa da prática** tem mais opções:
   - **Velocidade da música:** muda na hora;
   - **Marcar o começo aqui:** o trecho passa a começar no compasso onde a
     música está, a partir da próxima volta;
   - **Marcar o fim aqui:** o trecho termina no fim deste compasso e já
     recomeça. Para marcar um trecho na hora: pause no começo dele, marque o
     começo, continue, pause no fim e marque o fim;
   - **Escolher o trecho:** volta para a tela da prática;
   - **Recomeçar o trecho**, ou **Sair para o menu**.

Para voltar ao jogo normal, use **Jogar** no menu inicial.

## Configurações

| Opção | O que faz |
|---|---|
| Controles | os botões e teclas de cada jogador (veja abaixo) |
| Velocidade da pista | de 1 a 10: quão rápido as notas descem (não muda a música) |
| Modo precisão | janela de acerto de ±70 ms em vez de ±100 ms |
| Penalidade (erro) | apertar sem nota quebra o combo |
| Mostrar acertos | "Perfeito / Bom / Errou" e o erro em ms, sobre a pista |
| Ajuste de tempo | compensa o atraso do som ou da tela, de -300 a +300 ms. Use a sugestão do resultado |
| Modo canhoto | inverte a ordem das faixas |
| Mostrar FPS | os quadros por segundo, no canto |
| Volume | de 0 a 100% |
| Som de erro | o instrumento "engasga" quando você erra |
| Idioma | português, inglês ou espanhol. Na primeira vez, o do sistema |
| Teste de áudio | toca a primeira música da lista e mostra os relógios do áudio |

## Contas e controles

Uma conta guarda o nome, o mapeamento de botões, as teclas e o modo
(controle ou guitarra). Crie uma na linha **Conta** do seu cartão, na tela de
jogadores. O convidado usa o mapeamento do espaço (jogador 1, 2...).

Em **Configurações > Controles** (ou na linha **Controles** do cartão), escolha
uma ação e aperte o botão novo. Também dá para trocar o analógico do whammy e
o estilo dos botões frontais (automático, Xbox ou Nintendo), e restaurar o
padrão.

- **Guitarras USB** são reconhecidas como guitarra. Mapeie os trastes uma vez
  e eles ficam salvos na conta.
- **Teclado:** pode ser dividido entre jogadores. Cada um escolhe teclas que
  não se repitam; o cartão avisa se faltam teclas ou se há teclas repetidas.
- **Select + Start** fecha o jogo no console.

Controles padrão:

| Ação | R36S | Teclado (jogador 1) |
|---|---|---|
| Trastes verde, vermelho, amarelo, azul, laranja | R1, R2, L2, L1, botão de baixo | A, S, D, F, G |
| Palhetada (modo guitarra) | direcional cima / baixo | cima / baixo |
| Star power | Select, ou o analógico para cima | Espaço |
| Whammy | analógico para os lados | W |
| Pausa | Start | Esc |
| Menus: ok / volta | botão de baixo / da direita | Enter / Esc |

## Onde ficam os arquivos

| | Console (PortMaster) | Windows |
|---|---|---|
| Músicas | `ports/arkhero/songs/` | `songs\` |
| Dados salvos | `ports/arkhero/conf/arkhero/` | `conf\arkhero\` (ou `%APPDATA%\arkhero\`) |
| Log | `ports/arkhero/log.txt` | `conf\arkhero\log.txt` |

O Windows usa o `%APPDATA%` quando não dá para gravar na pasta do jogo (em
Arquivos de Programas, por exemplo).

Na pasta de dados:

- `pastas.txt`: outras pastas de músicas, uma por linha (`#` comenta);
- `opcoes.cfg`: as configurações;
- `jogadores.txt`: as contas;
- `contas/`, `controles.cfg`, `controles2.cfg`...: os mapeamentos;
- `recordes.txt`: o recorde de cada música.

## Problemas comuns

- **Nenhuma música na lista:** confira se cada música tem uma pasta própria,
  com o chart e o áudio dentro, e se a pasta está em `songs/` ou no
  `pastas.txt`. O log diz quantas pastas foram olhadas e quantas músicas
  foram achadas.
- **As notas chegam antes ou depois do som:** jogue uma música inteira e use
  a sugestão de Ajuste de tempo da tela de resultado.
- **O controle não responde:** remapeie em Controles. Controles sem fio que
  dividem um mesmo receptor USB ainda podem se confundir; um por receptor
  funciona melhor.
- **Algo deu errado:** o `log.txt` registra o que o jogo achou, o que carregou
  e um resumo de cada partida. Mande ele junto quando relatar um problema.
