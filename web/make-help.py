#!/usr/bin/env python3
"""Writes the in-page game guide (dist/help.html) for the web build, in the
shape of the Rogue PC template's make-help.py. The content lives in
docs/web/magguide.py, shared with the Docs page (docs/web/build-docs.py)."""
import os, sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'docs', 'web'))
import magguide as g   # noqa: E402

esc = g.esc


def section(anchor, title, body):
    return '<h2 id="h-%s">%s</h2>%s' % (anchor, esc(title), body)


toc = [('about', 'About the game'), ('keys', 'Keyboard controls'), ('saving', 'Saving your game'),
       ('tips', 'Tips'), ('guide', "New player's guide"), ('history', 'History'),
       ('web', 'Playing in the browser'), ('version', 'About this version')]
parts = ['<p>' + esc(g.TAGLINE) + '</p><ul class="toc">' +
         ''.join('<li><a href="#h-%s">%s</a></li>' % (a, esc(t)) for a, t in toc) + '</ul>']
parts.append(section('about', 'About the game', g.ABOUT))
ess = ''.join('<div class="box"><h3>%s</h3>%s</div>' % (esc(c), g.dl(i)) for c, i in g.ESSENTIALS)
full = ''.join('<div>%s<span>%s</span></div>' % (g.kbd(k), esc(d)) for k, d in g.ALL_KEYS)
parts.append(section('keys', 'Keyboard controls',
                     '<div class="box key"><h3>The keys to remember</h3>' + g.dl(g.KEY_HINTS) + '</div>'
                     '<h3>Essential keys</h3><div class="grid">' + ess + '</div>'
                     '<details><summary>Complete key list (%d commands)</summary>' % len(g.ALL_KEYS) +
                     '<div class="all">' + full + '</div></details>'))
parts.append(section('saving', 'Saving your game', g.SAVING))
parts.append(section('tips', 'Tips', g.TIPS))
parts.append(section('guide', "New player's guide",
                     ''.join('<h3>%s</h3>%s' % (esc(t), b) for t, b in g.GUIDE)))
parts.append(section('history', 'History', g.HISTORY))
parts.append(section('web', 'Playing in the browser', g.WEB))
parts.append(section('version', 'About this version', g.about_version() + '<h3>Credits</h3>' + g.CREDITS))
print('\n'.join(parts))
