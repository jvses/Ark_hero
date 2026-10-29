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

# ---------------------------------------------------------------------------
# Audio: Opus (opusfile) e Ogg Vorbis (vorbisfile), achados pelo pkg-config.
#   PC:        as do sistema (pacman -S opusfile libvorbis), dinamicas.
#   container: as de third_party/xiph-<arch>/, estaticas (ARKHERO_AUDIO_STATIC),
#              compiladas pelo tools/container_build.sh. Vao para dentro do
#              binario: o pacote nao ganha .so novo.
# ---------------------------------------------------------------------------
option(ARKHERO_AUDIO_STATIC "linkar opus/vorbis estaticos (build do console)" OFF)
find_package(PkgConfig REQUIRED)
pkg_check_modules(AUDIO REQUIRED IMPORTED_TARGET opusfile vorbisfile)

# o clangd do editor le as flags daqui (ver .clangd)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# game/ nao usa a SDL: entra no jogo e tambem no teste, que compila sem ela
set(ARKHERO_GAME_SRC
    src/game/acao.c
    src/game/biblioteca.c
    src/game/chart.c
    src/game/chartfile.c
    src/game/game.c
    src/game/judge.c
    src/game/menu.c
    src/game/midifile.c
    src/game/opcoes.c
    src/game/score.c
    src/game/songini.c
)

add_executable(arkhero
    src/main.c
    src/platform/video.c
    src/platform/input.c
    src/platform/audio.c
    src/platform/carregador.c
    src/platform/musica.c
    src/draw/draw.c
    src/draw/formas.c
    src/draw/pista.c
    ${ARKHERO_GAME_SRC}
)

target_include_directories(arkhero PRIVATE src)
# dr_mp3.h (um arquivo so; a implementacao entra no carregador.c). SYSTEM: os
# avisos de dentro dele nao sao do jogo
target_include_directories(arkhero SYSTEM PRIVATE third_party/dr_libs)

target_link_libraries(arkhero PRIVATE SDL3::SDL3)
if(ARKHERO_AUDIO_STATIC)
    # --static: o pkg-config lista tambem as dependencias (ogg, opus, libm)
    # SYSTEM: avisos dentro dos headers de terceiros (vorbisfile.h) nao sao nossos
    target_include_directories(arkhero SYSTEM PRIVATE ${AUDIO_STATIC_INCLUDE_DIRS})
    target_link_libraries(arkhero PRIVATE ${AUDIO_STATIC_LDFLAGS})
else()
    target_link_libraries(arkhero PRIVATE PkgConfig::AUDIO)
endif()

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

# ---------------------------------------------------------------------------
# Dados do jogo (imagens, depois scripts Lua). O jogo le tudo a partir de
# SDL_GetBasePath(), a pasta do executavel. No console o pacote tem a pasta
# assets/ copiada de verdade (tools/package_port.sh). Aqui no build, um link
# simbolico para a pasta do repositorio: editou um arquivo, o jogo ja enxerga,
# sem recompilar e sem copiar nada.
# ---------------------------------------------------------------------------
add_custom_command(TARGET arkhero POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -f $<TARGET_FILE_DIR:arkhero>/assets
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:arkhero>/assets
)

# ---------------------------------------------------------------------------
# Testes da logica do jogo, sem janela e SEM a SDL (ARQUITETURA 6.9).
# O alvo nao linka a SDL: se alguem usar uma funcao da SDL em game/, ele para
# de compilar. E a prova de que a regra esta sendo cumprida.
# Rodar: ctest --test-dir build/local   (ou ./build/local/test_game)
# ---------------------------------------------------------------------------
enable_testing()
add_executable(test_game tests/test_game.c ${ARKHERO_GAME_SRC})
target_include_directories(test_game PRIVATE src)
target_link_libraries(test_game PRIVATE m)   # fabs nos testes
target_compile_options(test_game PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wshadow>
)
add_test(NAME test_game COMMAND test_game)
