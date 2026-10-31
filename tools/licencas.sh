#!/usr/bin/env bash
# Copia as licencas de tudo que vai nos pacotes para uma pasta so, plana, com
# um LICENSE.<componente>.txt por componente (o formato que o PortMaster pede).
# As da Xiph (BSD) EXIGEM o aviso de copyright junto do binario.
#
#   tools/licencas.sh <pasta de destino>
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LIC="$1"
mkdir -p "$LIC"

# a do proprio jogo (o LICENSE da raiz do repositorio)
if [ -f "$ROOT/LICENSE" ]; then
    cp "$ROOT/LICENSE" "$LIC/LICENSE.arkhero.txt"
else
    echo "AVISO: sem LICENSE na raiz do repositorio: o pacote vai sem a licenca do jogo"
fi

for par in "SDL3:LICENSE.txt" "libogg:COPYING" "libvorbis:COPYING" "opus:COPYING" "opusfile:COPYING"; do
    nome="${par%%:*}"; arquivo="${par#*:}"
    tarball="$(ls -1 "$ROOT"/third_party/src/"$nome"-*.tar.gz 2>/dev/null | head -1)"
    [ -f "$tarball" ] || { echo "ERRO: $nome-*.tar.gz nao encontrado (licenca)"; exit 1; }
    pasta="$(basename "$tarball" .tar.gz)"
    tar xzf "$tarball" -O "$pasta/$arquivo" > "$LIC/LICENSE.$nome.txt"
done
cp "$ROOT/third_party/dr_libs/README.md" "$LIC/LICENSE.dr_mp3.txt"
sed -n '/^This software is available as a choice/,$p' "$ROOT/third_party/dr_libs/dr_mp3.h" >> "$LIC/LICENSE.dr_mp3.txt"
cp "$ROOT/third_party/stb/README.md" "$LIC/LICENSE.stb_truetype.txt"
sed -n '/^This software is available under 2 licenses/,$p' "$ROOT/third_party/stb/stb_truetype.h" >> "$LIC/LICENSE.stb_truetype.txt"
cp "$ROOT/assets/fonts/FONTE.txt" "$LIC/LICENSE.terminal16unicode.txt"
echo "++ licencas: $(ls "$LIC" | wc -l) arquivos"
