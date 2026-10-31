## Notes

Ark Hero is a guitar rhythm game written in C with SDL3 for Linux handhelds: https://github.com/jvses/Ark_hero

It plays the songs made by the Clone Hero community. No songs are included: copy each song folder (`notes.chart` or `notes.mid`, `song.ini` and the audio in ogg, opus, mp3 or wav) into `ports/arkhero/songs/`. Subfolders are fine, and the song list follows them. To use a library that is somewhere else on the card, write its path in `ports/arkhero/conf/arkhero/pastas.txt`, one per line. Without songs, the Demonstration entry plays sample notes with a metronome.

Up to 4 players: the built-in controls, USB controllers, USB guitars and the keyboard, each with its own mapping (Settings > Controls). Each player picks controller mode (press the frets on time) or guitar mode (hold the frets and strum). Menus in English, Portuguese and Spanish.

Thanks to the Clone Hero community for the chart format and the songs, to the authors of SDL, libogg, libvorbis, opus, opusfile, dr_mp3 and stb_truetype, and to artdzyk for the Terminal 16 Unicode font.

## Controls

| Button | Action |
|--|--|
| R1, R2, L2, L1, bottom face button | Green, red, yellow, blue and orange frets |
| D-Pad Up / Down | Strum (guitar mode) and menus |
| Select or a push up on the stick | Star power |
| Stick left / right | Whammy |
| Bottom / right face button | Confirm / back in the menus |
| Start | Pause |
| Select + Start | Quit |

## Compile

```bash
git clone https://github.com/jvses/Ark_hero.git
cd Ark_hero
./tools/docker_build.sh aarch64
./tools/package_port.sh
```
