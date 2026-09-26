# Ambiente de build do Ark Hero.
#
# POR QUE Debian 12 (bookworm) E NAO O MAIS NOVO:
#   bookworm traz glibc 2.36. Um binario linkado contra glibc 2.36 roda em
#   qualquer sistema com glibc >= 2.36, porque a glibc e compativel para
#   frente (versionamento de simbolos). O contrario NAO vale: binario
#   compilado no Arch (glibc 2.42) nao roda em lugar nenhum alem do Arch.
#
#   Piso de 2.36 cobre:
#     dArkOS  -> 2.41  (confirmado no seu aparelho, CFW_GLIBC=241)
#     EmuELEC -> 2.36  (exatamente no limite)
#     ROCKNIX / Batocera / muOS -> mais novos
#   Fica de fora o ArkOS antigo, que o proprio PortMaster esta abandonando.
#
#   Se um dia precisar de um piso mais baixo, e so trocar esta linha por
#   debian:bullseye (2.31). Nada mais no projeto muda.
FROM debian:bookworm

ARG DEBIAN_FRONTEND=noninteractive

# Headers de desenvolvimento das dependencias da SDL3.
# Detalhe importante: a SDL carrega EGL, GLESv2, ALSA, udev, X11 e Wayland
# por dlopen() em tempo de execucao, nao no link. Instalar tudo isso aqui
# so habilita os backends -- a lib final nao fica amarrada a versao nenhuma
# desses pacotes, e vai usar o que existir no sistema do jogador.
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
        pkg-config \
        file \
        ca-certificates \
        libdrm-dev \
        libgbm-dev \
        libegl1-mesa-dev \
        libgles2-mesa-dev \
        libudev-dev \
        libdbus-1-dev \
        libasound2-dev \
        libpulse-dev \
        libpipewire-0.3-dev \
        libx11-dev \
        libxext-dev \
        libxrandr-dev \
        libxcursor-dev \
        libxi-dev \
        libxfixes-dev \
        libxss-dev \
        libxtst-dev \
        libxkbcommon-dev \
        libwayland-dev \
        wayland-protocols \
        libdecor-0-dev \
        libibus-1.0-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /work
