# Como traduzir o Ark Hero

Cada idioma é um arquivo de texto nesta pasta: `en.txt` (inglês), `es.txt`
(espanhol)... O nome do arquivo é o código do idioma; ele aparece em
Configurações > Idioma. O português não tem arquivo: são os textos do
próprio jogo.

## O formato

Uma linha por texto: o original em português, ` = ` (com os espaços), e a
tradução.

```
idioma = Français
Jogar = Jouer
%d música(s) = %d chanson(s)
```

- A linha `idioma = ...` é o nome que aparece no menu.
- Linhas começando com `#` são comentários.
- O lado esquerdo tem de ser **igual** ao texto do jogo, com os acentos e
  a pontuação (copie dos arquivos que já existem). O que faltar aparece em
  português.
- `%d` (um número), `%s` (um texto), `%ld`, `%.0f`... têm de continuar na
  tradução, **na mesma ordem**. Se não estiverem, o jogo ignora a linha (e
  avisa no log.txt). `%%` é o próprio sinal de porcentagem.
- Escreva com os acentos de verdade (o arquivo é UTF-8): a fonte do jogo tem
  o latim com acentos (á, ç, ñ, ü, ¿, ¡...), boa parte do grego e do
  cirílico, e as setas ← ↑ → ↓. O que ela não tiver aparece como `?`.
- A tela é pequena (640x480) e cada letra ocupa 12 pixels: as dicas do
  rodapé cabem em uns 52 caracteres, os cartões dos jogadores em uns 23. Uma
  tradução bem mais comprida que o original é cortada com "..".

## Começar um idioma novo

No PC, na pasta do projeto:

```
python3 tools/textos.py novo fr "Français"
```

Isso cria `assets/lang/fr.txt` com todos os textos do jogo, cada um
"traduzido" para ele mesmo: troque o lado direito. Sem o Python, copie o
`en.txt` com o nome novo e troque o lado direito dele.

## Conferir

```
python3 tools/textos.py
```

Mostra, para cada idioma, o que falta traduzir, o que sobrou (textos que o
jogo não usa mais) e as linhas com os `%d/%s` diferentes.

No console, basta copiar o arquivo para `ports/arkhero/assets/lang/` e
escolher o idioma nas Configurações.
