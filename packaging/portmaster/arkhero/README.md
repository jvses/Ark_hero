# Ark Hero

Jogo de ritmo de guitarra em C + SDL3, feito para handhelds ARM.

**Estado atual: teste de portabilidade.** Esta build nao e o jogo -- ela desenha
uma bola quicando por 10 segundos e encerra. Serve para validar toolchain,
empacotamento e execucao no aparelho.

## Conteudo da pasta

    arkhero.aarch64      binario ARM 64 bits
    arkhero.x86_64       binario x86 64 bits
    libs.aarch64/        libSDL3 para ARM
    libs.x86_64/         libSDL3 para x86
    conf/                unica pasta gravavel (config, cache, scores)
    log.txt              gerado a cada execucao

## Requisitos

glibc 2.36 ou mais novo. Testado em dArkOS (glibc 2.41).

## Licenca

O codigo do jogo e do autor. A SDL3 e distribuida sob a licenca zlib.
Nenhum asset de Guitar Hero ou Clone Hero acompanha este pacote.
