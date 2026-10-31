# Compilação cruzada para Windows 64 bits com o MinGW-w64 (no Debian:
# gcc-mingw-w64-x86-64). Usado pelo tools/container_build.sh windows.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)

# programas (pkg-config, ninja) são do Linux; bibliotecas e headers, do
# MinGW ou das pastas em CMAKE_PREFIX_PATH (third_party/*-windows)
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

# os testes rodam pelo wine (ctest); no Debian ele fica em /usr/lib/wine
find_program(WINE NAMES wine wine64 PATHS /usr/lib/wine)
if(WINE)
    set(CMAKE_CROSSCOMPILING_EMULATOR ${WINE})
endif()
