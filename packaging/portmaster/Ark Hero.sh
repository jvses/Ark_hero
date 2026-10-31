#!/bin/bash

XDG_DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}

if [ -d "/opt/system/Tools/PortMaster/" ]; then
  controlfolder="/opt/system/Tools/PortMaster"
elif [ -d "/opt/tools/PortMaster/" ]; then
  controlfolder="/opt/tools/PortMaster"
elif [ -d "$XDG_DATA_HOME/PortMaster/" ]; then
  controlfolder="$XDG_DATA_HOME/PortMaster"
else
  controlfolder="/roms/ports/PortMaster"
fi

source $controlfolder/control.txt
[ -f "${controlfolder}/mod_${CFW_NAME}.txt" ] && source "${controlfolder}/mod_${CFW_NAME}.txt"
get_controls

GAMEDIR=/$directory/ports/arkhero
CONFDIR="$GAMEDIR/conf"
BINARY=arkhero.${DEVICE_ARCH}

mkdir -p "$CONFDIR"
cd $GAMEDIR

> "$GAMEDIR/log.txt" && exec > >(tee "$GAMEDIR/log.txt") 2>&1

# options, accounts, records and controls go to conf/arkhero/
export XDG_DATA_HOME="$CONFDIR"
export LD_LIBRARY_PATH="$GAMEDIR/libs.${DEVICE_ARCH}:$LD_LIBRARY_PATH"
export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"
# extra mappings for USB controllers and guitars
[ -f "$controlfolder/gamecontrollerdb.txt" ] && export SDL_GAMECONTROLLERCONFIG_FILE="$controlfolder/gamecontrollerdb.txt"
export ARKHERO_FULLSCREEN=1

$ESUDO chmod +x "$GAMEDIR/$BINARY"

# no gptokeyb: the game reads the controllers itself and quits on Select+Start
pm_platform_helper "$GAMEDIR/$BINARY"

./$BINARY

pm_finish
