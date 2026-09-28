/* Stage 5: the web page. Windows filled (map, messages, status, inventory,
 * visible), layout survives a reload, resize keeps the map whole, Help opens
 * the guide, R saves and ends -> overlay -> Play again restores, Q quits ->
 * Hall of Heroes as text -> overlay, save deleted. Shots web/shots/s5-*. */
const { start } = require('./lib.cjs');
(async () => {
	let ok = true;
	const check = (c, m) => { console.log((c ? 'ok   ' : 'FAIL ') + m); if (!c) ok = false; };
	const t = await start(8761);
	const hasSave = () => t.page.evaluate(() => { try { Module.FS.stat('/mag/save/savefile.mag'); return true; } catch (e) { return false; } });
	try {
		t.url += '?seed=4';
		await t.open();
		await t.waitFor(/What is your name/);
		await t.type('Tester'); await t.press('Enter');
		await t.waitFor(/Level: 1 /);
		for (let i = 0; i < 4; i++) { await t.press('Z'); await t.idle(400); await t.press('Escape'); }
		await t.idle(2500); await t.press('s'); await t.idle(500);
		const w = await t.page.evaluate(() => {
			const vis = document.getElementById('vis').textContent;
			const F = Module.mag.frame();
			return { hist: Module.mag.history().length, inv: F.inv.length, vis, stat: F.scr.slice(23 * 80, 24 * 80).some(v => (v & 255) > 32) };
		});
		check(w.hist > 0 && w.inv > 5 && w.stat && /Monsters|Items/i.test(w.vis), `windows filled: ${w.hist} messages, ${w.inv} inventory lines, status, visible list`);
		await t.shot('s5-windows');
		/* help */
		await t.page.click('#btn-help'); await t.idle(600);
		const help = await t.page.evaluate(() => document.getElementById('help-body').textContent);
		check(/Keyboard controls/.test(help) && /About this version/.test(help) && /DawnLike/.test(help), 'Help shows the guide');
		await t.shot('s5-help');
		await t.press('Escape');
		/* resize */
		for (const [W, H] of [[1000, 650], [1440, 900], [760, 500], [1280, 800]]) {
			await t.page.setViewportSize({ width: W, height: H }); await t.idle(400);
			const r = await t.page.evaluate(() => { const c = document.querySelector('#t-map canvas'); return [c.width, c.height, document.querySelector('#t-map').clientWidth]; });
			check(r[0] > 0 && r[1] > 0, `resize ${W}x${H}: map canvas ${r[0]}x${r[1]} in a ${r[2]} px window`);
		}
		/* zoom survives a reload */
		await t.page.click('#t-map .t button[title="Bigger text"]', { force: true })   /* A+ on the Map title bar */; await t.idle(900);
		const z = await t.page.evaluate(() => document.querySelector('#t-map canvas').style.width);
		check(await hasSave(), 'autosave exists');
		await t.page.reload();
		await t.waitFor(/Welcome back/, 20000);
		await t.idle(800);
		const z2 = await t.page.evaluate(() => document.querySelector('#t-map canvas').style.width);
		check(z === z2, `zoom kept over a reload (${z})`);
		/* R: save and leave -> overlay -> Play again continues */
		await t.press('R'); await t.idle(500); await t.press('y'); await t.idle(1200);
		await t.press('Space'); await t.idle(800);
		let ov = await t.page.evaluate(() => !document.getElementById('overlay').hidden && document.getElementById('overlay-msg').textContent);
		check(/saved/.test(ov || ''), 'R: overlay says saved: ' + ov);
		check(await hasSave(), 'R: the save stays');
		await t.shot('s5-saved');
		await t.page.click('#btn-restart');
		await t.waitFor(/Welcome back/, 20000);
		check(true, 'Play again: the game continues');
		/* Q: quit -> Hall of Heroes (text) -> overlay, save gone */
		await t.idle(500);
		await t.press('Q'); await t.idle(300); await t.press('y'); await t.idle(800);
		const s = await t.screen();
		check(/Hall Of Heroes/.test(s) && await t.page.evaluate(() => document.getElementById('game').classList.contains('txt')), 'Q: Hall of Heroes shown as text');
		await t.shot('s5-heroes');
		await t.press('Space'); await t.idle(800);
		ov = await t.page.evaluate(() => !document.getElementById('overlay').hidden && document.getElementById('overlay-msg').textContent);
		check(/over/.test(ov || ''), 'Q: overlay: ' + ov);
		check(!(await hasSave()), 'Q: the save is deleted');
		check(t.errors.length === 0, 'no page errors ' + t.errors.join(' | '));
	} catch (e) { console.log('FAIL ' + e.message); ok = false; }
	await t.close();
	process.exit(ok ? 0 : 1);
})();
