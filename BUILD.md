# Ark Hero -- como compilar, empacotar e portar

Documento de referencia do processo. A ideia e que ele seja repetivel: os
mesmos comandos, sempre, independente do que mudou no codigo.

---

## 1. As duas rotas de build (e por que sao duas)

| | Rota de desenvolvimento | Rota de release |
|---|---|---|
| Onde | direto no seu Arch | container Debian 12 |
| SDL3 | a do sistema (`pacman -S sdl3`) | compilada do tarball |
| Velocidade | segundos | minutos (a primeira vez, bastante) |
| Roda no R36S? | **nao** | sim |
| Comando | `./tools/build_local.sh run` | `./tools/docker_build.sh aarch64` |

Voce vai usar a rota de desenvolvimento 95% do tempo. A de release so quando
for levar algo pro aparelho.

**Por que a rota local nao serve pro console:** seu Arch tem glibc 2.42. Um
binario compilado ali carrega referencias a simbolos que so existem na 2.42.
O dArkOS tem 2.41 -- o binario nem carrega. Nao e questao de "quase funciona":
falha no `execve` com erro de simbolo nao encontrado.

---

## 2. As camadas de compatibilidade

Sao quatro coisas distintas, e vale nao confundi-las.

### Camada 1 -- arquitetura (aarch64 x x86_64)
Instrucoes de CPU diferentes. Resolvida compilando uma vez por arquitetura.
O pacote leva as duas, e o script escolhe via `$DEVICE_ARCH`.

### Camada 2 -- glibc (a que importa de verdade)
A glibc e compativel **para frente**, nunca para tras. Binario feito contra
2.36 roda em 2.36, 2.41, 2.99. Binario feito contra 2.42 so roda em >= 2.42.

Por isso o container e Debian 12 (glibc 2.36) e nao algo recente:

    dArkOS   2.41   <- seu aparelho (CFW_GLIBC=241)
    EmuELEC  2.36   <- o mais restritivo que ainda importa
    ROCKNIX  mais novo
    Batocera mais novo

2.36 e o piso certo hoje. Fica de fora so o ArkOS antigo, que o proprio
PortMaster esta descontinuando pelo mesmo motivo. Precisar de mais alcance no
futuro = trocar uma linha no `docker/Dockerfile` para `debian:bullseye` (2.31).

### Camada 3 -- bibliotecas do sistema (EGL, GLES, ALSA, DRM)
Essas **nao** viajam no pacote, e nem devem. A SDL as carrega por `dlopen()`
em tempo de execucao, nao no link. Ou seja: a `libSDL3.so` que voce compila no
Debian 12 vai usar o Mesa/Panfrost do dArkOS, o do ROCKNIX, o do que for.
E exatamente isso que faz uma unica build servir varios firmwares.

E tambem por isso a SDL **nao** deve ser linkada estaticamente: isso
congelaria essas dependencias.

### Camada 4 -- achar a libSDL3 empacotada
Duas travas, de proposito:
- `INSTALL_RPATH "$ORIGIN/libs.<arch>"` no CMake -- o binario procura a lib
  numa pasta relativa a ele mesmo. Funciona ate rodando na mao.
- `LD_LIBRARY_PATH` no script de launch -- o jeito padrao do PortMaster.

---

## 3. Docker em cinco minutos

Se voce nunca usou, sao so tres conceitos:

**Imagem** = um sistema de arquivos pronto, imutavel, descrito por um
`Dockerfile`. A nossa e "um Debian 12 com gcc, cmake e os headers da SDL".
Voce constroi uma vez.

**Container** = uma execucao daquela imagem. Descartavel (`--rm` apaga no
fim). Nada do que acontece dentro dele afeta seu sistema.

**Bind mount** (`-v "$ROOT:/work"`) = a pasta do projeto no seu disco aparece
dentro do container. Nao e copia: e a mesma pasta. O container compila, e os
arquivos ja estao no seu disco quando ele morre. E assim que o build sai de la.

Um detalhe a mais no nosso caso: **`--platform linux/arm64`**. Isso roda uma
imagem ARM na sua maquina x86. Funciona porque o kernel Linux tem `binfmt_misc`
-- quando ele ve um binario ARM, chama o `qemu-aarch64-static` pra interpretar.
Na pratica: dentro do container, `uname -m` responde `aarch64` e o `gcc` e um
gcc ARM nativo. Zero cross-compile, zero toolchain file, zero sysroot. So e
lento, porque e emulado.

E lento onde importa pouco: uma vez na sua maquina, em vez de toda vez na
maquina do jogador.

---

## 4. Setup inicial (uma vez so)

```bash
# daemon do docker (voce ja esta no grupo docker)
sudo systemctl enable --now docker

# o interpretador ARM + registro no binfmt_misc
sudo pacman -S qemu-user-static qemu-user-static-binfmt

# conferir que funcionou: deve imprimir "aarch64"
docker run --rm --platform linux/arm64 debian:bookworm uname -m
```

Se esse ultimo comando responder `aarch64`, esta tudo pronto. Se der
`exec format error`, o binfmt nao pegou -- reinicie ou rode
`sudo systemctl restart systemd-binfmt`.

---

## 5. Dia a dia (desktop)

```bash
./tools/build_local.sh run
```

Compila e abre a janela. ESC ou Start fecham antes dos 10s.

Variaveis uteis:

```bash
ARKHERO_FULLSCREEN=1 ./build/local/arkhero.x86_64   # simula o handheld
SDL_VIDEODRIVER=dummy ./build/local/arkhero.x86_64  # sem janela, so o log
```

---

## 6. Release (pacote pro console)

```bash
./tools/docker_build.sh aarch64     # DEMORADO na primeira vez
./tools/docker_build.sh x86_64      # opcional
./tools/package_port.sh
```

**A primeira execucao compila a SDL3 inteira sob emulacao.** Reserve tempo e
deixe rodando. Depois disso ela fica em `third_party/sdl3-aarch64/` e as
execucoes seguintes compilam so o jogo, em segundos.

Para forcar a recompilacao da SDL3 (troca de versao, mudanca de flags):

```bash
rm -rf third_party/sdl3-aarch64 build/sdl3-aarch64
```

Saida final:

```
dist/
├── Ark Hero.sh
├── arkhero/
│   ├── arkhero.aarch64
│   ├── arkhero.x86_64
│   ├── libs.aarch64/libSDL3.so.0
│   ├── libs.x86_64/libSDL3.so.0
│   ├── conf/
│   ├── port.json
│   └── README.md
└── arkhero.zip
```

---

## 7. Instalar no R36S

Pela rede:

```bash
scp -r dist/arkhero "dist/Ark Hero.sh" ark@<ip-do-aparelho>:/roms/ports/
```

Ou copie o cartao no PC: `dist/Ark Hero.sh` e `dist/arkhero/` vao para
`ports/` na particao de ROMS. Atualize a lista de jogos no EmulationStation
e "Ark Hero" aparece na secao Ports.

O `arkhero.zip` e o formato que o PortMaster usa para instalar -- util depois,
quando for distribuir.

---

## 8. O que ler no log.txt

Depois de rodar no aparelho, `ports/arkhero/log.txt` deve conter:

```
video driver  : kmsdrm          <- se vier "dummy" ou faltar, ha problema
render driver : opengles2       <- confirma Panfrost funcionando
janela        : 640x480 px
gamepads      : 1
  [0] id=... <nome do controle>  <- ja resolve o marco 1 do roadmap
encerrando: ~600 frames em ~10000 ms (~60.0 fps)
```

Os ~60 fps sao o numero que interessa. Se vier 30, ha algo errado no vsync ou
o KMS esta em modo de 30Hz.

---

## 9. Problemas comuns

| Sintoma | Causa | Solucao |
|---|---|---|
| `exec format error` no docker run | binfmt ARM nao registrado | `sudo pacman -S qemu-user-static-binfmt` |
| `Cannot connect to the Docker daemon` | daemon parado | `sudo systemctl start docker` |
| arquivos do build com dono root | faltou `--user` | ja esta no `docker_build.sh` |
| `version GLIBC_2.4x not found` no console | build feito fora do container | use `tools/docker_build.sh` |
| `libSDL3.so.0: cannot open` | pasta `libs.<arch>` ausente no pacote | rode `tools/package_port.sh` de novo |
| tela preta, log diz `video em uso: dummy` | SDL nao achou KMS/DRM | conferir permissao em `/dev/dri/card0` |
| `Could not create EGL window surface` no handheld | perfil de GL nao declarado | ver abaixo |
| `Couldn't find dependency package for XXXX` no configure da SDL3 | falta um pacote `-dev` na imagem | ver abaixo |

### "Could not create EGL window surface"

Aconteceu na primeira execucao no R36S e a correcao ja esta no `src/main.c`.
Fica registrado porque a armadilha vai reaparecer em qualquer codigo novo que
crie uma janela.

Em KMS/DRM a SDL cria uma EGL surface **junto com a janela**, antes de existir
qualquer renderer. Para escolher o EGLConfig ela olha o perfil de GL pedido
(`src/video/SDL_egl.c`, por volta da linha 823):

    attribs[i++] = EGL_RENDERABLE_TYPE;
    if (gl_config.profile_mask == SDL_GL_CONTEXT_PROFILE_ES) {
        ... EGL_OPENGL_ES2_BIT
    } else {
        attribs[i++] = EGL_OPENGL_BIT;   // OpenGL desktop
    }

Sem perfil definido, ela pede **OpenGL desktop**. O Mali-G31 com Panfrost so
expoe configs de OpenGL ES: nenhum config casa e a janela nao nasce.

No desktop o erro nao aparece, porque o Mesa oferece os dois perfis. E o tipo
de bug que so existe no aparelho -- mais um motivo pra testar no hardware cedo.

A correcao e declarar o perfil **antes** de criar a janela:

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

e criar a janela com `SDL_WINDOW_OPENGL`.

### Dependencia faltando no configure da SDL3

O erro tem sempre esta cara:

    CMake Error at cmake/macros.cmake:433 (message):
      Couldn't find dependency package for XTEST.  Please install the needed
      packages or configure with -DSDL_X11_XTEST=OFF

Duas saidas. A certa e **instalar o pacote**: adicione o `-dev` correspondente
no `docker/Dockerfile` e reconstrua a imagem. O nome quase sempre segue o
padrao `lib<algo>-dev` (XTEST -> `libxtst-dev`, XRANDR -> `libxrandr-dev`).

A outra e desligar o backend (`-DSDL_X11_XTEST=OFF` em `container_build.sh`).
Só faça isso se tiver certeza de que nao precisa dele -- desligar X11 inteiro,
por exemplo, deixaria a build so utilizavel em aparelhos KMS/DRM puros.

Depois de corrigir, a proxima execucao ja refaz o configure do zero (o script
apaga o cache), entao e so rodar `tools/docker_build.sh <arch>` de novo.

---

## 10. Mapa do repositorio

```
src/                     codigo do jogo
CMakeLists.txt           mesmo build nas tres rotas
docker/Dockerfile        Debian 12 = piso de glibc 2.36
tools/
  build_local.sh         build rapido no desktop
  docker_build.sh        roda no host, dispara o container
  container_build.sh     roda dentro do container: SDL3 + jogo
  package_port.sh        monta dist/
packaging/portmaster/
  Ark Hero.sh            script de launch
  arkhero/port.json      metadados do PortMaster
third_party/src/         tarballs das dependencias
prebuilt/<arch>/         artefatos compilados (gerado)
dist/                    pacote final (gerado)
```
