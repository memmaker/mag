/*
 *  		MAG - A Dungeon Adventuring Game
 *
 *	Copyright (C) 1986,87,88 by Michael J. Teixeira
 *
 *  General permission to copy or modify, but not for profit, is hereby
 *  granted, provided that the above copyright notice is included.
 *
 */

#include	<stdio.h>
#include	<string.h>
#include	<ctype.h>
#include	"mag.h"

extern char	wizpword[];

/*
 * NEWLEV: have the player go up or down one level
 */

void
newlev(dir, c, save)
register	dir;
unsigned char	c;
register	save;
{
	register DUNGEON	*d;
	char			new[15],
				foo[100];
	int			i;

#ifdef PORT
	port_sound("stairs");
#endif
	/* get rid of lamp residue surrounding the stairs */
	if (u.u_r && !(u.u_r->r_data & R_TORCHED))
		know(u.u_d, NO);

	/* don't save if going up from a labyrinth */
	if (save && (!(u.u_data & UD_LABYRINTH) || dir == DOWN))
		save_lev();

	nummons = numlobjs = 0;

	/* when entering/exiting a lab, the level doesn't actually change */
	if (!(u.u_data & UD_LABYRINTH)) {
		if (dir == UP) {
			if (--u.u_dlevel < u.u_lo_level)
				u.u_lo_level = u.u_dlevel;
		} else if (++u.u_dlevel > u.u_hi_level)
			u.u_hi_level = u.u_dlevel;
	}

	if (!u.u_dlevel) {
		for (i = 0; wizpword[i]; i++)
			new[i] = wizpword[i] + '^';

		new[i] = 0;

		cl_scr();
		swrite("You escaped!");
		bell();
		i_set(3, 0);

		if (in_inv(SAPPHIRE)) {
			swrite("You have retrieved the sacred Sudbury Sapphire from its deep prison!");
			i_set(5, 0);
			swrite("                         CONGRATULATIONS!");
			i_set(7, 0);
			swrite("Emerging from the dungeon entrance, you stride forth into the sunshine");
			i_set(8, 0);
			swrite("tightly clasping the Sapphire.  You triumphantly raise your prize above");
			i_set(9, 0);
			swrite("your head allowing the sun to shine through it and send brilliant blue");
			i_set(10, 0);
			swrite("beams scattering about you.  Only then do you notice that deep within");
			i_set(11, 0);
			sprintf(foo, "the scintillating gemstone can be seen a single word: '%s'.", new);
			swrite(foo);
			i_set(13, 0);
			swrite("Not quite sure just what it means, you stash the Sapphire back in your");
			i_set(14, 0);
			swrite("pack with a smile on your face and begin the trek down the dirt path");
			i_set(15, 0);
			swrite("back home.");
		} else swrite("However, you have failed your quest for the Sudbury Sapphire.");

		killer = "escaped";
		doexit();		
	}

	numdoors = numtraps = numrooms = 0;

	if ((u.u_data & UD_LABYRINTH) && dir == DOWN) {	/* create a lab! */
		cl_dun();
		numrooms = 1;
		rooms[0].r_uline = 4;
		rooms[0].r_ucol = 5;
		rooms[0].r_lline = 18;
		rooms[0].r_lcol = 73;
		mk_room(rooms);
		r_to_maze(rooms);
		mk_traps();
		(d = room_loc(rooms))->d_data |= D_STAIRCASE;
		d->d_what = UPSTAIR;
		rooms[0].r_data |= R_STAIRCASE;
		u.u_d = room_loc(rooms);
	} else if (!restore_lev(u.u_dlevel)) {	/* no saved lev? make one */
		cl_dun();
		mk_lev();
		lev_mon();
		u.u_d = find_loc(c);
	}

	if (u.u_r = what_room(u.u_d))
		readyroom(u.u_r);

	if (!blind())
		know(u.u_d, YES);

	blineflg = REDRAW_ALL;
	cl_scr();
	doremap();

	if ((u.u_data & UD_LABYRINTH) && dir == UP) {
		pline("Whew!  You've made it back to your normal plane of existence.");
		u.u_data &= ~UD_LABYRINTH;
	}
}

/*
 * TELEPORT: zot player from one location on the dungeon to another
 */

int
teleport(wd)
register DUNGEON	*wd;
{
	register DUNGEON	*d = u.u_d;

	redraw();
#ifdef PORT
	port_sound("teleport");
#endif

	if (u.u_dir != -1)	/* are we moving right now? */
		stop();

	if (u.u_r) {	/* clean up area before teleporting */
		if (!(u.u_r->r_data & R_TORCHED))
			know(u.u_d, NO);
		else red_box(u.u_r->r_uline, u.u_r->r_ucol, u.u_r->r_lline,
							u.u_r->r_lcol);
	}

	/* if destination given, use it.  otherwise find a random one */
	if (wd)
		u.u_d = wd;
	else
		do u.u_d = location(STELETO);
		while (what_room(u.u_d)->r_data & R_TROVE);

	u.u_stuckmon = (MONSTER *)0;
	putwhat(d);
	u.u_r = what_room(u.u_d);

	if (!blind())
		know(u.u_d, YES);

	readyroom(u.u_r);
	redraw();
	u.u_data &= ~(UD_STUCK | UD_STRANGLE);
	rm_event(nostuck);

	return NO;
}

/*
 * MAPPING: display the whole dungeon and make secret doors visible
 */

void
mapping()
{
	register DUNGEON *d;
	
	for (d = &dun[0][0]; d < &dun[LINES-1][COLUMNS]; d++) {
		/* make the secret doors visible */
		if ((d->d_data & (D_DOOR | D_WALL)) == (D_DOOR | D_WALL)) {
			d->d_what = DOORC;
			d->d_data &= ~(D_WALL | D_STONE);
		}

		if ((d->d_data & (D_DOOR | D_STAIRCASE | D_WALL)) ||
				d->d_what == POOL || d->d_what == CORRIDOR) {
			d->d_data |= D_SEEN;
			putwhat(d);
		}
	}
}

/*
 * WEIGHT: returns the weight of the given object
 */

long
weight(type, off, num, data)
register	type,
		off,
		num;
{
	long	w;

	if (type == WAND)
		w = (off < DIGGING ? 2722L : (off < CANCEL ? 4536L : 1361L))
								* (long)num;
	else if (!oweight[type])
		w = (long)(wtable[type][off - objoff[type]]) * (long)num;
	else w = (long)oweight[type] * (long)num;

	if (data & O_ETHEREAL)
		return w / 2L;

	return w;
}

/*
 * NEEDEXP: returns the number of exp pts for the given level
 */

long
needexp(el)
register	el;
{
	return (long)((1L << (el - 1)) * 100L);
}

/*
 * CK_EXP: check player's exp pts to see if he went up a level
 */

void
ck_exp()
{
	long		need;
	register	newhp;

	if (u.u_elevel >= 20)	/* max out at level 20 */
		return;

	need = needexp(u.u_elevel + 1);

	if (u.u_epoints < need)
		return;

	newhp = rnd(9) + 2;
	u.u_hp += newhp;
	u.u_maxhp += newhp;
	u.u_elevel++;
#ifdef PORT
	port_sound("level");
#endif

	pline("Congratulations!  You have reached the rank of %s.",
						titles[u.u_elevel-1]);

	blineflg = REDRAW_ALL;
	ck_exp();
}

/*
 * NEAR_LOC: pick a random location within one space of given location
 */

DUNGEON	*
near_loc(d)
register DUNGEON *d;
{
	register		count = 200;
	register DUNGEON	*nd;

	do nd = mvindir(rnd(8), d, 1);
	while (nd->d_data & (D_MONSTER | D_STONE) && count--);

	/* after a number of tries, give up to prevent everless loop */
	if (!count) {
		pline("near_loc: Out of tries!");
		return location(SFLOOR);
	}

	return nd;
}

/*
 * LIST_OBJ: list all item of certain type given by last key typed.  for ex)
 * if user typed '%' it would list all food
 */

int
list_obj()
{
	register	line;
	
	if ((line = pr_obj(-((int)(strchr(CITEMS,typed)-CITEMS)), NO)) == -2)
		pline("Nothing appropriate to list.");
	else if (line != -1) {
		set_vis(1);
		more(line < 22 ? line : 21, 0, NO);
		cl_scr();
		set_act(0);
		set_vis(0);
	}

	return NO;
}

/*
 * ISCURSED: return if given obj is cursed.  if HOW, print message
 */

int
iscursed(o, how)
register OBJECT	*o;
register	how;
{
	if (o->o_data & O_CURSED) {
		if (how) {
			pline("You can't.  The %s appears to be adhered to you.",
							typename(o));
			bell();
		}

		return YES;
	}

	return NO;
}

/*
 * GETHAND: ask user to select left or right hand
 */

int
gethand(l, r)
register OBJECT	*l,
		*r;
{
	char		foo[50];
	register	c;

	strcpy(foo, "@Which hand");

	if ((l && r) || (l && l->o_offset == TWOHAND))
		strcat(foo, " (both in use)");
	else if (l)
		strcat(foo, " (left in use)");
	else if (r)
		strcat(foo, " (right in use)");

	pline("%s? ", foo);
	go_imaginary();

	for (;;) {
		if ((c = getlower()) == -1) {   /* ESC */
			erase();
			return -1;
		}

		if (strchr("lrLR", c))
			break;

		pline("@Type 'l' or 'r', please: ");
		bell();
		go_imaginary();
	}

	return (c == 'r' || c == 'R');
}

/*
 * RELEVEL: make player go up one exp level
 */

void
relevel()
{
	pline("You feel more experienced!");
	u.u_epoints = needexp(u.u_elevel + 1);
	ck_exp();
}

/*
 * GETSCORE: calc player's score.  if HOW, calc real score if !HOW, calc
 * score without enchantment bonuses.  the user never knows his real score
 * until the end of the game
 */

long
getscore(how)
register	how;
{
	long		score = u.u_score;
	register OBJECT	*o;

	for (o = inv; o < &inv[numinv]; o++) {
		switch (o->o_type) {
		case POTION:
		case SCROLL:
		case WAND:
		case RING:
			score += (o->o_type * 25 * o->o_quantity);
			break;

		case WEAPON:
			if (how)
				score += ((o->o_enchant + o->o_wepench)*100);
			break;

		case ARMOR:
		case SHIELD:
			if (how)
				score += (o->o_enchant * 100);
			break;

		case KEY:
			score += 500L;
			break;

		case GEM:
			score += ((o->o_offset - SPARKLING + 1) * 123 *
							o->o_quantity);
			break;

		case TREASURE:
			if (o->o_offset == PLATINUM)
				score += o->o_quantity * 5;
			else if (o->o_offset == GOLD)
				score += o->o_quantity;
			else if (o->o_offset == ELECTRUM)
				score += o->o_quantity / 2;
			else score += o->o_quantity / 10;
		}
	}

	if (in_inv(SAPPHIRE))
		score *= 2L;

	/* player always starts with 400 pts so we have to always subt it */
	if (score < 400L * how)
		return 0L;

	return score - (400L * how);
}

/*
 * SAPP_LEV: returns whether currently on the sapphire level
 */

int
sapp_lev()
{
	return (u.u_dlevel == 25 && !(u.u_data & UD_BEENSAPP));
}

/*
 * GET_STR: get a str char by char
 */

char	*
get_str(echo)
register	echo;
{
	static char	input[50];
	register char	*cp = input;
	register	c;

	for (;;) {
		go_imaginary();
		c = ctgetch();

		switch (c) {
		case '\177':	/* DEL & ^H */
		case '\010':
			if (cp > input) {
				cp--;
				at(i_line[active_page], --i_col[active_page],
						' ');
			} else bell(); /* ring if backspacing too much */

			continue;

		case '\015':	/* ^M */
			*cp = 0;
			return input;

		case '\033':	/* ESC */
			return "-1";

		case '\025':	/* ^U */
			while (cp > input) {
				cp--;
				at(i_line[active_page], --i_col[active_page],
						' ');
			}

			continue;

		case '\027':	/* ^W, del prev word */
			do {
				if (cp > input) {
					cp--;
					at(i_line[active_page],
						--i_col[active_page], ' ');
				}
			} while (cp > input && !ispunct(*cp)&&!isspace(*cp));

			continue;

		default:
			if (isprint(c) && cp - input < 49) {
				*cp++ = (char)c;
				at(i_line[active_page], i_col[active_page],
					(echo ? (char)c : '?'));
				i_col[active_page]++;
			} else bell();	/* ring if typed an illegal char */
		}
	}
}

/*
 * BELL: beep if option not turned off
 */

void
bell()
{
	if (u.u_options & O_BELL)
		putchar(7);
}
