# Ark Hero

*A guitar rhythm game in C11 + SDL3 that plays Clone Hero charts, made for
Linux handhelds (R36S and others, through PortMaster) and also built for
Windows. Menus in English, Portuguese and Spanish.*

Jogo de ritmo de guitarra no estilo Guitar Hero / Clone Hero, escrito em C11
com SDL3, feito para rodar nativamente em handhelds Linux (alvo principal: R36S
com dArkOS) e distribuído no formato do PortMaster. Também tem build para
Windows. O jogo lê as músicas (charts) que a comunidade do Clone Hero já
produziu.

**Estado:** jogável, versão 0.1. Testado no PC (Linux, teclado, controles e
guitarra USB); as versões anteriores foram testadas no R36S.

- Charts `.chart` e `.mid`, com áudio em Opus, Ogg Vorbis, MP3 ou WAV (as
  faixas separadas de cada instrumento são misturadas).
- Guitarra, baixo, rítmica, guitarra coop e teclado, nas quatro dificuldades.
- Sustains, HOPO, tap, star power (com o whammy), multiplicador, recordes.
- Até 4 jogadores ao mesmo tempo: controles, guitarras USB e o teclado
  (que pode ser dividido entre jogadores).
- Cada jogador escolhe o modo controle (aperta os trastes no tempo) ou o modo
  guitarra (segura os trastes e palheta).
- Contas de jogador: os controles, as teclas e o modo ficam salvos na conta.
- Os menus funcionam inteiros pela guitarra (palhetada e trastes).
- Organização da lista por pastas, título ou artista; ajuste de tempo;
  modo precisão; idiomas.

Nenhuma música acompanha o jogo: quem joga traz a própria biblioteca.

## Instalando

**PortMaster (R36S e outros handhelds):** extraia o `arkhero.zip` na pasta
`ports/` da partição de ROMS (fica `ports/Ark Hero.sh` e `ports/arkhero/`), ou
use a instalação por zip do próprio PortMaster. As músicas vão em
`ports/arkhero/songs/`, uma pasta por música (`notes.chart` ou `notes.mid`,
`song.ini` e o áudio). Uma biblioteca em outro lugar do cartão pode ser
indicada em `ports/arkhero/conf/arkhero/pastas.txt`.

**Windows (10 ou mais novo, 64 bits):** extraia o
`ArkHero-<versão>-windows-x86_64.zip` e rode o `arkhero.exe`. As músicas vão
em `songs\`, ou indique a pasta do seu Clone Hero em
`conf\arkhero\pastas.txt`. F11 alterna a tela cheia. Veja o `README.txt` do
pacote.

## Controles padrão

| Ação | R36S | Teclado (jogador 1) |
|---|---|---|
| Trastes verde, vermelho, amarelo, azul, laranja | R1, R2, L2, L1, botão de baixo | A, S, D, F, G |
| Palhetada (modo guitarra) | direcional cima / baixo | cima / baixo |
| Star power | Select, ou o analógico para cima | Espaço |
| Whammy | analógico para os lados | W |
| Pausa | Start | Esc |
| Menus: ok / volta | botão de baixo / da direita | Enter / Esc |
| Sair | Select + Start | fechar a janela |

Tudo pode ser trocado em Configurações > Controles, para cada conta.

## Compilando

Para desenvolver no PC Linux (precisa da SDL3, da opusfile e da libvorbis
instaladas, ou de `-DCMAKE_PREFIX_PATH=<pasta da SDL3>`):

```bash
./tools/build_local.sh run
./build/local/test_game          # os testes da lógica, sem SDL
```

Os pacotes de distribuição são compilados em containers Docker (Debian 12),
que compilam a SDL3 e as bibliotecas de áudio a partir de `third_party/src/`:

```bash
./tools/docker_build.sh aarch64     # o binário do console (arm64, emulado pelo qemu-user-static)
./tools/package_port.sh             # dist/arkhero.zip (PortMaster)

./tools/docker_build.sh windows     # arkhero.exe, por compilação cruzada (MinGW-w64)
./tools/package_windows.sh          # dist/ArkHero-<versão>-windows-x86_64.zip
```

## Estrutura

```
src/platform/   janela, renderer, input, áudio, arquivos (SDL3)
src/game/       lógica do jogo (sem SDL; testada em tests/)
src/draw/       desenho
assets/         fonte, imagens de fundo e traduções (assets/lang/)
tools/          scripts de build e empacotamento
docker/         ambientes de build (Debian 12: console e Windows)
packaging/      script de launch e metadados do PortMaster; recursos do Windows
third_party/    código-fonte das dependências (SDL3, Xiph, dr_mp3, stb_truetype)
```

As licenças das bibliotecas e da fonte vão nos pacotes, na pasta `licenses/`.

## Sobre o uso de IA

Este é um projeto de aprendizado de C, SDL3 e Lua, e o uso de IA muda
conforme a branch:

- **`main`**: o código do jogo é escrito por mim. Uso IA como orientadora,
  para tirar dúvidas, discutir abordagens e entender melhor a biblioteca e o
  código. Os scripts de compilação, docker e empacotamento foram feitos quase
  inteiramente com IA.
- **`claudio`**: o jogo é desenvolvido com IA (Claude), sob a minha direção.
  Eu defino o que entra em cada etapa, testo no PC e no R36S e faço os
  commits; o código é escrito pela IA. O registro de cada etapa (o que pedi e
  o que foi feito) está em [PROGRESSO.md](PROGRESSO.md).
