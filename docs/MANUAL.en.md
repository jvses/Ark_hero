# Ark Hero manual

*Versão em português: [MANUAL.md](MANUAL.md).*

Ark Hero is a guitar rhythm game that plays the songs (charts) made by the
Clone Hero community. No songs are included: you bring your own library.

- [Installing](#installing)
- [Songs](#songs)
- [A game, step by step](#a-game-step-by-step)
- [How to play](#how-to-play)
- [Pause](#pause)
- [Practice mode](#practice-mode)
- [Settings](#settings)
- [Accounts and controls](#accounts-and-controls)
- [Where the files are](#where-the-files-are)
- [Common problems](#common-problems)

## Installing

**PortMaster (R36S and other Linux handhelds):** extract `arkhero.zip` into
the `ports/` folder of the ROMS partition. You get `ports/Ark Hero.sh` and
`ports/arkhero/`. PortMaster's own install-from-zip works too. The game shows
up in the Ports list.

**Windows 10 or newer (64 bits):** extract
`ArkHero-<version>-windows-x86_64.zip` anywhere and run `arkhero.exe`. F11
switches between window and full screen.

**Linux PC:** build it from source (see the [README](../README.md)).

## Songs

Each song is a folder with:

- the chart: `notes.chart` or `notes.mid`;
- `song.ini` (name, artist, charter, delay);
- the audio: `song`, `guitar`, `bass`, `rhythm`, `keys`, `vocals`, `drums`...
  as `.opus`, `.ogg`, `.mp3` or `.wav`. Separate tracks are mixed together.

Where to put them:

- on the handheld: `ports/arkhero/songs/`;
- on Windows: the `songs\` folder next to `arkhero.exe`.

Subfolders are fine ("Guitar Hero 3/Tier 1/Song"), and the song list follows
them. To use a library somewhere else (another card, your Clone Hero folder on
the PC), write its path in `pastas.txt`, one folder per line (see
[Where the files are](#where-the-files-are)). After adding songs with the
game open, use "Scan for new songs" at the end of the list.

Not read yet: `.sng` files, drums, vocals and album art.

With no songs at all, the "Demo (no song)" entry plays sample notes with a
metronome.

## A game, step by step

1. **Play**, in the main menu.
2. **Players:** each player joins by pressing Confirm on their own controller.
   On the keyboard, Enter. Each player gets a card with four lines:
   - **Account:** a saved name, or guest;
   - **Mode:** controller or guitar (see [How to play](#how-to-play));
   - **Controls:** that account's buttons;
   - **Ready:** when everyone is ready, the game moves on to the songs.
3. **Songs:** pick the song. Left/Right jump a page. The first entry sorts the
   list by folder, title or artist.
4. **Setup:** each player picks the instrument (guitar, bass, rhythm, co-op
   guitar, keys) and the difficulty, among the ones the chart has, and gets
   ready. If someone takes too long, the leader (the first player) presses
   Confirm again and the song starts without them.
5. The song loads and the game starts. At the end you get each player's
   results: score, percentage of notes, best streak and, after 20 or more
   hits, an [audio offset](#settings) suggestion.

The menus work with a guitar alone: strum moves (up/down), the green fret
confirms and the red fret goes back.

## How to play

Notes come down the highway. Each color is a fret: green, red, yellow, blue
and orange. Each player picks one of these modes:

- **Controller mode** (the default): pressing the button on time plays the
  note. There is no strumming. On a chord, the buttons pressed have to be
  exactly the chord's.
- **Guitar mode:** hold the frets and strum. On a single note, lower frets
  held at the same time are fine. HOPOs (notes with a colored center) and taps
  play without strumming, by changing frets.

The rest works the same in both modes:

- **Sustains:** after hitting the head, hold until the end of the tail.
  Letting go early doesn't break the streak; it just stops adding points.
- **Streak and multiplier:** every 10 hits in a row raise the multiplier, up
  to 4x.
- **Star power:** notes with a cyan ring make up phrases. A whole phrase hit
  fills 25% of the meter, and whammy on those sustains fills more. With the
  meter at 50% or more, press the star power button: the multiplier doubles
  while the meter drains (16 s when full).
- **Overstrum penalty:** pressing (or strumming) with no note breaks the
  streak. It can be turned off.

## Pause

Start (or Esc on the keyboard) pauses the game. From the pause menu you can
resume, restart or go back to the song list.

When you resume, the song **rewinds 2 seconds**, with a countdown on screen,
so you can get back into the rhythm. Until the song reaches the point where
you paused:

- the notes you had already hit show up faded ("ghosts"), to play along;
- pressing with no note isn't penalized;
- the sustain you were holding and your star power stay as they were.

If someone's controller disconnects mid-song, the others keep playing and the
dropped player's notes just go by. If nobody is left, the game pauses.

## Practice mode

To drill a hard section, looped and slower, without high scores.

1. **Practice**, in the main menu. Then players, song and setup, as in a
   normal game.
2. **The practice screen:**
   - **Start (A)** and **End (B):** the song's sections ("Intro", "Verse 1",
     "Solo"...), the same ones as in Clone Hero. If the chart has none, the
     game makes one every 8 measures ("Measure 1", "Measure 9"...). Below, the
     section's time and number of notes;
   - **Song speed:** from 50% to 100%, in steps of 5. A slower speed also
     lowers the pitch, like a slowed-down tape;
   - **Start practice.** The audio loads while you choose.
3. **The section** starts 2 seconds before A, with a countdown. When it
   reaches B, it goes back to A by itself. At the top you see the speed, the
   section, the loop number and the note percentage of the previous loop.
4. **The practice pause menu** has more options:
   - **Song speed:** changes right away;
   - **Mark the start here:** the section starts at the measure where the
     song is, from the next loop on;
   - **Mark the end here:** the section ends at the end of this measure and
     restarts right away. To mark a section on the fly: pause at its start,
     mark the start, resume, pause at its end and mark the end;
   - **Choose the section:** back to the practice screen;
   - **Restart the section**, or **Quit to menu**.

To go back to normal play, use **Play** in the main menu.

## Settings

| Option | What it does |
|---|---|
| Controls | each player's buttons and keys (see below) |
| Highway speed | 1 to 10: how fast the notes come down (the song doesn't change) |
| Precision mode | a ±70 ms hit window instead of ±100 ms |
| Overstrum penalty | pressing with no note breaks the streak |
| Show hit timing | "Perfect / Good / Miss" and the timing error in ms, on the highway |
| Audio offset | makes up for sound or screen lag, -300 to +300 ms. Use the suggestion on the results screen |
| Lefty mode | reverses the lane order |
| Show FPS | frames per second, in the corner |
| Volume | 0 to 100% |
| Miss sound | the instrument "chokes" when you miss |
| Language | English, Portuguese or Spanish. The first time, the system's |
| Audio test | plays the first song in the list and shows the audio clocks |

## Accounts and controls

An account keeps a name, the button mapping, the keys and the mode
(controller or guitar). Create one on the **Account** line of your card, on
the Players screen. A guest uses the mapping of their slot (player 1, 2...).

In **Settings > Controls** (or on your card's **Controls** line), pick an
action and press the new button. You can also switch the whammy stick, the
face button style (automatic, Xbox or Nintendo), and restore the defaults.

- **USB guitars** are recognized as guitars. Map the frets once and they are
  saved with your account.
- **Keyboard:** can be split between players. Each one picks keys that don't
  repeat; the card warns about missing or repeated keys.
- **Select + Start** quits the game on the handheld.

Default controls:

| Action | R36S | Keyboard (player 1) |
|---|---|---|
| Green, red, yellow, blue, orange frets | R1, R2, L2, L1, bottom face button | A, S, D, F, G |
| Strum (guitar mode) | D-Pad up / down | Up / Down |
| Star power | Select, or a push up on the stick | Space |
| Whammy | stick left / right | W |
| Pause | Start | Esc |
| Menus: confirm / back | bottom / right face button | Enter / Esc |

## Where the files are

| | Handheld (PortMaster) | Windows |
|---|---|---|
| Songs | `ports/arkhero/songs/` | `songs\` |
| Saved data | `ports/arkhero/conf/arkhero/` | `conf\arkhero\` (or `%APPDATA%\arkhero\`) |
| Log | `ports/arkhero/log.txt` | `conf\arkhero\log.txt` |

Windows uses `%APPDATA%` when the game's folder isn't writable (under Program
Files, for example).

In the data folder:

- `pastas.txt`: other song folders, one per line (`#` starts a comment);
- `opcoes.cfg`: the settings;
- `jogadores.txt`: the accounts;
- `contas/`, `controles.cfg`, `controles2.cfg`...: the mappings;
- `recordes.txt`: each song's high score.

## Common problems

- **No songs in the list:** check that each song has its own folder, with the
  chart and the audio inside, and that the folder is in `songs/` or in
  `pastas.txt`. The log says how many folders were checked and how many songs
  were found.
- **Notes arrive before or after the sound:** play a whole song and use the
  audio offset suggestion on the results screen.
- **A controller doesn't respond:** remap it in Controls. Wireless controllers
  that share a single USB receiver can still get mixed up; one per receiver
  works better.
- **Something went wrong:** `log.txt` records what the game found, what it
  loaded and a summary of each game. Send it along when you report a problem.
