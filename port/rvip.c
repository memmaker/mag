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
#include <string.h>
#include <ctype.h>
#undef exit

extern int	openfile;
extern char	wizpword[];
int		port_saved;	/* R saved the game: keep the files on exit */
int		port_started;	/* a game (new or restored) is on screen */
extern int	fe_at_cmd;
void		fe_idle(void);

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
	while (port_pop_key() >= 0)
		;	/* an item letter no command took */
	if (now - last >= 2000 && !fe_kbhit()) {
		last = now;
		port_save_game();
	}
	fe_idle();	/* side windows; the prompt line waits for a command */
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
	port_started = 0;	/* the last screen (Hall of Heroes) is text */
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

	fe_at_cmd = 0;
	auto_stop();
	if (k == '\r' && wizard != YES)
		k = port_menu();	/* Enter: the command menu (RVIP 3b) */
	if (k == 'i' || k == 128 + 0x3d)	/* i, F3: inventory with a cursor */
		k = port_inventory();
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

	if (port_inv_again) {
		/* after an item action the inventory comes back, unless a
		   monster is in view (RVIP 3c) */
		port_inv_again = 0;
		new_monster(YES);
		for (dir = 0; dir < nummons; dir++)
			if (seen_mon[dir])
				return 0;
		return port_command('i');
	}
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
			/* arrived: stop here; the player presses the key again to
			   take the stairs (RVIP finetuning: auto-stairs only walks) */
			dir = auto_mode;
			auto_stop();
			if (dir == '<' && u.u_dlevel == 1)
				pline("These stairs lead out of the dungeon. Press < to leave.");
			return 0;
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
	fe_sleep(40);		/* let each step be seen */
	fe_at_cmd = 0;
	return dirkeys[dir];
}

/* ---- sound (RVIP R7): item commands name their event ---- */
void
port_sound_verb(const char *verb)
{
	static const char *map[][2] = {
		{ "quaff", "quaff" }, { "eat", "eat" }, { "zap", "zap" },
		{ "put on/remove", "ring" }, { "read", "scroll" }, { "wield", "wield" },
		{ "throw", "shoot" }, { "wear/take off", "wear" }, { "drop", "drop" },
		{ "unlock with", "unlock" }, { "strap on/unstrap", "wear" },
		{ "ignite a torch with", "ignite" }, { "ignite", "ignite" },
	};
	unsigned i;

	for (i = 0; i < sizeof map / sizeof map[0]; i++)
		if (!strcmp(verb, map[i][0])) {
			port_sound(map[i][1]);
			return;
		}
}

/* ---- graveyard + leaderboard beacon (RVIP step 12) ----
 *
 * One report per finished run, from the game's own end paths (MAIN.C
 * doquit / doexit): quit, death (killer = MAG's `killer`, no articles),
 * escaped with the Sudbury Sapphire = win, escaped without it = quit.
 * Score as the Hall of Heroes gets it (0 after wizard mode). */
long	getscore();
void	fe_beacon(const char *q);

static void
enc(char *d, const char *s, int n)
{
	static const char hex[] = "0123456789ABCDEF";
	int	i = 0;

	for (; *s && i < n - 4; s++)
		if (isalnum((unsigned char)*s) || strchr("-_.~", *s))
			d[i++] = *s;
		else {
			d[i++] = '%';
			d[i++] = hex[(unsigned char)*s >> 4];
			d[i++] = hex[*s & 15];
		}
	d[i] = 0;
}

void
port_run_end(const char *kill)
{
	char	q[400], nm[80], kl[130];
	const char *ev = "death";

	if (!strcmp(kill, "quit"))
		ev = "quit";
	else if (!strcmp(kill, "escaped"))
		ev = in_inv(SAPPHIRE) ? "win" : "quit";
	enc(nm, u.u_name, sizeof nm);
	enc(kl, kill, sizeof kl);
	snprintf(q, sizeof q, "g=mag&ev=%s&name=%s%s%s&depth=%d&score=%ld&turns=%ld&lvl=%d",
		ev, nm, strcmp(ev, "death") ? "" : "&killer=", strcmp(ev, "death") ? "" : kl,
		u.u_dlevel, wizard ? 0L : getscore(YES), (long)u.u_moves, u.u_elevel);
	fe_beacon(q);
}
