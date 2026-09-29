/*
 * MAG in the browser: draws the frames port/fe_web.c sends (Module.mag).
 * Tiles mode: tiling windows (map with DawnLike tiles, messages, status,
 * inventory, visible list, pop-up over the map) like the other web ports.
 * Text mode: one window with the original IBM text screen as HTML text.
 * The text windows are trimmed, coloured lines from the game (RVIP W0
 * rule 6); the map is the only canvas. Keyboard, mouse, saves in IndexedDB.
 * Copied from Rogue PC's web/roguepc.js (RVIP template). Loaded before
 * mag-core.js.
 */
(function () {
	'use strict';

	var DIR = '/mag', SAVE = DIR + '/save/savefile.mag', LAYOUT_FILE = DIR + '/web-layout.json';
	var FONT = '"DejaVu Sans Mono", Menlo, Consolas, "Liberation Mono", monospace';
	var GUT = 6, TITLE_H = 20, BORDER = 2;
	var TILE_STEPS = [12, 14, 16, 18, 20, 24, 28, 32, 40, 48, 56, 64];
	var PAL = [];                      /* CGA colours, from the game (js_init) */
	/* CP437 -> Unicode, for the map (Tiles: None) and tests */
	var CP437 = ' ☺☻♥♦♣♠•◘○◙♂♀♪♫☼►◄↕‼¶§▬↨↑↓→←∟↔▲▼' +
		' !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~⌂' +
		'ÇüéâäàåçêëèïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒáíóúñÑªº¿⌐¬½¼¡«»░▒▓│┤╡╢╖╕╣║╗╝╜╛┐└┴┬├─┼╞╟╚╔╩╦╠═╬╧╨╤╥╙╘╒╓╫╪┘┌█▄▌▐▀' +
		'αßΓπΣσµτΦΘΩδ∞φε∩≡±≥≤⌠⌡÷≈°∙·√ⁿ²■ ';
	/* port/fe.h FK_* */
	var FK = { ArrowUp: 0x100, ArrowDown: 0x101, ArrowLeft: 0x102, ArrowRight: 0x103, Home: 0x104, End: 0x105,
		PageUp: 0x106, PageDown: 0x107, Insert: 0x108, Delete: 0x109 };
	var FK_KP0 = 0x10a, FK_KPDOT = 0x114, FK_KPENTER = 0x115, FK_KPPLUS = 0x116, FK_KPMINUS = 0x117,
		FK_KPSTAR = 0x118, FK_KPSLASH = 0x119, FK_F1 = 0x11a, FK_CLICK = 0x124;

	/* tests: queue keys (strings as characters, numbers as key codes) */
	window.magKeys = function (a) { a.forEach(function (k) { events.push(typeof k === 'string' ? k.charCodeAt(0) : k); }); };
	var events = [], clickAt = 0, sounds = [], app;
	var dpr = Math.max(1, Math.min(3, window.devicePixelRatio || 1));
	var L = null, rects = {};
	var fontSheets = [];
	var kind = 'text';                 /* what C sent last: 'text' or 'tiles' */
	var F = null;                      /* last tiles frame */
	var hero = { y: 0, x: 0 }, off = { x: 0, y: 0 }, lastLevel = -1;
	var mapCv, mapCtx, mapPrev = null;

	function $(id) { return document.getElementById(id); }
	function clamp(v, lo, hi) { return Math.max(lo, Math.min(hi, v)); }

	/* DawnLike (port/mkdawn.py): 16x16, 32 per row; the game picks the slot */
	/* Tiles button: DawnLike -> None (text map in the windows) -> PC screen (text mode) */
	var TILESETS = [['tiles-dawn.png', 'DawnLike'], ['tiles-dawn.png', 'DawnLike|a', 'tiles-dawn-1.png'], [null, 'None'], [null, 'PC screen']];
	var dawn = new Image(), dawnReady = false;
	dawn.onload = function () { dawnReady = true; if (L) applyDom(); };
	dawn.src = 'tiles-dawn.png';
	/* tiles drawn: DawnLike picked and loaded (a late onload can't turn None back) */
	function tilesOn() { return dawnReady && L && !L.noTiles; }
	function tile(ctx, t, x, y, w, h, im) {
		if (tilesOn()) ctx.drawImage(im || dawn, (t & 31) * 16, (t >> 5) * 16, 16, 16, x, y, w, h);
	}
	/* DawnLike|a (opt-in): the map swaps to DawnLike's second frame
	 * (port/mkdawn.py tiles-dawn-1.png) twice a second, redrawing only the
	 * cells whose sprite or floor has a different 2nd frame (the game's
	 * tile_anim table, js_init) */
	var dawn1 = new Image(), frame = 0, anim = null;
	function animOn() { return tilesOn() && L.anim && L.mode !== 'text' && kind === 'tiles'; }
	setInterval(function () {
		if (!app.running || !F || !mapCtx || document.hidden || !animOn() || !anim) { frame = 0; return; }
		if (!dawn1.src) dawn1.src = 'tiles-dawn-1.png';
		if (!dawn1.naturalWidth) return;
		frame ^= 1;
		var s = L.tile, h = cellH(), im = frame ? dawn1 : dawn;
		for (var i = 0; i < 22 * 80; i++) {
			var t = F.t[i], u = F.u[i];
			if (!(anim[t] || anim[u])) continue;
			var x = (i % 80) * s, y = ((i / 80) | 0) * h;
			mapCtx.fillStyle = '#000'; mapCtx.fillRect(x, y, s, h);
			if (u >= 0) tile(mapCtx, u, x, y, s, h, im);
			if (t >= 0) tile(mapCtx, t, x, y, s, h, im);
		}
	}, 500);
	/* fonts from the index page's fonts/ (web/build.sh writes fonts.json) */
	function face(id) { var n = L && (id === 'map' ? L.mapFace : L.face); return n ? '"' + n + '", ' + FONT : FONT; }
	function cellH() { return L.tile; }

	function cp(g) { return CP437.charAt(g); }

	/* ---------- text windows (RVIP W0 rule 6): HTML lines from the game ----------
	 * port/fe_web.c sends each changed row trimmed, CP437 as UTF-8, colour
	 * runs "\x05#fg" or "\x05#fg/#bg" up to \x06, with the row colour and
	 * icon tile, and the rows in use; the WM sets the text size (A−/A+ per
	 * window), the pop-up follows Messages, the PC screen is a fixed grid. */
	var P_MSG = 1, P_STAT = 2, P_INV = 3, P_POP = 4, P_TEXT = 5;
	var txt = [], cur = { p: -1, y: 0, x: 0 }, pop = { r0: 0, c0: 0 };
	function paneEl(p) {
		return p === P_INV ? document.querySelector('#t-inv .body') : p === P_POP ? $('pop').firstElementChild :
			document.querySelector((p === P_MSG ? '#t-msg' : p === P_STAT ? '#t-stat' : '#t-text') + ' pre');
	}
	function pane(p) { return txt[p] || (txt[p] = { el: paneEl(p), lines: [], css: [], tile: [] }); }
	var RUN = /\x05#[0-9a-f]{6}(?:\/#[0-9a-f]{6})?/;
	function esc(s) { return s.replace(/&/g, '&amp;').replace(/</g, '&lt;'); }
	function rowHtml(p, y) {
		var s = txt[p].lines[y] || '', cx = cur.p === p && cur.y === y ? cur.x : -1;
		if (cx >= 0) {                              /* the cursor: one cell, past the end if need be */
			var chars = Array.from(s), k = 0, i;
			for (i = 0; i < chars.length; i++) {
				if (chars[i] === '\x05') { i += RUN.exec(chars.slice(i).join(''))[0].length - 1; continue; }
				if (chars[i] > '\x06' && k++ === cx) break;
			}
			while (i >= chars.length) { chars.push(' '); if (k++ === cx) i = chars.length - 1; }
			chars[i] = '\x03' + chars[i] + '\x04';
			s = chars.join('');
		}
		return esc(s).replace(/[\x04\x06]/g, '</span>').replace(/\x03/g, '<span class="cur">')
			.replace(/\x05(#[0-9a-f]{6})(?:\/(#[0-9a-f]{6}))?/g, function (m, c, g) { return '<span style="color:' + c + (g ? ';background:' + g : '') + '">'; });
	}
	/* list icon: the sprite as a CSS sprite sized in em (A+ grows it) */
	function icon(t) {
		if (!tilesOn() || !(t >= 0)) return null;
		var e = document.createElement('i');
		e.className = 'wm-ic mag-ic';
		e.style.backgroundPosition = -(t & 31) + 'em ' + -(t >> 5) + 'em';
		return e;
	}
	function drawRow(p, y) {
		var T = txt[p], d = T && T.el.children[y];
		if (!d) return;
		d.innerHTML = rowHtml(p, y);
		d.style.color = T.css[y] || '';
		if (p === P_INV) { var ic = icon(T.tile[y]); if (ic) d.insertBefore(ic, d.firstChild); }
	}
	function setRows(p, n) {
		var T = pane(p);
		while (T.el.children.length < n) { T.el.appendChild(document.createElement('div')); drawRow(p, T.el.children.length - 1); }
		while (T.el.children.length > n) T.el.removeChild(T.el.lastChild);
	}
	function redrawPane(p) { if (txt[p]) for (var y = 0; y < txt[p].el.children.length; y++) drawRow(p, y); }
	/* Messages follows the newest line unless the player scrolled up to read
	 * back (checked before a change); a key follows again */
	var keyFollow = false, msgFollow = null, msgTimer = false;
	document.addEventListener('keydown', function () { keyFollow = true; }, true);
	function msgMark() {
		var b = document.querySelector('#t-msg .body');
		if (!b) return;
		if (msgFollow == null) msgFollow = b.scrollTop + b.clientHeight >= b.scrollHeight - 4;
		if (!msgTimer) {
			msgTimer = true;
			Promise.resolve().then(function () {
				if (msgFollow || keyFollow) b.scrollTop = b.scrollHeight;
				msgFollow = null; keyFollow = false; msgTimer = false;
			});
		}
	}
	/* the pop-up: the Messages size; in the map body at the original's column */
	function popFont() { $('pop').style.fontSize = RvipWM.fontSize('msg') + 'px'; placePop(); }
	function placePop() {
		var el = $('pop');
		if (el.hidden || !rects.map || !L) return;
		RvipWM.popup(el, { x: pop.c0 * L.tile - Math.max(0, off.x) });
	}
	/* the PC screen: the text windows' font or the IBM VGA web font, at the Messages size */
	function textFont() {
		var el = $('t-text').querySelector('pre');
		el.style.fontSize = RvipWM.fontSize('msg') + 'px';
		el.style.fontFamily = L && L.face ? '"' + L.face + '", ' + FONT : '"WebPlus_IBM_VGA_9x16", ' + FONT;
	}
	/* a click on a text row: its row and cell (the character under the pointer) */
	function rowCell(el, e) {
		var rows = el.children, y, d;
		for (y = 0; y < rows.length; y++) { var b = rows[y].getBoundingClientRect(); if (e.clientY < b.bottom) break; }
		d = rows[Math.min(y, rows.length - 1)];
		if (!d) return null;
		var r = document.caretRangeFromPoint ? document.caretRangeFromPoint(e.clientX, e.clientY) : null, x = 0;
		if (r && d.contains(r.startContainer)) {
			var w = document.createTreeWalker(d, NodeFilter.SHOW_TEXT), n;
			while ((n = w.nextNode()) && n !== r.startContainer) x += Array.from(n.data).length;
			x += Array.from(r.startContainer.data.slice(0, r.startOffset)).length;
		}
		return { y: Math.min(y, rows.length - 1), x: x };
	}

	/* ---------- tiles mode: the map ---------- */
	function shapeMap() {
		var s = L.tile, h = cellH();
		mapCv.width = Math.round(80 * s * dpr); mapCv.height = Math.round(22 * h * dpr);
		mapCv.style.width = 80 * s + 'px'; mapCv.style.height = 22 * h + 'px';
		mapCtx = mapCv.getContext('2d');
		mapCtx.setTransform(dpr, 0, 0, dpr, 0, 0);
		mapCtx.imageSmoothingEnabled = false;
		mapCtx.fillStyle = '#000'; mapCtx.fillRect(0, 0, 80 * s, 22 * h);
		mapPrev = null;
	}
	function drawMap() {
		if (!F) return;
		var s = L.tile, h = cellH(), c = mapCtx, i, r, col;
		c.font = (L.mapFace ? '' : 'bold ') + Math.round(s * 0.9) + 'px ' + face('map');
		c.textAlign = 'center'; c.textBaseline = 'middle';
		for (i = 0; i < 22 * 80; i++) {
			var v = F.scr[80 + i], t = F.t[i], u = F.u[i];
			if (mapPrev && mapPrev.v[i] === v && mapPrev.t[i] === t && mapPrev.u[i] === u) continue;
			r = (i / 80) | 0; col = i % 80;
			var x = col * s, y = r * h, a = v >> 8;
			c.fillStyle = '#000'; c.fillRect(x, y, s, h);
			if (t === -2) continue;
			if (!tilesOn()) t = u = -1;          /* Tiles: None, the characters */
			var im = frame && animOn() && dawn1.naturalWidth ? dawn1 : dawn;
			if (u >= 0) tile(c, u, x, y, s, h, im);
			if (t >= 0) tile(c, t, x, y, s, h, im);
			else {
				var g = v & 255;
				if ((a >> 4) & 7) { c.fillStyle = PAL[(a >> 4) & 7]; c.fillRect(x, y, s, h); }
				if (g && g !== 32) { c.fillStyle = PAL[a & 15]; c.fillText(cp(g), x + s / 2, y + h / 2 + 1); }
			}
		}
		mapPrev = { v: F.scr.slice(80, 80 + 22 * 80), t: F.t.slice(), u: F.u.slice() };
	}
	/* keep the hero in the middle half of the map window */
	function scrollMap() {
		var r = rects.map;
		if (!r) return;
		var ch = cellH();
		off = RvipWM.center(mapCv, (hero.x + 0.5) * L.tile, (hero.y + 0.5) * ch, 80 * L.tile, 22 * ch,
			r[2] - BORDER, r[3] - BORDER - ($('game').classList.contains('wm-single') ? 0 : TITLE_H));
	}

	function drawTiles(full) {
		if (full) shapeMap();
		drawMap(); placePop();
	}

	/* ---------- layout ---------- */
	/*
	 *   +---------------------------+   bottom: y of map | lower part
	 *   |            map            |   side:   x of left | inventory
	 *   +-------------+-------------+   stat:   y of messages | status
	 *   |  messages   |             |
	 *   +-------------+  inventory  |
	 *   |  status     |             |
	 *   +-------------+-------------+
	 */
	var WINS = ['map', 'msg', 'stat', 'inv', 'vis'], wm = null;

	function areaSize() {
		var g = $('game');
		return { w: g.clientWidth, h: g.clientHeight };
	}
	function defaultLayout() {
		var A = areaSize(), W = A.w, H = A.h;
		if (W < 400 || H < 300) { W = 1280; H = 720; }
		var font = 13, tile = TILE_STEPS[0];
		TILE_STEPS.forEach(function (t) { if (80 * t + BORDER <= W && 22 * t + BORDER <= H * 0.66) tile = t; });
		var mapH = 22 * tile + BORDER, lower = H - mapH - GUT;
		var statH = TITLE_H + BORDER + 2 * Math.round(font * 1.3) + 4;
		return { v: 1, mode: 'tiles', tile: tile, auto: true,
			split: { bottom: (mapH + GUT / 2) / H, side: 0.5, stat: clamp((lower - statH - GUT / 2) / lower, 0.3, 0.95) }, wm: null };
	}
	function loadLayout() {
		var d = defaultLayout();
		try {
			var s = JSON.parse(Module.FS.readFile(LAYOUT_FILE, { encoding: 'utf8' }));
			if (s && s.v === 1) {
				if (!s.auto) {
					d.auto = false;
					if (TILE_STEPS.indexOf(s.tile) >= 0) d.tile = s.tile;
				}
				if (s.mode === 'text') d.mode = 'text';
				if (s.sound) d.sound = true;
				if (s.noTiles) d.noTiles = true;
				if (s.anim) d.anim = true;
				if (typeof s.face === 'string') d.face = s.face;
				if (typeof s.mapFace === 'string') d.mapFace = s.mapFace;
				if (s.wm) d.wm = s.wm;
				if (s.font && d.wm && !d.wm.fs) d.wm.fs = { msg: s.font.msg, stat: s.font.stat, inv: s.font.inv, vis: s.font.vis };   /* old layout: sizes were ours */
			}
		} catch (err) { /* nothing saved yet */ }
		L = d;
	}
	var saveTimer = 0;
	function saveLayout() {
		clearTimeout(saveTimer);
		saveTimer = setTimeout(function () {
			try { Module.FS.writeFile(LAYOUT_FILE, JSON.stringify(L)); app.sync(); }
			catch (err) { console.warn('layout not saved', err); }
		}, 400);
	}
	function place(el, r) {
		el.style.left = r[0] + 'px'; el.style.top = r[1] + 'px';
		el.style.width = Math.max(0, r[2]) + 'px'; el.style.height = Math.max(0, r[3]) + 'px';
	}
	function showText() { return L.mode === 'text' || kind === 'text'; }

	/* place the windows for the current mode and redraw everything */
	function applyDom() {
		if (!L) return;
		if (!wm) makeWM();
		$('game').classList.toggle('txt', showText());
		wm.apply();
	}
	/* windows: the shared tiling window manager (rvip-wm.js, RVIP.md 5b);
	 * text mode (the whole 80x25 screen) hides them for #t-text */
	function makeWM() {
		var d = defaultLayout().split, A = areaSize();
		var line = Math.round(13 * 1.3) + 4, stat = 2 * Math.round(13 * 1.3) + 4;
		wm = RvipWM({
			area: $('game'), menu: $('btn-layout'),
			wins: [{ id: 'map', title: 'Map' }, { id: 'msg', title: 'Messages' }, { id: 'stat', title: 'Status' }, { id: 'inv', title: 'Inventory' }, { id: 'vis', title: 'Visible' }],
			multi: { d: 'v', r: d.bottom, a: 'map', b: { d: 'h', r: 0.4, a: { d: 'v', r: d.stat, a: 'msg', b: 'stat' }, b: { d: 'h', r: 0.5, a: 'inv', b: 'vis' } } },
			single: { d: 'v', r: line / A.h, a: 'msg', b: { d: 'v', r: 1 - stat / (A.h - line), a: 'map', b: 'stat' } },
			state: L.wm,
			save: function (st) { L.wm = st; saveLayout(); },
			layout: function (r) {
				var A = areaSize(), txt = showText();
				rects = r; rects.text = [0, 0, A.w, A.h];
				$('t-text').hidden = !txt;
				place($('t-text'), rects.text);
				renderTiles();
				$('game').style.setProperty('--tf', L.face ? '"' + L.face + '", ' + FONT : FONT);   /* the text windows */
				$('pop').style.display = txt ? 'none' : '';
				textFont();
				if (!txt) { drawTiles(true); scrollMap(true); placePop(); }
			},
			/* A− / A+: the map steps its tiles; the text windows are the WM's; the pop-up and PC screen follow Messages */
			zoom: { map: function (px, d) { zoomMap(d); }, msg: function () { popFont(); textFont(); } },
			onReset: resetLayout
		});
	}
	function setMode(m) {
		if (!L || L.mode === m) return;
		L.mode = m;
		syncIcons();
		if (atCmd && app.running) events.push(12);
		applyDom(); saveLayout();
	}

	function zoomMap(d) {
		var i = clamp(TILE_STEPS.indexOf(L.tile) + d, 0, TILE_STEPS.length - 1);
		L.tile = TILE_STEPS[i]; L.auto = false;
		applyDom(); saveLayout();
		app.status('Map tiles: ' + L.tile + ' px');
		setTimeout(function () { app.status(''); }, 1200);
	}
	function resetLayout() {
		var m = L.mode, snd = L.sound, nt = L.noTiles, an = L.anim, fc = L.face, mf = L.mapFace;
		L = defaultLayout(); L.mode = m; L.sound = snd; L.noTiles = nt; L.anim = an; L.face = fc; L.mapFace = mf; L.wm = wm.state();
		applyDom(); saveLayout();
	}

	/* ---------- called by the game (port/fe_web.c) ---------- */
	var autoMore = 0;
	function renderSound() { $('chk-sound').checked = !!(L && L.sound); }
	function renderMore() { $('chk-more').checked = !!autoMore; }
	/* Tiles: DawnLike -> None -> PC screen */
	function tileset() { return !L ? 0 : L.mode === 'text' ? 3 : L.noTiles ? 2 : L.anim ? 1 : 0; }
	function renderTiles() {
		$('btn-tiles').textContent = 'Tiles: ' + TILESETS[tileset()][1];
		renderMapSel();
	}
	var atCmd = 0, visStr = '';
	/* the game builds the inventory rows with or without icons */
	function syncIcons() {
		if (Module._web_set_icons) Module._web_set_icons(tileset() < 2 ? 1 : 0);
		if (Module._web_set_textmode) Module._web_set_textmode(L && L.mode === 'text' ? 1 : 0);
		redrawPane(P_INV);
	}
	function cycleTiles() {
		if (!L) return;
		var n = (tileset() + 1) % TILESETS.length;
		L.noTiles = n === 2; L.anim = n === 1;
		syncIcons();
		if (n === 3) setMode('text');
		else if (L.mode === 'text') setMode('tiles');
		else { applyDom(); saveLayout(); if (atCmd && app.running) events.push(12); }
		/* both lists follow at once: Visible from its cached string, the
		 * Inventory rows come from the game on a redraw (^L at the prompt) */
		$('vis')._vis = null; if (visStr) mag.vis(visStr);
	}
	/* map font: on the Map title bar (on hover), Tiles: None only */
	var mapSel = document.createElement('select');
	mapSel.title = 'Map font (Tiles: None)';
	mapSel.innerHTML = '<option value="">Default font</option>';
	mapSel.addEventListener('pointerdown', function (e) { e.stopPropagation(); });
	function renderMapSel() {
		var bs = document.querySelector('#t-map .wm-btns');
		if (bs && mapSel.parentNode !== bs) bs.insertBefore(mapSel, bs.firstChild);
		mapSel.hidden = tileset() !== 2;
		mapSel.value = (L && L.mapFace) || '';
	}
	function loadFace(n, now) {
		if (!n) { if (now) applyDom(); return; }
		var ff = new FontFace(n, 'url(../fonts/' + n + '.woff)');
		ff.load().then(function () { document.fonts.add(ff); applyDom(); })
			.catch(function () { app.status('Could not load the font ' + n + '.', true); setTimeout(function () { app.status(''); }, 2000); });
	}
	var mag = {
		init: function (ntiles, am, animPtr, pal) {
			PAL = pal.split(',');
			anim = Module.HEAPU8.slice(animPtr, animPtr + ntiles);
			autoMore = am; renderMore();
			if (!L) loadLayout();
			renderSound();
			syncIcons();
			$('sel-font').value = L.face || '';
			loadFace(L.face); loadFace(L.mapFace); loadFace('WebPlus_IBM_VGA_9x16');
			$('game').hidden = false;
			applyDom(); popFont();
		},
		/* the map: page 0 cells, the game's tiles and floors, the hero's cell */
		map: function (scr, t, u, hy, hx, lvl) {
			F = { scr: Module.HEAPU16.slice(scr >> 1, (scr >> 1) + 2000),
				t: Module.HEAP32.slice(t >> 2, (t >> 2) + 1760), u: Module.HEAP32.slice(u >> 2, (u >> 2) + 1760) };
			var moved = hy !== hero.y || hx !== hero.x;
			hero.y = hy; hero.x = hx;
			if (kind !== 'tiles') { kind = 'tiles'; lastLevel = lvl; applyDom(); return; }
			if (showText()) return;
			drawMap();
			if (moved || lvl !== lastLevel) scrollMap(lvl !== lastLevel);
			lastLevel = lvl;
		},
		/* a text window's row y: the game's trimmed line, row colour, icon tile */
		line: function (p, y, s, c, t) {
			var T = pane(p);
			if (p === P_TEXT && kind !== 'text' && L && L.mode !== 'text') { kind = 'text'; applyDom(); }   /* the game ended: its last screen */
			if (p === P_MSG) msgMark();
			T.lines[y] = s; T.css[y] = c; T.tile[y] = t;
			drawRow(p, y);
		},
		rows: function (p, n) { if (p === P_MSG) msgMark(); setRows(p, n); if (p === P_POP) placePop(); },
		cursor: function (p, y, x) {   /* the one text cursor; p < 0: none */
			var o = cur.p, oy = cur.y;
			cur = { p: p, y: y, x: x };
			if (txt[o]) drawRow(o, oy);
			if (txt[p]) drawRow(p, y);
		},
		popup: function (rows, cols, r0, c0, bg) {
			var el = $('pop');
			pop = { r0: r0, c0: c0 };
			setRows(P_POP, 0); txt[P_POP].lines = []; txt[P_POP].css = [];
			el.style.background = bg || '';
			el.hidden = !rows;
			placePop();
		},
		prompt: function (s) { RvipWM.prompt.text(s); },   /* the live message row over the map */
		vis: function (s) {
			/* lines "M<hex glyph><name>\t<colour>\t<tile>" from the game */
			visStr = s;
			RvipWM.visible($('vis'), s.replace(/^([MI])([0-9a-f]{2})/gm, function (m, k, h) { return k + cp(parseInt(h, 16)); })
				.replace(/\t(#[0-9a-f]{6})\t(-?\d+)$/gm, function (m, c, t) { return '\t' + c + '\t' + t; }), icon);
		},
		/* the screen as text (tests, W10): the visual page */
		screen: function () {
			var S = Module._web_vram ? Module.HEAPU16.subarray(Module._web_vram() >> 1, (Module._web_vram() >> 1) + 2000) : null, out = [];
			if (!S) return '';
			for (var r = 0; r < 25; r++) {
				var l = '';
				for (var i = 0; i < 80; i++) l += cp(S[r * 80 + i] & 255);
				out.push(l.replace(/\s+$/, ''));
			}
			return out.join('\n');
		},
		/* tests: the map frame and the Inventory rows as text */
		frame: function () { return F && { scr: F.scr, t: F.t, u: F.u, inv: (txt[P_INV] ? txt[P_INV].lines.slice(0, txt[P_INV].el.children.length) : []).map(function (l) { return { s: l.replace(/\x05[^\x06]*?(?=[^#\/0-9a-f])|\x06/g, '') }; }) }; },
		history: function () { return txt[P_MSG] ? txt[P_MSG].lines.slice(0, txt[P_MSG].el.children.length) : []; },
		/* a game sound event (port_sound): sound/<event>.wav, off by default */
		sound: function (ev) {
			sounds.push(ev);
			if (sounds.length > 50) sounds.shift();
			if (L && L.sound && window.RVIPSound) RVIPSound.play([ev], 0.6);
		},
		sounds: function () { return sounds.slice(); },
		mode: function () { return L ? L.mode : ''; },
		toggle: function () { setMode(L.mode === 'text' ? 'tiles' : 'text'); },
		key: function (ac) { atCmd = ac; if (kind === 'tiles') RvipWM.prompt.wait(ac); return events.length ? events.shift() : -1; },
		click: function () { return clickAt; },
		pending: function () { return events.length ? 1 : 0; },
		flush: function () { events.length = 0; },
		sync: function () { app.sync(); },
		end: function (saved) {
			app.running = false;
			app.sync(function () {
				$('overlay-msg').textContent = saved ? 'Your game has been saved. Play again to continue it.' : 'The game is over.';
				$('overlay').hidden = false;
			});
		}
	};

	/* ---------- input ---------- */
	function onKey(e) {
		if (!app.running || e.isComposing || e.metaKey) return;
		var k = e.key, code = e.code || '', m = /^Numpad(\d)$/.exec(code), c;
		if (e.target && /^(INPUT|TEXTAREA)$/.test(e.target.tagName)) return;
		if (k === 'F12') { mag.toggle(); e.preventDefault(); return; }
		if (m) c = FK_KP0 + +m[1];
		else if (code === 'NumpadEnter') c = FK_KPENTER;
		else if (code === 'NumpadDecimal') c = FK_KPDOT;
		else if (code === 'NumpadAdd') c = FK_KPPLUS;
		else if (code === 'NumpadSubtract') c = FK_KPMINUS;
		else if (code === 'NumpadMultiply') c = FK_KPSTAR;
		else if (code === 'NumpadDivide') c = FK_KPSLASH;
		else if (k === 'Enter') c = 13;
		else if (k === 'Escape') c = 27;
		else if (k === 'Backspace') c = 8;
		else if (k === 'Tab') c = 9;
		else if (FK[k]) c = FK[k];
		else if (/^F([1-9]|10)$/.test(k)) c = FK_F1 + (+k.substr(1)) - 1;
		else if (k.length === 1) {
			c = k.charCodeAt(0);
			if (e.ctrlKey && !e.altKey) {
				var u = k.toUpperCase().charCodeAt(0);
				if (u >= 64 && u <= 95) c = u & 0x1f; else return;
			}
			if (c > 126) return;
		}
		else return;
		events.push(c);
		e.preventDefault();
	}
	/* a click on a text cell (row, col of the 80x25 screen) */
	function click(r, c) {
		if (!app.running || r < 0 || r > 24 || c < 0 || c > 79) return;
		clickAt = (r << 8) | c;
		events.push(FK_CLICK);
	}
	function canvasXY(cv, e) {
		var b = cv.getBoundingClientRect();
		return { x: e.clientX - b.left, y: e.clientY - b.top, w: b.width, h: b.height };
	}

	/* ---------- saves: IndexedDB (IDBFS) ---------- */
	function hasSave() { try { Module.FS.stat(SAVE); return true; } catch (e) { return false; } }
	/* the save is several files (savefile.mag, maglevel.N, build.id): exported as one JSON */
	function packFile() {
		if (!hasSave()) return null;
		var FS = Module.FS, files = {};
		FS.readdir(DIR + '/save').forEach(function (n) {
			if (n === '.' || n === '..') return;
			var d = FS.readFile(DIR + '/save/' + n), s = '';
			for (var i = 0; i < d.length; i++) s += String.fromCharCode(d[i]);
			files[n] = btoa(s);
		});
		FS.writeFile('/tmp/mag-save.json', JSON.stringify({ game: 'mag', files: files }));
		return '/tmp/mag-save.json';
	}
	function clearSaveDir() {
		var FS = Module.FS;
		FS.readdir(DIR + '/save').forEach(function (n) { if (n !== '.' && n !== '..') FS.unlink(DIR + '/save/' + n); });
	}
	function putPack(file, data) {
		var j;
		try { j = JSON.parse(new TextDecoder().decode(data)); } catch (e) { j = null; }
		if (!j || j.game !== 'mag' || !j.files) return 'That is not a MAG save (mag-save.json).';
		Object.keys(j.files).forEach(function (n) {
			var s = atob(j.files[n]), d = new Uint8Array(s.length);
			for (var i = 0; i < s.length; i++) d[i] = s.charCodeAt(i);
			Module.FS.writeFile(DIR + '/save/' + n.replace(/[\/\\]/g, ''), d);
		});
	}

	/* ---------- startup ---------- */
	app = RvipApp({ name: 'mag', save: packFile, clear: clearSaveDir, put: putPack, helpText: 'Press F1 in the game for its own help.' });
	window.Module = {
		mag: mag,
		/* ?seed=N: a fixed dungeon for tests (MAIN.C's undocumented s<N>) */
		arguments: (function () { var m = /[?&]seed=(\d+)/.exec(location.search); return m ? ['s' + m[1]] : []; })(),
		preRun: [function () {
			var FS = Module.FS;
			FS.mkdirTree(DIR);
			FS.mount(Module.IDBFS, {}, DIR);
			FS.chdir(DIR);
			Module.addRunDependency('idbfs');
			FS.syncfs(true, function (err) {
				if (err) app.status('Could not read saved games from IndexedDB (' + err + '). Saving may not work in this browser mode.', true);
				try { FS.mkdir(DIR + '/save'); } catch (e) { /* exists */ }
				loadLayout();
				Module.removeRunDependency('idbfs');
			});
		}],
		onRuntimeInitialized: function () { app.running = true; app.status(''); },
		print: function (s) { console.log(s); },
		printErr: function (s) { console.warn(s); },
		setStatus: function (s) { if (s && !app.running) app.status(s.replace(/\(\d+\/\d+\)/, '').trim() || 'Loading…'); },
		onAbort: function (what) { app.crashed(what); }
	};
	window.addEventListener('pagehide', function () { if (hasSave()) app.sync(); });
	document.addEventListener('visibilitychange', function () { if (document.hidden) app.sync(); });
	setInterval(function () { if (app.running) app.sync(); }, 15000);
	document.addEventListener('keydown', onKey);
	document.addEventListener('DOMContentLoaded', function () {
		mapCv = document.querySelector('#t-map canvas');
		$('btn-tiles').onclick = cycleTiles;
		$('chk-more').onchange = function () { autoMore = this.checked ? 1 : 0; Module._web_set_auto_more(autoMore); };
		$('btn-restart').onclick = function () { location.reload(); };
		$('chk-sound').onchange = function () { if (!L) return; L.sound = this.checked; saveLayout(); };
		RvipWM.dropdown($('btn-file'), $('menu-file'));
		RvipWM.dropdown($('btn-audio'), $('menu-audio'));
		RvipWM.fonts.then(function (list) {
			[[$('sel-font'), 'face'], [mapSel, 'mapFace']].forEach(function (a) {
				RvipWM.fontOptions(a[0]);
				a[0].value = (L && L[a[1]]) || '';
			});
		}).catch(function () { });
		[[$('sel-font'), 'face'], [mapSel, 'mapFace']].forEach(function (a) {
			a[0].onchange = function () { if (!L) return; L[a[1]] = this.value; saveLayout(); loadFace(this.value, true); this.blur(); };
		});
		document.querySelectorAll('button').forEach(function (b) {
			b.addEventListener('mousedown', function (e) { e.preventDefault(); });
		});
		mapCv.addEventListener('mousedown', function (e) {
			var p = canvasXY(mapCv, e);
			click(1 + Math.floor(p.y / cellH()), Math.floor(p.x / L.tile));
		});
		/* the PC screen and the pop-up: the cell under the pointer */
		$('t-text').addEventListener('mousedown', function (e) {
			var c = rowCell(this.querySelector('pre'), e);
			if (c) click(c.y, c.x);
		});
		$('pop').addEventListener('mousedown', function (e) {
			var c = rowCell(this.firstElementChild, e);
			if (c) click(pop.r0 + c.y, pop.c0 + c.x);
		});
		document.addEventListener('contextmenu', function (e) {
			if (app.running && e.target.closest && e.target.closest('#t-map canvas, #pop, #t-text')) { e.preventDefault(); events.push(27); }
		});
	});
	var resizeTimer = 0;
	window.addEventListener('resize', function () {
		if (!L) return;
		clearTimeout(resizeTimer);
		resizeTimer = setTimeout(function () {
			if (L.auto) L.tile = defaultLayout().tile;
			applyDom();
		}, 150);
	});
})();
