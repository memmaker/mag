/*
 * pcvideo.c - the IBM PC that MAG was written for, emulated (RVIP port, 2026)
 *
 * MAG draws through BIOS video service 16 (set mode, cursor shape and
 * position, scroll/clear, write char+attribute) on two text pages, flips
 * pages with _setvisualpage() and reads keys through BIOS service 0x16.
 * Here the two pages are arrays of (attribute << 8 | CP437 character),
 * exactly what video memory at B800:0000 held; the frontend (fe_web.c,
 * fe_tty.c) shows the visual page and queues keys. Like Rogue PC's
 * port/pcvideo.c, but at the BIOS level because MAG calls int86() itself.
 */

#include <ctype.h>
#include <dirent.h>
#include <fnmatch.h>
#include "port.h"
#include "fe.h"

unsigned short vram[2][25][80];
int vis_page = 0, cur_on = 1, cur_row[2], cur_col[2];

/* ---- BIOS video, int 10h ------------------------------------------------ */
static void
scroll_up(int page, int n, int attr, int r0, int c0, int r1, int c1)
{
	int r, c;

	if (r1 > 24) r1 = 24;
	if (c1 > 79) c1 = 79;
	for (r = r0; r <= r1; r++)
		for (c = c0; c <= c1; c++)
			vram[page][r][c] = (n == 0 || r + n > r1)
				? (attr << 8 | ' ') : vram[page][r + n][c];
}

static int
video(union REGS *r)
{
	int page = r->h.bh & 1, n;

	switch (r->h.ah) {
	case 0:		/* set mode: clears both pages */
		memset(vram, 0, sizeof vram);
		for (page = 0; page < 2; page++)
			scroll_up(page, 0, 7, 0, 0, 24, 79);
		break;
	case 1:		/* cursor shape: bit 5 of the start line hides it */
		cur_on = !(r->h.ch & 0x20);
		break;
	case 2:		/* cursor position */
		cur_row[page] = r->h.dh < 25 ? r->h.dh : 24;
		cur_col[page] = r->h.dl < 80 ? r->h.dl : 79;
		break;
	case 6:		/* scroll up the displayed page (al = 0: clear) */
		scroll_up(vis_page, r->h.al, r->h.bh, r->h.ch, r->h.cl, r->h.dh, r->h.dl);
		break;
	case 9:		/* write char + attribute cx times at the page's cursor */
		for (n = 0; n < r->x.cx && cur_col[page] + n < 80; n++)
			vram[page][cur_row[page]][cur_col[page] + n] = r->h.bl << 8 | r->h.al;
		break;
	}
	return 0;
}

/* ---- BIOS keyboard, int 16h ---------------------------------------------- */
/* extended keys -> IBM scan codes (al = 0, ah = scan) */
static const struct { int key, scan; } xkeys[] = {
	{FK_HOME, 0x47}, {FK_UP, 0x48}, {FK_PGUP, 0x49}, {FK_LEFT, 0x4b},
	{FK_RIGHT, 0x4d}, {FK_END, 0x4f}, {FK_DOWN, 0x50}, {FK_PGDN, 0x51},
	{FK_INS, 0x52}, {FK_DEL, 0x53},
	{FK_KP7, 0x47}, {FK_KP8, 0x48}, {FK_KP9, 0x49}, {FK_KP4, 0x4b},
	{FK_KP6, 0x4d}, {FK_KP1, 0x4f}, {FK_KP2, 0x50}, {FK_KP3, 0x51},
	{FK_KP0, 0x52}, {FK_KPDOT, 0x53},
	{FK_F1, 0x3b}, {FK_F2, 0x3c}, {FK_F3, 0x3d}, {FK_F4, 0x3e},
	{FK_F5, 0x3f}, {FK_F6, 0x40}, {FK_F7, 0x41}, {FK_F8, 0x42},
	{FK_F9, 0x43}, {FK_F10, 0x44},
};

/* next key as the BIOS returns it: ah = scan code, al = ASCII (0: extended) */
static unsigned
bios_key(void)
{
	int k;
	unsigned i;

	for (;;) {
		k = fe_getkey(-1);
		if (k == FK_KP5)	k = '.';
		if (k == FK_KPENTER)	k = '\r';
		if (k == FK_KPPLUS)	k = '+';
		if (k == FK_KPMINUS)	k = '-';
		if (k == FK_KPSTAR)	k = '*';
		if (k == FK_KPSLASH)	k = '/';
		if (k == '\n')		k = '\r';
		if (k >= 0 && k < 0x100)
			return k;
		for (i = 0; i < sizeof xkeys / sizeof *xkeys; i++)
			if (xkeys[i].key == k)
				return xkeys[i].scan << 8;
	}
}

int
int86(int intno, union REGS *in, union REGS *out)
{
	unsigned k;

	if (out != in)
		*out = *in;
	if (intno == 0x10)
		return video(out);
	if (intno == 0x16 && in->h.ah == 0) {
		k = bios_key();
		out->h.al = k & 0xff;
		out->h.ah = k >> 8;
	}
	return 0;
}

short
_setvisualpage(short page)
{
	short old = vis_page;

	vis_page = page & 1;
	return old;
}

/* ---- conio --------------------------------------------------------------- */
int
kbhit(void)
{
	return fe_kbhit();
}

int
getch(void)
{
	return bios_key() & 0xff;
}

/* ---- time ------------------------------------------------------------------ */
void
port_delay(int ticks)
{
	fe_present();
	fe_sleep(ticks * 55);	/* 18.2 ticks per second */
}

/* ---- DOS paths ------------------------------------------------------------- */
#undef fopen
#undef open
#undef access
#undef remove

/* game data (help, pics) are read-only and may live elsewhere (MAG_DATA);
   everything else (save/, options.mag, heroes.mag) stays in the cwd */
const char *
port_path(const char *dos)
{
	static char buf[4][256];
	static int n;
	const char *data = getenv("MAG_DATA");
#ifdef __EMSCRIPTEN__
	if (!data)
		data = "/magdata";	/* preloaded by web/build.sh */
#endif
	char *p = buf[n = (n + 1) & 3], *q;

	if (data && (!strncasecmp(dos, "help\\", 5) || !strncasecmp(dos, "pics\\", 5)))
		snprintf(p, 256, "%s/%s", data, dos);
	else
		snprintf(p, 256, "%s", dos);
	for (q = p; *q; q++)
		if (*q == '\\')
			*q = '/';
	return p;
}

static DIR *find_dir;
static char find_pat[64];

int
_dos_findnext(struct find_t *ft)
{
	struct dirent *e;

	if (!find_dir)
		return 1;
	while ((e = readdir(find_dir)))
		if (!fnmatch(find_pat, e->d_name, 0)) {
			snprintf(ft->name, sizeof ft->name, "%s", e->d_name);
			return 0;
		}
	closedir(find_dir);
	find_dir = NULL;
	return 1;
}

int
_dos_findfirst(const char *pattern, unsigned attr, struct find_t *ft)
{
	char dir[256], *s;

	snprintf(dir, sizeof dir, "%s", port_path(pattern));
	if ((s = strrchr(dir, '/'))) {
		*s = 0;
		snprintf(find_pat, sizeof find_pat, "%s", s + 1);
	} else {
		snprintf(find_pat, sizeof find_pat, "%s", dir);
		strcpy(dir, ".");
	}
	if (find_dir)
		closedir(find_dir);
	if (!(find_dir = opendir(dir)))
		return 1;
	return _dos_findnext(ft);
}
