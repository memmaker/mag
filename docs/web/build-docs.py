#!/usr/bin/env python3
"""Writes docs/web/mag-docs.html: MAG's page for the Roguelikes Docs
(RVIP Part 2 "Docs page"), from docs/web/magguide.py, the same content as
the web Help (web/make-help.py). The cloud run has no access to the Mac's
~/Desktop/Games/Roguelikes/Docs; there, move magguide.py's dicts into a
GAMES entry of build-docs.py + guides.py (essentials, complete key list,
Tips, new-player guide, "In the browser" instead of "On this computer")
and rebuild with the Docs' own template."""
import os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import magguide as g   # noqa: E402

esc = g.esc
CSS = '''
:root { --bg: #0b0b0d; --panel: #16161a; --line: #2b2b33; --text: #d8d8de; --dim: #8a8a96; --accent: #d9b24c; }
* { box-sizing: border-box; }
body { margin: 0; background: var(--bg); color: var(--text); font: 15px/1.6 system-ui, -apple-system, "Segoe UI", sans-serif; }
main { max-width: 1000px; margin: 0 auto; padding: 24px 16px 64px; }
h1 { color: var(--accent); margin: 0 0 4px; font-size: 28px; letter-spacing: .04em; }
.tag { color: var(--dim); margin: 0 0 16px; }
h2 { font-size: 19px; margin: 32px 0 10px; padding-bottom: 6px; border-bottom: 1px solid var(--line); }
h3 { font-size: 12px; text-transform: uppercase; letter-spacing: .08em; color: var(--accent); margin: 18px 0 8px; }
p, ul { max-width: 76ch; }
a { color: var(--accent); }
.facts { display: grid; grid-template-columns: max-content 1fr; gap: 4px 16px; margin: 12px 0; }
.facts dt { color: var(--dim); }
.facts dd { margin: 0; }
.toc { display: flex; flex-wrap: wrap; gap: 6px; padding: 0; list-style: none; }
.toc a { display: block; padding: 3px 10px; border: 1px solid var(--line); border-radius: 999px; color: var(--text); text-decoration: none; }
.grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(280px, 1fr)); gap: 12px; }
.box { background: #1c1c22; border: 1px solid var(--line); border-radius: 8px; padding: 12px 14px; }
.key { background: #221d10; border-color: #4a3d18; }
dl { margin: 0; display: grid; grid-template-columns: minmax(5.5em, max-content) 1fr; gap: 6px 12px; align-items: baseline; }
dd { margin: 0; }
.all { columns: 3 240px; column-gap: 24px; }
.all div { break-inside: avoid; display: flex; gap: 10px; padding: 3px 0; border-bottom: 1px dashed var(--line); }
kbd { display: inline-block; min-width: 1.7em; padding: 0 6px; text-align: center; font: 600 12px/1.6 ui-monospace, Menlo, monospace;
	background: #22222a; border: 1px solid #3a3a46; border-bottom-width: 2px; border-radius: 4px; }
code { font: 13px ui-monospace, Menlo, monospace; background: #22222a; padding: 0 4px; border-radius: 3px; }
.or, .plus { color: var(--dim); font-size: 12px; margin: 0 3px; }
'''

FACTS = [
    ('Game', 'MAG (Mike\'s Adventure Game), ' + g.VERSION),
    ('Author', 'Michael J. Teixeira'),
    ('Years', '1986-89 (DOS); a UNIX version from about 1985'),
    ('Lineage', 'Original game inspired by Rogue (no Rogue code)'),
    ('Licence', 'Free to copy or modify, not for profit (source headers)'),
    ('Play', '<a href="https://ruzzoli.de/roguelikes/mag/">ruzzoli.de/roguelikes/mag</a> (in the browser)'),
    ('Source', '<a href="%s">memmaker/mag</a>' % g.REPO),
]


def section(anchor, title, body):
    return '<h2 id="%s">%s</h2>%s' % (anchor, esc(title), body)


toc = [('about', 'About'), ('keys', 'Keys'), ('tips', 'Tips'), ('guide', "New player's guide"),
       ('saving', 'Saving'), ('browser', 'In the browser'), ('history', 'History'), ('credits', 'Credits')]
body = ['<h1>MAG</h1><p class="tag">%s</p>' % esc(g.TAGLINE),
        '<dl class="facts">' + ''.join('<dt>%s</dt><dd>%s</dd>' % (esc(k), v) for k, v in FACTS) + '</dl>',
        '<ul class="toc">' + ''.join('<li><a href="#%s">%s</a></li>' % (a, esc(t)) for a, t in toc) + '</ul>']
body.append(section('about', 'About', g.ABOUT))
ess = ''.join('<div class="box"><h3>%s</h3>%s</div>' % (esc(c), g.dl(i)) for c, i in g.ESSENTIALS)
full = ''.join('<div>%s<span>%s</span></div>' % (g.kbd(k), esc(d)) for k, d in g.ALL_KEYS)
body.append(section('keys', 'Keys', '<div class="box key"><h3>The keys to remember</h3>' + g.dl(g.KEY_HINTS) +
                    '</div><h3>Essentials</h3><div class="grid">' + ess + '</div>'
                    '<h3>Complete key list (%d commands)</h3><div class="all">%s</div>' % (len(g.ALL_KEYS), full)))
body.append(section('tips', 'Tips', g.TIPS))
body.append(section('guide', "New player's guide", ''.join('<h3>%s</h3>%s' % (esc(t), b) for t, b in g.GUIDE)))
body.append(section('saving', 'Saving', g.SAVING))
body.append(section('browser', 'In the browser', g.WEB))
body.append(section('history', 'History', g.HISTORY))
body.append(section('credits', 'Credits and this version', g.CREDITS + g.about_version()))

page = ('<!DOCTYPE html>\n<html lang="en"><head><meta charset="utf-8">'
        '<meta name="viewport" content="width=device-width, initial-scale=1">'
        '<title>MAG · Docs</title><style>%s</style></head><body><main>%s</main></body></html>\n'
        % (CSS, '\n'.join(body)))
open(os.path.join(HERE, 'mag-docs.html'), 'w').write(page)
print('docs/web/mag-docs.html', len(page), 'bytes')
