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

/* ---- auto-explore (Z) and walking to stairs (< >), RVIP steps 2 and 3 ----
 *
 * Both walk one step per turn through the game's own walk command (a
 * direction key fed into dispatch() by port_auto()). The known grid is what
 * the player has seen: D_SEEN cells and whatever the game has drawn on its
 * page-0 shadow screen (optscr), remembered per level because MAG forgets
 * the floor of dark rooms when you leave them (know(d, NO)).
 * Targets: known passable cells next to unknown space that the player has
 * not stood on yet, and items not yet stood on. Avoids known traps, water,
 * monsters and magically locked doors; stuck doors are pushed open by the
 * walk itself. Stops on a new message, a monster coming into view, any key,
 * or a step that did not move the player.
 */
#define EXPLORE_KEY	'Z'
static const char dirkeys[NUMDIRS] = { 'k', 'u', 'l', 'n', 'j', 'b', 'h', 'y' };
static const int dl[NUMDIRS] = { -1, -1, 0, 1, 1, 1, 0, -1 };
static const int dc[NUMDIRS] = { 0, 1, 1, 1, 0, -1, -1, -1 };

static unsigned char known[LINES][COLUMNS], stood[LINES][COLUMNS];
static int auto_mode;		/* 0 off, 'Z' explore, '<' '>' walk to stairs */
static int grid_level = -1;
static DUNGEON *last_pos;
static char last_msg[85];
static unsigned char seen_mon[MAXMONS];

static void
update_known(void)
{
	int l, c;
	DUNGEON *d;

	if (grid_level != u.u_dlevel || (u.u_data & UD_LABYRINTH)) {
		/* a new level (a labyrinth changes under the same number) */
		memset(known, 0, sizeof known);
		memset(stood, 0, sizeof stood);
		grid_level = (u.u_data & UD_LABYRINTH) ? -2 : u.u_dlevel;
	}
	for (l = 1; l < LINES - 1; l++)
		for (c = 0; c < COLUMNS; c++) {
			d = &dun[l][c];
			if ((d->d_data & D_SEEN) || (visual_page == 0 &&
					(unsigned char)optscr[0][l][c] != ' '))
				known[l][c] = 1;
		}
	stood[scrline(u.u_d)][scrcol(u.u_d)] = 1;
}

/* can the walk step onto d (known cells only) */
static int
passable(int l, int c)
{
	DUNGEON *d;

	if (l < 1 || l >= LINES - 1 || c < 0 || c >= COLUMNS || !known[l][c])
		return NO;
	d = &dun[l][c];
	if (d->d_data & D_STONE)
		return NO;
	if (d->d_what == TRAPC || d->d_what == POOL)
		return NO;	/* known trap, water */
	if ((d->d_data & D_DOOR) && (what_door(d)->dr_data & DR_WITHLOCK))
		return NO;	/* magically locked: never picked */
	return YES;
}

/* the diagonal rule of move() (nodiagmove) */
static int
diag_ok(int l, int c, int dir)
{
	DUNGEON *d = &dun[l][c];

	if (!(dir % 2))
		return YES;
	return !(mvindir((dir + 7) % 8, d, 1)->d_data & (D_DOOR | D_STONE) ||
		mvindir((dir + 1) % 8, d, 1)->d_data & (D_DOOR | D_STONE));
}

static int
is_target(int l, int c)
{
	DUNGEON *d = &dun[l][c];
	int i, nl, nc;

	if (auto_mode == '>' || auto_mode == '<')
		return (d->d_data & D_STAIRCASE) && known[l][c] &&
			d->d_what == (auto_mode == '>' ? DNSTAIR : UPSTAIR);
	if (stood[l][c])
		return NO;
	if ((d->d_data & D_OBJECT) && strchr(CITEMS, optscr[0][l][c]))
		return YES;	/* an item on the floor */
	for (i = 0; i < NUMDIRS; i++) {
		nl = l + dl[i];
		nc = c + dc[i];
		if (nl >= 1 && nl < LINES - 1 && nc >= 0 && nc < COLUMNS && !known[nl][nc])
			return YES;
	}
	return NO;
}

/* first step (direction) of the shortest path to the nearest target */
static int
next_dir(void)
{
	static short from[LINES][COLUMNS];
	static int queue[LINES * COLUMNS];
	int head = 0, tail = 0, l0 = scrline(u.u_d), c0 = scrcol(u.u_d);
	int p, l, c, i, nl, nc;

	memset(from, -1, sizeof from);
	from[l0][c0] = 8;
	queue[tail++] = l0 * COLUMNS + c0;
	while (head < tail) {
		p = queue[head++];
		l = p / COLUMNS;
		c = p % COLUMNS;
		if (p != l0 * COLUMNS + c0 && is_target(l, c)) {
			/* walk back to the first step */
			while (from[l][c] != 8) {
				i = from[l][c];
				nl = l - dl[i];
				nc = c - dc[i];
				if (nl == l0 && nc == c0)
					return i;
				l = nl;
				c = nc;
			}
			return -1;
		}
		for (i = 0; i < NUMDIRS; i++) {
			nl = l + dl[i];
			nc = c + dc[i];
			if (nl < 1 || nl >= LINES - 1 || nc < 0 || nc >= COLUMNS ||
					from[nl][nc] != -1 || !passable(nl, nc) ||
					!diag_ok(l, c, i))
				continue;
			from[nl][nc] = i;
			queue[tail++] = nl * COLUMNS + nc;
		}
	}
	return -1;
}

/* monsters in view now; returns YES if one came into view */
static int
new_monster(int reset)
{
	MONSTER *m;
	int n, newone = NO;

	for (m = mons, n = 0; m < &mons[nummons]; m++, n++) {
		int vis = !(m->m_data & M_DEAD) && insight(m->m_d) &&
			lookmon(m) == m->m_perm->p_letter;
		if (vis && !seen_mon[n] && !reset)
			newone = YES;
		seen_mon[n] = vis;
	}
	return newone;
}

/* the first step would attack a monster: stop and say so */
static int
blocked(int dir)
{
	DUNGEON *d = mvindir(dir, u.u_d, 1);

	if ((d->d_data & D_MONSTER) && insight(d)) {
		pline("There's %s in the way.", article(mname(what_mon(d)), NO));
		return YES;
	}
	return NO;
}

static void
auto_stop(void)
{
	auto_mode = 0;
}

void
port_explore_stop(void)
{
	auto_stop();
}

/* a command key typed at the command prompt (MOVE.C dispatch):
 * returns the key to run, or 0 when the port handled it */
int
port_command(int k)
{
	int dir, want;

	auto_stop();
	if (k != EXPLORE_KEY && k != '>' && k != '<')
		return k;
	if (k != EXPLORE_KEY) {
		want = k == '>' ? DNSTAIR : UPSTAIR;
		if (u.u_d->d_what == want || wizard == YES || u.u_dlevel == 0)
			return k;	/* on the stairs: take them */
	}
	if (blind() || confused()) {
		pline("You can't find your way like this.");
		return 0;
	}
	update_known();
	auto_mode = k;
	if ((dir = next_dir()) < 0) {
		auto_stop();
		if (k == EXPLORE_KEY)
			pline("There is nothing left here that you can reach.");
		else
			pline("You don't know where a%s staircase is.",
				k == '>' ? " down" : "n up");
		return 0;
	}
	if (blocked(dir)) {
		auto_stop();
		return 0;
	}
	new_monster(YES);
	strcpy(last_msg, message);
	last_pos = u.u_d;
	return dirkeys[dir];
}

/* the next key while exploring or walking to stairs, 0 = ask the player */
int
port_auto(void)
{
	int dir, want;

	if (!auto_mode)
		return 0;
	update_known();
	if (fe_kbhit() || (message[0] && strcmp(message, last_msg)) || new_monster(NO) ||
			u.u_d == last_pos || blind() || confused()) {
		auto_stop();
		return 0;
	}
	if (auto_mode == '>' || auto_mode == '<') {
		want = auto_mode == '>' ? DNSTAIR : UPSTAIR;
		if (u.u_d->d_what == want) {
			dir = auto_mode;
			auto_stop();
			if (dir == '<' && u.u_dlevel == 1) {
				/* never walk out of the dungeon by accident */
				pline("These stairs lead out of the dungeon. Press < to leave.");
				return 0;
			}
			return dir;	/* arrived: take the stairs */
		}
	}
	if ((dir = next_dir()) < 0) {
		if (auto_mode == EXPLORE_KEY)
			pline("Explored everything you can reach.");
		auto_stop();
		return 0;
	}
	if (blocked(dir)) {
		auto_stop();
		return 0;
	}
	last_pos = u.u_d;
	strcpy(last_msg, message);
	fe_present();
	fe_sleep(25);		/* let the walk be seen */
	return dirkeys[dir];
}
