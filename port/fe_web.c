/*
 * fe_web.c - browser frontend of the MAG port (RVIP, Emscripten)
 *
 * C only describes the frame; web/mag.js draws it (Module.mag). The frame
 * is the visual BIOS page (80x25 cells of attribute << 8 | CP437 char) and
 * the cursor. Input waits with Asyncify (emscripten_sleep); saves live in
 * IndexedDB (IDBFS mounted on the working directory by mag.js).
 */

#include <emscripten.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "port.h"
#include "fe.h"
#include "vgafont.h"
#include "mag.h"
#include "tiles.h"
#undef exit

int fe_click_row, fe_click_col;
int fe_at_cmd;			/* the game waits for a command (prompt line) */
int fe_auto_more = 1;		/* top-line =-More-= needs no key (web option) */
extern int port_started;

int tile_for(int l, int c, int *under);

EM_JS(void, js_init, (const void *font, int ntiles, int auto_more),
	{ Module.mag.init(font, ntiles, auto_more); });
EM_JS(void, js_text, (const void *scr, int cr, int cc, int con),
	{ Module.mag.text(scr, cr, cc, con); });
EM_JS(void, js_tiles, (const void *scr, const void *vr, const int *t, const int *u,
	const void *inv, const void *at, int ninv, int pr0, int pc0, int pr1, int pc1,
	int cr, int cc, int con, int hy, int hx, int lvl),
	{ Module.mag.tiles(scr, vr, t, u, inv, at, ninv, pr0, pc0, pr1, pc1, cr, cc, con, hy, hx, lvl); });
EM_JS(void, js_msg, (const char *s), { Module.mag.msg(UTF8ToString(s)); });
EM_JS(void, js_vis, (const char *s), { Module.mag.vis(UTF8ToString(s)); });
EM_JS(void, js_sound, (const char *s), { Module.mag.sound(UTF8ToString(s)); });
EM_JS(int, js_key, (int atcmd), { return Module.mag.key(atcmd); });
EM_JS(int, js_pending, (void), { return Module.mag.pending(); });
EM_JS(int, js_click, (void), { return Module.mag.click(); });
EM_JS(void, js_flush_keys, (void), { Module.mag.flush(); });
EM_JS(void, js_sync, (void), { Module.mag.sync(); });
EM_JS(void, js_end, (int saved), { Module.mag.end(saved); });
EM_JS(void, fe_beacon, (const char *q), {
	try { q = UTF8ToString(q); if (window.RvipWM && RvipWM.report) RvipWM.report(q);
	else fetch('/roguelikes/beacon?' + q, { keepalive: true, mode: 'no-cors' }).catch(function () {}); } catch (e) {}
});

void
fe_init(void)
{
	static int inited;
	FILE *f;
	char buf[64];

	if (inited)
		return;
	inited = 1;
	if ((f = fopen("web.cfg", "r"))) {
		while (fgets(buf, sizeof buf, f))
			if (!strncmp(buf, "auto_more=", 10))
				fe_auto_more = buf[10] == '1';
		fclose(f);
	}
	js_init(vgafont, NTILES, fe_auto_more);
}

EMSCRIPTEN_KEEPALIVE void
web_set_auto_more(int on)
{
	FILE *f;

	fe_auto_more = on;
	if ((f = fopen("web.cfg", "w"))) {
		fprintf(f, "auto_more=%d\n", on);
		fclose(f);
		js_sync();
	}
}

/* ---- messages, inventory and visible list (tiles-mode windows) ---- */

/* every top-line message (VISUAL1.C pline) */
void
port_msg(const char *m)
{
	static char line[160];
	const unsigned char *p = (const unsigned char *)m;
	char *q = line;

	/* CP437 -> UTF-8 is the page's job; keep ASCII here */
	while (*p && q < line + sizeof line - 1) {
		*q++ = *p < 128 ? *p : '?';
		p++;
	}
	*q = 0;
	if (*line)
		js_msg(line);
}

static char inv_l[MAXINV][81];
static unsigned char inv_at[MAXINV];
static int inv_n;

/* the game waits for a command: fetch what the side windows show (game
   names and colours; form()/obj_str() use static buffers, so only here) */
void
fe_idle(void)
{
	static char vis[8192];
	char *p = vis, *e = vis + sizeof vis - 120;
	OBJECT *o;
	MONSTER *m;
	LEVOBJ *l;
	int i;

	fe_at_cmd = 1;
	for (inv_n = 0, o = inv; o < &inv[numinv] && inv_n < MAXINV; o++, inv_n++) {
		snprintf(inv_l[inv_n], 81, "%s", form(o, NO));
		inv_at[inv_n] = o->o_type + 1 + (o->o_type > 6);	/* pr_obj O_COLORINV */
	}
	*p = 0;
	for (m = mons; m < &mons[nummons] && p < e; m++) {
		int l0 = scrline(m->m_d), c0 = scrcol(m->m_d);
		if (!(m->m_data & M_DEAD) && (unsigned char)optscr[0][l0][c0] == (unsigned char)m->m_perm->p_letter)
			p += sprintf(p, "M%02x%.60s\t%d\n", (unsigned char)m->m_perm->p_letter,
				m->m_perm->p_name, m->m_perm->p_data & 15);
	}
	for (l = lobjs; l < &lobjs[numlobjs] && p < e; l++) {
		int l0 = scrline(l->l_d), c0 = scrcol(l->l_d);
		if ((unsigned char)optscr[0][l0][c0] == (unsigned char)CITEMS[l->l_o.o_type])
			p += sprintf(p, "I%02x%.80s\t%d\n", (unsigned char)CITEMS[l->l_o.o_type],
				obj_str(&l->l_o), obj_color(&l->l_o) & 15);
	}
	for (i = 0; vis[i]; i++)
		if ((unsigned char)vis[i] > 126)
			vis[i] = '?';
	js_vis(vis);
}

/* ---- the frame ---- */
static int map_t[22 * 80], map_u[22 * 80];
static int pop_r0 = -1, pop_c0, pop_r1, pop_c1;

/* bounding box of a full text page (help, tomb, the game's own lists) */
static int
page_bbox(int pg, int *r0, int *c0, int *r1, int *c1)
{
	int r, c, any = 0;

	*r0 = 25; *c0 = 80; *r1 = -1; *c1 = -1;
	for (r = 0; r < 25; r++)
		for (c = 0; c < 80; c++) {
			unsigned short v = vram[pg][r][c];
			if (((v & 0xff) != ' ' && (v & 0xff)) || (v >> 12 & 7)) {
				if (r < *r0) *r0 = r;
				if (r > *r1) *r1 = r;
				if (c < *c0) *c0 = c;
				if (c > *c1) *c1 = c;
				any = 1;
			}
		}
	if (cur_on && any) {
		if (cur_row[pg] < *r0) *r0 = cur_row[pg];
		if (cur_row[pg] > *r1) *r1 = cur_row[pg];
		if (cur_col[pg] < *c0) *c0 = cur_col[pg];
		if (cur_col[pg] + 1 > *c1) *c1 = cur_col[pg] + 1 > 79 ? 79 : cur_col[pg] + 1;
	}
	return any;
}

void
fe_present(void)
{
	int l, c, r0 = -1, c0 = 0, r1 = 0, c1 = 0;

	fe_init();
	if (!port_started) {	/* title, character setup: the text screen */
		js_text(vram[vis_page], cur_row[vis_page], cur_col[vis_page], cur_on);
		return;
	}
	for (l = 1; l <= 22; l++)
		for (c = 0; c < 80; c++)
			map_t[(l - 1) * 80 + c] = tile_for(l, c, &map_u[(l - 1) * 80 + c]);
	if (pop_r0 >= 0)
		r0 = pop_r0, c0 = pop_c0, r1 = pop_r1, c1 = pop_c1;
	else if (vis_page != 0 && !page_bbox(vis_page, &r0, &c0, &r1, &c1))
		r0 = -1;
	js_tiles(vram[0], vram[vis_page], map_t, map_u, inv_l, inv_at, inv_n,
		r0, c0, r1, c1, cur_row[vis_page], cur_col[vis_page], cur_on,
		scrline(u.u_d) - 1, scrcol(u.u_d), u.u_dlevel);
}

int
fe_getkey(int msdelay)
{
	double start = emscripten_get_now();
	int k;

	fe_present();	/* the game relies on the wait to show the screen */
	for (;;) {
		if ((k = js_key(fe_at_cmd)) >= 0) {
			if (k == FK_CLICK) {
				int rc = js_click();

				fe_click_row = rc >> 8;
				fe_click_col = rc & 0xff;
			}
			return k;
		}
		if (msdelay >= 0 && emscripten_get_now() - start >= msdelay)
			return -1;
		emscripten_sleep(10);
	}
}

int
fe_kbhit(void)
{
	static double last;

	/* long computations poll this: let the page paint now and then */
	if (emscripten_get_now() - last > 50) {
		last = emscripten_get_now();
		fe_present();
		emscripten_sleep(0);
	}
	return js_pending();
}

void
fe_flush(void)
{
	js_flush_keys();
}

void
fe_sleep(int ms)
{
	emscripten_sleep(ms);
}

double
fe_now(void)
{
	return emscripten_get_now();
}

void
fe_popup(int r0, int c0, int r1, int c1)
{
	/* tiles mode shows it as a pop-up; text mode draws the cells */
	pop_r0 = r0; pop_c0 = c0; pop_r1 = r1; pop_c1 = c1;
}

void
fe_sync(void)
{
	js_sync();
}

/* the program ends: show the last screen until a key, then the page offers
   a new game (no EXIT_RUNTIME: IndexedDB must stay open for the sync) */
void
fe_exit(int code)
{
	extern int port_saved;

	(void)code;
	fe_getkey(-1);
	js_end(port_saved);
	for (;;)
		emscripten_sleep(1000);
}

int
port_auto_more(void)
{
	return fe_auto_more && port_started;
}

void
port_sound(const char *ev)
{
	if (ev && *ev)
		js_sound(ev);
}
