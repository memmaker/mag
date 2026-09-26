/*
 * fe_tty.c - headless native frontend for tests (RVIP port, 2026)
 *
 * No terminal: keys come from the environment, the screen goes to a file.
 *   MAG_KEYS="..."	keys to type (C escapes: \r \e \n \\ \xNN, \^W = Ctrl-W; \F1..\F10,
 *			\U \D \L \R arrows)
 *   MAG_RANDOM=N	then N random keys (MAG_SEED picks the sequence;
 *			no quit/save keys so the run goes the distance)
 *   MAG_DUMP=file	the visual page as text, rewritten at every key wait
 * When the keys run out the screen is dumped and the program exits 0.
 * Build: port/Makefile (gcc/clang, optionally -fsanitize=address).
 */

#include <ctype.h>
#include <time.h>
#include "port.h"
#include "fe.h"
#undef exit

int fe_click_row, fe_click_col;
static const char *keys;
static long nrandom = 0;
static unsigned seed = 1;
static long nkeys;

static const char CP437[256][4] = {
#include "cp437utf8.h"
};

static void
dump(void)
{
	const char *f = getenv("MAG_DUMP");
	FILE *fp;
	int r, c, end;

	if (!f || !(fp = (fopen)(f, "w")))
		return;
	for (r = 0; r < 25; r++) {
		for (end = 80; end > 0 && (vram[vis_page][r][end - 1] & 0xff) == ' '; end--)
			;
		for (c = 0; c < end; c++)
			fputs(CP437[vram[vis_page][r][c] & 0xff], fp);
		fputc('\n', fp);
	}
	fprintf(fp, "cursor %d,%d %s page %d\n", cur_row[vis_page], cur_col[vis_page],
		cur_on ? "on" : "off", vis_page);
	fclose(fp);
}

void
fe_init(void)
{
	static int done;
	const char *s;

	if (done)
		return;
	done = 1;
	keys = getenv("MAG_KEYS");
	if ((s = getenv("MAG_RANDOM")))
		nrandom = atol(s);
	if ((s = getenv("MAG_SEED")))
		seed = atoi(s);
}

void
fe_present(void)
{
	/* run the tile lookup the web frame does, so ASan checks it too */
	extern int port_started;
	int tile_for(int l, int c, int *under);
	static long sum;
	int l, c, u;

	if (port_started)
		for (l = 1; l <= 22; l++)
			for (c = 0; c < 80; c++)
				sum += tile_for(l, c, &u) + u;
}

static int
next_scripted(void)
{
	int k;

	if (!keys || !*keys)
		return -1;
	if (*keys != '\\')
		return (unsigned char)*keys++;
	keys++;
	switch (k = *keys++) {
	case 'r': return '\r';
	case 'n': return '\n';
	case 'e': return 27;
	case 'b': return 8;
	case 'U': return FK_UP;
	case 'D': return FK_DOWN;
	case 'L': return FK_LEFT;
	case 'R': return FK_RIGHT;
	case '^': return *keys ? *keys++ & 0x1f : -1;
	case 'x': k = (int)strtol(keys, (char **)&keys, 16); return k;
	case 'F': k = (int)strtol(keys, (char **)&keys, 10); return FK_F1 + k - 1;
	case 0: keys--; return -1;
	default: return k;
	}
}

static int
next_random(void)
{
	static const char pool[] =
		"hjklyubnhjklyubnhjklyubnHJKLYUBN12345.:;sfgiIvx>+cdeptqrwzaAFTVoC\r \033/?!\"$%)*,=[]";
	int k;

	if (nrandom == 0)
		return -1;
	nrandom--;
	seed = seed * 1103515245 + 12345;
	k = (seed >> 16) % (sizeof pool - 1 + 12);
	if (k >= (int)sizeof pool - 1)
		return 'a' + (seed >> 8) % 26;	/* item letters */
	return (unsigned char)pool[k];
}

int
fe_getkey(int msdelay)
{
	int k;

	fe_init();
	dump();
	nkeys++;
	if ((k = next_scripted()) >= 0)
		return k;
	if ((k = next_random()) >= 0)
		return k;
	if (msdelay >= 0)
		return -1;
	fe_exit(0);
	return -1;
}

int
fe_kbhit(void)
{
	return 0;	/* redraws are never interrupted in tests */
}

void
fe_flush(void)
{
}

void
fe_sleep(int ms)
{
	(void)ms;	/* tests don't wait for animations */
}

void
fe_popup(int r0, int c0, int r1, int c1)
{
}

void
fe_sync(void)
{
}

double
fe_now(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

void
fe_exit(int code)
{
	dump();
	fprintf(stderr, "mag: exit %d after %ld keys\n", code, nkeys);
	exit(code);
}

/* tiles-mode windows exist only on the web */
int fe_at_cmd;
void fe_idle(void) {}
void port_msg(const char *m) { (void)m; }
int port_auto_more(void) { return 0; }
void port_sound(const char *ev) { (void)ev; }
void fe_beacon(const char *q) { if (getenv("MAG_BEACON")) fprintf(stderr, "beacon %s\n", q); }
