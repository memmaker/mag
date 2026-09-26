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
#include	<conio.h>
#include	<dos.h>
#include	"mag.h"

/*
 * PUTWHAT: a generic procedure to print whatever should be at a dungeon
 * location
 */

void
putwhat(d)
register DUNGEON *d;
{
	if (d == u.u_d) {
		color(BROWN | INTENSE);
		dat(d, u.u_sym);
	} else {
		if ((d->d_data & D_MONSTER) && insight(d)) {
			register struct monster	*m = what_mon(d);

			color(m->m_perm->p_data & (WHITE | INTENSE));
			dat(d, (unsigned char)lookmon(m));
		} else if ((d->d_data & D_OBJECT) && insight(d)) {
			color(BLUE | INTENSE);

			if (u.u_data & UD_DELUSION) {	/* random */
				color((rnd(7) + 1) | INTENSE);
				dat(d, CITEMS[rnd(NUMTYPES)]);
			} else {
				register LEVOBJ	*l = what_obj(d);

				color(obj_color(&l->l_o));
				dat(d, CITEMS[l->l_o.o_type]);
			}
		} else if (d->d_data & D_SEEN) {
			if (d->d_data & D_DOOR) {
				color(BROWN);

				if (d->d_data & D_WALL)
					dat(d, what_door(d)->dr_appear);
				else dat(d, DOORC);
			} else if (d->d_data & D_WALL) {
				color(BROWN);
				dat(d, d->d_what);
			} else if (d->d_what == POOL) {
				color(CYAN);
				dat(d, POOL);
			} else if (d->d_data & D_STAIRCASE) {
				color(GREEN | INTENSE);
				dat(d, d->d_what);
			} else if (d->d_what == TRAPC) {
				color(MAGENTA);
				dat(d, d->d_what);
			} else {
				color(GREEN);
				dat(d, d->d_what);
			}
		} else dat(d, ROCK);
	}
} 

/*
 * LOOKMON: return the appearance symbol for a monster
 */

int
lookmon(m)
register MONSTER *m;
{
	if ((m->m_data & M_INVIS) && !(u.u_data & UD_SEEINV) && !(m->m_data &
			M_DETECTED))
		return (int)m->m_d->d_what;	/* can't see the mon */

	if (u.u_data & UD_DELUSION)
		return (int)pmon[rnd(NUMMON)].p_letter;	/* random */

	return (int)m->m_perm->p_letter;	/* the normal appearance */
}

/*
 * REDRAW: reprint everything on the screen that needs to be reprinted
 */

void
redraw()
{
	register DUNGEON	*d;
	register		l,
				c;

	/* loop thru the lines of the screen */
	for (l = 1; l < LINES-1; l++) {
		if (kbhit())	/* redisplay can be interrupted */
			break;

		/* if the line is 'dirty', redraw what needs redrawing */
		if (red_data[l][0] != -1) {
			for (d = &dun[l][red_data[l][0]]; d <= &dun[l][
							red_data[l][1]]; d++)
				putwhat(d);

			red_data[l][0] = -1;	/* reset dirty flag */
		}
	}

	/* make sure that the area around the player always gets redrawn */
	l = scrline(u.u_d) - 1;
	c = scrcol(u.u_d) - 1;
	set_red(l, c, c + 2);
	set_red(l + 1, c, c + 2);
	set_red(l + 2, c, c + 2);
}

/*
 * SET_RED: marks an area of a line to be redrawn
 */

void
set_red(l, c1, c2)
register	l,
		c1,	/* start col */
		c2;	/* end col */
{
	if (red_data[l][0] == -1 || c2 > red_data[l][1])
		red_data[l][1] = c2;

	if (red_data[l][0] == -1 || c1 < red_data[l][0])
		red_data[l][0] = c1;
}

/*
 * RED_BOX: marks a rectangle to be redrawn
 */

void
red_box(l1, c1, l2, c2)
{
	register	l;

	for (l = l1; l <= l2; l++)
		set_red(l, c1, c2);
}

/*
 * REMAP: handles doing a whole screen redraw
 */

int
remap()
{
	erase();
	cl_scr();
	blineflg = REDRAW_ALL;
	doremap();

	return NO;
}

/*
 * DOREMAP: actually reprints the screen char by char
 */

void
doremap()
{
	register DUNGEON *d;

	for (d = &dun[0][0]; d < &dun[LINES-1][COLUMNS-1]; d++)
		putwhat(d);
}

/*
 * VIEW: have the player look around and describe everything interesting that
 * he sees
 */

int
view()
{
	register DUNGEON	*d;
	register LEVOBJ		*l;
	register OBJECT		*o;
	TRAP			*t;
	MONSTER			*m;
	int			flg = NO;

	if (blind()) {
		pline("You can't see anything -- you're blind.");
		bell();

		return NO;
	}

	if (u.u_data & UD_DELUSION) {
		pline("Like wow!  Everything is SOOO pretty!");
		return NO;
	}

	color(WHITE);

	/* first look at all the monsters around */
	for (m = mons; m < &mons[nummons]; m++) {
		if ((m->m_data & M_DEAD) || !vinsight(m->m_d))
			continue;

		flg++;	/* keep track of how many things have been seen */

		if (m->m_data & M_SLEEPING)
			pline("@A sleeping %s%s", mname(m), m->m_ni ?
						" which is carrying:" : ".");
		else pline("@%s%s", article(mname(m), YES), m->m_ni ?
						" which is carrying:" : ".");

		viewwait(m->m_d);

		/* go thru the mons inventory too */
		for (o = m->m_i; o < &m->m_i[m->m_ni]; o++) {
			pline("@%s%s", obj_str(o), (o+1) < &m->m_i[m->m_ni] ?
						" and" : "");
			viewwait(m->m_d);
		}
	}
 
	/* next, look at all the traps */
	for (t = traps; t < &traps[numtraps]; t++) {
		if (insight(t->t_d) && t->t_d->d_what == TRAPC) {
			flg++;
			pline("@A %s trap%s.", t->t_t->t_name, (t->t_d ==
					u.u_d ? " (beneath you)" : ""));
			viewwait(t->t_d);
		}
	}

	/* look at all of the objects lying around */
	for (l = lobjs; l < &lobjs[numlobjs]; l++) {
		if (vinsight(l->l_d)) {
			flg++;
			pline("@%s.%s", obj_str(&l->l_o), (l->l_d == u.u_d ?
						" (beneath you)" : ""));
			viewwait(l->l_d);
		}
	}

	/* lastly, look for any staricases */
	for (d = &dun[0][0]; d < &dun[LINES-1][COLUMNS-1]; d++) {
		if (d->d_data & D_STAIRCASE && vinsight(d)) {
			flg++;
			pline("@A%s staircase.%s", (d->d_what == DNSTAIR ?
				" down" : "n up"),(d == u.u_d ?
				" (beneath you)" : ""));
			viewwait(d);
		}
	}
			
	if (!flg)
		pline("You don't see anything interesting.");
	else erase();

	return NO;
}

/*
 * VINSIGHT: a special proc to decide if player can see something while
 * viewing
 */

int
vinsight(d)
register DUNGEON	*d;
{
	return (insight(d) && ((u.u_r && u.u_r == what_room(d)) ||
						inrange(u.u_d, d, 1)));
}

/*
 * VIEWWAIT: print a MORE message to allow the player to see the item that
 * he is viewing
 */

void
viewwait(d)
register DUNGEON	*d;
{
	if (proceed)	/* if the proceed flag is set, don't do MOREs */
		return;

	i_set(0, strlen(message));
	color(WHITE | INTENSE);
	swrite(" =-More-=");
	cursor(YES);
	doset(scrline(d), scrcol(d));
	domore(YES);
	cursor(NO);
	i_set(0, strlen(message));
	swrite("         ");
}

/*
 * KNOW: make the area adjacent to the player seen or unseen depending on HOW
 */

void
know(d, how)
register DUNGEON	*d;
register		how;
{
	register	i;

	for (i = 0; i < 9; i++) {
		d += circle[i];

		if (how && (!(d->d_data & D_WALL) || what_room(d) == u.u_r))
			d->d_data |= D_SEEN;
		else if (d->d_what == FLOOR)
			d->d_data &= ~D_SEEN;
	}
}

/*
 * INSIGHT: returns if the player can see a given dungeon location from his
 * current position and condition (i.e. blind)
 */

int
insight(d)
register DUNGEON	*d;
{
	register ROOM		*r = what_room(d);
	register MONSTER	*m;

	if (d->d_data & D_MONSTER) {
		if ((m = what_mon(d))->m_data & M_DETECTED)
			return YES;

		if ((m->m_data & M_INVIS) && !(u.u_data & UD_SEEINV))
			return NO;
	}

	return ((!blind() && ((r && r == u.u_r && (r->r_data & R_TORCHED))
		|| inrange(u.u_d, d, 1)))) || ((d->d_data & D_OBJECT) &&
					(what_obj(d)->l_data & L_DETECTED));
}

/*
 * INRANGE: return whether a location is within a certain radius of another
 * location
 */

int
inrange(d1, d2, r)
register DUNGEON	*d1,	
			*d2;
register		r;
{
	return (scrline(d1) >= scrline(d2) - r && scrline(d1) <= scrline(d2)
		+ r && scrcol(d1) >= scrcol(d2) - r && scrcol(d1) <=
							scrcol(d2) + r);
}

/*
 * CURSOR: turn the cursor either on or off
 */

void
cursor(mode)
register	mode;
{
	union REGS	r;

	/* BIOS video service 1: set cursor size */
	r.h.ah = 1;
	r.h.ch = (char)((scrtype == MONOCHROME ? 2 : 0) + (mode ? 0 : 32));
	r.h.cl = (char)((scrtype == MONOCHROME ? 12 : 7) + (mode ? 7 : 12));
	int86(16, &r, &r);
}

/*
 * SETUPSCR: put the video adaptor in the correct video mode
 */

void
setupscr()
{
	union REGS	r;

	if (scrtype == COLOR)
		r.h.al = 3;	/* color text 80x25 mode */
	else {
		if (scrtype == MONOCHROME)
			r.h.al = 7;	/* monochrome text 80x25 mode */
		else r.h.al = 2;	/* b/w text 80x25 */

		out_attrib = WHITE;
	}

	r.h.ah = 0;		/* BIOS video service 0: set screen mode */
	int86(16, &r, &r);

	r.h.ah = 1;		/* BIOS video service 1: set cursor size */
	r.h.ch = (char)(scrtype == COLOR ? 0 : 2);
	r.h.cl = (char)(scrtype == COLOR ? 7 : 12);
	int86(16, &r, &r);
}

/*
 * OBJ_COLOR: returns the color of a given object
 */

char
obj_color(o)
register OBJECT	*o;
{
	switch (o->o_offset) {
	case COPPER:
		return BROWN;
		break;

	case PLATINUM:
		return (WHITE | INTENSE);
		break;

	case GOLDEN:
	case OILFLASK:
	case GOLD:
		return YELLOW;
		break;

	case KSILVER:
	case SKELETON:
	case SPARKLING+4:	/* opaque gem */
	case SPARKLING+6:	/* opalescent gem */
	case SPARKLING+8:	/* brilliant gem */
	case SILVER:
		return WHITE;
		break;

	case SLIMEMOLD:
	case CHEESE:
	case SPARKLING+5:	/* green gem */
		return GREEN;
		break;

	case SAPPHIRE:
	case SPARKLING:
	case SPARKLING+10:
		return (WHITE | INTENSE | BLINKING);
		break;

	case SPARKLING+1:	/* crimson gem */
	case SPARKLING+7:	/* firey gem */
	case SPARKLING+9:	/* amber gem */
		return (RED | INTENSE);
		break;

	case SPARKLING+2:	/* purple gem */
		return (BLUE | RED);
		break;

	case SPARKLING+3:	/* turquoise gem */
		return CYAN;
		break;

	default:
		return (BLUE | INTENSE);
	}
}
