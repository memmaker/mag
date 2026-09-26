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
#undef exit

int fe_click_row, fe_click_col;

EM_JS(void, js_init, (const void *font), { Module.mag.init(font); });
EM_JS(void, js_text, (const void *scr, int cr, int cc, int con),
	{ Module.mag.text(scr, cr, cc, con); });
EM_JS(int, js_key, (void), { return Module.mag.key(); });
EM_JS(int, js_pending, (void), { return Module.mag.pending(); });
EM_JS(int, js_click, (void), { return Module.mag.click(); });
EM_JS(void, js_flush_keys, (void), { Module.mag.flush(); });
EM_JS(void, js_sync, (void), { Module.mag.sync(); });
EM_JS(void, js_end, (int saved), { Module.mag.end(saved); });

void
fe_init(void)
{
	static int inited;

	if (inited)
		return;
	inited = 1;
	js_init(vgafont);
}

void
fe_present(void)
{
	fe_init();
	js_text(vram[vis_page], cur_row[vis_page], cur_col[vis_page], cur_on);
}

int
fe_getkey(int msdelay)
{
	double start = emscripten_get_now();
	int k;

	fe_present();	/* the game relies on the wait to show the screen */
	for (;;) {
		if ((k = js_key()) >= 0) {
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
	/* tiles mode shows it as a pop-up (stage 5); text mode draws the cells */
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
