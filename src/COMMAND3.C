/*
 *  		MAG - A Dungeon Adventuring Game
 *
 *	Copyright (C) 1986,87,88 by Michael J. Teixeira
 *
 *  General permission to copy or modify, but not for profit, is hereby
 *  granted, provided that the above copyright notice is included.
 *
 */

#include	<string.h>
#include	<stdio.h>
#include	<process.h>
#include	"mag.h"

/*
 * VERSION: display the current MAG version number
 */

int
version()
{
	pline("MAG version PC-1.1, Summer 1989 - by Michael J. Teixeira");

	return NO;
}

/*
 * SEARCH: the go-between procedure for utilize and DOSEARCH
 */

int
search()
{
	dosearch(YES);
	return YES;
}

/*
 * DOSEARCH: look around the player for secret doors and traps.  if HOW is
 * false, then there is a 100% chance of finding things (this is mostly for
 * rings of searching)
 */

void
dosearch(how)
register	how;
{
	register		i;
	register DUNGEON	*d = u.u_d;

	for (i = 0; i < 9; i++) {
		d += circle[i];

		if ((d->d_data & (D_DOOR | D_WALL)) == (D_DOOR | D_WALL) &&
						(!how || !rnd(4))) {
			d->d_what = DOORC;
			d->d_data &= ~(D_WALL | D_STONE);
			pline("You found a secret door!");
		/* traps are always found on the first try */
		} else if (d->d_data & D_TRAPPED && d->d_what != TRAPC) {
			d->d_what = TRAPC;
			pline("You found a %s trap!",what_trap(d)->t_t->t_name);
		} else continue;

		stop();
		d->d_data |= D_SEEN;
		putwhat(d);

		return;	/* only one item can be found per search */
	}

	if (how)	/* ring of searching doesn't print this message */
		pline("@Nothing...");
}

/*
 * STEPUP: go-between for UTILIZE and STEPS
 */

int
stepup()
{
	return steps(UP);
}

/*
 * STEPDOWN: go-between for UTILIZE and STEPS
 */

int
stepdown()
{
	return steps(DOWN);
}

/*
 * STEPS: all uses of staircases are handled through this procedure
 */

int
steps(dir)
int	dir;
{
	/* make sure there is the proper type of stairs under player */
	if (((dir == UP && u.u_d->d_what != UPSTAIR) || (dir == DOWN &&
			u.u_d->d_what != DNSTAIR)) && wizard != YES) {
		pline("There isn't a%s staircase here.", (dir == UP ?
					"n ascending" : " decending"));
		return NO;
	}

	if (u.u_stuckmon) {
		pline("You can't escape the %s!", mname(u.u_stuckmon));
		bell();

		return YES;
	}

	if (u.u_data & UD_LEVITATE) {
		pline("You can't seem to move vertically.");
		bell();

		return YES;
	}

	if (dir == UP) {
		pline("You climb up the stone steps...");
		newlev(UP, DNSTAIR, YES);
	} else {
		pline("You step down into the murky darkness...");
		newlev(DOWN, UPSTAIR, YES);
	}

	return YES;
}

/*
 * REST: use one time unit doing nothing
 */

int
rest()
{
	return YES;
}

/*
 * NOTHIN: a command that does nothing and uses no time units
 */

int
nothin()
{
	return NO;
}

/*
 * HOWMOVES: convert the number of time units into International Standard
 * Adventuring Time (ISAT) units and print the value
 */

int
howmoves()
{
	register	days = (int)(u.u_moves / 4320),
			hours = (int)((u.u_moves % 4320) / 180),
			minutes = (int)(((u.u_moves % 4320) % 180) / 3);
	char		str[100];

	str[0] = '\0';

	if (days)
		sprintf(str, "%d day%s ", days,(days == 1 ? "," : "s,"));

	if (hours)
		sprintf(&str[strlen(str)], "%d hour%s ", hours, (hours == 1 ?
			"," : "s,"));

	sprintf(&str[strlen(str)], "%d minute%s", minutes, (minutes == 1 ?
		" (ISAT)." : "s (ISAT)."));

	pline("You have been adventuring for %s", str);

	return NO;
}

char	*fullness[8] = {
	"on the verge of hungry.",
	"almost hungry.",
	"not too hungry.",
	"somewhat full.",
	"full.",
	"rather full.",
	"stuffed.",
	"like you're just about to explode!",
};

/*
 * STOMACH: print a string describing our heroes hunger-factor
 */

int
stomach()
{
	register	howmuch = (int)((efood->e_when - u.u_moves) / 250L);

	pline("You feel %s", fullness[howmuch]);

	return NO;
}

/*
 * EXTINGUISH: smothers a torch which is on a dungeon wall
 */

int
extinguish()
{
	register DUNGEON	*d;
	register		i;

	/* look around player for torches */
	for (d = u.u_d, i = 0; i < 9; i++) {
		d += circle[i];

		if (u.u_r && strchr(TORCHC, d->d_what)) { /* a torch? */
			/* make sure that it is burning */
			if (u.u_r->r_data & R_TORCHED && !(u.u_r->r_data &
							R_MAGICLIT)) {
				dk_room(u.u_r);
				red_box(u.u_r->r_uline, u.u_r->r_ucol,
					u.u_r->r_lline, u.u_r->r_lcol);
				u.u_r->r_data &= ~R_TORCHED;
				know(u.u_d, YES);
				pline("The torch is now out.");
			} else {
				pline("The torch is not burning.");
				bell();
			}

			return YES;
		}
	}

	pline("You don't see any torch around here.");
	bell();

	return YES;
}

/*
 * STARTMACRO: begin or restart remembering all the keys typed
 */

int
startmacro()
{
	if (u.u_options & O_MAKEMACRO)
		pline("Restarting macro...");
	else pline("Remembering macro...");

	u.u_options |= O_MAKEMACRO;
	macroptr = macro; /* reset macro ptr to beginning of macro buf */

	return NO;
}

/*
 * ENDMACRO: stop remembering keystrokes to macro buffer
 */

int
endmacro()
{
	if (u.u_options & O_MAKEMACRO) {
		pline("Macro declared.");
		zapmacro();	/* zap the end-macro key from the buf */
		u.u_options &= ~O_MAKEMACRO;
	} else pline("You haven't started remembering a macro yet!");

	return NO;
}

/*
 * ZAPMACRO: removes the last keystroke stuck in the macro buffer
 */

void
zapmacro()
{
	if (u.u_options & O_MAKEMACRO)
		*--macroptr = '\0';
}

/*
 * EXECUTE: sets the macro to execute mode
 */

int
execute()
{
	if (u.u_options & O_MAKEMACRO)
		endmacro();	/* be a nice guy and auto close macro */

	u.u_options |= O_DOMACRO;
	macroptr = macro;

	return NO;
}

/*
 * EXCHANGE: automatically swaps the right-hand weapon with the secondary
 * wep if possible
 */

int
exchange()
{
	register OBJECT	*o;

	if (!secweap) {
		pline("You don't have anything designated as your secondary weapon.");

		return NO;
	}

	o = rightweapon;

	/* what a mess!  if secondary wep is a 2-hander, make sure we can
	   free BOTH hands to wield it */
	if ((secweap->o_offset == TWOHAND && ((o && iscursed(o, YES)) ||
			(leftweapon && iscursed(leftweapon, YES)) ||
			(shield && shield->o_offset != BUCKLER &&
			iscursed(shield, YES))))||(o && !unwield(RIGHTHAND)))
		return YES;

	/* if 2-hander, now free up that left arm */
	if (secweap->o_offset == TWOHAND) {
		if (leftweapon)
			unwield(LEFTHAND);
		else if (shield && shield->o_offset != BUCKLER)
			unstrap();
	}

	if (iscursed(secweap, NO)) {
		pline("The weapon adheres to your hand!");
		bell();
		secweap->o_data |= O_SEECURSED;
	}

	rightweapon = secweap;		/* now using sec wep */
	rightweapon->o_data |= O_INUSE;

	secweap = o;			/* sec wep is now old primary wep */

	pline(form(rightweapon, NO));

	return YES;
}

/*
 * SETSECWEP: sets the secondary wep to the item given
 */

int
setsecwep(o)
register OBJECT	*o;
{
	/* player can't set the sec wep to one that's in use */
	if (o->o_data & O_INUSE) {
		pline("You are already wielding that!");
		bell();

		return -1;
	}

	secweap = o;

	return YES;
}

/*
 * MULTIREST: set multi-command to optional # of rests
 */

int
multirest()
{
	typed = '.';
	ntimes = mrest * (ntimes + 1);

	if (ntimes < 0) {
		pline("You can't rest THAT long!");
		ntimes = 0;
	}

	return NO;
}

/*
 * MULTISEARCH: set multi-command to optional # of searches
 */

int
multisearch()
{
	typed = 's';
	ntimes = msearch * (ntimes + 1);

	if (ntimes < 0) {
		pline("You can't search THAT many times!");
		ntimes = 0;
	}

	return NO;
}

/*
 * SAYTITLE: prints the experience title of our hero
 */

int
saytitle()
{
	pline("You have attained the rank of %s.", titles[u.u_elevel-1]);

	return NO;
}

/*
 * GETSHELL: creates a DOS if we can find and fit command.com into memory.
 * note that the shell is created on video page 0 to prevent problems
 * with any programs run within DOS
 */

int
getshell()
{
	register char	*cs;

	cl_scr();
	color(WHITE);
	i_set(0, 0);
	swrite("Type 'exit' to return...");

	cursor(YES);

	/* attempt to get path from environment */
	if ((cs = getenv("COMSPEC")) == NULL)
		cs = "\\command.com";		/* default */

	if (spawnl(P_WAIT, cs, NULL) == -1) {
		cursor(NO);
		pline("Sorry, could not find/execute 'command.com'.");
		bell();
		/* force a more so user can see message */
		more(i_line[active_page], i_col[active_page], YES);
	} else cursor(NO);

	remap();

	return NO;
}

/*
 * LOGBUG: let's player type a line of text to be written to a file called
 * 'bugs'.  mainly for debugging, this procedure is normally inactive
 */

int
logbug()
{
#ifdef	DONT
	register char	*line;
	register FILE	*fp;

	if ((fp = fopen("bugs", "a")) == NULL)
		pline("Sorry, couldn't open bugs file for appending.");
	else {
		pline("@Enter the bug: ");

		if (atoi(line = get_str(YES)) != -1 && *line) {
			fprintf(fp, "%s\n", line);
			fclose(fp);
			pline("Ok, I'll fix that bug later.");
		} else pline("Come on, get with the show.");
	}
#else
	pline("Sorry, this version doesn't have any bugs!");
#endif

	return NO;
}

