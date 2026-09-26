#!/usr/bin/env python3
"""Writes the in-page game guide for the web build (stdout -> dist/help.html)
and, with --page FILE, the standalone page (docs/web/mag-docs.html).

The content comes from the desktop key guides in
~/Desktop/Games/Roguelikes/Docs (build-docs.py + guides.py, entry mag.html),
as for the other games; only the keys-to-remember box is written here."""
import html, importlib.util, os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
DOCS = os.path.expanduser('~/Desktop/Games/Roguelikes/Docs')
PAGE = 'mag.html'
sys.path.insert(0, DOCS)
spec = importlib.util.spec_from_file_location('build_docs', os.path.join(DOCS, 'build-docs.py'))
docs = importlib.util.module_from_spec(spec)
spec.loader.exec_module(docs)
from guides import GUIDES, SAVING   # noqa: E402

game = next(g for g in docs.GAMES if g['file'] == PAGE)
info = dict(game['info'])
esc = html.escape
kbd = docs.kbd

KEY_HINTS = [('F1', 'In-game help screens'), ('Z', 'Auto-explore: walk to what you have not seen yet'),
             ('Enter', 'Menu of all commands'), ('i', 'Inventory with a cursor: letter = main action, Enter = all actions'),
             ('> <', 'Take the stairs, or walk to the nearest known staircase'),
             ('R', 'Save and leave (the game also autosaves)'), ('F12', 'Tiles / text')]


def dl(items):
    return '<dl>' + ''.join(f'<dt>{kbd(k)}</dt><dd>{esc(d)}</dd>' for k, d in items) + '</dl>'


def section(anchor, title, body):
    return f'<h2 id="h-{anchor}">{esc(title)}</h2>{body}'


def body():
    toc = [('about', 'About the game'), ('keys', 'Keyboard controls'), ('saving', 'Saving your game'),
           ('tips', 'Tips'), ('guide', "New player's guide"), ('web', 'Playing in the browser'),
           ('version', 'About this version')]
    parts = ['<p>' + esc(game['tagline']) + '</p><ul class="toc">' +
             ''.join(f'<li><a href="#h-{a}">{esc(t)}</a></li>' for a, t in toc) + '</ul>']
    parts.append(section('about', 'About the game', info['About the game']))
    ess = ''.join(f'<div class="box"><h3>{esc(c)}</h3>{dl(i)}</div>' for c, i in game['essentials'])
    all_keys = game['all']() if callable(game['all']) else game['all']
    full = ''.join(f'<div>{kbd(k)}<span>{esc(d)}</span></div>' for k, d in all_keys)
    parts.append(section('keys', 'Keyboard controls',
                         '<div class="box key"><h3>The keys to remember</h3>' + dl(KEY_HINTS) + '</div>'
                         '<h3>Essential keys</h3><div class="grid">' + ess + '</div>'
                         f'<details><summary>Complete key list ({len(all_keys)} commands)</summary>'
                         '<div class="all">' + full + '</div></details>'))
    parts.append(section('saving', 'Saving your game', SAVING[PAGE]))
    parts.append(section('tips', 'Tips', info['Tips']))
    parts.append(section('guide', "New player's guide", ''.join(f'<h3>{esc(t)}</h3>{b}' for t, b in GUIDES[PAGE])))
    parts.append(section('web', 'Playing in the browser', info['In the browser']))
    parts.append(section('version', 'About this version', info['Credits']))
    return '\n'.join(parts)


if len(sys.argv) > 2 and sys.argv[1] == '--page':
    css = open(os.path.join(HERE, 'index.html'), encoding='utf-8').read()
    style = css[css.index(':root'):css.index('</style>')]
    out = ('<!DOCTYPE html>\n<html lang="en"><head><meta charset="utf-8">'
           '<meta name="viewport" content="width=device-width, initial-scale=1"><title>MAG — keys and guide</title>'
           '<style>' + style.replace('overflow: hidden;', '') + '\n#help-body{max-width:980px;margin:0 auto}</style></head>'
           '<body><div id="help-body"><h1 style="color:var(--accent)">MAG</h1>' + body() + '</div></body></html>\n')
    open(sys.argv[2], 'w', encoding='utf-8').write(out)
else:
    print(body())
