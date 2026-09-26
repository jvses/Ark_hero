#!/usr/bin/env bash
# Roda DENTRO do container (ou direto no console, no plano B).
# Compila a SDL3 (uma vez, fica em cache) e depois o jogo, e deixa os
# artefatos prontos em prebuilt/<arch>/.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

ARCH="$(uname -m)"
[ "$ARCH" = "arm64" ] && ARCH="aarch64"

JOBS="${JOBS:-$(nproc)}"
SDL_PREFIX="$ROOT/third_party/sdl3-$ARCH"
OUT="$ROOT/prebuilt/$ARCH"

echo "==> Ark Hero | arch=$ARCH | jobs=$JOBS"
echo "==> glibc do ambiente de build: $(ldd --version | head -1)"

# ---------------------------------------------------------------------------
# 1) SDL3 -- so compila se ainda nao existir. Esse passo e o demorado
#    (emulado em arm64 pode levar bastante); depois fica cacheado no disco.
# ---------------------------------------------------------------------------
if [ ! -f "$SDL_PREFIX/lib/libSDL3.so" ]; then
    TARBALL="$(ls -1 "$ROOT"/third_party/src/SDL3-*.tar.gz | head -1)"
    [ -f "$TARBALL" ] || { echo "ERRO: tarball da SDL3 nao encontrado em third_party/src/"; exit 1; }

    SRCDIR="$ROOT/build/sdl3-src-$ARCH"
    echo "==> extraindo $(basename "$TARBALL")"
    rm -rf "$SRCDIR"; mkdir -p "$SRCDIR"
    tar xzf "$TARBALL" -C "$SRCDIR" --strip-components=1

    # Limpa tambem o diretorio de build: um cache de CMake de um configure
    # que falhou (dependencia faltando, por exemplo) guarda variaveis que
    # confundem a proxima tentativa mesmo depois do problema resolvido.
    rm -rf "$ROOT/build/sdl3-$ARCH"

    echo "==> configurando SDL3"
    cmake -S "$SRCDIR" -B "$ROOT/build/sdl3-$ARCH" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$SDL_PREFIX" \
        -DSDL_SHARED=ON \
        -DSDL_STATIC=OFF \
        -DSDL_TESTS=OFF \
        -DSDL_EXAMPLES=OFF \
        -DSDL_TEST_LIBRARY=OFF \
        -DSDL_INSTALL_TESTS=OFF

    echo "==> compilando SDL3 (demora)"
    cmake --build "$ROOT/build/sdl3-$ARCH" -j "$JOBS"
    cmake --install "$ROOT/build/sdl3-$ARCH"
else
    echo "==> SDL3 ja compilada em $SDL_PREFIX (apague a pasta pra refazer)"
fi

# ---------------------------------------------------------------------------
# 2) O jogo
# ---------------------------------------------------------------------------
echo "==> compilando Ark Hero"
cmake -S "$ROOT" -B "$ROOT/build/$ARCH" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$SDL_PREFIX" \
    -DARKHERO_ARCH="$ARCH"
cmake --build "$ROOT/build/$ARCH" -j "$JOBS"

# ---------------------------------------------------------------------------
# 3) Coleta dos artefatos
# ---------------------------------------------------------------------------
mkdir -p "$OUT"
cp -a "$SDL_PREFIX"/lib/libSDL3.so.0* "$OUT"/
cp -a "$ROOT/build/$ARCH/arkhero.$ARCH" "$OUT"/

# strip corta simbolos de debug: menos MB no cartao, sem efeito no runtime
strip --strip-unneeded "$OUT/arkhero.$ARCH" 2>/dev/null || true
find "$OUT" -name 'libSDL3.so.0.*' -type f -exec strip --strip-unneeded {} \; 2>/dev/null || true

echo
echo "==> pronto. artefatos em prebuilt/$ARCH:"
ls -la "$OUT"
echo
echo "==> versao minima de glibc exigida pelo binario:"
objdump -T "$OUT/arkhero.$ARCH" 2>/dev/null | grep -o 'GLIBC_[0-9.]*' | sort -uV | tail -3 || true
