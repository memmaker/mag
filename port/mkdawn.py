#!/usr/bin/env python3
"""MAG's tile set: DawnLike (DragonDePlatino, CC BY 4.0; palette by
DawnBringer), sprites looked up by name in DawnLikeAtlas (Tommy Ettinger,
github.com/tommyettinger/DawnLikeAtlas, renamed/<name>_0.png, 16x16).

One set only (RVIP R4): every slot port/tiles.c can return comes from
DawnLike; names MAG shares with the atlas map directly, the rest take a
stand-in from the same set (hand table below). Writes
  port/tiles-dawn.png   32 sprites per row, 16x16
  port/tiles.h          slot tables for tiles.c
and prints the coverage numbers. Run: python3 port/mkdawn.py [tilesets dir]
(default $RVIP_TILESETS or ~/Games/rvip-tools/tilesets). Also writes
  port/tiles-dawn-1.png  DawnLike's second animation frame, same slots"""
import os, re, sys
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
# sprites by name: dawnlike_names.tsv (name, frame, sheet, col, row) next to
# the original DawnLike sheets, from rvip-tools/tilesets (dawnlike_index.py
# made it from DawnLikeAtlas); override the folder with $RVIP_TILESETS
TS = sys.argv[1] if len(sys.argv) > 1 else os.environ.get(
    'RVIP_TILESETS', os.path.expanduser('~/Games/rvip-tools/tilesets'))
POS = {}
for line in open(os.path.join(TS, 'dawnlike_names.tsv')).read().splitlines()[1:]:
    n, fr, sheet, c, r = line.split('\t')
    if fr == '0':
        POS.setdefault(n, (sheet, int(c), int(r)))
# atlas names from sheets dawnlike_names.tsv skips (Commissions/)
POS.setdefault('warrior s', ('Commissions/Warrior.png', 0, 0))
NAMES = set(POS)
_sheets = {}
def crop(sheet, c, r):
    if sheet not in _sheets:
        _sheets[sheet] = Image.open(os.path.join(TS, 'DawnLike', sheet)).convert('RGBA')
    return _sheets[sheet].crop((c * 16, r * 16, c * 16 + 16, r * 16 + 16))
def sprite(name, frame=0):
    """frame 1: the same cell of DawnLike's <sheet>1.png where it has one"""
    sheet, c, r = POS[name]
    if frame and sheet.endswith('0.png') and os.path.exists(os.path.join(TS, 'DawnLike', sheet[:-5] + '1.png')):
        sheet = sheet[:-5] + '1.png'
    return crop(sheet, c, r)


def src(path):
    return open(os.path.join(HERE, '..', 'src', path), 'rb').read().decode('latin-1').replace('\r', '')


# ---- the game's own lists ----------------------------------------------
MON = re.findall(r'^"([^"]+)",\s*\'(.)\'', src('MONSTER.H'), re.M)
obj_h = src('OBJECT.H')
pobj = obj_h[obj_h.index('pobj['):obj_h.index('};', obj_h.index('pobj['))]
OBJ = re.findall(r'"([^"]+)",', pobj)
def fakes(name):
    s = obj_h[obj_h.index(name + '[]'):]
    return re.findall(r'"([^"]+)"', s[:s.index('};')])
FPOT, FWAND, FRING, FSCROLL = fakes('fpotions'), fakes('fwands'), fakes('frings'), fakes('fscrolls')
assert len(MON) == 55 and len(OBJ) == 151, (len(MON), len(OBJ))

# ---- stand-ins (DawnLike names) for everything without an exact name ----
MON_HAND = {
    'axe beak': 'terror bird', 'babbler': 'gremlin', 'crystalizer': 'crystal golem',
    'fire drake': 'firedrake', 'dark elf': 'elf', 'fairy': 'forest fairy', 'gorgon': 'bull',
    'harpy': 'giant bat', 'invisixplode': 'gas spore', 'lizard man': 'lizard',
    'norker': 'hobgoblin', 'troglodyte': 'troll', 'unicorn': 'white unicorn',
    'vine crawler': 'small rotting plant', 'xuthus': 'killer beetle', 'yeth hound': 'hell hound',
    'zephyr': 'air elemental', 'apparition': 'ghost', 'banshee': 'wraith', 'coatyl': 'cobra',
    'red dragon': 'searwyrm', 'white dragon': 'icewyrm', 'blue dragon': 'storrmwyrm',
    'Imperial Dragon': 'kingwyrm', 'efreeti': 'fire elemental', 'frost fiend': 'ice devil',
    'golem': 'clay golem', 'inflicter': 'mind flayer', 'jaculus': 'garter snake',
    'kleptum': 'leprechaun', 'mystic': 'priest', 'naga': 'red naga', 'opinicus': 'griffon',
    'polymorpher': 'chameleon', 'queen snake': 'snake', 'reaper': 'death',
    'sulfuric cloud': 'fog cloud', 'transmigrant': 'shade', 'umbathri': 'umber hulk',
    'volcano demon': 'lava demon', 'will-o-wisp': 'wisp', 'zabloron': 'jabberwock',
}
OBJ_HAND = {
    'beef jerky': 'marbled cut of meat', 'moldy cheese wedge': 'c ration',
    'stale bread roll': 'loaf of bread', 'dried fruit': 'apple', 'mushroom': 'spotted mushroom',
    'long bow': 'longbow', 'sling bullet': 'small bullets', 'hand axe': 'axe',
    'two-handed sword': 'two handed sword',
    'banded': 'banded mail', 'chain': 'chain shirt', 'coconut': 'hawaiian shirt',
    'leather': 'lacquered armor', 'padded': 'peasant robes', 'plate': 'iron armor',
    'ring': 'mail', 'scale': 'scale armor', 'splint': 'bronze armor', 'studded leather': 'brass armor',
    'buckler': 'small shield', 'medium shield': 'elven shield', 'great shield': 'large shield',
    'mithril shield': 'polished silver shield',
    'copper': 'key', 'silver': 'key', 'golden': 'key', 'skeleton': 'key',
    'The Sudbury Saphire': 'gleaming blue gem', 'compass of translocation': 'crystal ball',
    'cross': 'magic skull', 'oil flask': 'bottle', 'tinderbox': 'lamp',
    'vial of ethereal oil': 'clear potion', 'chime of opening': 'bell',
    'sparkling': 'gleaming white gem', 'crimson': 'gleaming red gem', 'purple': 'gleaming violet gem',
    'turquoise': 'gleaming blue gem', 'opaque': 'dull gray gem', 'green': 'gleaming green gem',
    'opalescent': 'dull white gem', 'firey': 'gleaming orange gem', 'brilliant': 'gleaming clear gem',
    'amber': 'gleaming yellow gem', 'glowing': 'dull yellow gem', 'xanthick': 'dull brown gem',
    'platinum': 'pile of silver coins', 'gold': 'pile of gold coins',
    'electrum': 'pile of copper coins',
}
# object offsets whose sprite is chosen by the random appearance (tiles.c)
POT0, SCR0, WAND0, RING0, WEAP0 = 7, 27, 48, 69, 94
STAFFS = range(54, 62)          # digging .. teleport to: shown as staffs
TREASURE_SILVER = 150           # "silver" treasure (a name shared with the key)

FEAT = {  # TL_* terrain and the player
    'PLAYER': 'warrior s', 'HWALL': 'lit brick wall left right', 'VWALL': 'lit brick wall up down',
    'ULWALL': 'lit brick wall right down', 'URWALL': 'lit brick wall left down',
    'LLWALL': 'lit brick wall right up', 'LRWALL': 'lit brick wall left up',
    'TTORCH': 'lit brick wall left right down', 'BTORCH': 'lit brick wall left right up',
    'LTORCH': 'lit brick wall right up down', 'RTORCH': 'lit brick wall left up down',
    'MARBLE': 'lit rock wall center', 'CORRIDOR': 'night dirt floor nswe', 'FLOOR': 'night tile floor nswe',
    'DOOR': 'open wooden door front', 'LOCKED': 'locked wooden door front',
    'UPSTAIR': 'small stairs up', 'DNSTAIR': 'small stairs down', 'POOL': 'deep water tile',
    'TRAP': 'magic trap tile', 'STAFF': 'quarterstaff',
}
TRAPS = ['webbing a', 'magic trap tile', 'rolling boulder trap tile', 'small rotting plant',
         'dart trap tile', 'sleeping gas trap tile', 'bear trap tile', 'arrow trap tile',
         'teleportation trap tile']

# ---- one slot per entity: every id is remapped on its own (mag-dawnlike.rec) ----
CLASSES = [(0, 'food'), (7, 'potion'), (27, 'scroll'), (48, 'wand'), (69, 'ring'), (94, 'weapon'),
           (109, 'armor'), (119, 'shield'), (123, 'key'), (127, 'item'), (135, 'gem'), (147, 'money')]
def obj_class(i):
    return [c for o, c in CLASSES if o <= i][-1]
def slug(s):
    return re.sub(r'[^a-z0-9]+', '_', s.lower()).strip('_')

slots, ents, ids = [], [], set()   # slot -> DawnLike name; slot -> (category, id, display name)
def ent(cat, name, display, sprite_name):
    assert sprite_name in NAMES, 'no DawnLike sprite named: ' + sprite_name
    i, n = slug(name), 2
    while (cat, i) in ids:
        i, n = '%s_%d' % (slug(name), n), n + 1
    ids.add((cat, i))
    ents.append((cat, i, display))
    slots.append(sprite_name)
    return len(slots) - 1

exact = {'mon': 0, 'obj': 0, 'app': 0}
mon_t = []
for name, _ in MON:
    if name in NAMES:
        exact['mon'] += 1
    mon_t.append(ent('monster', name, name, name if name in NAMES else MON_HAND[name]))
# potions, scrolls, wands, rings show their appearance (tiles.c); the kind's own slot is the class sprite
# delusion shows, one per class
CLASS_SLOT = {c: ent('object', 'any ' + c, 'any %s (delusion)' % c, n) for c, n in
              (('potion', 'clear potion'), ('scroll', 'blank scroll'), ('wand', 'crystal wand'), ('ring', 'gold ring'))}
obj_t = []
for i, name in enumerate(OBJ):
    if POT0 <= i < WEAP0:
        obj_t.append(CLASS_SLOT[obj_class(i)])
        continue
    n = name if (name in NAMES and i != TREASURE_SILVER) else OBJ_HAND.get(name)
    if i == TREASURE_SILVER:
        n = 'pile of silver coins'
    if name in NAMES and i != TREASURE_SILVER:
        exact['obj'] += 1
    obj_t.append(ent('object', name, '%s (%s)' % (name, obj_class(i)), n))

def appearance(fk, kind, pool_re):
    """Sprite per random appearance: '<appearance> <kind>' when DawnLike
    has it, else the next unused sprite of that kind"""
    pool = sorted(n for n in NAMES if re.fullmatch(pool_re, n))
    used, out = set(), []
    for a in fk:
        n = '%s %s' % (a.lower(), kind)
        if n in NAMES:
            exact['app'] += 1
        else:
            n = next(p for p in pool if p not in used and p not in ['%s %s' % (x.lower(), kind) for x in fk])
        used.add(n)
        out.append(ent('object', kind + ' ' + a, '%s %s' % (a, kind), n))
    return out
pot_t = appearance(FPOT, 'potion', r'[a-z ]+ potion')
wand_t = appearance(FWAND, 'wand', r'[a-z ]+ wand')
ring_t = appearance(FRING, 'ring', r'[a-z ]+ ring')
scr_t = appearance(FSCROLL, 'scroll', r'(ancient|mystery|dusty) [a-z]+ scroll')
FEAT_NAME = {
    'PLAYER': 'you (the player)', 'HWALL': 'wall, horizontal', 'VWALL': 'wall, vertical',
    'ULWALL': 'wall, top left corner', 'URWALL': 'wall, top right corner',
    'LLWALL': 'wall, bottom left corner', 'LRWALL': 'wall, bottom right corner',
    'TTORCH': 'wall torch, top wall', 'BTORCH': 'wall torch, bottom wall',
    'LTORCH': 'wall torch, left wall', 'RTORCH': 'wall torch, right wall',
    'MARBLE': 'marble (solid rock)', 'CORRIDOR': 'corridor (plain)', 'FLOOR': 'room floor (plain)',
    'DOOR': 'door', 'LOCKED': 'locked door', 'UPSTAIR': 'stairs up', 'DNSTAIR': 'stairs down',
    'POOL': 'pool', 'TRAP': 'trap (unknown kind)', 'STAFF': 'staff',
}
FEAT_CAT = {'PLAYER': 'monster', 'STAFF': 'object'}
feat_t = {k: ent(FEAT_CAT.get(k, 'world'), k, FEAT_NAME[k], v) for k, v in FEAT.items()}
TRAP_NAMES = re.findall(r'^\s*"([^"]+)",', src('VARS.H')[src('VARS.H').index('trapd[NUMTRAPS]'):], re.M)[:len(TRAPS)]
assert len(TRAP_NAMES) == len(TRAPS), TRAP_NAMES
trap_t = [ent('world', 'trap ' + t, t + ' trap', n) for t, n in zip(TRAP_NAMES, TRAPS)]
# autotiled floors (tiles.c): slot base+m is bordered on the sides of mask m
# (n8 s4 w2 e1), 'c' = no border, base+15 = 'nswe'
def autotile(style, kind):
    base = len(slots)
    for m in range(16):
        sides = ''.join(c for b, c in ((8, 'n'), (4, 's'), (2, 'w'), (1, 'e')) if m & b)
        n = style + ' ' + (sides or 'c')
        ent('world', '%s %d' % (kind, m), '%s, border %s' % (kind, sides or 'none'), n)
    return base
feat_t['FLOORS'] = autotile('night tile floor', 'room floor')
feat_t['CORRS'] = autotile('night dirt floor', 'corridor')

# ---- the sheet ------------------------------------------------------------
N = len(slots)
for fr, out in ((0, 'tiles-dawn.png'), (1, 'tiles-dawn-1.png')):
    img = Image.new('RGBA', (32 * 16, (N + 31) // 32 * 16), (0, 0, 0, 0))
    for i, n in enumerate(slots):
        img.paste(sprite(n, fr), ((i % 32) * 16, (i // 32) * 16))
    img.save(os.path.join(HERE, out))

def anim_line():
    """slots whose 2nd frame differs (DawnLike|a redraws only those)"""
    a, b = (Image.open(os.path.join(HERE, f)).convert('RGBA') for f in ('tiles-dawn.png', 'tiles-dawn-1.png'))
    f = [int(a.crop(((i % 32) * 16, (i // 32) * 16, (i % 32) * 16 + 16, (i // 32) * 16 + 16)).tobytes() !=
             b.crop(((i % 32) * 16, (i // 32) * 16, (i % 32) * 16 + 16, (i // 32) * 16 + 16)).tobytes()) for i in range(N)]
    return 'static const unsigned char tile_anim[%d] = {%s};' % (N, ','.join(map(str, f)))

def arr(name, a):
    return 'static const short %s[%d] = {%s};' % (name, len(a), ','.join(map(str, a)))
o = ['/* generated by mkdawn.py from DawnLikeAtlas (DawnLike by DragonDePlatino, CC BY 4.0) */',
     '#define NTILES %d' % N, '#define TILES_PER_ROW 32',
     arr('mon_tile', mon_t), arr('obj_tile', obj_t), arr('pot_tile', pot_t),
     arr('wand_tile', wand_t), arr('ring_tile', ring_t), arr('scroll_tile', scr_t),
     arr('trap_tile', trap_t), anim_line(),
     'static const char *const tile_id[%d] = {%s};' % (N, ','.join('"%s/%s"' % e[:2] for e in ents))]
o += ['#define TL_%s %d' % (k, v) for k, v in feat_t.items()]
open(os.path.join(HERE, 'tiles.h'), 'w').write('\n'.join(o) + '\n')
nf = len(FEAT) + len(TRAPS)
print('%d sprites; monsters %d/55 by name, %d stand-ins; object kinds %d/151 by name; '
      'appearances %d/%d by name; features %d; coverage 100%% of %d slots from DawnLike'
      % (N, exact['mon'], 55 - exact['mon'], exact['obj'], exact['app'],
         len(FPOT) + len(FWAND) + len(FRING) + len(FSCROLL), nf,
         55 + 151 + len(FPOT) + len(FWAND) + len(FRING) + len(FSCROLL) + nf))

# ---- remapping: the stacked atlas + mag-dawnlike.rec (rvip-tools/tilesets/dawnlike_rec.py) ------------
sys.path.insert(0, TS)
import dawnlike_rec
cell = dawnlike_rec.stack(HERE, extra_pos={'warrior s': POS['warrior s']})
REC = os.path.join(HERE, 'mag-dawnlike.rec')
kept = dawnlike_rec.write_rec(REC,
    ['MAG tile mapping: port/mkdawn.py writes it, the remapper edits it',
     '(remapper port/mag-dawnlike.rec); tiles.c reads it at startup (c-rec).',
     'Re-running mkdawn.py keeps every icon below and adds new ids.'],
    [('id', 'dawnlike'), ('name', 'DawnLike'), ('file', 'dawnlike-0.png'), ('anim_file', 'dawnlike-1.png'),
     ('tile_w', 16), ('tile_h', 16), ('off_x', 0), ('off_y', 0), ('gap_x', 0), ('gap_y', 0), ('scenes', 'mag-scenes.rec')],
    {'world': 'World (terrain, walls, traps)', 'monster': 'Monsters', 'object': 'Objects'},
    [(c, i, display, cell(slots[n])) for n, (c, i, display) in enumerate(ents)
     if (c, i) not in (('world', 'floor'), ('world', 'corridor'))])   # always autotiled, never drawn
print('mag-dawnlike.rec: %d ids (%d kept from the old file)' % (len(ents) - 2, kept))
