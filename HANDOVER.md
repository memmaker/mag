# MAG: handover

All RVIP stages 1-9 done. Live: https://ruzzoli.de/roguelikes/mag/ · repo
https://github.com/memmaker/mag (remote `memmaker`, branch `main`) · shrine
https://ruzzoli.de/roguelikes/shrine/mag.html. Stages 1-6 were first done in a
cloud session (the old private `memmaker/mag-cloud` repo is deleted).

## The game

- MAG = *Mike's Adventure Game*, "version PC-1.1, Summer 1989" (`version()` in
  `src/COMMAND3.C`), Michael J. Teixeira, DOS 1988 (1985 UNIX version per the
  DOS Games Archive). Licence: header of every source file ("general permission
  to copy or modify, but not for profit"). Tree: `li.insp` under Rogue.
- Case R with the DOS-Rogue notes (RVIP.md 5.5). 36 C files in `src/`, uppercase
  DOS names, CRLF; upstream import commit `cc36a63` (byte-identical to the
  source zip in `Original_Dos_Version_from_1988.zip`, SourceForge
  `mikesadvgame`).
- `data/help/help.1-7`, `data/pics/{header,herobox,tomb}` are the originals from
  that 1988 zip (lowercased, LF, `^Z` stripped, CP437 kept). MAIN.C refuses to
  start without them. README.DOC is the shrine manual.
- Quirks: saves are raw struct dumps with pointers, so a save only fits the
  build that wrote it (`save/build.id` stamp drops others). `R` saves *and
  exits*; restore deletes the save. Wizard mode `^W` + `frakola` (immortal; a
  second `^W` turns it off); argv `l<N>` start level, `s<N>` seed (page
  `?seed=N`).

## Build, test, deploy

- `sh web/build.sh` (emcc; `web/toolchain.sh`): each `src/*.C` with `-x c
  -std=gnu89 -fcommon -w -funsigned-char -DPORT -include port.h`, Asyncify, IDBFS
  on `/mag` (cwd: `save/`, `options.mag`, `heroes.mag`, `web-layout.json`,
  `web.cfg` = auto_more). Loads the shared `../rvip-*.js`. Runs `web/mksounds.py`
  (synthesizes `dist/sound/<event>.wav`, PC-speaker square waves made for MAG).
- Stage 6 sound search (2026-09-29): MAG's only sound is the DOS bell. Donnie Russell's
  MAGHD/JSMikesAdvGame ports have effects of unknown origin and no licence: not used.
  No music.
- `sh web/deploy.sh` (guard: clean tree, HEAD == `memmaker/main`).
- Native headless: `make -C port CC=clang [ASAN=1]` (plain macOS gcc fails on
  vars.h); `port/fe_tty.c` env `MAG_KEYS`, `MAG_RANDOM=N`, `MAG_SEED`,
  `MAG_DUMP`, `MAG_DATA=data`, `MAG_BEACON=1`. `python3
  port/tests/explore_stairs.py [seeds]`.
- Browser tests: `node web/tests/stage1..6.cjs` (Playwright, serves `web/dist`
  with the shared `rvip-*.js` next to `mag/`).
- Help: `web/make-help.py` reads the Docs entry (`~/Desktop/Games/Roguelikes/Docs`
  `build-docs.py` GAMES `mag.html` + `guides.py`) → `dist/help.html`;
  `--page` writes `docs/web/mag-docs.html`.

## Port map

- `port/port.h` (force-included) + `port/inc/` replace the Microsoft C / DOS
  headers; `port/pcvideo.c` emulates BIOS int 10h/16h (two 80x25 pages), DOS
  paths (`help\x` → `/magdata/help/x`).
- `port/rvip.c`: autosave from `dispatch()` (`src/MOVE.C`, `port_idle`),
  explore `Z` + `<`/`>` walk-to-stairs (`port_auto`, `port_command`, BFS
  `next_dir`; known grid = `D_SEEN` or non-blank `optscr[0]`, kept in
  `known[][]`), sounds (`port_sound`, `port_sound_verb`), beacon
  `port_run_end` (from MAIN.C `doquit`/`doexit`; escape with the Sapphire =
  win; lvl/depth/score/turns all present).
- `port/menu.c`: Enter menu (Enter = wizard `wmonstat` only in wizard mode),
  inventory `i`/F3, item menus; item actions run as game commands with the item
  letter queued (`port_push_key`).
- `port/tiles.c` + `port/mkdawn.py` → `port/tiles-dawn.png` (+ `-1` animation
  frame) / `tiles.h`: DawnLike only, 100 % of 323 slots (name hits + stand-ins,
  unidentified items by appearance name). Credits in `port/dawnlike/CREDITS.txt`.
- Remapping (2026-09-30): `port/mag-dawnlike.rec` maps every tile id
  (`world/`, `monster/`, `object/` + slug of the game's name) to a cell of
  `dawnlike-0.png` (all DawnLike sheets stacked, `-1` = second frame). Edit it with
  `remapper port/mag-dawnlike.rec` (~/Projects/remapper; F2 previews
  `port/mag-scenes.rec`). `fe_web.c` reads it at startup with c-rec
  (~/Projects/c-rec, by path in `web/build.sh`, preloaded as
  /magdata/tiles.rec; `MAG_REC=` picks another rec). No rec → `tiles.h` +
  `tiles-dawn.png` as before. `mkdawn.py` keeps the rec's icons when re-run;
  `mkscenes.py` rebuilds the scenes and asserts every id is in one.
- `port/fe_web.c`: fixed-size (declared by the user 2026-09-30): the Map
  canvas is always the whole 80x25 visual page on one grid fitted to the window
  (square cells with tiles, the font's cell otherwise; box/shade glyphs
  stretched to the cell, no gaps); pop-ups and text pages draw on it, One window
  = the PC screen. Messages (from the `pline()` hook), Status, Inventory,
  Visible are HTML lines (`be_line`/`be_rows`, CGA colour runs). auto_more
  skips the top-line `=-More-=`. Food shows light green (was CGA blue).
- Upstream fixes: `^Z` bytes, K&R variadic `pline`, `possitems()` `final[-1]`,
  `long t; time(&t)` on wasm32, MAIN.C name prompt `cp[25]`.

## Open

- Win beacon (`in_inv(SAPPHIRE)` on escape) never reached live.
- Keys picked inside our lists bypass `tgetch()`, so macros (`{ }`) don't
  record them.
- Blink attribute not shown (steady text).
