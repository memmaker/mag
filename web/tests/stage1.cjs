/* Stage 1: a new game starts, random keys don't crash it, the autosave
 * survives a reload (IndexedDB), Export/New game exist. */
const { start } = require('./lib.cjs');
(async () => {
	const t = await start(8731);
	let ok = true;
	const check = (c, m) => { console.log((c ? 'ok   ' : 'FAIL ') + m); if (!c) ok = false; };
	try {
		await t.open();
		let s = await t.waitFor(/What is your name/);
		check(/MAG/.test(s) && /Teixeira/.test(s), 'title page');
		await t.shot('s1-title');
		await t.type('Tester');
		await t.press('Enter');
		s = await t.waitFor(/Level: 1 /);
		check(/MAG version PC-1\.1/.test(s), 'new game, version message: ' + s.split('\n')[0]);
		await t.shot('s1-newgame');
		/* a few moves so the autosave (every 2 s at the command prompt) runs */
		{
			await t.press('s', 3); await t.idle(2500); await t.press('s'); await t.idle(500);
			const before = (await t.screen()).split('\n').find(l => /^Level:/.test(l));
			const has = await t.page.evaluate(() => { try { Module.FS.stat('/mag/save/savefile.mag'); return true; } catch (e) { return false; } });
			check(has, 'autosave file in /mag/save');
			await t.page.evaluate(() => new Promise(r => Module.FS.syncfs(false, r)));
			await t.page.reload();
			await t.page.waitForFunction(() => window.Module && Module.mag && /Level:/.test(Module.mag.screen()), null, { timeout: 20000 });
			s = await t.screen();
			const after = s.split('\n').find(l => /^Level:/.test(l));
			check(/Welcome back, Tester/.test(s), 'restored after reload: ' + s.split('\n')[0]);
			check(before.replace(/Scr:.*/, '') === after.replace(/Scr:.*/, ''), 'status line kept: ' + after);
			await t.shot('s1-restored');
		}
		/* random keys (no quit/save/up-stairs), like the ASan run */
		const pool = 'hjklyubnhjklyubnHJKLYUBN.s;:fgivx>cdeptqrwzaAFTV ' + '\u001b';
		let seed = 7;
		for (let i = 0; i < 400; i++) {
			seed = (seed * 1103515245 + 12345) >>> 0;
			const c = pool[(seed >>> 16) % pool.length];
			await t.page.keyboard.press(c === ' ' ? 'Space' : c === '\u001b' ? 'Escape' : c);
		}
		await t.press('Escape', 3);
		await t.idle(2500);
		s = await t.screen();
		console.log(s);
		check(!/crashed/.test(await t.page.textContent('#status')), 'no crash after 400 random keys');
		console.log('after random keys: ' + (/Hall Of Heroes|R\.I\.P/.test(s) ? 'game over screen' : 'still playing'));
		check(t.errors.length === 0, 'no page errors ' + t.errors.join(' | '));
	} catch (e) { console.log('FAIL ' + e.message); ok = false; }
	await t.close();
	process.exit(ok ? 0 : 1);
})();
