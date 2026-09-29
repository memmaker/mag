#!/usr/bin/env python3
"""Synthesize MAG's sound effects at build time: one <event>.wav per event the port
names (port_sound() in src/*.C, verbs in port/rvip.c; asserted) into <out>; the page
plays sound/<event>.wav. MAG's only sound was the PC speaker's bell, so these are
made for it in that voice: pure square waves, "noise" as a random-pitch buzz.
Stdlib only. Usage (repo root): python3 web/mksounds.py <out>"""
import glob, math, os, random, re, struct, sys, wave

R = 22050
rnd = random.Random(1989)

def tone(f0, f1, dur, vol=.3, dec=2.0):
    out, ph, n = [], 0.0, int(R * dur)
    for i in range(n):
        t = i / n
        ph += f0 * (f1 / f0) ** t / R
        x = 1 if ph % 1 < .5 else -1
        out.append(x * vol * (1 - t) ** dec)
    return out

def noise(dur, vol=.3, dec=2.0, lo=100, hi=1500, swell=False):
    """PC-speaker 'noise': a square wave that hops to a random pitch every 2 ms"""
    out, ph, f, n = [], 0.0, lo, int(R * dur)
    for i in range(n):
        t = i / n
        if i % 44 == 0:
            f = rnd.uniform(lo, hi)
        ph += f / R
        out.append((1 if ph % 1 < .5 else -1) * vol * (math.sin(math.pi * t) if swell else (1 - t) ** dec))
    return out

def mix(*parts):
    out = [0.0] * max(map(len, parts))
    for p in parts:
        for i, x in enumerate(p):
            out[i] += x
    return out

def notes(fs, d=.07, **k):
    return sum((tone(f, f, d, **k) for f in fs), [])

SOUNDS = {
    'hit':      lambda: noise(.07, .3, 2, 80, 400) + tone(200, 80, .05),
    'miss':     lambda: noise(.12, .2, lo=800, hi=2500, swell=True),
    'kill':     lambda: tone(400, 60, .3, .3, 1),
    'hurt':     lambda: noise(.1, .35, 2, 50, 250) + tone(110, 50, .08),
    'mmiss':    lambda: noise(.1, .15, lo=600, hi=1800, swell=True),
    'death':    lambda: notes([440, 415, 392, 370], .2, vol=.3, dec=.2) + tone(349, 110, .9, .3, 1),
    'level':    lambda: notes([523, 659, 784, 1047], .07, vol=.3, dec=.2) + tone(1047, 1047, .2, .3, 1),
    'stairs':   lambda: notes([659, 587, 523, 494, 440], .05, vol=.25, dec=.4),
    'teleport': lambda: sum((tone(300 + 150 * i, 1200 + 300 * i, .05, .25, .3) for i in range(5)), []),
    'pickup':   lambda: tone(600, 900, .05, .25, 1),
    'gold':     lambda: notes([1319, 1760, 2093], .045, vol=.25, dec=.5),
    'quaff':    lambda: sum((tone(f, f * 2, .04, .2, 1) for f in (250, 330, 280, 400)), []),
    'eat':      lambda: sum((noise(.04, .3, 3, 60, 300) + [0.0] * 1300 for _ in range(3)), []),
    'zap':      lambda: tone(2000, 300, .2, .25, .5),
    'scroll':   lambda: noise(.18, .15, lo=1500, hi=4000, swell=True),
    'ring':     lambda: tone(1760, 1760, .12, .2, 1) + tone(2349, 2349, .2, .2, 2),
    'wield':    lambda: tone(1500, 1400, .15, .2, 2) + tone(2200, 2100, .12, .15, 3),
    'wear':     lambda: noise(.1, .25, 2, 100, 500) + noise(.08, .2, 2, 150, 600),
    'shoot':    lambda: tone(1200, 400, .12, .2, 1),
    'drop':     lambda: tone(200, 100, .06, .3, 3),
    'unlock':   lambda: tone(900, 900, .03, .25, 2) + [0.0] * 1500 + tone(1200, 1200, .04, .25, 2),
    'ignite':   lambda: noise(.3, .2, 1, 200, 3000) + tone(300, 500, .1, .15, 1),
}

events = set()
for f in glob.glob('src/*.C'):
    src = open(f, encoding='latin-1').read()
    events |= set(re.findall(r'port_sound\("(\w+)"', src))
    events |= {e for p in re.findall(r'port_sound\([^;]*\?\s*"(\w+)"\s*:\s*"(\w+)"\)', src) for e in p}
events |= set(re.findall(r'\{ "[^"]+", "(\w+)" \}', open('port/rvip.c', encoding='latin-1').read()))
assert events == set(SOUNDS), 'events vs sounds: %s' % sorted(events ^ set(SOUNDS))

out = sys.argv[1]
os.makedirs(out, exist_ok=True)
for ev in sorted(events):
    s = SOUNDS[ev]()
    with wave.open(os.path.join(out, ev + '.wav'), 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(R)
        w.writeframes(b''.join(struct.pack('<h', int(max(-1, min(1, x)) * 32000)) for x in s))
