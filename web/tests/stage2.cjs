/* Stage 2: Z explores (the map grows, one step a turn), > walks to the known
 * down staircase and stops on it, the next > takes it. Monsters in the way are fought by walking
 * into them (the explorer never attacks by itself). SEED env = dungeon. */
const { start } = require('./lib.cjs');
const DIRS = [[-1, -1, 'y'], [-1, 0, 'k'], [-1, 1, 'u'], [0, -1, 'h'], [0, 1, 'l'], [1, -1, 'b'], [1, 0, 'j'], [1, 1, 'n']];
function adjacentMonster(s) {
	const L = s.split('\n');
	for (let r = 1; r < 23; r++) {
		const c = (L[r] || '').indexOf('☻');
		if (c < 0) continue;
		for (const [dr, dc, k] of DIRS) {
			const ch = (L[r + dr] || '')[c + dc] || ' ';
			if (/[a-zA-Z]/.test(ch)) return k;
		}
	}
	return null;
}
async function play(seed, port) {
	const t = await start(port);
	const cells = s => s.split('\n').slice(1, 23).join('').replace(/ /g, '').length;
	t.url += '?seed=' + seed;
	await t.open();
	await t.waitFor(/What is your name/);
	await t.type('Tester'); await t.press('Enter');
	let s = await t.waitFor(/Level: 1 /);
	const c0 = cells(s);
	let fights = 0, zs = 0, stairs = 0, max = 0, same = 0, seen = false;
	for (let i = 0; i < 160; i++) {
		s = await t.screen();
		if (/Level: 2 /.test(s) || /Hall Of Heroes|R\.I\.P/.test(s)) break;
		max = Math.max(max, cells(s));
		seen = seen || /»/.test(s);   /* once the walk arrives, the player hides the » */
		const m = adjacentMonster(s);
		same = m ? same + 1 : 0;
		if (m && same < 5) { fights++; await t.press(m); }
		else if (i >= 25 && seen) { stairs++; await t.press('>'); await t.idle(400); }
		else { zs++; await t.press('Z'); await t.idle(400); }
		await t.press('Escape');
		if (i === 3) await t.shot('s2-exploring-' + seed);
	}
	s = await t.screen();
	await t.shot('s2-end-' + seed);
	const r = { c0, max, zs, fights, stairs, level2: /Level: 2 /.test(s), errors: t.errors.slice() };
	await t.close();
	return r;
}
(async () => {
	let ok = true;
	const check = (c, m) => { console.log((c ? 'ok   ' : 'FAIL ') + m); if (!c) ok = false; };
	try {
		const a = await play(process.env.EXPLORE_SEED || 3, 8733);
		check(a.max > a.c0 + 100, `seed 3: explore grew the map ${a.c0} -> ${a.max} cells (${a.zs} Z, ${a.fights} attacks)`);
		const b = await play(process.env.STAIRS_SEED || 1, 8734);
		check(b.level2, `seed 1: > walked to the known stairs, > again went down (${b.stairs} >, ${b.fights} attacks)`);
		check(a.errors.length + b.errors.length === 0, 'no page errors ' + a.errors.concat(b.errors).join(' | '));
	} catch (e) { console.log('FAIL ' + e.message); ok = false; }
	process.exit(ok ? 0 : 1);
})();
