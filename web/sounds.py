#!/usr/bin/env python3
"""Fill web/sound/<event>.wav for MAG's sound events from the Dubtrain
Angband Sound Pack (RVIP A6b/R7). Run on the Mac:

    python3 web/sounds.py ~/Downloads/"Dubtrain Angband Sound Pack v3.1.0"

MAG has no sounds of its own; the port names an event at each game action
(port_sound(): src/*.C under #ifdef PORT, item commands via
port_sound_verb() in port/rvip.c). Each event takes the first sample of the
first Angband event in its list that the pack's sound.cfg names and that
exists on disk. web/build.sh copies web/sound into dist/sound; the page
(Sound button, off by default) plays sound/<event>.wav."""
import os, re, shutil, sys

MAP = {  # MAG event -> Angband/Dubtrain events to try
    'hit': ['hit', 'hit_good'], 'miss': ['miss'], 'kill': ['kill', 'n_kill'],
    'hurt': ['mon_hit', 'mon_bite', 'mon_claw'], 'mmiss': ['miss'],
    'death': ['death'], 'level': ['level'], 'stairs': ['stairs_down', 'tplevel'],
    'teleport': ['teleport'], 'pickup': ['pickup', 'drop'], 'gold': ['money1', 'store5', 'pickup'],
    'quaff': ['quaff'], 'eat': ['eat'], 'zap': ['zap_rod', 'zap', 'use_staff'],
    'scroll': ['study', 'cast_spell'], 'ring': ['wear', 'wield'], 'wield': ['wield'], 'wear': ['wield'],
    'shoot': ['shoot'], 'drop': ['drop'], 'unlock': ['locksmith', 'opendoor'],
    'ignite': ['breathe_fire'],
}
# DASP's event names lie (Angband's `miss` is a bow sample): fixed samples
# for the attack events (RVIP finetuning, Sound)
OVERRIDE = {'miss': 'plc_miss_swish.wav', 'mmiss': 'plc_miss_swish.wav',
            'hit': 'plc_hit_body.wav', 'shoot': 'plc_miss_arrow.wav', 'hurt': 'mco_hit_whip.wav'}

pack = sys.argv[1] if len(sys.argv) > 1 else os.path.expanduser('~/Downloads/Dubtrain Angband Sound Pack v3.1.0')
cfg = None
for root, _, files in os.walk(pack):
    if 'sound.cfg' in files:
        cfg = os.path.join(root, 'sound.cfg')
        break
if not cfg:
    sys.exit('no sound.cfg under ' + pack)
names = {}
for line in open(cfg, encoding='latin-1'):
    m = re.match(r'\s*([a-z0-9_]+)\s*=\s*(.*)', line)
    if m:
        names[m.group(1)] = m.group(2).split()
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'sound')
os.makedirs(out, exist_ok=True)
for ev, cands in MAP.items():
    o = OVERRIDE.get(ev)
    if o and os.path.exists(os.path.join(os.path.dirname(cfg), o)):
        shutil.copy(os.path.join(os.path.dirname(cfg), o), os.path.join(out, ev + '.wav'))
        print('%-9s <- %s (fixed)' % (ev, o))
        continue
    for c in cands:
        wavs = [w for w in names.get(c, []) if os.path.exists(os.path.join(os.path.dirname(cfg), w))]
        if wavs:
            shutil.copy(os.path.join(os.path.dirname(cfg), wavs[0]), os.path.join(out, ev + '.wav'))
            print('%-9s <- %s (%s)' % (ev, c, wavs[0]))
            break
    else:
        print('%-9s    no sample' % ev)
