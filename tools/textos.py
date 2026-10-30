#!/usr/bin/env python3
"""Os textos traduzíveis do Ark Hero (ver src/game/texto.h).

    tools/textos.py                  confere cada assets/lang/*.txt: o que falta
                                     traduzir e o que sobrou (texto que o jogo
                                     não usa mais)
    tools/textos.py novo fr Francais cria assets/lang/fr.txt com todos os textos,
                                     para traduzir o lado direito do " = "

Os textos são achados no código: tudo o que estiver dentro de T("...") ou
N_("...") nos arquivos de src/.
"""
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PASTA_LANG = os.path.join(RAIZ, "assets", "lang")


def sem_comentarios(c):
    """tira /* */ e // do código, sem mexer no que está dentro de strings"""
    saida, i, n = [], 0, len(c)
    while i < n:
        if c[i] == '"':
            j = i + 1
            while j < n and c[j] != '"':
                j += 2 if c[j] == "\\" else 1
            saida.append(c[i:j + 1])
            i = j + 1
        elif c.startswith("/*", i):
            i = c.find("*/", i + 2)
            i = n if i < 0 else i + 2
        elif c.startswith("//", i):
            i = c.find("\n", i)
            i = n if i < 0 else i
        else:
            saida.append(c[i])
            i += 1
    return "".join(saida)


def desfaz_escapes(s):
    """os escapes de C de um literal (\\", \\\\, \\n); o resto, UTF-8, fica como está"""
    return re.sub(r'\\(.)', lambda m: {"n": "\n", "t": "\t"}.get(m.group(1), m.group(1)), s)


def textos_do_codigo():
    """{texto: [arquivo:linha, ...]} de todos os T(...) e N_(...)"""
    achados = {}
    for pasta, _, arquivos in os.walk(os.path.join(RAIZ, "src")):
        for nome in sorted(arquivos):
            if not nome.endswith((".c", ".h")):
                continue
            caminho = os.path.join(pasta, nome)
            codigo = sem_comentarios(open(caminho, encoding="utf-8").read())
            for m in re.finditer(r"(?<![A-Za-z0-9_])(T|N_)\(", codigo):
                if nome == "texto.h" and m.group(1) == "N_":
                    continue   # a própria definição da macro
                # os literais dentro dos parênteses da chamada (pega os dois lados de um "?:")
                i, prof = m.end(), 1
                while i < len(codigo) and prof:
                    ch = codigo[i]
                    if ch == '"':
                        j = i + 1
                        while codigo[j] != '"':
                            j += 2 if codigo[j] == "\\" else 1
                        texto = desfaz_escapes(codigo[i + 1:j])
                        linha = codigo.count("\n", 0, i) + 1
                        achados.setdefault(texto, []).append(f"{os.path.relpath(caminho, RAIZ)}:{linha}")
                        i = j + 1
                        continue
                    prof += ch == "("
                    prof -= ch == ")"
                    i += 1
    return achados


def le_idioma(caminho):
    """(nome do idioma, {texto: tradução}) de um arquivo de idioma"""
    nome, pares = "", {}
    for linha in open(caminho, encoding="utf-8"):
        linha = linha.strip()
        if not linha or linha.startswith("#") or " = " not in linha:
            continue
        chave, valor = (x.strip() for x in linha.split(" = ", 1))
        if chave == "idioma":
            nome = valor
        else:
            pares[chave] = valor
    return nome, pares


def formatos(s):
    """os %d/%s de um texto, como o jogo compara (src/game/texto.c)"""
    return re.findall(r"%[-+ #0-9.]*([*hlLzjt]*[a-zA-Z])", s.replace("%%", ""))


def confere():
    textos = textos_do_codigo()
    print(f"{len(textos)} texto(s) traduzível(is) no código")
    idiomas = sorted(f for f in os.listdir(PASTA_LANG) if f.endswith(".txt")) if os.path.isdir(PASTA_LANG) else []
    if not idiomas:
        print(f"nenhum idioma em {PASTA_LANG}")
    problemas = 0
    for arq in idiomas:
        nome, pares = le_idioma(os.path.join(PASTA_LANG, arq))
        faltam = [t for t in textos if t not in pares]
        sobram = [t for t in pares if t not in textos]
        errados = [t for t in pares if t in textos and formatos(t) != formatos(pares[t])]
        print(f"\n{arq} ({nome or 'sem a linha idioma = ...'}): {len(pares)} tradução(ões)")
        for t in faltam:
            print(f"  falta : {t}    ({textos[t][0]})")
        for t in sobram:
            print(f"  sobra : {t}")
        for t in errados:
            print(f"  %d/%s diferentes (o jogo ignora): {t} = {pares[t]}")
        problemas += len(faltam) + len(errados)
    return 1 if problemas else 0


def novo(codigo, nome):
    caminho = os.path.join(PASTA_LANG, f"{codigo}.txt")
    if os.path.exists(caminho):
        sys.exit(f"{caminho} já existe")
    textos = textos_do_codigo()
    with open(caminho, "w", encoding="utf-8") as f:
        f.write(f"# Ark Hero - {nome}\n")
        f.write("# Traduza o lado direito de cada \" = \". Os %d, %s... têm de continuar,\n")
        f.write("# na mesma ordem. Ver COMO_TRADUZIR.md.\n\n")
        f.write(f"idioma = {nome}\n\n")
        for t in sorted(textos, key=str.lower):
            f.write(f"{t} = {t}\n")
    print(f"criado {caminho} com {len(textos)} texto(s)")


if __name__ == "__main__":
    if len(sys.argv) >= 2 and sys.argv[1] == "novo":
        if len(sys.argv) != 4:
            sys.exit("uso: tools/textos.py novo <codigo> <Nome do idioma>")
        novo(sys.argv[2], sys.argv[3])
    else:
        sys.exit(confere())
