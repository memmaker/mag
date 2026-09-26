#!/usr/bin/env python3
"""Native tests of the RVIP additions (explore Z, stair walking < >), driving
port/mag-native (fe_tty.c) with scripted keys and reading MAG_DUMP.
Usage: python3 port/tests/explore_stairs.py [seeds]"""
import os, subprocess, sys, tempfile, re
HERE = os.path.dirname(os.path.abspath(__file__))
BIN = os.path.join(HERE, '..', 'mag-native')
DATA = os.path.join(HERE, '..', '..', 'data')

def run(keys, seed):
    d = tempfile.mkdtemp(prefix='magt')
    os.mkdir(os.path.join(d, 'save'))
    env = dict(os.environ, MAG_DATA=DATA, MAG_KEYS=keys, MAG_DUMP=os.path.join(d, 'dump'),
               ASAN_OPTIONS='detect_leaks=0', HOME=d)
    p = subprocess.run([BIN, 's%d' % seed], cwd=d, env=env, capture_output=True, text=True, timeout=120)
    if 'ERROR' in p.stderr or 'runtime error' in p.stderr:
        print(p.stderr); raise SystemExit('sanitizer error')
    return open(os.path.join(d, 'dump')).read()

def level(s):
    m = re.search(r'Level: (\d+)', s)
    return int(m.group(1)) if m else None

def mapped(s):
    return sum(1 for l in s.split('\n')[1:23] for ch in l if ch != ' ')

seeds = [int(a) for a in sys.argv[1:]] or list(range(1, 11))
ok = 0
for seed in seeds:
    base = 'Tester\\r'
    # explore: each Z walks until something stops it; ESC clears messages
    s0 = run(base, seed)
    s1 = run(base + 'Z\\e' * 60, seed)
    # stairs: explore until the down stairs are known, then '>' walks there
    # and takes them (a fight or message stops the walk: '>' again resumes)
    s2 = run(base + 'Z\\e' * 60 + '>\\e' * 10, seed)
    down_known = '\u00bb' in s1
    res = (mapped(s1) > mapped(s0), level(s2) == 2 or not down_known or 'in the way' in s2)
    print('seed %2d explore %4d -> %4d cells  down stairs seen: %-5s  after >: level %s  %-4s %s' % (
        seed, mapped(s0), mapped(s1), down_known, level(s2), 'ok' if all(res) else 'CHECK', s2.split('\n')[0]))
    ok += all(res)
print('%d/%d ok' % (ok, len(seeds)))
