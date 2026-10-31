# Ark Hero

*A guitar rhythm game in C11 + SDL3 that plays Clone Hero charts, made for
Linux handhelds (R36S and others, through PortMaster) and also built for
Windows. Menus in English, Portuguese and Spanish.*

Jogo de ritmo de guitarra no estilo Guitar Hero / Clone Hero, escrito em C11
com SDL3, feito para rodar nativamente em handhelds Linux (alvo principal: R36S
com dArkOS) e distribuído no formato do PortMaster. Também tem build para
Windows. O jogo lê as músicas (charts) que a comunidade do Clone Hero já
produziu.

**Estado:** jogável, versão 0.1. Testado no PC (Linux e Windows, teclado,
controles e guitarra USB); as versões anteriores foram testadas no R36S.

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
- Modo prática: um trecho da música (pelas seções do chart, ou marcado na
  hora) em loop, de 50% a 100% da velocidade.
- Ao despausar, a música volta 2 segundos, com uma contagem, e as notas já
  tocadas aparecem apagadas para tocar junto.
- Organização da lista por pastas, título ou artista; ajuste de tempo;
  modo precisão; idiomas.

Nenhuma música acompanha o jogo: quem joga traz a própria biblioteca.

O [manual](docs/MANUAL.md) explica tudo: as músicas, a partida, a prática,
as configurações, as contas e os problemas comuns
([English manual](docs/MANUAL.en.md)).

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
docs/           o manual do jogo
tools/          scripts de build e empacotamento
docker/         ambientes de build (Debian 12: console e Windows)
packaging/      script de launch e metadados do PortMaster; recursos do Windows
third_party/    código-fonte das dependências (SDL3, Xiph, dr_mp3, stb_truetype)
```

## Licença e créditos

O código do Ark Hero é software livre, sob a **GNU General Public License
v3.0** ([LICENSE](LICENSE)): qualquer um pode usar, estudar, modificar e
redistribuir, desde que as versões modificadas que forem distribuídas também
fiquem livres, com o código aberto e a mesma licença.

A fonte e as imagens de fundo não são do projeto e seguem as licenças dos
seus autores:

- **Fonte:** "Terminal 16 Unicode", de artdzyk, baixada do
  [dafont](https://www.dafont.com/pt/terminal-16-unicode.font) ("100% Grátis").
  Detalhes em [assets/fonts/FONTE.txt](assets/fonts/FONTE.txt).
- **Imagens de fundo** (detalhes em
  [assets/images/CREDITOS.txt](assets/images/CREDITOS.txt)):
  - `vaporwave_guitar1.png`: foto de Denis Feofilaktov, no
    [Unsplash](https://unsplash.com/pt-br/fotografias/guitarra-eletrica-e-amplificador-em-iluminacao-roxa-fraca-6pZwlZ0ff6s)
    ([Licença Unsplash](https://unsplash.com/license));
  - `vaporwave_guitar2.png`: foto de Xie lipton, no
    [Unsplash](https://unsplash.com/pt-br/fotografias/guitarra-eletrica-roxa-e-preta-SjrgIBKSLW0)
    ([Licença Unsplash](https://unsplash.com/license));
  - `vaporwave_guitar3.png`: foto de ANTONI SHKRABA production, no
    [Pexels](https://www.pexels.com/pt-br/foto/bateria-percussao-tambores-instrumento-de-percussao-8041089/)
    ([Licença Pexels](https://www.pexels.com/license/)).

As bibliotecas usadas, cada uma com a sua licença (todas compatíveis com a
GPL-3.0):

| Biblioteca | Para quê | Licença |
|---|---|---|
| [SDL3](https://libsdl.org) | janela, vídeo, input, áudio | zlib |
| [libogg](https://xiph.org/ogg/), [libvorbis](https://xiph.org/vorbis/) | áudio Ogg Vorbis | BSD 3 cláusulas |
| [opus](https://opus-codec.org), [opusfile](https://opus-codec.org) | áudio Opus | BSD 3 cláusulas |
| [dr_mp3](https://github.com/mackron/dr_libs) | áudio MP3 | domínio público (Unlicense) ou MIT-0 |
| [stb_truetype](https://github.com/nothings/stb) | a fonte | domínio público (Unlicense) ou MIT |

Os pacotes levam todas essas licenças na pasta `licenses/`. O formato dos
charts e as músicas são da comunidade do Clone Hero.

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
