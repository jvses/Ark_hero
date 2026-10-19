# Ark Hero -- build do jogo.
#
# Este arquivo e o MESMO nos tres cenarios:
#   1. build nativo no desktop (SDL3 do sistema)
#   2. build dentro do container Debian 12 (SDL3 compilada em third_party/sdl3-<arch>)
#   3. build direto no console, se algum dia for necessario (plano B)
#
# A unica coisa que muda entre eles e o CMAKE_PREFIX_PATH.

cmake_minimum_required(VERSION 3.16)
project(ArkHero VERSION 0.0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ---------------------------------------------------------------------------
# Nome curto da arquitetura. O PortMaster usa exatamente esses nomes em
# DEVICE_ARCH ("aarch64" / "x86_64"), e o binario e a pasta de libs seguem eles.
# ---------------------------------------------------------------------------
if(NOT DEFINED ARKHERO_ARCH)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
        set(ARKHERO_ARCH "aarch64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
        set(ARKHERO_ARCH "x86_64")
    else()
        set(ARKHERO_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
    endif()
endif()
message(STATUS "Ark Hero: arquitetura alvo = ${ARKHERO_ARCH}")

find_package(SDL3 CONFIG REQUIRED)

add_executable(arkhero
    src/main.c
    src/platform/video.c
    src/platform/input.c
    src/platform/input.c
    src/draw/desenhos.c
    src/game/game_state.c
    
)

target_include_directories(arkhero PRIVATE src)

target_link_libraries(arkhero PRIVATE SDL3::SDL3)

target_compile_options(arkhero PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wshadow>
)

set_target_properties(arkhero PROPERTIES
    OUTPUT_NAME "arkhero.${ARKHERO_ARCH}"
    # $ORIGIN = pasta do proprio binario. Com isso o executavel acha a
    # libSDL3 empacotada em libs.<arch>/ mesmo se LD_LIBRARY_PATH nao estiver
    # setado -- o script do PortMaster seta de qualquer forma, mas assim o
    # pacote tambem funciona rodando na mao, o que ajuda a depurar.
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "$ORIGIN/libs.${ARKHERO_ARCH}"
)
