# MAG: handover

## Cloud experiment (read this first)

This repo runs the RVIP import in a Claude Code **cloud** session. Everything
the procedure normally takes from sibling folders on the maintainer's Mac is
bundled under `rvip/`:

- `rvip/RVIP.md` — the procedure (snapshot; the canonical copy lives on the
  Mac). **Write lessons into `rvip/LESSONS.md`** (new file, one bullet per
  lesson naming the RVIP section it belongs to); never edit `rvip/RVIP.md`.
- `rvip/web/rvip-wm.js`, `rvip/web/rvip-sound.js` — shared page code every
  game loads (window manager, sound). Use, don't fork.
- `rvip/templates/roguepc/` — the **DOS / IBM PC Rogue** web port (RVIP
  section R-PC, worked example): `port/pcvideo.c` (the BIOS/conio text
  screen emulated as an 80x25 cell buffer with attributes), `port/fe_web.c`
  (the cells and keys to the page), `port/tiles.c` + `port/mktiles.py` +
  `port/dawnhack/`, `port/classicrogue/`, `port/nethack/` (the tile sets
  Rogue-family ports use: DawnLike + NetHack, see the RVIP rule), the VGA
  font (`port/Bm437_IBM_VGA_9x16.otb`, `port/vgafont.h`, `mkfont.py`),
  `web/build.sh`, `web/roguepc.js` + `web/index.html`, `web/make-help.py`,
  `HANDOVER.md`, `README.md`. Copy its solutions.
- `rvip/templates/xrogue/` — the curses-shim-with-panes Rogue port
  (R-frontend section) as a second reference for panes, explore and stairs.

**The game.** MAG — "A Dungeon Adventuring Game", Copyright 1986–88 Michael
J. Teixeira, "general permission to copy or modify, but not for profit"
(the licence text is the header of every source file). DOS, C (Microsoft/
Turbo C era: `<dos.h>`, `<conio.h>`, `<process.h>`, `interrupt far`
handlers for Ctrl-Break, screen attributes via `out_attrib`/`scrtype`,
COLOR/MONOCHROME modes in `MAG.H`), 36 files in `src/` dated 1988–89,
uppercase DOS names, CRLF line endings. Rogue family, case **R** with the
**R-PC** notes (DOS Rogue). Find its version string and any docs inside the
sources; there is no readme in the drop. Lineage: an original 1980s DOS
roguelike inspired by Rogue; verify on the web (RogueBasin "MAG", Teixeira)
and record author/year/base for the Mac side's tree entry.

**Route.** Keep the game code and replace the DOS layer: `conio`/`dos.h`
calls (`cprintf`, `gotoxy`, `textattr`, `putch`, `getch`, `kbhit`,
`int86`, direct video, interrupt vectors) go through one `port/` layer that
keeps an 80x25 cell buffer and a key queue, exactly like `pcvideo.c`/
`fe_web.c` do for Rogue PC; the page only blits cells and sends keys. Build
with Emscripten (Asyncify for the key waits, IDBFS for saves). Expect W8
fixes (16-bit `int` assumptions, `far` pointers, `long` on wasm32, CRLF,
K&R prototypes: compile with `-std=gnu89 -fcommon -w`).

**Tiles:** MAG is a text game. The Rogue-family rule (RVIP R4) applies:
DawnLike + NetHack fallback as in the template, one set, ≥95% coverage of
every monster/object/feature glyph MAG draws (count them from `MONSTER.H`,
`OBJECT.H`, `OBJDEF.H` with a script), or text mode. Record the numbers.

**Differences from a local run**
- No browser pane and no ruzzoli.de deploy key. Stages 1–6 are in scope.
  Tests: a headless node run if the page allows (the template has none;
  Playwright/Chromium is the main check: `/opt/pw-browsers` or `npx
  playwright install chromium`), driving `web/dist` served by
  `python3 -m http.server`: new game, random keys, save/reload/restore,
  explore, stairs, menus, tiles (canvas pixels), screenshots into
  `web/shots/`. Write `web/deploy.sh` like the template's (target
  `ruzzoli.de/roguelikes/mag/`); never run it.
- Toolchain: Emscripten (`git clone https://github.com/emscripten-core/emsdk
  && ./emsdk install latest && ./emsdk activate latest`, ~2 min), gcc/clang
  + ASan for a native headless build (random keys, isolated `HOME`). Record
  every command in `web/toolchain.sh`. If Emscripten cannot be installed,
  write that file with what you tried and the errors, commit, push, stop.
- Commit after every stage (`RVIP: stage N <topic>`) and **push to `origin`**
  (github.com/memmaker/mag, private). Every commit message ends with
  `Co-Authored-By: Claude Fable 5.1 <noreply@anthropic.com>`. Never force-push.
- Never `fetch()` `.cfg`/data files from the page: preload them and read
  with `Module.FS.readFile`.
- The Docs page (stage 6) is not here: write `docs/web/mag-docs.html` in
  the shape of the template's `make-help.py` output and note it for the Mac.
- The upstream source came as a folder with no history: the first commit is
  the drop, our commits go on top on `main`.

## RVIP progress

### Stage 1 — build (done, 2026-09-26)

- **Case R, R-PC notes** (DOS roguelike on the BIOS text screen). Game:
  "MAG version PC-1.1, Summer 1989 - by Michael J. Teixeira" (`version()`
  in `src/COMMAND3.C`). No docs in the drop: `data/mkhelp.py` rebuilds the
  missing `help/help.1-7` and `pics/header|tomb|herobox` from the game's own
  tables and the coordinates the code draws into them (MAIN.C refuses to
  start without them).
- **Port layer** (`port/`, game code untouched except `#ifdef PORT` hooks):
  `port/port.h` is force-included (`-include port.h`) and replaces the Microsoft
  C / DOS headers (`port/inc/{dos,conio,graph,io,process,malloc,memory}.h`);
  `port/inc/*.h` are lowercase symlinks to `src/*.H`. `port/pcvideo.c`
  emulates BIOS int 10h (two 80x25 pages of attr<<8|CP437, cursor, scroll,
  write char+attr), int 16h keys (scan codes for arrows/F-keys/keypad),
  `kbhit/getch`, `_setvisualpage`, `_dos_findfirst`, DOS paths (`help\x` ->
  `/magdata/help/x`, `save\x` -> `save/x`). `port/rvip.c`: autosave
  (`port_idle()` from `dispatch()` in MOVE.C, every >=2 s at the command
  prompt, same files as `R`), build stamp `save/build.id` (the saves hold raw
  pointers, so a save only fits the build that wrote it; others are dropped),
  `port_exit()` deletes the autosave at game end unless `R` saved.
- **Frontends:** `port/fe_web.c` (Module.mag in `web/mag.js`: init(font),
  text(vram, cursor), key/pending/flush, sync, end) and `port/fe_tty.c`
  (native headless test: `MAG_KEYS`, `MAG_RANDOM=N`, `MAG_SEED`, `MAG_DUMP`,
  `MAG_DATA=data`).
- **Build:** `sh web/build.sh` (emcc 6.0.10 via emsdk, `web/toolchain.sh`):
  each `src/*.C` compiled with `-x c` (emcc treats `.C` as C++), flags
  `-O2 -std=gnu89 -fcommon -w -funsigned-char -DPORT -Iinc -I. -include port.h
  -Wno-error=incompatible-function-pointer-types,int-conversion,incompatible-pointer-types,implicit-function-declaration`;
  link `-sASYNCIFY -sASYNCIFY_STACK_SIZE=65536 -sSTACK_SIZE=1048576
  -sALLOW_MEMORY_GROWTH -sFORCE_FILESYSTEM -lidbfs.js -sEXIT_RUNTIME=0`,
  `--preload-file data/help@/magdata/help` (+pics). IDBFS mounted on `/mag`
  (cwd; `save/`, `options.mag`, `heroes.mag`). wasm 275 KB.
  Native: `make -C port [ASAN=1]` (gcc; clang 18 here has no ASan runtime).
- **Tests:** ASan+UBSan native, 40 seeds x up to 30000 random keys (~75k keys
  played) clean after the fixes below; emcc `-fsanitize=address` build through
  new game, save/reload and 400 random keys to death: clean. Playwright:
  `node web/tests/stage1.cjs` (title, new game, autosave, reload -> "Welcome
  back", status line equal, 400 random keys, no page errors), screenshots
  `web/shots/s1-*.png`.
- **Upstream fixes (`port:` commits):** DOS `^Z` EOF bytes; K&R variadic
  `pline(str, ...)`; `<sys\types.h>`; `possitems()` wrote `final[-1]`;
  `long t; time(&t)` (8-byte time_t into a 4-byte long on wasm32: stack
  overwrite in `main()` and the tombstone).
- **Quirks:** DOS `int` was 16 bit, saves are raw struct dumps with pointers
  (fine within one build, see build stamp). `R` saves *and exits*; restore
  deletes the save (autosave rewrites it). `Enter` (^M) is the wizard
  command `wmonstat` in COMMAND.H (free for the Enter menu: wizard-only).
  Wizard password (WIZARD.C): `frakola` (^W). Game start argv: `l<N>` start
  level, `s<N>` seed (MAIN.C, undocumented).
- **Tiles count (DawnLike, `DawnLikeAtlas/renamed`, 4152 names):** MAG draws
  55 monsters (`MONSTER.H`), 12 object classes / 151 kinds (`OBJECT.H`,
  `VARS.H` CITEMS), ~15 features (walls, 4 corners, corridor, floor, door,
  up/down stairs, pool, marble, trap, 4 wall torches, player). Exact name
  hits: monsters 13/55, object kinds 16/151; unidentified potions/wands/
  scrolls/rings show MAG's random appearance names (`fpotions` "bubbly",
  `fwands` "ebony" ...), many of which DawnLike has by name ("bubbly potion",
  "ebony wand"). Every monster has a fitting DawnLike stand-in (drakes,
  wyrms, nymphs, golems, spirits, vortices, ...). **Decision: DawnLike only,
  one set, 100% of slots via name + hand table (stage 4), no NetHack mix.**
- **Lineage (web, 2026-09-26):** MobyGames / DOS Games Archive / RogueBasin
  "Mike's Adventure Game": MAG = *Mike's Adventure Game*, by Michael J.
  Teixeira, DOS 1988 (RogueBasin: "1985 if you count the original UNIX
  version"), freeware with C source; "an early Rogue clone", "somewhere
  between the complexity of the original rogue and that of hack", uses the
  PC extended character set like PC Rogue; goal: the Sudbury Sapphire, held
  by the Imperial Dragons. CRPG Addict played it as Game 52 (1988). New code
  inspired by Rogue: tree entry `<li class="insp">` under Rogue, 1988 (1985
  UNIX), Michael J. Teixeira. (RogueBasin/dosgames pages are blocked from the
  cloud; facts above from search-result text, verify on the Mac.)

### Stage 2 — explore + stairs (done, 2026-09-26)

- **Explore key `Z`** (MAG's `x` is "swap to the secondary weapon"; `Z` is
  unbound in `COMMAND.H`). Code: `port/rvip.c` (`port_command()`,
  `port_auto()`, `next_dir()` BFS). **Main-loop hook** in `dispatch()`
  (`src/MOVE.C`, `#ifdef PORT`): before reading a command
  `typed = port_auto()` (next step while exploring/walking), after reading
  `typed = port_command(typed)` (0 = handled by the port -> `continue`).
  Steps go through the game's own walk command (a direction key).
- **Known-grid test:** `dun[l][c].d_data & D_SEEN` or a non-blank cell in the
  game's page-0 shadow screen `optscr[0]`, remembered per level in
  `known[][]` (MAG clears `D_SEEN` of dark-room floors when you leave:
  `know(d, NO)`); `stood[][]` = cells the player stood on. Targets: unvisited
  known passable cells next to unknown cells, and unvisited items.
  Passable = not `D_STONE`, not a known trap (`TRAPC`) or water (`POOL`),
  not a door with `DR_WITHLOCK` (magic locks are never picked); diagonals
  follow `nodiagmove()`. Stops: new top-line message, a monster coming into
  view, any key, a step that did not move, blind/confused; a visible monster
  on the next step -> "There's a jackal in the way." (never attacks).
- **Stairs:** `>`/`<` on the right stairs (or in wizard mode) run the
  original `stepdown`/`stepup`; elsewhere they walk to the nearest *known*
  staircase (`D_STAIRCASE`, `DNSTAIR`/`UPSTAIR`) and take it on arrival;
  `+`/`-` keep the original meaning. Walking to the level-1 up staircase
  stops there with "These stairs lead out of the dungeon. Press < to leave."
- **Tests:** native `python3 port/tests/explore_stairs.py [seeds]` (needs
  `make -C port ASAN=1`; `MAG_KEYS` escapes `\^W` = Ctrl-W); browser
  `node web/tests/stage2.cjs` (page `?seed=N` passes MAG's undocumented `sN`
  argument: seed 3 explores 117 -> ~290 cells, seed 1 walks to the stairs
  and reaches level 2; it fights adjacent monsters by walking into them).
  Wizard mode for tests: `^W` + `frakola`, `#` maps walls/stairs only (room
  floors stay unknown, so the walk can't use them).
- Help screens (`data/mkhelp.py`) and the page hint mention `Z` and `<`/`>`.
- **Next: stage 3** (Enter menu + inventory). Enter (^M) is the wizard
  command `wmonstat`: take it only when `wizard != YES`. Item actions: the
  game's `utilize()` (COMMAND1.C, `util[]` table: command letters
  `"qezprwtWcd\025L\013SI/\011])[sX"`) reads the item letter with
  `ctgetch()`; push the letter into a port key queue. Item lines:
  `form(o, NO)`, colours `o_type + 1 + (o_type > 6)` (O_COLORINV).
