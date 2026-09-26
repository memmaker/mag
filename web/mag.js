/*
 * MAG in the browser: draws the frames port/fe_web.c sends (Module.mag),
 * queues keys, keeps the save directory in IndexedDB. Text mode: the
 * original IBM PC text screen (VGA 9x16 font, CP437, CGA colours, blink).
 * Loaded before mag-core.js. Layout and window code follow Rogue PC's
 * web/roguepc.js (RVIP template).
 */
(function () {
	'use strict';

	var DIR = '/mag', SAVE = DIR + '/save/savefile.mag', LAYOUT_FILE = DIR + '/web-layout.json';
	var PAL = ['#000000', '#0000aa', '#00aa00', '#00aaaa', '#aa0000', '#aa00aa', '#aa5500', '#aaaaaa',
		'#555555', '#5555ff', '#55ff55', '#55ffff', '#ff5555', '#ff55ff', '#ffff55', '#ffffff'];
	/* CP437 -> Unicode, for text windows and tests */
	var CP437 = ' ☺☻♥♦♣♠•◘○◙♂♀♪♫☼►◄↕‼¶§▬↨↑↓→←∟↔▲▼' +
		' !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~⌂' +
		'ÇüéâäàåçêëèïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒáíóúñÑªº¿⌐¬½¼¡«»░▒▓│┤╡╢╖╕╣║╗╝╜╛┐└┴┬├─┼╞╟╚╔╩╦╠═╬╧╨╤╥╙╘╒╓╫╪┘┌█▄▌▐▀' +
		'αßΓπΣσµτΦΘΩδ∞φε∩≡±≥≤⌠⌡÷≈°∙·√ⁿ²■ ';
	/* port/fe.h FK_* */
	var FK = { ArrowUp: 0x100, ArrowDown: 0x101, ArrowLeft: 0x102, ArrowRight: 0x103, Home: 0x104, End: 0x105,
		PageUp: 0x106, PageDown: 0x107, Insert: 0x108, Delete: 0x109 };
	var FK_KP0 = 0x10a, FK_KPDOT = 0x114, FK_KPENTER = 0x115, FK_KPPLUS = 0x116, FK_KPMINUS = 0x117,
		FK_KPSTAR = 0x118, FK_KPSLASH = 0x119, FK_F1 = 0x11a, FK_CLICK = 0x124;

	window.magKeys = function (a) { a.forEach(function (k) { events.push(typeof k === 'string' ? k.charCodeAt(0) : k); }); };
	var events = [], clickAt = 0, running = false, L = null;
	var fontSheets = [], T = null;

	function $(id) { return document.getElementById(id); }
	function status(msg, isError) {
		var s = $('status');
		s.textContent = msg;
		s.className = isError ? 'error' : '';
		s.hidden = !msg;
	}
	function blinkOn() { return ((performance.now() / 229) | 0) & 1; }
	function curOn() { return ((performance.now() / 115) | 0) & 1; }

	/* VGA font from the wasm heap (port/vgafont.h): one sheet per colour */
	function buildFont(fontPtr) {
		var H16 = Module.HEAPU16, c, x, y, g, mask = new Uint8Array(144 * 256);
		for (g = 0; g < 256; g++)
			for (y = 0; y < 16; y++) {
				var row = H16[(fontPtr >> 1) + g * 16 + y];
				for (x = 0; x < 9; x++)
					if ((row >> (8 - x)) & 1) mask[((g >> 4) * 16 + y) * 144 + (g & 15) * 9 + x] = 1;
			}
		for (c = 0; c < 16; c++) {
			var cv = document.createElement('canvas');
			cv.width = 144; cv.height = 256;
			var ctx = cv.getContext('2d'), im = ctx.createImageData(144, 256);
			var r = parseInt(PAL[c].substr(1, 2), 16), gg = parseInt(PAL[c].substr(3, 2), 16), b = parseInt(PAL[c].substr(5, 2), 16);
			for (var i = 0; i < mask.length; i++)
				if (mask[i]) { im.data[i * 4] = r; im.data[i * 4 + 1] = gg; im.data[i * 4 + 2] = b; im.data[i * 4 + 3] = 255; }
			ctx.putImageData(im, 0, 0);
			fontSheets[c] = cv;
		}
	}

	/* ---------- text mode: the 80x25 screen ---------- */
	var textCv, textCtx;
	function drawText() {
		if (!T || !textCtx) return;
		var c = textCtx, r, col, v, a, s = T.scr;
		c.setTransform(2, 0, 0, 2, 0, 0);
		c.imageSmoothingEnabled = false;
		c.fillStyle = '#000'; c.fillRect(0, 0, 720, 400);
		var bl = blinkOn();
		for (r = 0; r < 25; r++)
			for (col = 0; col < 80; col++) {
				v = s[r * 80 + col]; a = v >> 8;
				if ((a >> 4) & 7) { c.fillStyle = PAL[(a >> 4) & 7]; c.fillRect(col * 9, r * 16, 9, 16); }
				var g = v & 255;
				if (g && g !== 32 && (!(a & 0x80) || bl))
					c.drawImage(fontSheets[a & 15], (g & 15) * 9, (g >> 4) * 16, 9, 16, col * 9, r * 16, 9, 16);
			}
		if (T.con && curOn()) {
			a = s[T.cr * 80 + T.cc] >> 8;
			c.fillStyle = PAL[(a & 15) || 7];
			c.fillRect(T.cc * 9, T.cr * 16 + 13, 9, 2);
		}
	}
	function fitText() {
		var g = $('game'), sc = Math.min(g.clientWidth / 720, g.clientHeight / 400);
		textCv.style.width = Math.floor(720 * sc) + 'px';
		textCv.style.height = Math.floor(400 * sc) + 'px';
	}
	function applyDom() {
		var g = $('game');
		g.classList.add('txt');
		var t = $('t-text');
		t.hidden = false;
		t.style.left = '0px'; t.style.top = '0px';
		t.style.width = g.clientWidth + 'px'; t.style.height = g.clientHeight + 'px';
		$('btn-text').classList.add('on');
		['btn-tiles', 'btn-zoom-in', 'btn-zoom-out', 'btn-layout'].forEach(function (b) { $(b).disabled = true; });
		fitText(); drawText();
	}

	/* ---------- called by the game (port/fe_web.c) ---------- */
	var mag = {
		init: function (font) {
			buildFont(font);
			$('game').hidden = false;
			applyDom();
		},
		text: function (scr, cr, cc, con) {
			T = { scr: Module.HEAPU16.slice(scr >> 1, (scr >> 1) + 2000), cr: cr, cc: cc, con: con };
			drawText();
		},
		/* the screen as text (tests, W10) */
		screen: function () {
			if (!T) return '';
			var out = [];
			for (var r = 0; r < 25; r++) {
				var l = '';
				for (var c = 0; c < 80; c++) l += CP437.charAt(T.scr[r * 80 + c] & 255);
				out.push(l.replace(/\s+$/, ''));
			}
			return out.join('\n');
		},
		key: function () { return events.length ? events.shift() : -1; },
		click: function () { return clickAt; },
		pending: function () { return events.length ? 1 : 0; },
		flush: function () { events.length = 0; },
		sync: function () { syncFiles(); },
		end: function (saved) {
			running = false;
			syncFiles(function () {
				$('overlay-msg').textContent = saved ? 'Your game has been saved. Play again to continue it.' : 'The game is over.';
				$('overlay').hidden = false;
			});
		}
	};

	/* blinking attribute and cursor */
	setInterval(function () { if (running) drawText(); }, 115);

	/* ---------- input ---------- */
	function onKey(e) {
		if (!$('help').hidden) {
			if (e.key === 'Escape') { $('help').hidden = true; e.preventDefault(); }
			return;
		}
		if (!running || e.isComposing || e.metaKey) return;
		if (e.target && /^(INPUT|TEXTAREA)$/.test(e.target.tagName)) return;
		var k = e.key, code = e.code || '', m = /^Numpad(\d)$/.exec(code), c;
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

	/* ---------- saves: IndexedDB (IDBFS) ---------- */
	var syncing = false, syncAgain = false, pendingCbs = [];
	function syncFiles(cb) {
		if (!Module.FS) { if (cb) cb(); return; }
		if (typeof cb === 'function') pendingCbs.push(cb);
		if (syncing) { syncAgain = true; return; }
		syncing = true;
		var cbs = pendingCbs; pendingCbs = [];
		Module.FS.syncfs(false, function (err) {
			syncing = false;
			if (err) status('Saving to browser storage (IndexedDB) failed: ' + err + '. Use "Export save" to keep a copy.', true);
			cbs.forEach(function (f) { f(err); });
			if (syncAgain) { syncAgain = false; syncFiles(); }
		});
	}
	function hasSave() { try { Module.FS.stat(SAVE); return true; } catch (e) { return false; } }
	/* the save is several files (savefile.mag, maglevel.N, build.id): export them as one JSON */
	function exportSave() {
		if (!hasSave()) { status('There is no saved game yet.', true); setTimeout(function () { status(''); }, 2000); return; }
		var FS = Module.FS, files = {};
		FS.readdir(DIR + '/save').forEach(function (n) {
			if (n === '.' || n === '..') return;
			var d = FS.readFile(DIR + '/save/' + n), s = '';
			for (var i = 0; i < d.length; i++) s += String.fromCharCode(d[i]);
			files[n] = btoa(s);
		});
		var a = document.createElement('a');
		a.href = URL.createObjectURL(new Blob([JSON.stringify({ game: 'mag', files: files })], { type: 'application/json' }));
		a.download = 'mag-save.json';
		document.body.appendChild(a); a.click();
		setTimeout(function () { URL.revokeObjectURL(a.href); a.remove(); }, 1000);
	}
	function clearSaveDir() {
		var FS = Module.FS;
		FS.readdir(DIR + '/save').forEach(function (n) { if (n !== '.' && n !== '..') FS.unlink(DIR + '/save/' + n); });
	}
	function importSave(file) {
		var r = new FileReader();
		r.onload = function () {
			var j;
			try { j = JSON.parse(r.result); } catch (e) { j = null; }
			if (!j || j.game !== 'mag' || !j.files) { status('That is not a MAG save (mag-save.json).', true); return; }
			if (!confirm('Replace the current game with "' + file.name + '"?')) return;
			running = false;
			clearSaveDir();
			Object.keys(j.files).forEach(function (n) {
				var s = atob(j.files[n]), d = new Uint8Array(s.length);
				for (var i = 0; i < s.length; i++) d[i] = s.charCodeAt(i);
				Module.FS.writeFile(DIR + '/save/' + n.replace(/[\/\\]/g, ''), d);
			});
			syncFiles(function (err) { if (!err) location.reload(); });
		};
		r.readAsText(file);
	}
	function newGame() {
		if (!confirm('Delete the saved game in this browser and start a new one?')) return;
		running = false;
		clearSaveDir();
		syncFiles(function (err) { if (!err) location.reload(); });
	}

	/* ---------- help ---------- */
	var helpLoaded = false;
	function toggleHelp() {
		var h = $('help');
		h.hidden = !h.hidden;
		if (!h.hidden && !helpLoaded) {
			helpLoaded = true;
			fetch('help.html').then(function (r) { if (!r.ok) throw new Error(r.status); return r.text(); })
				.then(function (t) { $('help-body').innerHTML = t; })
				.catch(function (err) { helpLoaded = false; $('help-body').textContent = 'Could not load the guide (' + err + '). Press F1 in the game for its own help.'; });
		}
		if (!h.hidden) $('help-body').focus();
	}

	/* ---------- startup ---------- */
	window.Module = {
		mag: mag,
		arguments: [],
		preRun: [function () {
			var FS = Module.FS;
			FS.mkdirTree(DIR);
			FS.mount(Module.IDBFS, {}, DIR);
			FS.chdir(DIR);
			Module.addRunDependency('idbfs');
			FS.syncfs(true, function (err) {
				if (err) status('Could not read saved games from IndexedDB (' + err + '). Saving may not work in this browser mode.', true);
				try { FS.mkdir(DIR + '/save'); } catch (e) { /* exists */ }
				Module.removeRunDependency('idbfs');
			});
		}],
		onRuntimeInitialized: function () { running = true; status(''); },
		print: function (s) { console.log(s); },
		printErr: function (s) { console.warn(s); },
		setStatus: function (s) { if (s && !running) status(s.replace(/\(\d+\/\d+\)/, '').trim() || 'Loading…'); },
		onAbort: function (what) { crashed(what); }
	};
	function crashed(err) {
		if (!running) return;
		running = false;
		var msg = (err && (err.message || err.reason && err.reason.message)) || String(err);
		console.error('[mag] crash:', err);
		status('The game crashed (' + msg + '). Reload the page to continue from the last autosave.', true);
	}
	window.addEventListener('unhandledrejection', function (e) {
		if (e.reason && e.reason.name === 'ExitStatus') return;
		crashed(e.reason);
	});
	window.addEventListener('error', function (e) {
		if (e.error && e.error.name === 'ExitStatus') return;
		if (e.error instanceof WebAssembly.RuntimeError || /mag-core/.test(e.filename || '')) crashed(e.error || e.message);
	});
	window.addEventListener('beforeunload', function (e) { if (running && hasSave()) syncFiles(); });
	document.addEventListener('visibilitychange', function () { if (document.hidden) syncFiles(); });

	document.addEventListener('keydown', onKey);
	document.addEventListener('DOMContentLoaded', function () {
		textCv = document.querySelector('#t-text canvas');
		textCv.width = 1440; textCv.height = 800;
		textCtx = textCv.getContext('2d');
		$('btn-export').onclick = exportSave;
		$('btn-import').onclick = function () { $('import-file').click(); };
		$('import-file').onchange = function () { if (this.files[0]) importSave(this.files[0]); this.value = ''; };
		$('btn-new').onclick = newGame;
		$('btn-help').onclick = toggleHelp;
		$('help-close').onclick = toggleHelp;
		$('btn-restart').onclick = function () { location.reload(); };
		document.querySelectorAll('button').forEach(function (b) {
			b.addEventListener('mousedown', function (e) { e.preventDefault(); });
		});
	});
	window.addEventListener('resize', function () { if (running) applyDom(); });
})();
