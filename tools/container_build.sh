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
# 1b) Audio: libogg, libvorbis, opus e opusfile (Xiph), ESTATICAS.
#     Ao contrario da SDL, elas nao carregam nada do sistema por dlopen: podem
#     ir para dentro do binario, e o pacote continua sem .so novo.
#     Ficam em third_party/xiph-<arch>/ (apague a pasta para refazer).
# ---------------------------------------------------------------------------
XIPH_PREFIX="$ROOT/third_party/xiph-$ARCH"
if [ ! -f "$XIPH_PREFIX/lib/libopusfile.a" ]; then
    rm -rf "$XIPH_PREFIX" "$ROOT/build/xiph-$ARCH"
    mkdir -p "$ROOT/build/xiph-$ARCH"

    # extrai third_party/src/<nome>-*.tar.gz e imprime a pasta
    extrai() {
        local tarball
        tarball="$(ls -1 "$ROOT"/third_party/src/"$1"-*.tar.gz | head -1)"
        [ -f "$tarball" ] || { echo "ERRO: $1-*.tar.gz nao encontrado em third_party/src/" >&2; exit 1; }
        local dir="$ROOT/build/xiph-$ARCH/$1"
        mkdir -p "$dir"
        tar xzf "$tarball" -C "$dir" --strip-components=1
        echo "$dir"
    }

    # biblioteca com CMake: estatica, com -fPIC (o jogo e um executavel PIE)
    cmake_lib() {
        local nome="$1"; shift
        local src; src="$(extrai "$nome")"
        echo "==> compilando $nome"
        cmake -S "$src" -B "$src/build" -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX="$XIPH_PREFIX" \
            -DCMAKE_INSTALL_LIBDIR=lib \
            -DCMAKE_PREFIX_PATH="$XIPH_PREFIX" \
            -DBUILD_SHARED_LIBS=OFF \
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
            -DBUILD_TESTING=OFF \
            "$@"
        cmake --build "$src/build" -j "$JOBS"
        cmake --install "$src/build"
    }

    cmake_lib libogg    -DINSTALL_DOCS=OFF
    cmake_lib libvorbis
    cmake_lib opus      -DOPUS_BUILD_PROGRAMS=OFF -DOPUS_BUILD_TESTING=OFF

    # opusfile 0.12 nao tem CMake; o configure ja vem pronto no tarball
    SRC_OPUSFILE="$(extrai opusfile)"
    echo "==> compilando opusfile"
    ( cd "$SRC_OPUSFILE" && \
      PKG_CONFIG_PATH="$XIPH_PREFIX/lib/pkgconfig" ./configure --prefix="$XIPH_PREFIX" \
          --enable-static --disable-shared --with-pic \
          --disable-http --disable-examples --disable-doc && \
      make -j "$JOBS" && make install )
else
    echo "==> audio (Xiph) ja compilado em $XIPH_PREFIX (apague a pasta pra refazer)"
fi

# ---------------------------------------------------------------------------
# 2) O jogo
# ---------------------------------------------------------------------------
echo "==> compilando Ark Hero"
cmake -S "$ROOT" -B "$ROOT/build/$ARCH" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$SDL_PREFIX;$XIPH_PREFIX" \
    -DARKHERO_AUDIO_STATIC=ON \
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
