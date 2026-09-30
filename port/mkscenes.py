#!/usr/bin/env python3
"""Preview scenes for the remapper (design time only, MAG never reads them).
Reads the ids from port/mag-dawnlike.rec, writes port/mag-scenes.rec:
  dungeon   a level as MAG builds them: rooms, corridors, doors, stairs,
            monsters and objects (all categories)
  world     every terrain id: all 16 autotile borders of room floor and
            corridor, walls, torches, doors, stairs, pool, marble, all traps
  bestiary  every monster, treasury  every object
Each scene asserts it shows every id of its category. Run after mkdawn.py:
  python3 port/mkscenes.py"""
import os, random, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.environ.get('RVIP_TILESETS', os.path.expanduser('~/Games/rvip-tools/tilesets')))
import dawnlike_rec
IDS = {c: [i for i, _ in v] for c, v in dawnlike_rec.read_ids(os.path.join(HERE, 'mag-dawnlike.rec')).items()}

FLOOR, CORR, DOOR = 1, 2, 3   # floor kinds as tiles.c's floor_kind() sees them


class Scene:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.cell = [[None] * w for _ in range(h)]   # 'category/id' or None
        self.kind = [[0] * w for _ in range(h)]

    def free(self, x, y):
        return self.kind[y][x] == FLOOR and self.cell[y][x] is None

    def room(self, x0, y0, x1, y1, torches=True):
        """interior x0..x1, y0..y1; walls around it"""
        for y in range(y0 - 1, y1 + 2):
            for x in range(x0 - 1, x1 + 2):
                top, bot, lft, rgt = y == y0 - 1, y == y1 + 1, x == x0 - 1, x == x1 + 1
                w = ('ulwall' if top and lft else 'urwall' if top and rgt else 'llwall' if bot and lft
                     else 'lrwall' if bot and rgt else 'hwall' if top or bot else 'vwall' if lft or rgt else None)
                if w:
                    if torches and (top or bot) and not (lft or rgt) and (x - x0) % 6 == 3:
                        w = 'ttorch' if top else 'btorch'
                    elif torches and (lft or rgt) and not (top or bot) and (y - y0) % 4 == 2:
                        w = 'ltorch' if lft else 'rtorch'
                    self.cell[y][x], self.kind[y][x] = 'world/' + w, 0
                else:
                    self.cell[y][x], self.kind[y][x] = None, FLOOR

    def corridor(self, pts, doors=('door', 'locked')):
        """straight segments through pts; a wall it crosses becomes a door"""
        n = 0
        for (ax, ay), (bx, by) in zip(pts, pts[1:]):
            dx, dy = (bx > ax) - (bx < ax), (by > ay) - (by < ay)
            x, y = ax, ay
            while True:
                c = self.cell[y][x]
                if c and c.startswith('world/') and ('wall' in c or 'torch' in c):
                    self.cell[y][x], self.kind[y][x] = 'world/' + doors[n % len(doors)], DOOR
                    n += 1
                elif self.kind[y][x] == 0:
                    self.kind[y][x] = CORR
                if (x, y) == (bx, by):
                    break
                x, y = x + dx, y + dy

    def put(self, key, x, y):
        self.cell[y][x] = key
        if key.startswith('world/'):    # stairs, traps and pools count as room floor
            self.kind[y][x] = FLOOR

    def scatter(self, keys, x0, y0, x1, y1, step=2):
        """keys on free room floor, every step-th cell (legible, floor between them)"""
        spots = [(x, y) for y in range(y0, y1 + 1, step) for x in range(x0, x1 + 1, step) if self.free(x, y)]
        assert len(spots) >= len(keys), (len(spots), len(keys))
        for k, (x, y) in zip(keys, spots):
            self.put(k, x, y)

    def autotile(self):
        """as tiles.c: plain floor and corridor cells get their autotile border mask (n8 s4 w2 e1); ground is
        what tile_raw() draws under a cell: a creature or object stands on its cell's floor or corridor, a
        door, stairs or trap on the room floor; walls, torches and pools stand on nothing"""
        def k(x, y):
            return self.kind[y][x] if 0 <= x < self.w and 0 <= y < self.h else 0

        def floor(x, y, t):
            m = 0
            for b, (dx, dy) in zip((8, 4, 2, 1), ((0, -1), (0, 1), (-1, 0), (1, 0))):
                n = k(x + dx, y + dy)
                if n != t and n != DOOR:
                    m |= b
            return 'world/%s_%d' % ('room_floor' if t == FLOOR else 'corridor', m)
        self.ground = [[None] * self.w for _ in range(self.h)]
        for y in range(self.h):
            for x in range(self.w):
                t, c = self.kind[y][x], self.cell[y][x]
                if c is None and t in (FLOOR, CORR):
                    self.cell[y][x] = floor(x, y, t)
                elif c and not c.startswith('world/') and t in (FLOOR, CORR):
                    self.ground[y][x] = floor(x, y, t)
                elif c and c.split('/')[1] in ('door', 'locked', 'upstair', 'dnstair') or (c or '').startswith('world/trap'):
                    self.ground[y][x] = floor(x, y, FLOOR)

    def used(self):
        return {c for grid in (self.cell, self.ground) for row in grid for c in row if c}


# legend characters: mnemonics first, then printable ASCII, then Latin-1 and on
MNEMONIC = {'world/room_floor_0': '.', 'world/corridor_12': '#', 'world/corridor_3': '"',
            'world/hwall': '-', 'world/vwall': '|', 'world/door': '+', 'world/locked': '=',
            'world/upstair': '<', 'world/dnstair': '>', 'world/pool': '~', 'world/marble': ':',
            'monster/player': '@', 'world/trap': '^'}


def rec(sc, sid, name, cats):
    return dawnlike_rec.scene_lines(sid, name, cats, sc.cell, sc.ground, MNEMONIC)


def check(sc, cat):
    missing = [i for i in IDS[cat] if cat + '/' + i not in sc.used()]
    assert not missing, (cat, missing)


def dungeon():
    rnd = random.Random(7)
    s = Scene(66, 24)
    rooms = [(2, 2, 14, 7), (22, 1, 34, 5), (44, 2, 62, 8), (4, 13, 16, 20), (26, 11, 40, 20), (48, 14, 62, 21)]
    for r in rooms:
        s.room(*r)
    for pts in ([(15, 4), (21, 4)], [(35, 3), (43, 3)], [(8, 8), (8, 12)], [(28, 6), (28, 10)],
                [(17, 16), (25, 16)], [(52, 9), (52, 13)], [(41, 18), (47, 18)], [(38, 10), (38, 8), (43, 8)]):
        s.corridor(pts)
    s.put('world/upstair', 4, 3)
    s.put('world/dnstair', 60, 20)
    s.put('world/pool', 30, 15)
    s.put('world/pool', 31, 15)
    for t in rnd.sample([i for i in IDS['world'] if i.startswith('trap')], 3):
        s.put('world/' + t, *rnd.choice([(x, y) for y in range(11, 21) for x in range(26, 41) if s.free(x, y)]))
    s.put('monster/player', 6, 4)
    mons = rnd.sample([i for i in IDS['monster'] if i != 'player'], 14)
    objs = rnd.sample(IDS['object'], 22)
    things = ['monster/' + m for m in mons] + ['object/' + o for o in objs]
    rnd.shuffle(things)
    spots = [(x, y) for (x0, y0, x1, y1) in rooms for y in range(y0, y1 + 1) for x in range(x0, x1 + 1) if s.free(x, y)]
    for k, xy in zip(things, rnd.sample(spots, len(things))):
        s.put(k, *xy)
    s.autotile()
    return rec(s, 'dungeon', 'Dungeon level', ['world', 'monster', 'object'])


def world():
    s = Scene(60, 22)
    s.room(2, 2, 12, 7)                      # borders none, n, s, w, e and the four corners
    s.room(16, 2, 26, 2, torches=False)     # one row: ns, nsw, nse
    s.room(30, 2, 30, 7, torches=False)     # one column: we, nwe, swe
    s.room(34, 2, 34, 2, torches=False)     # one cell: nswe
    s.room(38, 2, 56, 7)
    for i, t in enumerate(sorted(i for i in IDS['world'] if i.startswith('trap'))):
        s.put('world/' + t, 38 + 2 * i, 3)
    for i, k in enumerate(['upstair', 'dnstair', 'pool', 'pool', 'pool']):
        s.put('world/' + k, 40 + 2 * i, 6)
    s.corridor([(8, 8), (8, 10)])           # a door out of the big room
    s.corridor([(50, 8), (50, 10)], doors=('locked',))
    # a corridor lattice: turns, tees, a crossing and straights
    for y in (11, 15, 19):
        s.corridor([(4, y), (20, y)], doors=('door',))
    for x in (4, 12, 20):
        s.corridor([(x, 11), (x, 19)], doors=('door',))
    s.corridor([(26, 12), (32, 12)])        # dead ends west and east
    s.corridor([(36, 11), (36, 17)])        # dead ends north and south
    s.corridor([(40, 14), (40, 14)])        # a lone corridor cell
    for y in range(11, 20):
        for x in range(44, 56):
            s.cell[y][x] = 'world/marble'
    s.room(48, 14, 51, 16)                  # a room in the rock
    s.autotile()
    check(s, 'world')
    return rec(s, 'world', 'All terrain', ['world'])


def gallery(cat, sid, name, w):
    keys = [cat + '/' + i for i in IDS[cat]]
    h = 2 * -(-len(keys) // ((w - 3) // 2 + 1)) + 1   # rows for every key, one on every other cell of 3..w
    s = Scene(w + 4, h + 4)
    s.room(2, 2, w + 1, h + 1)
    s.scatter(keys, 3, 3, w, h)
    s.autotile()
    check(s, cat)
    return rec(s, sid, name, [cat])


def card():
    """the roguelikes index card (img/mag.png, 12x5 tiles at 2x): 30 monsters in a torch-lit room,
    drawn with the current mapping"""
    s = Scene(12, 5)
    s.room(1, 1, 10, 3)
    mons = [m for m in IDS['monster'] if m != 'player']
    s.scatter(['monster/' + mons[i * len(mons) // 30] for i in range(30)], 1, 1, 10, 3, step=1)
    s.autotile()
    return dawnlike_rec.render(os.path.join(HERE, 'mag-dawnlike.rec'), s.cell, s.ground)


INDEX = os.path.expanduser(os.environ.get('RVIP_INDEX', '~/Games/roguelikes-index'))
if os.path.isdir(os.path.join(INDEX, 'img')):
    card().save(os.path.join(INDEX, 'img', 'mag.png'))
    print('card: ' + os.path.join(INDEX, 'img', 'mag.png'))

out = ['# Remapper preview scenes for MAG (port/mkscenes.py writes them; design time only)', '', '%rec: Scene', '']
for r in (dungeon(), world(), gallery('monster', 'bestiary', 'Every monster', 29),
          gallery('object', 'treasury', 'Every object', 45)):
    out += r + ['']
open(os.path.join(HERE, 'mag-scenes.rec'), 'w').write('\n'.join(out))
print('mag-scenes.rec: 4 scenes; world, monster and object ids all shown')
