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
  in `src/COMMAND3.C`). The source drop had no data files; since
  2026-09-29 `data/help/help.1-7` and `data/pics/{header,herobox,tomb}` are the
  originals from the 1988 DOS release (SourceForge `mikesadvgame`,
  `Original_Dos_Version_from_1988.zip`, `game/HELP`, `game/PICS`), lowercased,
  CRLF -> LF and the DOS `^Z` EOF stripped (the port's `fopen(,"r")` keeps both),
  CP437 bytes as they are. They replace the earlier `data/mkhelp.py` rebuild
  (deleted; the port's `Z`/`<`/`>` hints are no longer on the F1 screens, the
  Enter menu and Help button list them). MAIN.C refuses to start without them.
  The zip's source (`MAG_SRC_...ZIP`, 36 files) is byte-identical to `src/` in
  the upstream commit `cc36a63`.
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
  staircase (`D_STAIRCASE`, `DNSTAIR`/`UPSTAIR`) and stop on it (press the
  key again to take it: auto-stairs only walks);
  `+`/`-` keep the original meaning. Walking to the level-1 up staircase
  stops there with "These stairs lead out of the dungeon. Press < to leave."
- **Tests:** native `python3 port/tests/explore_stairs.py [seeds]` (needs
  `make -C port ASAN=1`; `MAG_KEYS` escapes `\^W` = Ctrl-W); browser
  `node web/tests/stage2.cjs` (page `?seed=N` passes MAG's undocumented `sN`
  argument: seed 3 explores 117 -> ~290 cells, seed 1 walks to the stairs
  and takes them with a second `>` to reach level 2; it fights adjacent monsters by walking into them).
  Wizard mode for tests: `^W` + `frakola`, `#` maps walls/stairs only (room
  floors stay unknown, so the walk can't use them).
- The page hint mentions `Z` and `<`/`>` (the F1 screens are the originals since 2026-09-29).

### Stage 3 — Enter menu + inventory (done, 2026-09-26)

- **Files:** `port/menu.c` (menus, inventory, item prompts, key queue),
  hooks in `port/rvip.c` `port_command()` (Enter -> `port_menu()` unless in
  wizard mode, where ^M stays `wmonstat`; `i`/F3 -> `port_inventory()`) and
  `port_auto()` (reopen the inventory after an item action unless a monster
  is in view: `port_inv_again`). `src/COMMAND1.C` `utilize()`: the item
  prompt calls `port_item_prompt(util[cmnd].c_types, verb)` under
  `#ifdef PORT`.
- **Menu:** `port_menu()`: group box (1-5: Moving, Things you carry,
  Listing what you carry, Information, Macros and the game, as the help
  screens group them), then `group_menu()` with key + description; arrows /
  keypad 8 2, digits, Enter / keypad 5 / Right, Escape / keypad 0 / Left,
  the command's own key, mouse click (FK_CLICK row from `web/mag.js`).
  Boxes are drawn onto the visual BIOS page (`box_open()` sized to the
  content, rows 1-22 only, restored by `box_close()`); `fe_popup()` tells
  the frontend the box (used by the tiles page in stage 5).
- **Item actions run through the game:** the command key is returned to
  `dispatch()` and the item letter is queued (`port_push_key()`; pcvideo's
  `bios_key()` and `port_item_prompt()` read the queue first; `port_idle()`
  clears stale letters). Main action by pobj flags: eat, quaff, read, zap,
  put on, wear, strap, wield, use, unlock, throw, else examine
  (`pline(form(o, YES))`). Shift+letter drops (only where MAG's letter is
  lowercase: items 27+ are `A`-`Z` in MAG), Ctrl+letter examines,
  Enter/Space/click -> `item_menu()` (every fitting `util[]` action with its
  key + Examine), numpad `+ - *`, `0`/`.` close. Colours: the game's own
  O_COLORINV (`o_type + 1 + (o_type > 6)`).
- **Tests:** `node web/tests/stage3.cjs` (menu groups, group list, Escape,
  click runs `T`, inventory, item menu, letter eats, reopen/stay closed,
  wield prompt list); ASan random keys (25 seeds, Enter and `i` in the pool)
  clean.
- **Open:** 3d auto_more (MAG's `=-More-=` waits in `more()`/`domore()`)
  goes with the Messages window in stage 5. Keys picked inside our lists
  bypass `tgetch()`, so macros (`{ }`) don't record them.

### Stage 4 — tiles (done, 2026-09-26, resumed cloud session)

- **Set: DawnLike only** (DragonDePlatino, CC BY 4.0, palette DawnBringer),
  sprites looked up by name in DawnLikeAtlas `renamed/` (Tommy Ettinger);
  no NetHack mix, no text fallback on the map. `port/mkdawn.py [atlas dir]`
  reads MAG's own lists (`MONSTER.H` pmon, `OBJECT.H` pobj + the fake
  appearance arrays) and writes `port/tiles-dawn.png` (230 sprites, 16x16,
  32 per row) + `port/tiles.h` (slot tables). Credits:
  `port/dawnlike/CREDITS.txt` (Help page/README must repeat them).
- **Coverage (script output):** monsters 13/55 by exact name + 42 stand-ins
  from DawnLike (drakes/wyrms for the dragons: red=searwyrm, white=icewyrm,
  blue=storrmwyrm [atlas spelling], Imperial=kingwyrm; axe beak=terror bird,
  zephyr=air elemental, ...); object kinds 15/151 by name, rest stand-ins;
  unidentified potions/wands/rings/scrolls by their random appearance
  (27/87 appearance names exact, e.g. "bubbly potion", "ebony wand"; the
  rest take unused sprites of that kind); 30 feature slots (walls, 4
  corners, 4 wall torches, marble, corridor, floor, door, locked door,
  up/down stairs, pool, 9 trap kinds, staff, player). **100% of 323 slots
  from the one set** (>= 95% rule met).
- **Loader:** `port/tiles.c` `tile_for(l, c, &under)` decides per map cell
  from the game's page-0 shadow screen `optscr[0]` + its lists (player
  `u.u_d`, `mons[]` by `m_perm - pmon` when the shown letter matches, i.e.
  invisible/deluded monsters stay as MAG shows them, `lobjs[]` by kind or
  appearance `nameptr[off][1]`, `doors[]` locked state, `traps[]` kind);
  sprites with see-through parts get the floor under them. Native
  `fe_tty.c` runs the same lookup every frame so ASan covers it.
- **Frame:** `port/fe_web.c` `fe_present()` sends `js_tiles(vram[0], visual
  page, tile/under arrays 22x80, inventory lines, pop-up box, cursor, hero,
  level)` once a game runs (`port_started`), text frames before (title,
  name). Pop-up = `fe_popup()` box of the port menus, or the bounding box
  of page 1 (MAG's own full screens, tomb). `fe_idle()` (from
  `port_idle()`) builds the inventory (`form()`, O_COLORINV colours) and
  the Visible list (monster/object names from the game tables) only at the
  command prompt, since `form()`/`obj_str()` use static buffers.
- **Page:** `web/mag.js` is now the Rogue PC template's `roguepc.js`
  adapted: tiles mode default with Map / Messages / Status / Inventory /
  Visible windows (rvip-wm.js), pop-up over the map, text mode = VGA screen,
  Tiles/Text buttons + F12, Zoom, nearest-neighbour sprites at 12-64 px
  (scale = `L.tile`, DawnLike native 16). JS only blits; slots come from C.
- **Messages:** `pline()` hook (`port_msg`, `src/VISUAL1.C` `#ifdef PORT`)
  feeds the Messages window (repeats fold to `(xN)` in JS since MAG itself
  drops exact repeats of the last line). **auto_more** (3d): `more()` skips
  the top-line `=-More-=` while `fe_auto_more` (button, default on, kept in
  `/mag/web.cfg`); `fe_at_cmd` drives `RvipWM.prompt.wait()`.
- **Upstream fix:** MAIN.C name prompt wrote `cp[25]` (into the "-1" ESC
  literal, and one past `u_name[25]`): `port:` commit.
- **Tests:** `node web/tests/stage4.cjs` (tiles default, 117 sprite cells /
  0 text cells on seed 3, player sprite pixels, F12 text/tiles, inventory
  window); stage1-3 tests pass again (stage3 now clicks inside the pop-up
  and reads the eat message from the Messages history). ASan native 25
  seeds x 20000 random keys clean. Shots `web/shots/s4-*.png`.
- **Next: stage 5** (window polish, game end overlay, `web/deploy.sh`,
  `make-help.py`).

### Stage 5 — web page (done except the deploy, 2026-09-26)

- **Windows** (rvip-wm.js, as Rogue PC): Map (DawnLike, player centred via
  `RvipWM.center` with the hero cell from C), Messages (history from the
  `pline()` hook, `(xN)` folding, live top line + `RvipWM.prompt`), Status
  (row 23), Inventory (game lines + O_COLORINV colours), Visible (monsters
  and objects in view, names/colours from the game tables); pop-up over the
  map for port menus and MAG's page-1 screens; Text mode = one VGA window.
  Layout/zoom/mode in `/mag/web-layout.json` (IDBFS), auto_more in
  `/mag/web.cfg`. Resize 1000x650 / 1440x900 / 760x500 / 1280x800 checked.
- **Game end:** `port_exit()` clears `port_started`, so the last screen
  (Hall of Heroes, drawn on page 0) goes out as text; `fe_exit()` waits for
  a key, syncs, shows the overlay (saved / game over) with *Play again*.
  R (+ y) keeps the save and Play again restores ("Welcome back"); Q
  (+ y) and death delete it.
- **Help:** `web/make-help.py` -> `dist/help.html`, content from
  `docs/web/magguide.py` (shared with the stage 6 Docs page; stands in for
  the Mac's Docs `build-docs.py`/`guides.py` entry): about, keys to
  remember + essentials + complete key list, saving (web), tips, new
  player's guide, history, playing in the browser, About this version (W1:
  base = MAG PC-1.1, untouched import commit `cc36a63`, compare link to
  memmaker/mag main) + credits (Teixeira licence, DawnLike CC BY 4.0,
  VGA font CC BY-SA 4.0).
- **Deploy:** `web/deploy.sh` written (guard: clean tree, HEAD ==
  origin/main, build present; target `ruzzoli.de:/var/www/ruzzoli.de/
  roguelikes/mag/`), **never run** in the cloud (no key). Live URL after
  the Mac deploys: https://ruzzoli.de/roguelikes/mag/
- **Tests:** `node web/tests/stage5.cjs` (windows filled, Help, resize,
  zoom kept over reload, R -> overlay -> Play again restores, Q -> Hall of
  Heroes as text -> overlay, save deleted, no page errors); stage1-4 pass.
- **Open for the Mac:** run `sh web/build.sh` with `RVIP_WM=~/Games/rvip-tools/web`
  and `sh web/deploy.sh`, check the live URL; the page has no og: meta yet
  (`<!--og--><!--/og-->` placeholder for the selection-page step); no
  beacon (stage 9).

### Stage 6 — docs + sound (done in the cloud; Docs merge on the Mac, 2026-09-26)

- **Sound (R7):** the game names the events, no message matching:
  `port_sound()` calls under `#ifdef PORT` in `src/ATTACK.C` (hit/miss,
  death), `src/MATTACK.C` (hurt/mmiss), `src/MONSTER2.C` (kill),
  `src/MISC.C` (level, stairs in `newlev()`, teleport), `src/OBJECT.C`
  (pickup/gold), `src/COMMAND1.C` `utilize()` -> `port_sound_verb()`
  (`port/rvip.c`: quaff, eat, zap, ring, scroll, wield, shoot, wear, drop,
  unlock, ignite). `port/fe_web.c` -> `Module.mag.sound(ev)` ->
  `RVIPSound.play(['<ev>'])` = `sound/<ev>.wav` (shared rvip-sound.js),
  only when the **Sound** button is on (off by default, kept in
  web-layout.json). **No samples in the cloud** (the Dubtrain pack is on
  the Mac): run `python3 web/sounds.py <Dubtrain dir>` -> `web/sound/*.wav`
  (first sample of the mapped Angband events; prints the mapping), then
  `sh web/build.sh` copies them to `dist/sound`. Missing files stay silent.
- **Docs:** `docs/web/magguide.py` (tagline, about, essentials, 50-key list,
  tips, new-player guide, saving, in the browser, history, credits, W1
  version line) feeds both `web/make-help.py` (dist/help.html) and
  `docs/web/build-docs.py` -> `docs/web/mag-docs.html` (standalone page in
  the Docs' shape). The Mac's `~/Desktop/Games/Roguelikes/Docs` was not
  reachable: move the dicts into a `GAMES` entry of `build-docs.py` +
  `guides.py` there, rebuild, and switch `make-help.py` to import them like
  the template does.
- **Tests:** `node web/tests/stage6.cjs` (sound off by default, events sent
  during play [stairs gold hurt eat mmiss ...], no sample requests while
  off, requests when on, setting kept over reload, Docs page renders with
  all sections); stage1-5 pass; ASan native random keys clean.
- **Stages 1-6 done in the cloud.** Next: stage 7 (publish, Mac): merge
  `rvip/LESSONS.md` into RVIP.md, `sounds.py` + build + `deploy.sh`,
  selection-page card, Docs entry.

### Stage 7 — publish (done, Mac, 2026-09-26)

- **Mac check** (browser pane, own tab, `web/dist` on a local no-store
  server): name prompt -> tiles, explore `Z` (fights by walking into
  monsters), `>` walks to the stairs and descends (seed 1), Enter menu ->
  Things you carry -> Inventory -> item menu -> Eat, windows (drop-down,
  hide/show, Reset), gutter drag + zoom + Sound kept over reload, `R` y ->
  overlay -> Play again -> "Welcome back", death -> tomb -> Hall of Heroes ->
  overlay (save deleted), Help button (7 sections, 50 keys), F1 screens,
  Sound off by default, samples fetched only after turning it on, no `.cfg`
  fetch, no console errors. **Fixed** (`RVIP: stage 1-6 fixes (Mac)`): the
  Inventory window painted items in use white while the pop-up list used the
  type colours -> game's O_COLORINV in both (`port/fe_web.c`).
- **Toolchain (Mac):** Homebrew emcc 6.0.10; `web/build.sh` copies
  `rvip-wm.js`/`rvip-sound.js` from `~/Games/rvip-tools/web` and runs
  `web/sounds.py` (Dubtrain pack in `~/Downloads`, samples not committed;
  `hurt`/`scroll`/`wear`/`ignite`/`stairs` mapped to `mon_hit`/`study`/
  `wield`/`breathe_fire`/`stairs_down`).
- **Docs:** entry `mag.html` in `~/Desktop/Games/Roguelikes/Docs`
  (`build-docs.py` GAMES, `guides.py` GUIDES + SAVING; other pages
  byte-identical after the rebuild). `web/make-help.py` reads it like
  LambdaRogue and writes `docs/web/mag-docs.html` with `--page`;
  `docs/web/magguide.py` and `docs/web/build-docs.py` are gone.
- **Repos:** this folder = public **memmaker/mag** (remote `memmaker`,
  branch `main`), history without `rvip/`, `web/shots/`, LESSONS.md; the
  cloud history is private **memmaker/mag-cloud** (deleted 2026-09-27) (GitHub only;
  branch `mac-fixes` there too). Upstream commit `cc36a63`; README with the compare view.
  `web/deploy.sh` checks against `memmaker/main`.
- **Live:** https://ruzzoli.de/roguelikes/mag/ (`sh web/build.sh && sh
  web/deploy.sh`). Card on https://ruzzoli.de/roguelikes/ (`mag.png`: a
  torch-lit room of 28 DawnLike monster sprites + player + stairs, 12x5 at
  2x = 384x160), count 35. Tree: `li.insp` under Rogue after Rogue Clone,
  "1988 (UNIX version 1985) · Michael J. Teixeira" (RogueBasin: 1988, "1985
  if you count the original UNIX version"; "between the complexity of the
  original rogue and that of hack"). og block in `web/index.html` by hand.
  No Info button / ✦ / title link yet (stage 8).

Next: stage 8 (shrine). Template `~/Games/roguelikes-index/shrine/lambdarogue.html`.
Material:
- Manual/help: none in the drop (36 source files only, `~/Downloads/mag_src`).
  The in-game help screens are rebuilt by `data/mkhelp.py` (from COMMAND.H
  and the game's tables); the web guide (`web/dist/help.html`, Docs
  `mag.html`, `docs/web/mag-docs.html`). Check the web (DOS Games Archive
  zip, SourceForge "mikesadvgame") for an original MAG.DOC.
- Licence: header of every source file ("General permission to copy or
  modify, but not for profit ... copyright notice is included"), copyright
  1986, 87, 88 Michael J. Teixeira.
- Changelog: none; version string `version()` in `src/COMMAND3.C` ("PC-1.1,
  Summer 1989"), file dates 1988-89.
- Walkthrough: none known; CRPG Addict Game 52 posts
  (crpgaddict.blogspot.com/2011/03/game-52-mikes-aventure-game-mag-1988.html),
  RogueBasin "Mike's Adventure Game". Cheats: wizard mode `^W` + password
  `frakola` (WIZARD.C), in our build too; `l<N>`/`s<N>` start arguments.
- Links to add: Info button on the card, ✦ in the tree entry, `#bar h1` link
  to `../shrine/mag.html`; shrine gets its own og block (image `roguelikes/mag.png`).

### Stage 8 — shrine (done, Mac, 2026-09-26)

- **Live:** https://ruzzoli.de/roguelikes/shrine/mag.html (+ `shrine/mag/manual.html`,
  `shrine/mag/license.txt`), roguelikes-index commit `8eda39e` (deployed from a fresh
  clone: the main checkout has another session's uncommitted `index.html`). Card Info
  button, tree ✦ and the game page's `#bar h1` link (`954cbfe`, rebuilt + deployed) live.
  og block by hand (image `roguelikes/mag.png`, card text as description).
- **Lineage checked:** the "1985 UNIX version" is from the DOS Games Archive, not
  RogueBasin (RogueBasin: 1988, influences Rogue + Hack, "between rogue and hack");
  CRPG Addict: 1988, $10, a small instruction guide in the release. Tree entry unchanged.
- **Manual:** none in the source drop; shrine manual = our rebuilt F1 screens
  (`data/mkhelp.py`, marked as rebuilt; symbol screen left out). The CRPG Addict's
  "small instruction guide" is probably in the DOS zips (DOS Games Archive
  `pc-mag-rogue.zip` 80 kB, SourceForge `Original_Dos_Version_from_1988.zip` 196 kB):
  not downloaded (needs the user's OK); if it turns up, add it as `shrine/mag/mag.doc`.
- **Missing:** no change log / version history (only PC-1.1, Summer 1989), no
  walkthrough (CRPG Addict posts linked instead). Cheats: wizard `^W` + `frakola`, in our build.

Next: stage 9 (graveyard + leaderboard).

### Stage 9 — graveyard + leaderboard (done, Mac, 2026-09-26)

- **Hook:** `port_run_end(kill)` (`port/rvip.c`) from MAIN.C `doquit()` (before `topten`)
  and `doexit()` (before "Press any key...", so the tomb/escape screen already reported;
  covers death via ATTACK.C's tomb and the level-1 escape in MISC.C `newlev()`).
  ev: `"quit"` -> quit; `"escaped"` -> win if `in_inv(SAPPHIRE)`, else quit; anything else ->
  death with `killer` = MAG's `killer` (monster `p_name`, "monster" when unseen, trap/item
  names like "bear trap", no articles). URL-encoded in C, `fe_beacon()` (EM_JS in
  `port/fe_web.c`) only calls `RvipWM.report` (fallback fetch). Native `fe_tty.c` prints it
  with `MAG_BEACON=1`.
- **Fields:** g=mag, ev, name (`u.u_name`, the game asks it), killer (deaths only), depth
  (`u.u_dlevel`; 0 after an escape), score (`getscore(YES)`, 0 after wizard mode, as the Hall
  of Heroes), turns (`u.u_moves`), lvl (`u.u_elevel`). None missing.
- **Killer art:** `roguelikes-index` `killers/make.py` `mag()`: 55 DawnLike sprites from
  `port/tiles-dawn.png` by `mon_tile` (MONSTER.H order), commit `7fb03b1`, deployed.
- **Verified live** (browser pane, fetch patched): quit -> `ev=quit` 204; death (wizard `>` x22
  to level 23, wizard off with a second `^W`, since wizard mode is immortal) -> `ev=death
  &killer=white%20dragon` 204, sent at the tomb; escape on level 1 without the Sapphire ->
  `ev=quit&depth=0` 204; outbox empty each time. `/mag` IndexedDB deleted afterwards.
- **Open:** the win branch (`in_inv(SAPPHIRE)`) not reached live (no wizard item creation);
  it is the escape path above with the Sapphire in the pack. Native `make -C port` fails on the
  Mac (Apple clang: incompatible function pointer errors in VARS.H; the Makefile was written for
  Linux gcc), web build fine.

### W0 rule 6 (text windows as HTML) — 2026-09-28

- `port/fe_web.c` sends Messages (history kept in C, repeats "(xN)", row colours), Status
  (page-0 rows 23-24), Inventory (row colour + tile), the pop-up (port boxes and full text
  pages at the original's place, box background from the game) and the PC screen (title,
  setup, Tiles: PC screen) as trimmed lines: `be_line`/`be_rows`, CGA colour runs
  `"\x05#fg[/#bg]"`…`"\x06"`, CP437 as UTF-8, one `set_cursor()`. The map is the only canvas;
  the VGA font sheets, measure() and the anim pixel compare are gone (`tile_anim[]` in
  `tiles.h`, from `mkdawn.py`). PC screen = `<pre>` in the IBM VGA web font at the Messages size.
- Blink attribute is not shown (steady text). Native build: `make -C port CC=clang` (macOS gcc = clang
  without the Makefile's clang flags fails on vars.h, pre-existing).

### Original DOS release (2026-09-29)

- `Original_Dos_Version_from_1988.zip` (SourceForge `mikesadvgame`, user-approved download):
  F1 help, tomb, title and Hall of Heroes files now original (see Stage 1), `data/mkhelp.py`
  gone. Verified in the browser pane: title, F1 pages 1-4 + 5 (wizard), F2 Return pages 6-7
  (symbols/letters line up with the text), tombstone. README.DOC ("Ver PC-1.1") is the
  shrine manual (`shrine/mag/readme.txt`).
