#!/usr/bin/env bash
# Monta o pacote do PortMaster em dist/ a partir do que existe em prebuilt/.
# Nao compila nada: rode os builds antes.
#
#   ./tools/docker_build.sh aarch64
#   ./tools/docker_build.sh x86_64      (opcional)
#   ./tools/package_port.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST="$ROOT/dist"
PORT="$DIST/arkhero"

rm -rf "$DIST"
mkdir -p "$PORT/conf"

# 1) arquivos estaticos do port
cp "$ROOT/packaging/portmaster/Ark Hero.sh" "$DIST/"
cp -r "$ROOT/packaging/portmaster/arkhero/." "$PORT/"
chmod +x "$DIST/Ark Hero.sh"

# 2) binarios + libs, por arquitetura
FOUND=0
for ARCH in aarch64 x86_64; do
    SRC="$ROOT/prebuilt/$ARCH"
    [ -f "$SRC/arkhero.$ARCH" ] || { echo "-- pulando $ARCH (sem build)"; continue; }

    cp -a "$SRC/arkhero.$ARCH" "$PORT/"
    mkdir -p "$PORT/libs.$ARCH"

    # cp -L resolve o symlink: o resultado e UM arquivo regular chamado
    # libSDL3.so.0, que e exatamente o SONAME que o binario procura.
    #
    # Nada de symlink aqui, por dois motivos:
    #   - a particao de ROMS costuma ser FAT32/exFAT, que nao tem symlink;
    #   - o zip desreferencia symlinks por padrao, o que duplicaria a lib.
    cp -L "$SRC/libSDL3.so.0" "$PORT/libs.$ARCH/libSDL3.so.0"

    chmod +x "$PORT/arkhero.$ARCH" "$PORT/libs.$ARCH/libSDL3.so.0"
    echo "++ incluido $ARCH"
    FOUND=$((FOUND + 1))
done

[ "$FOUND" -gt 0 ] || { echo "ERRO: nenhum binario em prebuilt/. Rode tools/docker_build.sh antes."; exit 1; }

# 3) zip com a mesma estrutura que o PortMaster espera:
#    a raiz do zip contem "Ark Hero.sh" e a pasta "arkhero/"
( cd "$DIST" && zip -r -q arkhero.zip "Ark Hero.sh" arkhero )

echo
echo "==> pacote pronto:"
find "$DIST" -mindepth 1 -not -name arkhero.zip | sed "s|$ROOT/||" | sort
echo
echo "==> zip: dist/arkhero.zip  ($(du -h "$DIST/arkhero.zip" | cut -f1))"
echo
echo "Para instalar no console, copie para a particao de ROMS:"
echo "  dist/Ark Hero.sh   ->  /roms/ports/"
echo "  dist/arkhero/      ->  /roms/ports/arkhero/"
