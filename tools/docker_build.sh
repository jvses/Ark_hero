#!/usr/bin/env bash
# Roda NO HOST (seu Arch). Constroi a imagem e dispara o build dentro dela.
#
#   ./tools/docker_build.sh aarch64   -> binario pro R36S
#   ./tools/docker_build.sh x86_64    -> binario pra handhelds x86 / Batocera PC
set -euo pipefail

ARCH="${1:-aarch64}"
case "$ARCH" in
    aarch64) PLATFORM="linux/arm64" ;;
    x86_64)  PLATFORM="linux/amd64" ;;
    *) echo "uso: $0 [aarch64|x86_64]"; exit 1 ;;
esac

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="arkhero-build:$ARCH"

echo "==> construindo imagem $IMAGE ($PLATFORM)"
docker build --platform "$PLATFORM" -t "$IMAGE" "$ROOT/docker"

echo "==> compilando dentro do container"
# --user: os arquivos gerados saem com o SEU dono, nao root
# -v    : a pasta do projeto aparece em /work dentro do container
docker run --rm -t \
    --platform "$PLATFORM" \
    --user "$(id -u):$(id -g)" \
    -e HOME=/tmp \
    -e JOBS="$(nproc)" \
    -v "$ROOT:/work" \
    -w /work \
    "$IMAGE" tools/container_build.sh
