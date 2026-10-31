#!/usr/bin/env bash
# Monta o pacote do Windows em dist/ a partir de prebuilt/windows/.
# Nao compila nada: rode o build antes.
#
#   ./tools/docker_build.sh windows
#   ./tools/package_windows.sh
#
# Saidas: dist/windows/ArkHero/ e dist/ArkHero-<versao>-windows-x86_64.zip
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/prebuilt/windows"
VERSAO="$(sed -n 's/^project(ArkHero VERSION \([0-9.]*\).*/\1/p' "$ROOT/CMakeLists.txt")"
DIST="$ROOT/dist"
PASTA="$DIST/windows/ArkHero"
ZIP="ArkHero-$VERSAO-windows-x86_64.zip"

[ -f "$SRC/arkhero.exe" ] || { echo "ERRO: sem prebuilt/windows/arkhero.exe. Rode tools/docker_build.sh windows antes."; exit 1; }

rm -rf "$DIST/windows" "$DIST/$ZIP"
mkdir -p "$PASTA/assets" "$PASTA/songs"

cp "$SRC/arkhero.exe" "$SRC/SDL3.dll" "$PASTA/"
cp -rL "$ROOT/assets/fonts" "$ROOT/assets/images" "$ROOT/assets/lang" "$PASTA/assets/"
cp "$ROOT/packaging/PUT_YOUR_SONGS_HERE.txt" "$PASTA/songs/"
cp "$ROOT/packaging/windows/README.txt" "$PASTA/"
"$ROOT/tools/licencas.sh" "$PASTA/licenses"

# o zip com a pasta ArkHero/ dentro: extraiu, tem uma pasta so
( cd "$DIST/windows" && zip -r -q "$DIST/$ZIP" ArkHero )

echo
echo "==> zip: dist/$ZIP  ($(du -h "$DIST/$ZIP" | cut -f1))"
