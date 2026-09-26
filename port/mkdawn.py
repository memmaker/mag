#!/usr/bin/env python3
"""MAG's tile set: DawnLike (DragonDePlatino, CC BY 4.0; palette by
DawnBringer), sprites looked up by name in DawnLikeAtlas (Tommy Ettinger,
github.com/tommyettinger/DawnLikeAtlas, renamed/<name>_0.png, 16x16).

One set only (RVIP R4): every slot port/tiles.c can return comes from
DawnLike; names MAG shares with the atlas map directly, the rest take a
stand-in from the same set (hand table below). Writes
  port/tiles-dawn.png   32 sprites per row, 16x16
  port/tiles.h          slot tables for tiles.c
and prints the coverage numbers. Run: python3 port/mkdawn.py [atlas dir]
(default $DAWNLIKE_ATLAS or ~/tools/DawnLikeAtlas)."""
import os, re, sys
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ATLAS = sys.argv[1] if len(sys.argv) > 1 else os.environ.get(
    'DAWNLIKE_ATLAS', os.path.expanduser('~/tools/DawnLikeAtlas'))
REN = os.path.join(ATLAS, 'renamed')
FILES = {}
for f in sorted(os.listdir(REN)):
    m = re.fullmatch(r'(.+?)(_0)?\.png', f)
    if m and not re.search(r'_\d$', m.group(1)):
        FILES.setdefault(m.group(1), f)     # frame 0 of animated sprites
NAMES = set(FILES)


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

slots, index = [], {}
def slot(name):
    assert name in NAMES, 'not in DawnLikeAtlas: ' + name
    if name not in index:
        index[name] = len(slots)
        slots.append(name)
    return index[name]

exact = {'mon': 0, 'obj': 0, 'app': 0}
mon_t = []
for name, _ in MON:
    if name in NAMES:
        exact['mon'] += 1
    mon_t.append(slot(name if name in NAMES else MON_HAND[name]))
obj_t = []
for i, name in enumerate(OBJ):
    n = name if (name in NAMES and i != TREASURE_SILVER) else OBJ_HAND.get(name)
    if i == TREASURE_SILVER:
        n = 'pile of silver coins'
    if n is None and POT0 <= i < WEAP0:
        n = 'clear potion' if i < SCR0 else 'blank scroll' if i < WAND0 else 'crystal wand' if i < RING0 else 'gold ring'
    if name in NAMES and i != TREASURE_SILVER:
        exact['obj'] += 1
    obj_t.append(slot(n))

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
        out.append(slot(n))
    return out
pot_t = appearance(FPOT, 'potion', r'[a-z ]+ potion')
wand_t = appearance(FWAND, 'wand', r'[a-z ]+ wand')
ring_t = appearance(FRING, 'ring', r'[a-z ]+ ring')
scr_t = appearance(FSCROLL, 'scroll', r'(ancient|mystery|dusty) [a-z]+ scroll')
feat_t = {k: slot(v) for k, v in FEAT.items()}
trap_t = [slot(n) for n in TRAPS]

# ---- the sheet ------------------------------------------------------------
N = len(slots)
img = Image.new('RGBA', (32 * 16, (N + 31) // 32 * 16), (0, 0, 0, 0))
for i, n in enumerate(slots):
    img.paste(Image.open(os.path.join(REN, FILES[n])).convert('RGBA'), ((i % 32) * 16, (i // 32) * 16))
img.save(os.path.join(HERE, 'tiles-dawn.png'))

def arr(name, a):
    return 'static const short %s[%d] = {%s};' % (name, len(a), ','.join(map(str, a)))
o = ['/* generated by mkdawn.py from DawnLikeAtlas (DawnLike by DragonDePlatino, CC BY 4.0) */',
     '#define NTILES %d' % N, '#define TILES_PER_ROW 32',
     arr('mon_tile', mon_t), arr('obj_tile', obj_t), arr('pot_tile', pot_t),
     arr('wand_tile', wand_t), arr('ring_tile', ring_t), arr('scroll_tile', scr_t),
     arr('trap_tile', trap_t)]
o += ['#define TL_%s %d' % (k, v) for k, v in feat_t.items()]
open(os.path.join(HERE, 'tiles.h'), 'w').write('\n'.join(o) + '\n')
nf = len(FEAT) + len(TRAPS)
print('%d sprites; monsters %d/55 by name, %d stand-ins; object kinds %d/151 by name; '
      'appearances %d/%d by name; features %d; coverage 100%% of %d slots from DawnLike'
      % (N, exact['mon'], 55 - exact['mon'], exact['obj'], exact['app'],
         len(FPOT) + len(FWAND) + len(FRING) + len(FSCROLL), nf,
         55 + 151 + len(FPOT) + len(FWAND) + len(FRING) + len(FSCROLL) + nf))
