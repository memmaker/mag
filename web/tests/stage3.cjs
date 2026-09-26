/* Stage 3: Enter opens the command menu (groups -> commands, arrows,
 * digits, Escape, clicks), i opens the inventory with a cursor, a letter
 * runs the item's main action, Enter on an item opens its action menu,
 * item prompts ("What do you want to wield?") show the list with a cursor. */
const { start } = require('./lib.cjs');
(async () => {
	const t = await start(8736);
	let ok = true;
	const check = (c, m) => { console.log((c ? 'ok   ' : 'FAIL ') + m); if (!c) ok = false; };
	try {
		t.url += '?seed=1';
		await t.open();
		await t.waitFor(/What is your name/);
		await t.type('Tester'); await t.press('Enter');
		await t.waitFor(/Level: 1 /);
		await t.press('Enter');
		let s = await t.waitFor(/Commands/);
		check(/Moving/.test(s) && /Things you carry/.test(s) && /Macros and the game/.test(s), 'Enter: command groups');
		await t.shot('s3-menu');
		await t.press('ArrowDown'); await t.press('Enter');
		s = await t.waitFor(/Get what is lying here/);
		check(/Inventory/.test(s) && /\^U Use a miscellaneous item/.test(s), 'group 2 lists item commands with keys');
		if (!/\^U Use/.test(s)) console.log(s);
		await t.shot('s3-group');
		await t.press('Escape');
		await t.idle(200);
		s = await t.screen();
		check(!/Commands|Things you carry/.test(s), 'Escape closes the menu and restores the map');
		/* digit + click: group 4 (Information), click on "Version" */
		await t.press('Escape'); await t.press('Escape');
		await t.press('Enter'); await t.press('4');
		s = await t.waitFor(/Your title/);
		const row = s.split('\n').findIndex(l => /Your title/.test(l));
		const box = await t.page.locator('#t-text canvas').boundingBox();
		await t.page.mouse.click(box.x + box.width * 40 / 80, box.y + box.height * (row + 0.5) / 25);
		await t.idle(300);
		s = await t.screen();
		check(/^You have attained the rank of/.test(s) && !/Information/.test(s), 'click on "Your title" in the Information group runs T: ' + s.split('\n')[0]);
		/* inventory */
		await t.press('i');
		s = await t.waitFor(/Inventory/);
		check(/a\) 2 food rations|a\) a food ration/.test(s) && /tinderbox/.test(s), 'i: inventory list');
		await t.shot('s3-inventory');
		await t.press('ArrowDown'); await t.press('Enter');
		s = await t.waitFor(/Examine/);
		check(/Wield/.test(s) && /Drop/.test(s) && /Examine/.test(s), 'Enter on the weapon: its action menu');
		await t.shot('s3-itemmenu');
		await t.press('Escape'); await t.press('Escape');
		await t.press('i'); await t.waitFor(/Inventory/);
		await t.press('a');   /* food: main action = eat */
		s = await t.waitFor(/^(Yucko|Ahh|Yum|Oh\.  That sure|Mmmmmm)/);
		check(true, 'letter a eats: ' + s.split('\n')[0]);
		await t.press('Space'); await t.idle(300);
		s = await t.screen();
		const monsters = s.split('\n').slice(1, 23).join('').match(/[a-zA-Z]/g);
		if (monsters) check(!/Inventory/.test(s), 'the inventory stays closed: monster in view (' + monsters.join('') + ')');
		else check(/Inventory/.test(s), 'the inventory reopens after the action (no monster in view)');
		await t.press('Escape');
		await t.press('w');
		s = await t.waitFor(/What do you want to wield\?/);
		check(/dagger|mace|flail/.test(s) && /long bow/.test(s), 'item prompt shows the fitting items');
		await t.shot('s3-prompt');
		await t.press('Escape');
		check(t.errors.length === 0, 'no page errors ' + t.errors.join(' | '));
	} catch (e) { console.log('FAIL ' + e.message); ok = false; }
	await t.close();
	process.exit(ok ? 0 : 1);
})();
