/* Stage 4: tiles. New game (seed 3) in tiles mode: the map window shows
 * DawnLike sprites chosen by the game (Module.mag.frame().t), the player
 * sprite at the hero cell, canvas pixels are sprite colours (not text);
 * F12 switches to the VGA text screen and back. Screenshots web/shots/s4-*. */
const { start } = require('./lib.cjs');
(async () => {
	let ok = true;
	const check = (c, m) => { console.log((c ? 'ok   ' : 'FAIL ') + m); if (!c) ok = false; };
	const t = await start(8741);
	try {
		t.url += '?seed=3';
		await t.open();
		await t.waitFor(/What is your name/);
		await t.type('Tester'); await t.press('Enter');
		await t.waitFor(/Level: 1 /);
		await t.idle(800);
		const f = await t.page.evaluate(() => {
			const F = Module.mag.frame();
			const t = Array.from(F.t), n = t.filter(x => x >= 0).length;
			return { mode: Module.mag.mode(), sprites: n, text: t.filter(x => x === -1).length, kinds: new Set(t.filter(x => x >= 0)).size };
		});
		check(f.mode === 'tiles', 'tiles mode is the default');
		check(f.sprites > 20 && f.text === 0, `map cells as sprites: ${f.sprites} sprites (${f.kinds} kinds), ${f.text} text cells`);
		/* canvas pixels at the hero: a sprite, not a black/grey text cell */
		const px = await t.page.evaluate(() => {
			const cv = document.querySelector('#t-map canvas'), c = cv.getContext('2d');
			const F = Module.mag.frame(); let hy = -1, hx = -1;
			for (let i = 0; i < 1760; i++) if (F.t[i] === Math.max(...F.t)) { }
			const s = Module.mag.screen().split('\n');
			for (let r = 1; r < 23; r++) { const x = (s[r] || '').indexOf('☻'); if (x >= 0) { hy = r - 1; hx = x; } }
			const w = cv.width / 80, h = cv.height / 22;
			const d = c.getImageData(Math.floor(hx * w), Math.floor(hy * h), Math.floor(w), Math.floor(h)).data;
			const cols = new Set();
			for (let i = 0; i < d.length; i += 4) cols.add(d[i] + ',' + d[i + 1] + ',' + d[i + 2]);
			return { hy, hx, colours: cols.size };
		});
		check(px.hy >= 0 && px.colours >= 5, `player sprite drawn at ${px.hy},${px.hx} (${px.colours} colours)`);
		await t.shot('s4-tiles');
		for (let i = 0; i < 6; i++) { await t.press('Z'); await t.idle(500); await t.press('Escape'); }
		await t.shot('s4-tiles-explored');
		await t.press('F12'); await t.idle(300);
		check(await t.page.evaluate(() => Module.mag.mode()) === 'text', 'F12: text mode');
		await t.shot('s4-text');
		await t.press('F12'); await t.idle(300);
		check(await t.page.evaluate(() => Module.mag.mode()) === 'tiles', 'F12 again: tiles');
		/* the inventory window lists the pack with the game's own lines */
		const inv = await t.page.evaluate(() => Module.mag.frame().inv.map(l => l.s));
		check(inv.length > 0 && /^a[)\]] /.test(inv[0]), 'inventory window: ' + inv.slice(0, 3).join(' | '));
		check(t.errors.length === 0, 'no page errors ' + t.errors.join(' | '));
	} catch (e) { console.log('FAIL ' + e.message); ok = false; }
	await t.close();
	process.exit(ok ? 0 : 1);
})();
