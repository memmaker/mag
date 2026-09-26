#!/usr/bin/env python3
"""Write MAG's help/ and pics/ screens (RVIP port).

The source drop has no data files, but MAIN.C refuses to start without
help\\help.1 and pics\\header. These are reconstructed from the game's own
tables (COMMAND.H, MONSTER.H, VARS.H) and the coordinates the code draws
into them (COMMAND2.C helpsym, ATTACK.C tomb, HEROES.C topten, MAIN.C
setupgame). CP437, at most 22 lines of < 80 columns (prfile())."""
import os, re
HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, '..', 'src')

def w(p, lines):
    assert len(lines) <= 22, p
    for l in lines:
        assert len(l) < 80, (p, l)
    with open(os.path.join(HERE, p), 'wb') as f:
        f.write(('\n'.join(lines) + '\n').encode('cp437'))

def grid(rows):
    return [[' '] * 79 for _ in range(rows)]

def put(g, r, c, s):
    for i, ch in enumerate(s):
        g[r][c + i] = ch

def text(g):
    return [''.join(l).rstrip() for l in g]

os.makedirs(os.path.join(HERE, 'help'), exist_ok=True)
os.makedirs(os.path.join(HERE, 'pics'), exist_ok=True)

w('help/help.1', [
" MAG commands (1 of 4): moving           (help screens rebuilt for the port)",
"",
"   y  k  u      Walk one step. Arrow keys, Home, End, PgUp and PgDn",
"    \\ | /       (and the numeric keypad) walk too.",
"   h -@- l",
"    / | \\       Shift + direction (Y K U H L B J N) runs until something",
"   b  j  n      interesting happens.",
"",
"   f <dir>      Find: move in a direction until you reach something.",
"   .            Rest one turn.                 :  Rest several turns (F8).",
"   s            Search for secret doors and traps once.",
"   ;            Search several turns (F7).",
"   > or +       Go down a staircase.",
"   < or -       Go up a staircase.",
"   x            Explore (web port): walk to what you haven't seen yet, one",
"                step a turn; stops for monsters, messages and any key.",
"",
"   A number before a command repeats it:  9s  searches nine times.",
"   Walking into a monster attacks it. Locked doors need a key: ^K (Ctrl-K).",
])
w('help/help.2', [
" MAG commands (2 of 4): things you carry",
"",
"   i  (F3)  Inventory: everything you carry.",
"   g        Get what is lying here.        d   Drop something.",
"   e        Eat.                           q   Quaff (drink) a potion.",
"   r        Read a scroll.                 z   Zap a wand, staff or rod.",
"   w        Wield a weapon.                t   Throw (fire arrows with a bow).",
"   W        Wear or take off armor.        S   Strap on or unstrap a shield.",
"   p        Put on or remove a ring.       x   Swap in the secondary weapon",
"   X        Choose the secondary weapon.       (web port: explore is x).",
"   ^U       Use a miscellaneous item.      I   Ignite a torch.",
"   E        Extinguish a torch.            ^K  Unlock a door with a key.",
"   c        Call (name) a kind of object.  a   Objects you know (F5).",
"   C or P   How full your pack is.",
"",
"   Symbols list just one kind:  % food  ! potions  ? scrolls  / wands",
"   = rings  ) weapons  ] armor  [ shields  , keys  \" misc  * gems  $ money",
"",
"   At a \"What do you want to ...\" question press SPACE for a list of what",
"   fits, a symbol for one kind, the item's letter to choose it, ESC to stop.",
])
w('help/help.3', [
" MAG commands (3 of 4): information and the game",
"",
"   v  (F6)  View: describe what you can see here.",
"   ^H (F2)  What is which symbol on the screen (RETURN for all of them).",
"   F        How your stomach feels.         T   Your title (rank).",
"   A        How long you have been down here.",
"   ^R       Show the previous messages again (repeat for older ones).",
"   ^L       Redraw the screen.",
"   o  (F4)  Options: follow corridors, bell, auto-get, special messages,",
"            coloured inventories, save directory, your name.",
"   V        MAG's version.",
"   R        Remember (save) the game and leave. The next start continues it.",
"   Q  ^C    Quit the game.",
"   ESC      Stop what you are doing.",
"   F1  DEL  This help.",
"",
"   =-More-= waits for SPACE or RETURN; ESC stops a repeated command and",
"   P skips the rest of the Mores of this turn.",
])
w('help/help.4', [
" MAG commands (4 of 4): macros and hints",
"",
"   {        Start recording a macro (up to 20 keys).",
"   }        End the macro.",
"   ^E (F9)  Play the macro back.",
"",
"   Hints",
"   - Your goal: the Sudbury Sapphire, deep below, guarded by the Imperial",
"     Dragons. Bring it back up to the surface.",
"   - Rooms are dark until a torch burns: light wall torches with your",
"     tinderbox (I), carry oil flasks.",
"   - Eat before you faint: F tells you how hungry you are.",
"   - Unknown potions, scrolls, wands and rings get random names; use c to",
"     call them something you remember.",
"   - Levels you left are remembered: you can go back up.",
"",
"   The web port adds: Enter for a menu of every command, and < / > walk",
"   to the nearest known staircase when you are not standing on one.",
])
w('help/help.5', [
" MAG wizard commands (wizard mode only)",
"",
"   ^W     Enter or leave wizard mode (asks for the password).",
"   ^D  D  Detect objects or monsters.     ^G  Genocide a monster.",
"   G      Genocided monsters.             ^I  Identify an item.",
"   ^M     Monster statistics.             ^T  Light the room.",
"   ^X     Raise experience level.         m   Advance to experience level 18.",
"   #      Map the level.                  &   Experience statistics.",
"   @      Teleport.                       ^   Detect traps.",
"   M      Make a monster.                 O   Make an object.",
"   |      Make a wall.",
"",
"   A game that used wizard mode never enters the Hall of Heroes.",
])

# help.6: helpsym() draws the symbols: walls at (2,2) (2,4), marble (3,2),
# water (4,2), trap (5,2), floor (6,2), stairs (7,2) (8,2), you (10,2);
# the 12 item symbols (VARS.H CITEMS) at rows 2-7, columns 28 and 54
g = grid(11)
put(g, 0, 1, "What is which symbol")
for r, s in [(2, "walls"), (3, "a marble wall"), (4, "water"), (5, "a visible trap"),
             (6, "floor and corridors"), (7, "an up staircase"), (8, "a down staircase"), (10, "you")]:
    put(g, r, 6 if r == 2 else 4, s)
names = ["food", "a potion", "a scroll", "a wand, staff or rod", "a ring", "a weapon",
         "armor", "a shield", "a key", "a miscellaneous item", "a gemstone", "money"]
for i, s in enumerate(names):
    put(g, 2 + i % 6, 30 + i // 6 * 26, s)
w('help/help.6', text(g))

# help.7: the monsters, drawn at (2 + i/3, i%3 * 26) with their letters
src = open(os.path.join(SRC, 'MONSTER.H'), encoding='latin-1').read()
mons = re.findall(r'^"([^"]+)",\s*\'(.)\'', src, re.M)
assert len(mons) == 55, len(mons)
g = grid(21)
put(g, 0, 1, "The monsters of MAG")
for i, (name, _) in enumerate(mons):
    put(g, 2 + i // 3, i % 3 * 26 + 2, name)
w('help/help.7', text(g))

# pics/header: the title page (setupgame writes MAG at (6,36), the subtitle
# at (8,25), the author at (10,27), the name prompt at (13,8), (15,8))
g = grid(22)
H, V = '═', '║'
for c in range(2, 77):
    put(g, 1, c, H); put(g, 20, c, H)
for r in range(2, 20):
    put(g, r, 1, V); put(g, r, 77, V)
put(g, 1, 1, '╔'); put(g, 1, 77, '╗'); put(g, 20, 1, '╚'); put(g, 20, 77, '╝')
put(g, 4, 30, '░▒▓█ ■ █▓▒░')
put(g, 18, 22, "Seek the Sudbury Sapphire ■ Beware the dragons")
w('pics/header', text(g))

# pics/tomb: the headstone (month (9,36), year (10,36), level (12,7), name,
# title, killer and score centred on column 23, rows 11-17)
g = grid(22)
put(g, 2, 13, "_____________________")
put(g, 3, 11, "/                     \\")
put(g, 4, 10, "/        R.I.P.         \\")
for r in range(5, 19):
    put(g, r, 9, "|"); put(g, r, 35, "|")
put(g, 19, 3, "_____/" + "_" * 25 + "\\_____")
put(g, 11, 2, "Level")
put(g, 20, 3, "≈ " * 18)
w('pics/tomb', text(g))

# pics/herobox: the Hall of Heroes frame (title (3,28), entries rows 5-19)
g = grid(22)
for c in range(6, 73):
    put(g, 1, c, H); put(g, 20, c, H)
for r in range(2, 20):
    put(g, r, 5, V); put(g, r, 73, V)
put(g, 1, 5, '╔'); put(g, 1, 73, '╗'); put(g, 20, 5, '╚'); put(g, 20, 73, '╝')
w('pics/herobox', text(g))
