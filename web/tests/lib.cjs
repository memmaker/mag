/* Playwright helpers for the MAG web tests (RVIP W10): serve web/dist,
 * open the page in headless Chromium, read the game screen as text.
 * The page loads the shared ../rvip-wm.js, ../rvip-app.js, ../rvip-sound.js,
 * so the server root is laid out as on ruzzoli.de/roguelikes: mag/ = web/dist
 * next to links to rvip-tools/web (and the index page's fonts/). */
const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawn } = require('child_process');
const pw = require(process.env.PLAYWRIGHT || '/opt/node22/lib/node_modules/playwright');

const DIST = path.join(__dirname, '..', 'dist');
const SHOTS = path.join(__dirname, '..', 'shots');
const GAMES = path.join(__dirname, '..', '..', '..');

function serverRoot() {
	const root = fs.mkdtempSync(path.join(os.tmpdir(), 'mag-web-'));
	fs.symlinkSync(DIST, path.join(root, 'mag'));
	for (const f of ['rvip-wm.js', 'rvip-app.js', 'rvip-sound.js'])
		fs.symlinkSync(path.join(GAMES, 'rvip-tools', 'web', f), path.join(root, f));
	const fonts = path.join(GAMES, 'roguelikes-index', 'fonts');
	if (fs.existsSync(fonts)) fs.symlinkSync(fonts, path.join(root, 'fonts'));
	return root;
}

async function start(port) {
	const root = serverRoot();
	const srv = spawn('python3', ['-m', 'http.server', String(port), '-d', root], { stdio: 'ignore' });
	process.on('exit', () => srv.kill('SIGKILL'));   /* also when a test throws before close() */
	await new Promise(r => setTimeout(r, 700));
	const browser = await pw.chromium.launch();
	const ctx = await browser.newContext({ viewport: { width: 1280, height: 800 } });
	const page = await ctx.newPage();
	const errors = [];
	page.on('pageerror', e => errors.push(String(e)));
	page.on('console', m => { if ((m.type() === 'error' && !/Failed to load resource/.test(m.text())) || /Sanitizer|runtime error/.test(m.text())) errors.push(m.text()); });
	page.on('response', r => { if (r.status() >= 400 && !/\/beacon\?/.test(r.url())) errors.push(r.status() + ' ' + r.url()); });   /* the score beacon only exists on the server */
	const url = `http://localhost:${port}/mag/`;
	const t = {
		page, errors, url,
		async open() { await page.goto(t.url); await page.waitForFunction(() => window.Module && Module.mag && Module.mag.screen().length > 0, null, { timeout: 20000 }); },
		screen() { return page.evaluate(() => Module.mag.screen()); },
		async waitFor(re, ms = 10000) {
			const t0 = Date.now();
			for (;;) {
				const s = await t.screen();
				if (re.test(s)) return s;
				if (Date.now() - t0 > ms) throw new Error('timeout waiting for ' + re + '\n' + s);
				await page.waitForTimeout(100);
			}
		},
		async type(s) { await page.keyboard.type(s, { delay: 15 }); },
		async press(k, n = 1) { for (let i = 0; i < n; i++) await page.keyboard.press(k); },
		async shot(name) { await page.screenshot({ path: path.join(SHOTS, name + '.png') }); },
		async idle(ms = 300) { await page.waitForTimeout(ms); },
		async close() {
			await browser.close(); srv.kill('SIGKILL');
			for (const f of fs.readdirSync(root)) fs.unlinkSync(path.join(root, f));   /* links only */
			fs.rmdirSync(root);
		}
	};
	return t;
}
module.exports = { start };
