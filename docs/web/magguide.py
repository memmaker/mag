"""MAG's game guide: one source for the web Help page (web/make-help.py)
and the Docs page (docs/web/build-docs.py -> docs/web/mag-docs.html).

Stands in for the Mac's ~/Desktop/Games/Roguelikes/Docs entry (build-docs.py
GAMES entry + guides.py), which the cloud run cannot reach; the Mac side can
move these dicts into build-docs.py / guides.py as they are. Key lists are
the game's own commands (src/COMMAND.H, help screens data/mkhelp.py);
tips and guide are written from the source code (MAIN.C, MOVE.C, EVENT.C,
COMMAND*.C), in our own words."""
import html

esc = html.escape

VERSION = 'MAG version PC-1.1, Summer 1989'
UPSTREAM = 'cc36a63'          # the untouched source drop (first commit)
REPO = 'https://github.com/memmaker/mag'

TAGLINE = ("Mike's Adventure Game (1986-89): a DOS roguelike by Michael J. Teixeira, "
           "between Rogue and Hack in size. Find the Sudbury Sapphire, held by the "
           "Imperial Dragons deep below, and bring it back up.")

ABOUT = '''<p><strong>MAG</strong> is Michael J. Teixeira's <em>Dungeon Adventuring Game</em>,
written in C for the IBM PC and released as freeware with its source ("general permission
to copy or modify, but not for profit"). This is version PC-1.1 from the summer of 1989; the
game goes back to a UNIX version from about 1985. Like PC Rogue it draws the dungeon with the
PC's extended character set in colour: double-line walls, dotted floors, shaded corridors.</p>
<p>It sits between the original Rogue and Hack: 55 monsters (a to z, A to Z, four dragons),
151 kinds of objects in twelve classes (food, potions, scrolls, wands/staffs/rods, rings,
weapons, armour, shields, keys, miscellaneous things, gems, treasure), locked doors that need
the right key, dark rooms lit by wall torches, pools of water, traps, a secondary weapon for
bows and slings, and levels that stay as you left them.</p>'''

HISTORY = '''<p>MAG ("Mike's Adventure Game") is an original game inspired by Rogue, not a
Rogue source variant. Teixeira wrote a UNIX version around 1985 and the DOS version in
1986-88 (copyright 1986, 87, 88; files dated up to September 1989). It was spread as
freeware on bulletin boards and shareware disks, with the C source (Microsoft C, BIOS
video and keyboard calls). RogueBasin files it as an early Rogue clone; the CRPG Addict
played it in 1988's list.</p>
<p>The source drop this port starts from has no help or picture files (the game refuses to
start without them): the port rebuilds them from the game's own tables and the screen
positions the code writes to, and says so on the help screens.</p>'''

# the keys every player needs, grouped as the Help/Docs "essentials" boxes
ESSENTIALS = [
    ('Moving', [
        ('h j k l y u b n', 'Walk (arrows, keypad, Home/End/PgUp/PgDn too)'),
        ('Shift+dir', 'Run'),
        ('f dir', 'Find: move until something is reached'),
        ('Z', 'Explore (web port)'),
        ('> <', 'Stairs down / up; elsewhere: walk to the known stairs'),
        ('s', 'Search once'), ('.', 'Rest a turn'),
    ]),
    ('Things you carry', [
        ('i', 'Inventory (letter = use, Enter = all actions)'),
        ('g', 'Get'), ('d', 'Drop'), ('e', 'Eat'), ('q', 'Quaff'), ('r', 'Read'),
        ('z', 'Zap'), ('w', 'Wield'), ('t', 'Throw / fire'), ('W', 'Wear armour'),
        ('x', 'Swap to the secondary weapon'),
    ]),
    ('Information and the game', [
        ('Enter', 'Menu of every command (web port)'),
        ('F1', 'The help screens'), ('^R', 'Previous messages'),
        ('v', 'What is here'), ('F', 'Hunger'), ('o', 'Options'),
        ('R', 'Save and leave'), ('Q', 'Quit'),
    ]),
]

ALL_KEYS = [
    ('h j k l y u b n', 'Walk one step (also the arrow keys, Home, End, PgUp, PgDn, keypad)'),
    ('H J K L Y U B N', 'Run until something interesting happens'),
    ('f + direction', 'Find: move in a direction until you reach something'),
    ('.', 'Rest one turn'), (':', 'Rest several turns (F8)'),
    ('s', 'Search for secret doors and traps'), (';', 'Search several turns (F7)'),
    ('> or +', 'Go down a staircase'), ('< or -', 'Go up a staircase'),
    ('Z', 'Explore: walk to what you have not seen (web port)'),
    ('> <', 'Not on stairs: walk to the nearest known staircase (web port)'),
    ('Enter', 'Menu of every command (web port)'),
    ('i or F3', 'Inventory'), ('g', 'Get what is lying here'), ('d', 'Drop something'),
    ('e', 'Eat'), ('q', 'Quaff a potion'), ('r', 'Read a scroll'),
    ('z', 'Zap a wand, staff or rod'), ('w', 'Wield a weapon'),
    ('t', 'Throw (fire arrows with a bow)'), ('W', 'Wear or take off armour'),
    ('S', 'Strap on or unstrap a shield'), ('p', 'Put on or remove a ring'),
    ('x', 'Swap in the secondary weapon'), ('X', 'Choose the secondary weapon'),
    ('^U', 'Use a miscellaneous item'), ('I', 'Ignite a torch'), ('E', 'Extinguish a torch'),
    ('^K', 'Unlock a door with a key'), ('c', 'Call (name) a kind of object'),
    ('a or F5', 'Objects you know'), ('C or P', 'How full your pack is'),
    ('v or F6', 'View: describe what you can see here'),
    ('^H or F2', 'What is which symbol on the screen'),
    ('F', 'How your stomach feels'), ('T', 'Your title (rank)'),
    ('A', 'How long you have been down here'), ('^R', 'Show the previous messages again'),
    ('^L', 'Redraw the screen'), ('o or F4', 'Options'), ('V', "MAG's version"),
    ('R', 'Remember (save) the game and leave'), ('Q or ^C', 'Quit the game'),
    ('ESC', 'Stop what you are doing'), ('F1 or DEL', 'The help screens'),
    ('{', 'Start recording a macro'), ('}', 'End the macro'), ('^E or F9', 'Play the macro back'),
    ('number + command', 'Repeat a command (9s searches nine times)'),
]

KEY_HINTS = [
    ('F1', 'In-game help screens'),
    ('Z', 'Auto-explore: walk to what you have not seen yet'),
    ('Enter', 'Menu of all commands'),
    ('i', 'Inventory with a cursor: letter = main action, Enter = all actions'),
    ('> <', 'Take the stairs, or walk to the nearest known staircase'),
    ('R', 'Save and leave (the game also autosaves)'),
    ('F12', 'Tiles / text'),
]

TIPS = '''<ul>
<li><strong>Light.</strong> Rooms are dark until a wall torch burns. Stand next to one and
press <kbd>I</kbd> to light it with your tinderbox; a lit room shows everything in it.</li>
<li><strong>Keys and doors.</strong> Some doors carry a copper, silver or golden lock. Carry
the matching key (<kbd>,</kbd> items) and unlock with <kbd>^K</kbd>; a skeleton key opens
any lock. Explore walks around locked doors.</li>
<li><strong>Bows.</strong> You start with a long bow as the secondary weapon. Press
<kbd>x</kbd> to swap it in, fire arrows with <kbd>t</kbd>, and <kbd>x</kbd> back to the dagger
before a monster reaches you.</li>
<li><strong>Hunger.</strong> A <code>*</code> in front of a message means hungry, a
<code>!</code> means fainting: eat (<kbd>e</kbd>). <kbd>F</kbd> tells you how full you are.</li>
<li><strong>Unknown items.</strong> Potions, scrolls, wands and rings have random names each
game. Use <kbd>c</kbd> to call a kind something you will remember; <kbd>a</kbd> lists what
you know.</li>
<li><strong>Water.</strong> Some monsters hate water: a pool (<code>÷</code>) between you and
them can hold them off.</li>
<li><strong>Search.</strong> A dead end usually hides a secret door: <kbd>;</kbd> searches
several turns.</li>
<li><strong>Levels stay.</strong> Levels you leave are remembered: you can go back up to
rest or fetch what you left.</li>
</ul>'''

GUIDE = [
    ('Your goal', '''<p>Somewhere below lies the <strong>Sudbury Sapphire</strong>, guarded by
the Imperial Dragons. Take it and climb back out of the dungeon (the up staircase on the
first level leads outside). Points come from experience, treasure and how deep you went;
the Hall of Heroes keeps the best games.</p>'''),
    ('The screen', '''<p>The top line shows messages, the bottom line your level, hit points,
strength, armour class, experience level and score. You are the smiling face; letters are
monsters (lowercase are the weaker ones), symbols are objects: <code>%</code> food,
<code>!</code> potions, <code>?</code> scrolls, <code>/</code> wands, <code>=</code> rings,
<code>)</code> weapons, <code>]</code> armour, <code>[</code> shields, <code>,</code> keys,
<code>"</code> miscellaneous, <code>*</code> gems, <code>$</code> treasure.
<kbd>^H</kbd> explains every symbol.</p>'''),
    ('Your first levels', '''<p>You start with food, a dagger, a long bow with arrows,
studded leather armour, a shield and a tinderbox. Explore with <kbd>Z</kbd>, pick things up
by walking over them (auto-get) or with <kbd>g</kbd>, and fight by walking into monsters.
Jackals, giant rats and kobolds come in packs: fight them in a corridor so only one reaches
you at a time. Go down with <kbd>&gt;</kbd> once the level is explored.</p>'''),
    ('Getting stronger', '''<p>Experience levels raise your hit points. Read unknown scrolls
when nothing dangerous is near (enchant armour and weapon are common); quaff unknown potions
at full health. Wear the best armour you find and keep a spare food ration. Rings can be
cursed: a ring you can't remove is a reason to hope for a remove curse scroll.</p>'''),
]

SAVING = '''<ul>
<li><strong>Saving is automatic.</strong> The game is stored in this browser (IndexedDB)
between your commands (at most every two seconds). Reloading the page continues from there.</li>
<li><kbd>R</kbd> saves and ends the session, as in the original; press <em>Play again</em>
(or reload) to continue.</li>
<li>When your character dies or you quit with <kbd>Q</kbd>, the save is deleted: death is
final. The Hall of Heroes stays.</li>
<li>Each browser keeps <strong>one game</strong>. <em>New game</em> deletes it and starts over.</li>
<li><em>Export save</em> downloads the game (all its files, as <code>mag-save.json</code>);
<em>Import save</em> loads one. A save only fits the version of the page that wrote it: after
an update of this page an old save is dropped and a new game starts.</li>
<li>Window layout, zoom, tiles/text, auto_more and the game's options are stored in the same
browser storage.</li>
<li>Private/incognito windows and "clear site data" delete the stored game.</li>
</ul>'''

WEB = '''<ul>
<li><strong>Tiles / Text</strong> in the top bar (or <kbd>F12</kbd>) switch between the tiled
windows and the original IBM PC text screen: VGA font, CP437 symbols, CGA colours.</li>
<li><strong>Tiles windows:</strong> the map on top; Messages (with history) and Status below;
Inventory and Visible (monsters and objects in view) on the right. Menus and the game's own
full screens pop up over the map.</li>
<li><strong>Resize windows</strong> by dragging the gaps between them; <em>Windows</em> hides,
shows and resets them. Hover over a text window's title for <em>A−</em> / <em>A+</em>.</li>
<li><strong>Zoom:</strong> <em>Zoom −</em> / <em>Zoom +</em> change the map tiles; a map bigger
than its window follows you.</li>
<li><strong>auto_more</strong> (on by default) skips the top-line =-More-= prompts; every
message stays in the Messages window.</li>
<li><strong>Mouse:</strong> click an entry of a menu or list to choose it; right-click is Escape.</li>
<li>Browsers keep a few shortcuts for themselves (<kbd>Ctrl+W</kbd>, <kbd>Ctrl+T</kbd>,
<kbd>Ctrl+N</kbd>), so those never reach the game.</li>
<li>If the game ever crashes, a message appears at the top; reload the page to continue from
the last autosave.</li>
</ul>'''

CREDITS = '''<ul>
<li>MAG by <strong>Michael J. Teixeira</strong>, copyright 1986, 87, 88: "General permission
to copy or modify, but not for profit, is hereby granted, provided that the above copyright
notice is included."</li>
<li>Map sprites: <strong>DawnLike</strong> by DragonDePlatino with DawnBringer's palette
(CC BY 4.0); sprite names from DawnLikeAtlas by Tommy Ettinger (CC BY 4.0).</li>
<li>Text mode font: IBM VGA 9x16 from The Oldschool PC Font Resource by VileR (CC BY-SA 4.0).</li>
</ul>'''


def kbd(k):
    return ' '.join('<kbd>%s</kbd>' % esc(p) for p in k.split(' ') if p not in ('or', '+')) \
        if ' or ' not in k and ' + ' not in k else \
        ('<span class="or">or</span>' if ' or ' in k else '<span class="plus">+</span>').join(
            kbd(p) for p in (k.split(' or ') if ' or ' in k else k.split(' + ')))


def dl(items):
    return '<dl>' + ''.join('<dt>%s</dt><dd>%s</dd>' % (kbd(k), esc(d)) for k, d in items) + '</dl>'


def about_version():
    return ('<ul><li>Based on <strong>%s</strong> (Michael J. Teixeira), from a source drop '
            'without history.</li>'
            '<li>Original source: <a href="%s/tree/%s" target="_blank" rel="noopener">untouched '
            'import, commit %s</a>.</li>'
            '<li>Our changes (DOS layer replaced by an 80x25 BIOS screen emulation, explore, '
            'command menu, inventory, tiles, web build): '
            '<a href="%s/compare/%s...main" target="_blank" rel="noopener">memmaker/mag</a>.</li></ul>'
            % (VERSION, REPO, UPSTREAM, UPSTREAM, REPO, UPSTREAM))
