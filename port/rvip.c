/*
 * rvip.c - what the RVIP port adds to MAG's game side (2026)
 *
 * Autosave: the original saves only with R, which also ends the program,
 * and deletes the save when it restores it. The port writes the same files
 * (save\savefile.mag + the level file) while the game waits for a command,
 * at most every 2 seconds, and removes them when the game ends unless the
 * player saved with R. The files hold raw pointers into the program's
 * static data (as they did on DOS), so they only fit the build that wrote
 * them: save\build.id records the addresses they depend on.
 */

#include "mag.h"
#include "fe.h"
#include <stdint.h>
#include <time.h>
#undef exit

extern int	openfile;
extern char	wizpword[];
int		port_saved;	/* R saved the game: keep the files on exit */
static int	port_started;	/* a game (new or restored) is on screen */

static void
stamp(char *buf, int n)
{
	snprintf(buf, n, "MAG PC-1.1 RVIP %lx %lx %lx %lx %lx %d\n",
		(unsigned long)(uintptr_t)pmon, (unsigned long)(uintptr_t)dun,
		(unsigned long)(uintptr_t)regen, (unsigned long)(uintptr_t)&u,
		(unsigned long)(uintptr_t)pobj, (int)sizeof(MONSTER));
}

/* the save was written by this build (SAVE.C rest_game) */
int
port_save_ok(void)
{
	char want[160], have[160] = "";
	char path[64];
	FILE *f;

	stamp(want, sizeof want);
	snprintf(path, sizeof path, "%s\\build.id", u.u_savedir);
	if ((f = fopen(path, "r"))) {
		if (!fgets(have, sizeof have, f))
			*have = 0;
		fclose(f);
	}
	if (!strcmp(want, have))
		return YES;
	/* a save from another build would restore garbage pointers */
	snprintf(path, sizeof path, "%s\\savefile.mag", u.u_savedir);
	if (!access(path, 0)) {
		remove(path);
		clr_save();
	}
	return NO;
}

static void
write_stamp(void)
{
	char buf[160], path[64];
	FILE *f;

	stamp(buf, sizeof buf);
	snprintf(path, sizeof path, "%s\\build.id", u.u_savedir);
	if ((f = fopen(path, "w"))) {
		fputs(buf, f);
		fclose(f);
	}
}

/* what remember() writes, without asking and without leaving (SAVE.C) */
int
port_save_game(void)
{
	char path[64];
	int i, len;

	if (u.u_stuckmon || !u.u_dlevel)
		return NO;
	save_lev();
	snprintf(path, sizeof path, "%s\\savefile.mag", u.u_savedir);
	if ((openfile = open(path, (O_CREAT | O_TRUNC | O_WRONLY | O_BINARY),
			(S_IREAD | S_IWRITE))) == -1)
		return NO;
	dowrite(&u, sizeof (struct you));
	dowrite(&numevents, sizeof (numevents));
	dowrite(events, sizeof (struct event) * numevents);
	dowrite(genostr, sizeof (genostr));
	dowrite(&oldexp, sizeof (oldexp));
	dowrite(&oldelev, sizeof (oldelev));
	dowrite(&wizard, sizeof (wizard));
	dowrite(ostats, sizeof (ostats));
	dowrite(nameptr, sizeof (nameptr));
	for (i = 0; i < NUMOBJ; i++)
		if (ostats[i] == CALLED) {
			len = strlen(nameptr[i][0]) + 1;
			dowrite(&len, sizeof (len));
			dowrite(nameptr[i][0], len);
		}
	dowrite(&numinv, sizeof (numinv));
	dowrite(inv, sizeof (OBJECT) * numinv);
	close(openfile);
	write_stamp();
	fe_sync();
	return YES;
}

/* the game waits for a command (MOVE.C dispatch) */
void
port_idle(void)
{
	static double last;
	double now = fe_now();

	port_started = 1;
	if (now - last >= 2000 && !fe_kbhit()) {
		last = now;
		port_save_game();
	}
}

/* R: the save stays (SAVE.C remember) */
void
port_remember(void)
{
	port_saved = 1;
	write_stamp();
}

/* the program ends: death, quit, escape, R */
void
port_exit(int code)
{
	char path[64];

	if (port_started && !port_saved) {
		snprintf(path, sizeof path, "%s\\savefile.mag", u.u_savedir);
		remove(path);
		clr_save();
	}
	fe_exit(code);
}
