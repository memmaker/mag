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
#include "mag.h"
#include "tiles.h"
#undef exit

#define PALS "#000000,#0000aa,#00aa00,#00aaaa,#aa0000,#aa00aa,#aa5500,#aaaaaa,#555555,#5555ff,#55ff55,#55ffff,#ff5555,#ff55ff,#ffff55,#ffffff"

int fe_click_row, fe_click_col;
int fe_at_cmd;			/* the game waits for a command (prompt line) */
int fe_auto_more = 1;		/* top-line =-More-= needs no key (web option) */
static int fe_icons = 1;
static int fe_textmode;	/* the page shows the PC screen (Tiles: PC screen) */	/* the page shows tiles (not Tiles: None): item icons */
extern int port_started;

int tile_for(int l, int c, int *under);

EM_JS(void, js_init, (int ntiles, int auto_more, const unsigned char *anim, const char *pal),
	{ Module.mag.init(ntiles, auto_more, anim, UTF8ToString(pal)); });
EM_JS(void, js_map, (const void *scr, const int *t, const int *u, int hy, int hx, int lvl),
	{ Module.mag.map(scr, t, u, hy, hx, lvl); });
EM_JS(void, js_line, (int p, int y, const char *s, const char *c, int t),
	{ Module.mag.line(p, y, UTF8ToString(s), UTF8ToString(c), t); });
EM_JS(void, js_rows, (int p, int n), { Module.mag.rows(p, n); });
EM_JS(void, js_cursor, (int p, int y, int x), { Module.mag.cursor(p, y, x); });
EM_JS(void, js_popup, (int rows, int cols, int r0, int c0, const char *bg),
	{ Module.mag.popup(rows, cols, r0, c0, UTF8ToString(bg)); });
EM_JS(void, js_prompt, (const char *s), { Module.mag.prompt(UTF8ToString(s)); });
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
	js_init(NTILES, fe_auto_more, tile_anim, PALS);
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

/* Tiles: DawnLike or None (text map): the inventory rows change with it */
EMSCRIPTEN_KEEPALIVE void
web_set_icons(int on)
{
	fe_icons = on;
}

/* Tiles: PC screen: the whole visual page goes to the text screen window */
EMSCRIPTEN_KEEPALIVE void
web_set_textmode(int on)
{
	fe_textmode = on;
}

/* tests (Module.mag.screen): the visual page */
EMSCRIPTEN_KEEPALIVE void *
web_vram(void)
{
	return vram[vis_page];
}

/* ---- text windows (RVIP W0 rules 5, 6): HTML lines from the game ----
 * Each pane's rows go out as whole lines, once per change, trimmed (no
 * trailing blanks, no empty rows at the bottom): a cell whose CGA colours
 * are not the default light grey on black starts a run "\x05#fg" or
 * "\x05#fg/#bg" (up to "\x06"), with a row colour ("" default) and an icon
 * tile (-1 none). CP437 goes out as UTF-8. */
enum { P_MAP, P_MSG, P_STAT, P_INV, P_POP, P_TEXT, NPANES };
#define RMAX 512
static const char *pal[16] = { "#000000", "#0000aa", "#00aa00", "#00aaaa", "#aa0000", "#aa00aa", "#aa5500", "#aaaaaa",
	"#555555", "#5555ff", "#55ff55", "#55ffff", "#ff5555", "#ff55ff", "#ffff55", "#ffffff" };
static const char *CP437[256] = {
#include "cp437utf8.h"
};
static char *sent[NPANES][RMAX];
static const char *sent_css[NPANES][RMAX];
static int sent_tile[NPANES][RMAX], rows_sent[NPANES];
static int cur_p = -1, cur_y, cur_x, want_p = -1, want_y, want_x;

static void
be_line(int p, int y, const char *s, const char *css, int tile)
{
	if (y < 0 || y >= RMAX)
		return;
	if (sent[p][y] && !strcmp(sent[p][y], s) && sent_css[p][y] == css && sent_tile[p][y] == tile)
		return;
	free(sent[p][y]);
	sent[p][y] = strdup(s);
	sent_css[p][y] = css;
	sent_tile[p][y] = tile;
	js_line(p, y, s, css, tile);
}

static void
be_rows(int p, int n)
{
	if (n != rows_sent[p])
		js_rows(p, rows_sent[p] = n);
}

/* the page empties the pane: forget what it had */
static void
forget(int p)
{
	int y;

	for (y = 0; y < RMAX; y++) {
		free(sent[p][y]);
		sent[p][y] = NULL;
	}
	rows_sent[p] = 0;
}

/* the one text cursor (p < 0: none): set while a frame is built, sent
   after its lines (cursor_send) */
static void
set_cursor(int p, int y, int x)
{
	want_p = p; want_y = y; want_x = x;
}

static void
cursor_send(void)
{
	if (want_p == cur_p && (want_p < 0 || (want_y == cur_y && want_x == cur_x)))
		return;
	cur_p = want_p; cur_y = want_y; cur_x = want_x;
	js_cursor(cur_p, cur_y, cur_x);
}

static int
blank(unsigned short v)
{
	int g = v & 0xff;

	return (g == ' ' || !g) && !(v >> 12 & 7);
}

/* n cells (attribute << 8 | CP437) as a trimmed line with colour runs */
static char *
cells(const unsigned short *v, int n, char *out)
{
	char *q = out, run[24] = "", r[24];
	int i, end = n;

	while (end > 0 && blank(v[end - 1]))
		end--;
	for (i = 0; i < end; i++) {
		int a = v[i] >> 8, fg = a & 15, bg = a >> 4 & 7;

		*r = 0;
		if (fg != 7 || bg)
			sprintf(r, "\x05%s%s%s", pal[fg], bg ? "/" : "", bg ? pal[bg] : "");
		if (blank(v[i]))
			strcpy(r, run);		/* a blank doesn't break a run */
		if (strcmp(r, run)) {
			if (*run)
				*q++ = 6;
			q += sprintf(q, "%s", r);
			strcpy(run, r);
		}
		q += sprintf(q, "%s", CP437[v[i] & 0xff]);
	}
	if (*run)
		*q++ = 6;
	*q = 0;
	return out;
}

/* a plain line (messages, inventory: game strings in CP437) */
static char *
plain(const char *s, char *out)
{
	char *q = out;
	const unsigned char *p = (const unsigned char *)s;
	char *e;

	while (*p)
		q += sprintf(q, "%s", CP437[*p++]);
	*q = 0;
	for (e = q; e > out && e[-1] == ' '; )
		*--e = 0;
	return out;
}

/* ---- Messages: the game's top-line history, repeats folded "(xN)" ---- */
#define NHIST 400
static char hist[NHIST][96];
static int nhist, hist_n[NHIST];

/* every top-line message (VISUAL1.C pline) */
void
port_msg(const char *m)
{
	char line[96];

	snprintf(line, sizeof line, "%s", m);
	{ char *e = line + strlen(line); while (e > line && e[-1] == ' ') *--e = 0; }
	if (!*line)
		return;
	if (nhist && !strcmp(hist[nhist - 1], line)) {
		hist_n[nhist - 1]++;
		return;
	}
	if (nhist == NHIST) {		/* drop the oldest 100 at once */
		memmove(hist, hist + 100, sizeof hist[0] * (NHIST - 100));
		memmove(hist_n, hist_n + 100, sizeof hist_n[0] * (NHIST - 100));
		nhist -= 100;
	}
	strcpy(hist[nhist], line);
	hist_n[nhist++] = 1;
}

static void
send_msg(int live_row)
{
	static char buf[RMAX * 4], live[400], liveu[1200];
	unsigned short *v = vram[vis_page][0];
	int i, n = 0, end;

	for (i = 0; i < nhist; i++) {
		char s[120];

		if (hist_n[i] > 1)
			snprintf(s, sizeof s, "%s (x%d)", hist[i], hist_n[i]);
		else
			snprintf(s, sizeof s, "%s", hist[i]);
		be_line(P_MSG, n++, plain(s, buf), i == nhist - 1 ? pal[15] : pal[7], -1);
	}
	/* the live message line: prompts, =-More-= */
	*live = 0;
	if (live_row) {
		for (end = 80; end > 0 && blank(v[end - 1]); end--)
			;
		for (i = 0; i < end; i++)
			live[i] = (v[i] & 0xff) ? v[i] & 0xff : ' ';
		live[end] = 0;
	}
	js_prompt(plain(live, liveu));
	if (*live && (!nhist || strcmp(live, hist[nhist - 1]))) {
		be_line(P_MSG, n, liveu, pal[14], -1);
		if (cur_on && cur_row[vis_page] == 0)
			set_cursor(P_MSG, n, cur_col[vis_page]);
		n++;
	}
	be_rows(P_MSG, n);
}

/* ---- Inventory and Visible (the game waits for a command) ---- */
static char inv_l[MAXINV][81];
static unsigned char inv_at[MAXINV];
static int inv_t[MAXINV];	/* the item's sprite, -1 in text mode */
int obj_sprite(OBJECT *o);
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
		/* icons: "a) name" after the item's sprite (the page puts it
		   first); text: "a) ! name", the item's own symbol */
		char *f = form(o, NO);
		if (fe_icons)
			snprintf(inv_l[inv_n], 81, "%s", f);
		else
			snprintf(inv_l[inv_n], 81, "%.2s %c %s", f, CITEMS[o->o_type], f + 3);
		inv_t[inv_n] = fe_icons ? obj_sprite(o) : -1;
		inv_at[inv_n] = o->o_type + 1 + (o->o_type > 6);	/* pr_obj O_COLORINV */
	}
	*p = 0;
	for (m = mons; m < &mons[nummons] && p < e; m++) {
		int l0 = scrline(m->m_d), c0 = scrcol(m->m_d);
		if (!(m->m_data & M_DEAD) && (unsigned char)optscr[0][l0][c0] == (unsigned char)m->m_perm->p_letter)
			p += sprintf(p, "M%02x%.60s\t%s\t%d\n", (unsigned char)m->m_perm->p_letter,
				m->m_perm->p_name, pal[m->m_perm->p_data & 15], mon_tile[m->m_perm - pmon]);
	}
	for (l = lobjs; l < &lobjs[numlobjs] && p < e; l++) {
		int l0 = scrline(l->l_d), c0 = scrcol(l->l_d);
		if ((unsigned char)optscr[0][l0][c0] == (unsigned char)CITEMS[l->l_o.o_type])
			p += sprintf(p, "I%02x%.80s\t%s\t%d\n", (unsigned char)CITEMS[l->l_o.o_type],
				obj_str(&l->l_o), pal[obj_color(&l->l_o) & 15], obj_sprite(&l->l_o));
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

/* rows r0..r1, columns c0..c1 of the visual page into pane p */
static void
send_page(int p, int r0, int c0, int r1, int c1)
{
	static char buf[80 * 32];
	int r, used = 0;

	for (r = r0; r <= r1; r++) {
		be_line(p, r - r0, cells(&vram[vis_page][r][c0], c1 - c0 + 1, buf), "", -1);
		if (*buf)
			used = r - r0 + 1;
	}
	if (cur_on && cur_row[vis_page] >= r0 && cur_row[vis_page] <= r1 &&
	    cur_col[vis_page] >= c0 && cur_col[vis_page] <= c1) {
		set_cursor(p, cur_row[vis_page] - r0, cur_col[vis_page] - c0);
		if (cur_row[vis_page] - r0 >= used)
			used = cur_row[vis_page] - r0 + 1;
	}
	be_rows(p, used);
}

void
fe_present(void)
{
	static char buf[80 * 32];
	static int pr0 = -2, pc0, pr1, pc1;
	int l, c, r0 = -1, c0 = 0, r1 = 0, c1 = 0;

	fe_init();
	want_p = -1;
	if (!port_started || fe_textmode) {	/* title, setup, PC screen */
		send_page(P_TEXT, 0, 0, 24, 79);
		cursor_send();
		if (!port_started)
			return;
	}
	for (l = 1; l <= 22; l++)
		for (c = 0; c < 80; c++)
			map_t[(l - 1) * 80 + c] = tile_for(l, c, &map_u[(l - 1) * 80 + c]);
	js_map(vram[0], map_t, map_u, scrline(u.u_d) - 1, scrcol(u.u_d), u.u_dlevel);
	if (fe_textmode)
		return;
	want_p = -1;
	if (pop_r0 >= 0)
		r0 = pop_r0, c0 = pop_c0, r1 = pop_r1, c1 = pop_c1;
	else if (vis_page != 0 && !page_bbox(vis_page, &r0, &c0, &r1, &c1))
		r0 = -1;
	/* the pop-up: a port box or a full text page, at the original's place */
	if (r0 != pr0 || c0 != pc0 || r1 != pr1 || c1 != pc1) {
		pr0 = r0; pc0 = c0; pr1 = r1; pc1 = c1;
		forget(P_POP);
		if (cur_p == P_POP)
			cur_p = -1;	/* the page dropped it with the rows */
		js_popup(r0 < 0 ? 0 : r1 - r0 + 1, c1 - c0 + 1, r0, c0,
			r0 < 0 ? "" : pal[vram[vis_page][r0][c0] >> 12 & 7]);
	}
	if (r0 >= 0)
		send_page(P_POP, r0, c0, r1, c1);
	send_msg(!(r0 == 0));
	be_line(P_STAT, 0, cells(vram[0][23], 80, buf), "", -1);	/* MAG's status lines */
	be_line(P_STAT, 1, cells(vram[0][24], 80, buf), "", -1);
	be_rows(P_STAT, *buf ? 2 : 1);
	for (l = 0; l < inv_n; l++)
		be_line(P_INV, l, plain(inv_l[l], buf), pal[inv_at[l] & 15], inv_t[l]);
	be_rows(P_INV, inv_n);
	cursor_send();
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
