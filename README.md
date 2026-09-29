**RVIP port** of MAG, *Mike's Adventure Game* ("A Dungeon Adventuring Game"),
version PC-1.1 (Summer 1989) by Michael J. Teixeira, from the author's DOS C
source (36 files dated 1988-89, no history); commit `cc36a63` is that source
drop untouched (local copy `~/Downloads/mag_src`).
Play: https://ruzzoli.de/roguelikes/mag/
Our changes: https://github.com/memmaker/mag/compare/cc36a63...main

MAG is an early DOS roguelike (1988; RogueBasin: 1985 if you count the
original UNIX version), new code inspired by Rogue, "somewhere between the
complexity of the original rogue and that of hack": 55 monsters, 151 kinds of
objects in twelve classes, locked doors and keys, dark rooms lit by wall
torches, pools, a secondary weapon for bows. Goal: find the Sudbury Sapphire,
held by the Imperial Dragons, and bring it back to the surface.
Sources: [RogueBasin](https://www.roguebasin.com/index.php/Mike's_Adventure_Game),
[CRPG Addict, Game 52](http://crpgaddict.blogspot.com/2011/03/game-52-mikes-aventure-game-mag-1988.html),
[DOS Games Archive](https://www.dosgamesarchive.com/download/mag).

What this port adds (game code in `src/` touched only by `#ifdef PORT` hooks
and a few `port:` bug fixes):
- **DOS layer replaced at the BIOS level** (`port/pcvideo.c`): int 10h text
  video on two 80x25 pages, int 16h keys with scan codes, `conio`/`dos.h`
  stand-ins (`port/port.h`, `port/inc/`). Emscripten + Asyncify, saves in the
  browser's IndexedDB with autosave and a build stamp (the saves hold raw
  pointers). The help and picture files (`data/`) are the originals from the
  1988 DOS release (line ends and the DOS EOF byte converted).
- **Windows** (`web/mag.js`, shared `rvip-wm.js`): Map (tiles), Messages
  with history, Status, Inventory and Visible, pop-ups over the map; Text
  mode = the original VGA screen (IBM VGA 9x16 font, CGA colours), F12.
- **Explore** `Z`, **`<` / `>`** walk to the nearest known staircase and take
  it, **Enter menu** of all commands, **inventory cursor** with item menus,
  no `=-More-=` stops (auto_more) (`port/rvip.c`, `port/menu.c`).
- **Tiles**: DawnLike only, 100% of the 323 slots by name + same-set
  stand-ins (`port/mkdawn.py`, `port/tiles.c`), nearest-neighbour.
- **Sound**: events named by the game code (`port_sound()`), Dubtrain
  Angband samples (`web/sounds.py`), off by default.

Controls: `hjklyubn` / arrows / keypad walk and attack, Shift+direction
runs, `Z` explore, `>` `<` stairs, Enter command menu, `i` inventory, `e`
eat, `q` quaff, `r` read, `z` zap, `w` wield, `W` wear, `t` throw/fire,
`x` swap to the bow, `I` light a wall torch, `^K` unlock, F1 help screens,
`R` save and leave, `Q` quit. The page's Help button opens the game guide.

Build: `sh web/build.sh` -> `web/dist` (Homebrew emscripten, see
`web/toolchain.sh`). Native headless test build: `make -C port [ASAN=1]`.
Browser tests: `node web/tests/stage1.cjs` ... `stage6.cjs` (Playwright).
They serve `web/dist` as `mag/` next to the shared `rvip-*.js` from
`../rvip-tools/web`; set `PLAYWRIGHT` to a Playwright module directory when it
is not in `/opt/node22` (e.g. the one `npx playwright` unpacked under `~/.npm/_npx`).
Deploy: `sh web/deploy.sh`. Notes: `HANDOVER.md`.

Credits and licence: MAG by Michael J. Teixeira, copyright 1986, 87, 88:
"General permission to copy or modify, but not for profit, is hereby
granted, provided that the above copyright notice is included" (header of
every source file); this port is non-commercial. Map sprites: DawnLike by
DragonDePlatino with DawnBringer's palette (CC BY 4.0), sprite names from
DawnLikeAtlas by Tommy Ettinger (CC BY 4.0), see `port/dawnlike/CREDITS.txt`.
Text font: IBM VGA 9x16 from The Oldschool PC Font Resource by VileR
(CC BY-SA 4.0). Sound samples: Dubtrain's Angband sound pack. Web port:
memmaker.
