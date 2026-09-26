/* Stage 6: sound (off by default; the game names events, the page plays
 * sound/<event>.wav only when switched on; setting kept over a reload) and
 * the Docs page (docs/web/mag-docs.html renders, key list and tips). */
const fs = require('fs'), path = require('path');
const { start } = require('./lib.cjs');
(async () => {
	let ok = true;
	const check = (c, m) => { console.log((c ? 'ok   ' : 'FAIL ') + m); if (!c) ok = false; };
	/* test samples: a 50 ms silent wav per event (the real ones come from web/sounds.py) */
	const dir = path.join(__dirname, '..', 'dist', 'sound');
	fs.mkdirSync(dir, { recursive: true });
	const n = 2205, wav = Buffer.alloc(44 + n * 2);
	wav.write('RIFF', 0); wav.writeUInt32LE(36 + n * 2, 4); wav.write('WAVEfmt ', 8); wav.writeUInt32LE(16, 16);
	wav.writeUInt16LE(1, 20); wav.writeUInt16LE(1, 22); wav.writeUInt32LE(44100, 24); wav.writeUInt32LE(88200, 28);
	wav.writeUInt16LE(2, 32); wav.writeUInt16LE(16, 34); wav.write('data', 36); wav.writeUInt32LE(n * 2, 40);
	for (const e of ['hit', 'miss', 'kill', 'hurt', 'mmiss', 'stairs', 'pickup', 'gold', 'eat']) fs.writeFileSync(path.join(dir, e + '.wav'), wav);
	const t = await start(8771);
	const reqs = [];
	t.page.on('request', r => { if (/\/sound\//.test(r.url())) reqs.push(r.url().replace(/.*\//, '')); });
	try {
		t.url += '?seed=1';
		await t.open();
		await t.waitFor(/What is your name/);
		await t.type('Tester'); await t.press('Enter');
		await t.waitFor(/Level: 1 /);
		check(await t.page.evaluate(() => document.getElementById('btn-sound').textContent) === 'Sound: off', 'Sound is off by default');
		for (let i = 0; i < 30; i++) { await t.press('Z'); await t.idle(250); await t.press('Escape'); }
		const ev1 = await t.page.evaluate(() => Module.mag.sounds());
		check(ev1.length > 0, 'the game sends sound events: ' + [...new Set(ev1)].join(' '));
		check(reqs.length === 0, 'no samples loaded while sound is off');
		await t.page.click('#btn-sound'); await t.idle(200);
		await t.press('i'); await t.idle(300); await t.press('a'); await t.idle(1500);
		for (let i = 0; i < 10; i++) { await t.press('Z'); await t.idle(250); await t.press('Escape'); }
		check(reqs.length > 0, 'sound on: samples requested: ' + [...new Set(reqs)].join(' '));
		await t.idle(1000);
		await t.page.reload(); await t.waitFor(/Welcome back/, 20000);
		check(await t.page.evaluate(() => document.getElementById('btn-sound').textContent) === 'Sound: on', 'the Sound setting survives a reload');
		check(t.errors.length === 0, 'no page errors ' + t.errors.join(' | '));
		/* Docs page */
		await t.page.goto('file://' + path.join(__dirname, '..', '..', 'docs', 'web', 'mag-docs.html'));
		const d = await t.page.evaluate(() => ({ h: [...document.querySelectorAll('h2')].map(h => h.textContent), keys: document.querySelectorAll('.all div').length }));
		check(d.h.includes('Tips') && d.h.includes("New player's guide") && d.keys > 40, `Docs page: ${d.h.length} sections, ${d.keys} keys`);
		await t.page.screenshot({ path: path.join(__dirname, '..', 'shots', 's6-docs.png') });
	} catch (e) { console.log('FAIL ' + e.message); ok = false; }
	await t.close();
	fs.rmSync(dir, { recursive: true, force: true });
	process.exit(ok ? 0 : 1);
})();
