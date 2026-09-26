#!/bin/bash
# Ark Hero -- script de launch (padrao PortMaster)
#
# Regra do ambiente: este script so pode ESCREVER dentro de $GAMEDIR.
# Leitura fora dele e permitida (importante para a biblioteca de musicas).

XDG_DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}

# Onde mora o PortMaster varia por firmware
if [ -d "/opt/system/Tools/PortMaster/" ]; then
  controlfolder="/opt/system/Tools/PortMaster"
elif [ -d "/opt/tools/PortMaster/" ]; then
  controlfolder="/opt/tools/PortMaster"
elif [ -d "$XDG_DATA_HOME/PortMaster/" ]; then
  controlfolder="$XDG_DATA_HOME/PortMaster"
else
  controlfolder="/roms/ports/PortMaster"
fi

# control.txt define $directory, $ESUDO, $DEVICE_ARCH, $CFW_NAME,
# pm_message, pm_platform_helper, pm_finish, get_controls...
source $controlfolder/control.txt
[ -f "${controlfolder}/mod_${CFW_NAME}.txt" ] && source "${controlfolder}/mod_${CFW_NAME}.txt"

get_controls

GAMEDIR=/$directory/ports/arkhero
CONFDIR="$GAMEDIR/conf"

mkdir -p "$CONFDIR"
cd "$GAMEDIR"

# Tudo daqui pra frente vai pro log.txt, que e a unica janela de depuracao
# que voce tem quando o jogo roda na mao de outra pessoa.
> "$GAMEDIR/log.txt" && exec > >(tee "$GAMEDIR/log.txt") 2>&1

# Qualquer coisa que a SDL ou o jogo queira gravar cai em conf/
export XDG_DATA_HOME="$CONFDIR"
export XDG_CONFIG_HOME="$CONFDIR"
export XDG_CACHE_HOME="$CONFDIR"

export LD_LIBRARY_PATH="$GAMEDIR/libs.${DEVICE_ARCH}:$LD_LIBRARY_PATH"
export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"
export ARKHERO_FULLSCREEN=1

BINARY="$GAMEDIR/arkhero.${DEVICE_ARCH}"

# --- diagnostico: enquanto o port for experimental, isso vale ouro ---
echo "--- ambiente ---"
echo "CFW        : ${CFW_NAME} ${CFW_VERSION}"
echo "device     : ${DEVICE_NAME} (${DEVICE_ARCH})"
echo "glibc CFW  : ${CFW_GLIBC}"
ldd --version 2>/dev/null | head -1
echo "binario    : $BINARY"
echo "----------------"

if [ ! -f "$BINARY" ]; then
  pm_message "Erro: nao ha binario para ${DEVICE_ARCH} neste pacote."
  sleep 5
  exit 1
fi

$ESUDO chmod +x "$BINARY"

# Sem gptokeyb: o jogo le o gamepad nativamente pela SDL.
# Se um dia precisar de mapeamento pra teclado, e aqui que entra.
pm_platform_helper "$BINARY"

"$BINARY"

pm_finish
