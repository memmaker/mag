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

(nothing yet — start with stage 1)
