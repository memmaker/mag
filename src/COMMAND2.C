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
#include	"mag.h"

/*
 * USE: processed the use command
 */

int
use(o)
register OBJECT	*o;
{
	switch (o->o_offset) {
	case SAPPHIRE:
		if (o->o_charges > 0)
			domega(o);
		else pline("It sparkles briefly.");

		break;

	case COMPASS:
		docompass();
		break;

	case CROSS:
		docross();
		break;

	case MIRROR:
		domirror();
		break;

	case OILFLASK:
		throw(o);
		break;

	case TINDERBOX:
		/* fake the keyboard so that we can use the UTILIZE procedure
		   to handle lighting something */
		typed = 'L';
		utilize();

		break;

	case ETHEREAL:
		typed = 's';	/* again fake the kb to use UTILIZE */
		utilize();

		return -2;	/* vial used up */

	case CHIME:
		/* return UNLOCK because sometimes the chime melts */
		return unlock(o);
	}

	return NO;
}

/*
 * DOCROSS: handle whipping out a cross and attempting to scare a mon away
 */

void
docross()
{
	register MONSTER	*m;
	register		flg = NO;

	for (m = mons; m < &mons[nummons]; m++) {
		/* this is based on the theory that if you can see the mon,
		   the mon can see you...it's wrong, but who cares?? */
		if (!insight(m->m_d))
			continue;

		flg++;

		if (rnd(2) && strchr("qyABELRV", m->m_perm->p_letter) &&
					!(m->m_data & M_SLEEPING)) {
			pline("The %s is frightened at the sight!",mname(m));
			m->m_data |= M_SCARED;
		} else pline("The %s does not appear affected.", mname(m));
	}

	if (!flg)
		pline("Nothing happens.");
}

/*
 * DOMIRROR: handles whipping out a mirror and trying to use it on a mon
 */

void
domirror()
{
	register MONSTER	*m;
	register		flg = NO;

	for (m = mons; m < &mons[nummons]; m++) {
		/* if you can see mon...mon can see you (hopefully) */
		if (!insight(m->m_d))
			continue;

		flg++;

		if (rnd(2) && !(m->m_data & M_SLEEPING)) {
			switch (m->m_perm->p_letter) {
			case 'f':
				pline("The fairy is frightened by what it thinks is competition!");
				m->m_data |= M_SCARED;

				return;

			case 'g':
				pline("The gorgon is confused by itself!");
				m->m_data |= M_CONFUSED;

				return;

			case 'R':
				pline("The reaper fears death in itself!");
				m->m_data |= M_SCARED;

				return;

			case 'W':
				pline("The will-o-wisp explodes on sight!");

			/* using a mirror on a w-o-w can very dangerous */
				killer = "detonating will-o-wisp";
				explode(m->m_d);
				m->m_hp = -1;
				ck_mhp(m, NO);

				return;
			}
		}

		pline("The %s does not appear affected.", mname(m));
	}

	if (!flg)
		pline("Nothing happens.");
}

/*
 * LIGHT: set an invent item on fire
 */

int
light(o)
register OBJECT	*o;
{
	if (o->o_data & O_IGNITED) {
		pline("That is already burning.");
		bell();

		return NO;
	}

	/* since igniting something usually creates a separate new inv item,
	   we have to make sure we have room to put it somewhere */
	if (o->o_quantity > 1 && numinv >= MAXINV) {
		pline("You're carrying too much to light something.");
		return NO;
	}

	mk_event(5 + rnd(50), oilexplode);	/* oil can be dangerous! */

	/* create the new separate ignited object */
	if (o->o_quantity > 1) {
		lessweight(o, 1);
		o->o_quantity--;
		add_invent(create_o(MISC, 0, 0, (o->o_data | O_IGNITED),
							OILFLASK, 1), YES);
		return NO;
	}
	
	o->o_data |= O_IGNITED;

	return YES;
}

/*
 * UNLOCK: use a key/chime on a magically locked door into a treasure trove
 */

int
unlock(o)
register OBJECT	*o;
{
	register DUNGEON	*d;
	register DOOR		*dr;
	int			j = getdir(YES);

	if (j < 0) {		/* user aborted GETDIR */
		erase();
		return NO;
	}

	d = mvindir(j, u.u_d, 1);	/* set d to location in that dir */

	if (!(d->d_data & D_DOOR) || (d->d_data & D_WALL)) {
		pline("There isn't a door there.");
		bell();

		return NO;
	}

	if (o->o_offset == CHIME) {
		pline("Ding!  Ding!  Ding!");
		bell();
	}

	if (!((dr = what_door(d))->dr_data & DR_WITHLOCK)) {
		/* at this point, the door does not have a lock on it */

		if (o->o_offset == CHIME) {
			if (dr->dr_stuck) {
				pline("The door appears loose now.");
				dr->dr_stuck = 0;
			} else pline("The door seems unaffected.");
		} else {
			pline("That door doesn't have a lock.");
			bell();
		}

		return NO;
	}

	/* make sure the user is using the proper key */
	if (o->o_offset == SKELETON || (dr->dr_data & (1 << (o->o_offset -
					COPPER))) || o->o_offset == CHIME) {
		if (o->o_offset != CHIME)
			pline("The key is consumed as the door swings open!");
		else pline("Click!  The door swings open!");

		dr->dr_stuck = 0;
		dr->dr_data &= ~DR_WITHLOCK;

		if (o->o_offset == CHIME) {
			if (rnd(3))
				pline("The chime melts into nothingness!");
			else return NO;
		}

		return -2;	/* item is used up */
	}

	pline("The key doesn't fit.");

	return NO;
}

/*
 * STRAP: handles using a shield of any type
 */

int
strap(o)
register OBJECT	*o;
{
	register OBJECT	*os = shield;

	/* cannot unstrap shield or finished after removing shield, return */
 	if ((os && !unstrap()) || o == os)
		return NO;

	if (leftweapon) {	/* gotta unuse left-hand wep */
		if (!unwield(LEFTHAND))
			return NO;
	/* if using a 2-hander and not a buckler, unuse sword */
	} else if (rightweapon && rightweapon->o_offset == TWOHAND && o->
				o_offset != BUCKLER)
		if (!unwield(RIGHTHAND))
			return NO;

	o->o_data |= O_INUSE;
	ident(o);
	shield = o;
	u.u_ac += what_ac(o);
	blineflg = REDRAW_ALL;

	if (iscursed(o, NO)) {
		pline("The shield adheres to you!");
		bell();
		o->o_data |= O_SEECURSED;
	}

	return YES;
}

/*
 * UNSTRAP: take a shield off of the left arm
 */

int
unstrap()
{
	if (iscursed(shield, YES))
		return NO;

	shield->o_data &= ~O_INUSE;
	u.u_ac -= what_ac(shield);
	blineflg = REDRAW_ALL;
	pline("You are no longer using %s.", form(shield, YES));
	shield = (OBJECT *)0;

	return YES;
}

/*
 * IGNITE: ignite a torch stuck in the dungeon wall
 */

int
ignite(o)
register OBJECT	*o;
{
	register DUNGEON	*d = u.u_d;
	register		j;

	if (o->o_offset != TINDERBOX) {
		pline("You can't ignite a wall torch with that!");
		bell();

		return NO;
	}

	/* look one space around player for a torch */
	for (j = 1; j < 9; j++) {
		d += circle[j];

		/* you have to be in the room when we find a torch */
		if (u.u_r && strchr(TORCHC, d->d_what)) {
			/* make sure room is not magically lit already */
			if (u.u_r->r_data & R_MAGICLIT)
				pline("The torch will not ignite.");
			else {
				u.u_r->r_data |= R_TORCHED;
				readyroom(u.u_r);
				pline("The torch ignites.");
			}

			return NO;	/* cannot be more that one torch */
		}
	}

	pline("There isn't any wall torch around here.");
	bell();

	return NO;
}

/*
 * HELPCMD: list all of the command help files on the screen
 */

int
helpcmd()
{
	register	i;
	char		file[50];

	color(WHITE);
	set_act(1);
	set_vis(1);

	/* the last screen is for wizard commands and is only visible when
	   in wizard mode */
	for (i = 1; i < 5 + (wizard == YES); i++) {
		sprintf(file, "help\\help.%d", i);
		prfile(file);

		i_set(22, 0);
		swrite("Press SPACE to continue...");

		if (tgetch() == '\033')
			break;
	}

	cl_scr();
	set_act(0);
	set_vis(0);

	return NO;
}

char	*things[] = {	/* 1st char is symbol, others are description */
	"|stone wall",
	"-stone wall",
	".stone floor",
	" granite rock",
	"~water",
	"!a potion",
	"@you",
	"#a marble wall",
	"$a pile of coins",
	"%something edible",
	"^a visible trap",
	"*a gemstone",
	")a weapon",
	"=a ring",
	"[a shield",
	"]some armor",
	"\"a miscellaneous item",
	"<an up staircase",
	">a down staircase",
	",a key",
	"/a wand, staff, or rod",
	"?a scroll",
	0
};
		
/*
 * HELPSYM: give the player help with a screen symbol
 */

int
helpsym()
{
	register		i,
				c;
	register struct permmon	*p;

	pline("@What is which symbol? (RETURN for all) ");
	go_imaginary();	/* place the cursor properly */
	c = ctgetch();

	if (c == '\033') {
		ntimes = 0;	/* reset ntimes to stop multi-command */
		erase();

		return NO;
	}

	if (c == '\015') {	/* carriage return */
		color(WHITE);
		set_act(1);
		set_vis(1);

		prfile("help\\help.6");

		color(BROWN);
		at(2, 2, VWALL);
		at(2, 4, HWALL);
		at(3, 2, MARBLE);
		color(CYAN);
		at(4, 2, POOL);
		color(MAGENTA);
		at(5, 2, TRAPC);
		color(GREEN);
		at(6, 2, FLOOR);
		at(7, 2, UPSTAIR);
		at(8, 2, DNSTAIR);
		color(YELLOW);
		at(10, 2, PLAYER);

		color(BLUE);

		for (i = 0; i < 12; i++)
			at(2 + i % 6, 28 + i / 6 * 26, CITEMS[i]);

		color(WHITE);
		i_set(22, 0);
		swrite("Press any key to continue...");

		if (tgetch() != '\033') {
			prfile("help\\help.7");

			for (i = 0; i < NUMMON; i++) {
				color(pmon[i].p_data & (WHITE | INTENSE));
				at(2 + i / 3, i % 3 * 26, pmon[i].p_letter);
			}

			i_set(22, 0);
			color(WHITE);
			swrite("Press any key to continue...");
			tgetch();
		}

		cl_scr();
		set_act(0);
		set_vis(0);
		erase();

		return NO;
	}

	for (i = 0; things[i]; i++) {
		if (*things[i] == (char)c) {
			pline("'%c' is %s.", c, &things[i][1]);
			return NO;
		}
	}

	i = NO;

	for (p = pmon; p < &pmon[NUMMON]; p++) {
		if (p->p_letter == (char)c) {
			pline("'%c' is %s", c, article(p->p_name,NO));
			i++;
	/* no BREAK here because monsters can share the same symbol */
		}
	}

	if (!i)
		pline("That is an unknown symbol.");

	return NO;
}

/*
 * DOMEGA: the result of using the mystical Sudbury Sapphire
 */

void
domega(o)
register OBJECT	*o;
{
	register		i;
	register DUNGEON	*d;

	pline("The Sapphire becomes searing hot and eminates a blinding blue light!");

	o->o_enchant--;

	healme(YES, NO);

	pline("Wow!  Fireworks!");
	proceed = YES;
	d = u.u_d;

	/* explosions surround the player just out of range */
	for (i = 1; i < 9; i++) {
		d += 2 * circle[i];

		if (rnd(2) && d->d_what != ROCK)
			explode(d);
	}

	d = u.u_d;	/* prevent the player from taking damage */
	u.u_d = 0;

	/* zap random bolts of lightning all over the room */
	for (i = 0; i < 20; i++) {
		dozap(rnd(NUMDIRS), (u.u_r ? room_loc(u.u_r) : d), LIGHTNING,
							0);
		u.u_d = d;	/* reprint the player by replacing him */
		putwhat(d);
		u.u_d = 0;
	}

	u.u_d = d;
	pline("The Sapphire cools and dims.");
}

/*
 * DOCOMPASS: handles the player using a compass of translocation
 */

void
docompass()
{
	register DUNGEON	*d;
	
	/* take care of the two places where one won't work */
	if (sapp_lev() || u.u_data & UD_LABYRINTH) {
		pline("The needle is spinning around wildly...");
		pline("You can't make heads or tails of it!");

		return;
	}

	d = find_loc(DNSTAIR);

	pline("The needle points %s.", dirdesc[what_dir(u.u_d, d)]);

	/* sometimes the compass will teleport the player onto the staircase
	   but only if there isn't a monster on top of it (otherwise it
	   could be a very ugly scene) */
	if (!rnd(5) && !(d->d_data & D_MONSTER)) {
		pline("You feel a tug in that direction, then...yeeeow...");
		teleport(d);
	}
}
