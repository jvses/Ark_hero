#!/usr/bin/env bash
# Monta o pacote do PortMaster em dist/ a partir do que existe em prebuilt/.
# Nao compila nada: rode os builds antes.
#
#   ./tools/docker_build.sh aarch64
#   ./tools/docker_build.sh x86_64      (opcional)
#   ./tools/package_port.sh
#
# Saidas:
#   dist/arkhero.zip            o zip do port, igual ao que o PortMaster monta
#                               (instala pelo PortMaster ou extraindo em ports/)
#   dist/Ark Hero.sh, dist/arkhero/   o mesmo conteudo, extraido
#   dist/portmaster-pr/arkhero/       a pasta para o pull request no
#                               PortMaster-New (ports/arkhero/)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST="$ROOT/dist"
PM="$ROOT/packaging/portmaster"
PR="$DIST/portmaster-pr/arkhero"
JOGO="$PR/arkhero"

rm -rf "$DIST/portmaster-pr" "$DIST/arkhero" "$DIST/Ark Hero.sh" "$DIST/arkhero.zip"
mkdir -p "$JOGO"

# 1) a pasta do pull request: metadados do PortMaster + a pasta do jogo
cp "$PM/Ark Hero.sh" "$PM/port.json" "$PM/README.md" "$PM/gameinfo.xml" "$PM/screenshot.png" "$PR/"

# dados do jogo. Copia de verdade, sem symlink: a particao de ROMS e
# FAT32/exFAT. As musicas de teste nunca vao: tem direitos autorais.
mkdir -p "$JOGO/assets"
cp -rL "$ROOT/assets/fonts" "$ROOT/assets/images" "$ROOT/assets/lang" "$JOGO/assets/"
"$ROOT/tools/licencas.sh" "$JOGO/licenses"

mkdir -p "$JOGO/songs"
cp "$ROOT/packaging/PUT_YOUR_SONGS_HERE.txt" "$JOGO/songs/"

# 2) binarios + libs, por arquitetura
FOUND=0
for ARCH in aarch64 x86_64; do
    SRC="$ROOT/prebuilt/$ARCH"
    [ -f "$SRC/arkhero.$ARCH" ] || { echo "-- pulando $ARCH (sem build)"; continue; }

    cp -a "$SRC/arkhero.$ARCH" "$JOGO/"
    mkdir -p "$JOGO/libs.$ARCH"
    # cp -L resolve o symlink: UM arquivo regular chamado libSDL3.so.0, o
    # SONAME que o binario procura (FAT32 nao tem symlink, e o zip duplicaria)
    cp -L "$SRC/libSDL3.so.0" "$JOGO/libs.$ARCH/libSDL3.so.0"
    chmod +x "$JOGO/arkhero.$ARCH"
    echo "++ incluido $ARCH"
    FOUND=$((FOUND + 1))
done
[ "$FOUND" -gt 0 ] || { echo "ERRO: nenhum binario em prebuilt/. Rode tools/docker_build.sh antes."; exit 1; }

# 3) o zip, como o tools/build_release.py do PortMaster-New monta: o script
#    na raiz; dentro de arkhero/, os metadados (o README vira arkhero.md)
cp "$PR/Ark Hero.sh" "$DIST/"
cp -a "$JOGO" "$DIST/arkhero"
cp "$PR/port.json" "$PR/gameinfo.xml" "$PR/screenshot.png" "$DIST/arkhero/"
cp "$PR/README.md" "$DIST/arkhero/arkhero.md"
( cd "$DIST" && zip -r -q arkhero.zip "Ark Hero.sh" arkhero )

echo
echo "==> zip: dist/arkhero.zip  ($(du -h "$DIST/arkhero.zip" | cut -f1))"
echo "==> pasta do pull request: dist/portmaster-pr/arkhero/"
echo
echo "Para instalar no console, copie para a particao de ROMS:"
echo "  dist/Ark Hero.sh   ->  /roms/ports/"
echo "  dist/arkhero/      ->  /roms/ports/arkhero/"
