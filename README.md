# Ark Hero

Jogo de ritmo de guitarra no estilo Guitar Hero / Clone Hero, escrito em C11
com SDL3, feito para rodar nativamente em handhelds Linux (alvo principal: R36S
com dArkOS) e ser distribuído no formato do PortMaster. A ideia é ler as
músicas (charts) que a comunidade do Clone Hero já produziu. A integração com
Lua está planejada.

**Estado:** em desenvolvimento inicial. Hoje o jogo abre a janela (KMS/DRM +
OpenGL ES 2), lê o gamepad e roda a ~60 FPS no R36S.

## Compilando

Para desenvolver no PC (precisa da SDL3 instalada, ou de
`-DCMAKE_PREFIX_PATH=<pasta da SDL3>`):

```bash
./tools/build_local.sh run
```

Para o console (Docker + qemu-user-static para emular arm64; o container
Debian 12 compila a SDL3 a partir de `third_party/src/` e depois o jogo):

```bash
./tools/docker_build.sh aarch64
./tools/package_port.sh          # monta dist/ e dist/arkhero.zip
```

Para instalar, copie `dist/Ark Hero.sh` e `dist/arkhero/` para a pasta `ports/`
da partição de ROMS.

## Estrutura

```
src/platform/   janela, renderer e input (SDL3)
src/game/       lógica do jogo (sem SDL)
src/draw/       desenho
tools/          scripts de build e empacotamento
docker/         ambiente de build (Debian 12, glibc 2.36)
packaging/      script de launch e metadados do PortMaster
third_party/    código-fonte das dependências (SDL3, Lua)
```

## Sobre o uso de IA

Este é um projeto de aprendizado de C, SDL3 e Lua. O código do jogo é escrito
por mim; uso IA como orientadora, para tirar dúvidas, discutir abordagens e
entender melhor a biblioteca e o código.
