# Ark Hero 0.1.0

First public build. Ark Hero is a guitar rhythm game written in C with SDL3 that plays the songs made by the Clone Hero community. It is made for Linux handhelds through PortMaster (R36S and others), and there is a Windows build too.

**No songs are included.** Bring your own Clone Hero songs: one folder per song with `notes.chart` or `notes.mid`, `song.ini` and the audio (ogg, opus, mp3 or wav).

## Downloads

| File | For | How to install |
|---|---|---|
| `arkhero.zip` | PortMaster handhelds (aarch64) | Extract into the `ports/` folder of the ROMS card, or install it with PortMaster's install-from-zip. Songs go in `ports/arkhero/songs/`. |
| `ArkHero-0.1.0-windows-x86_64.zip` | Windows 10/11, 64 bits | Extract anywhere and run `arkhero.exe`. Songs go in `songs\`, or point `conf\arkhero\pastas.txt` at your Clone Hero songs folder. |

## What's in it

- `.chart` and `.mid` charts; separate instrument tracks are mixed.
- Guitar, bass, rhythm, co-op guitar and keys, in all four difficulties.
- Sustains, HOPOs, taps, star power (with whammy), multiplier, high scores.
- Up to 4 players: built-in controls, USB controllers, USB guitars and the keyboard (which can be split between players).
- Controller mode (press the frets on time) or guitar mode (hold the frets and strum), chosen per player.
- Player accounts: each account keeps its own button mapping, keys and mode.
- The menus work with a guitar alone (strum + frets).
- Song list by folder, title or artist; timing offset; precision mode; English, Portuguese and Spanish.

## Known limits

- Tested on PC (Linux) with keyboard, controllers and a USB guitar, and on the R36S (dArkOS) with earlier builds. Other handhelds and CFWs are untested so far: reports are welcome.
- The Windows build is new; it was built and its tests run with MinGW and wine, not yet on real Windows hardware.

---

# Ark Hero 0.1.0 (português)

Primeira versão pública. Jogo de ritmo de guitarra em C com SDL3 que toca as músicas feitas pela comunidade do Clone Hero, feito para handhelds Linux pelo PortMaster (R36S e outros), com build para Windows também. **Nenhuma música acompanha o jogo.**

- `arkhero.zip`: extraia na pasta `ports/` do cartão (ou instale pelo PortMaster). Músicas em `ports/arkhero/songs/`.
- `ArkHero-0.1.0-windows-x86_64.zip`: extraia e rode o `arkhero.exe`. Músicas em `songs\`, ou indique a pasta do Clone Hero em `conf\arkhero\pastas.txt`.

Até 4 jogadores (controles, guitarras USB, teclado), modo controle ou guitarra por jogador, contas com o mapeamento de cada um, menus pela guitarra, idiomas português, inglês e espanhol.
