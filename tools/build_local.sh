#!/usr/bin/env bash
# Build rapido pro desenvolvimento no desktop, usando a SDL3 do sistema.
# Nao serve pra distribuir (linka contra a glibc do Arch), so pra iterar.
#
#   ./tools/build_local.sh          compila
#   ./tools/build_local.sh run      compila e roda
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/build/local"

cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Debug
cmake --build "$BUILD" -j "$(nproc)"

BIN="$(ls -1 "$BUILD"/arkhero.* 2>/dev/null | head -1)"
echo "==> binario: $BIN"

if [ "${1:-}" = "run" ]; then
    exec "$BIN"
fi
