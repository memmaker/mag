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
#include	"mag.h"

/*
 * ZAP: have the player use a wand/staff/rod
 */

int
zap(o)
register OBJECT	*o;
{
	register	dir;

	if (o->o_charges < 1) {			/* wand is empty */
		pline("Nothing happens.");
		return NO;
	}

	/* decide if wand is a ray/bolt type (not detect or illum, etc) */
	if (pr_wand(o->o_offset, (MONSTER *)0)) {
		dir = getdir(YES); /* if it is a ray type, get dir */
		erase();

		if (dir < 0)		/* aborted */
			return -1;

		dozap(dir, u.u_d, o->o_offset, (MONSTER *)0);
	}

	if (pobj[o->o_offset].po_flag & SID)	/* some are self-ident */
		makeknown(o);

	o->o_charges--;

	return NO;
}

char	wcolor[NUMWANDS] = { BLUE, 0, 0, YELLOW, 0, YELLOW, YELLOW, 0,
		YELLOW, (GREEN | INTENSE), (RED | INTENSE), (WHITE | INTENSE),
		YELLOW, YELLOW, YELLOW, (BLUE | INTENSE), 0, YELLOW, 0,
		(CYAN | INTENSE), YELLOW };

/*
 * DOZAP: this actually handles the zapping of a wand, either by the player
 * or a monster
 */

void
dozap(dir, d, off, m)
register DUNGEON	*d;
MONSTER			*m;
{
	char		*bolt = "\xb3/\xc4\\";	/* symbols for screen */
	DUNGEON		*trail[25],
			*dn;
	register	len = rnd(10) + 10,
			i;

	if (dir == -1)	/* make sure it's a legal direction */
		return;

	if (off == WONDER)	/* the mysterious wand of wonder */
		do off = DRAINLIFE + rnd(NUMWANDS);
		while (wcolor[off - DRAINLIFE] == 0);

	killer = bname(off);
	trail[0] = d;		/* start memorizing ray's trail */

	for (i = 0; i < len; i++) {
		if (i)
			trail[i] = trail[i - 1];

		dn = mvindir(dir, trail[i], 1);	/* move ptr in ray dir */

		/* make sure the ray doesn't go outta bounds */
		if (!cinbounds(scrline(dn), scrcol(dn)))
			break;

		trail[i] = dn;

		/* if wand of digging dig thru walls & rock */
		if (off == DIGGING && letsdig(trail[i]))
			break;

		if ((trail[i]->d_data & D_STONE) && bounce(&dir, &trail[i])
				&& !m) {
			if (insight(trail[i]))
				pline("The %s bounces off a wall!",
								bname(off));
		} else if (trail[i]->d_data & D_MONSTER) {
			MONSTER	*mo = what_mon(trail[i]);

			/* does ray miss the mon? mag missles always hit */
			if (rnd(20) < needtohit(mo->m_perm->p_ac, 8) &&
							off != MAGMIS) {
				if (insight(trail[i]))
					pline("The %s wizzed past the %s!",
						bname(off), mname(mo));

				/* mons stop guarding stairs when bothered */
				if (mo->m_data & M_PARALYZED && mo->m_d->
							d_data & D_STAIRCASE)
					mo->m_data &= ~M_PARALYZED;

				if (rnd(2))	/* chance mon will waken */
					mo->m_data |= (M_WAKEUP|M_AGGRAVATED);
			} else {
				/* mons always waken when HIT */
				mo->m_data |= (M_WAKEUP | M_AGGRAVATED);

				/* sometimes become unscared when hit */
				if (mo->m_data & M_SCARED && rnd(2))
					mo->m_data &= ~M_SCARED;

				/* mons stop guarding stairs when hit */
				if (mo->m_data & M_PARALYZED && mo->m_d->
							d_data & D_STAIRCASE)
					mo->m_data &= ~M_PARALYZED;

				if (insight(trail[i]))
					pline("The %s hit the %s!",bname(off),
								mname(mo));
				wand_dam(off, mo, m);

				if (rnd(2))	/* might be end of zap */
					break;
			}
		} else if (trail[i] == u.u_d) {
			stop();

			if (rnd(20) >= needtohit(u.u_ac, 8) || off==MAGMIS) {
				pline("The %s hit you!", bname(off));

				if (lrfinger(MAGABSORB) && !rnd(3))
				      pline("Your ring sucks up the energy!");
				else wand_dam(off, (MONSTER *)0, m);

				if (rnd(2))	/* might be end of zap */
					break;
			} else pline("The %s wizzed past you!", bname(off));
		}

		if (off == POLYMORPH)	/* multi-colored ray for poly */
			color((char)rnd(16));
		else color(wcolor[off - DRAINLIFE]);

		dat(trail[i], bolt[dir % 4]);	/* print the ray */

		/* magic missile have a trail of only one space, all others
		   have a trail of 5 spaces */
		if (i >= (off == MAGMIS ? 1 : 5))
			putwhat(trail[i - (off == MAGMIS ? 1 : 5)]);

		if (!m)
			tick_tock(1);
	}

	for (len = 0; len < i; len++)	/* clean up the screen */
		putwhat(trail[len]);
}

struct	{
	int	bd_loc[2],
		bd_newdir[2];
} bdata[4] = {
{	{ DOWN, LEFT }, 	{ UPLT, DNRT }		},
{	{ LEFT, UP },		{ UPRT, DNLT }		},
{	{ UP, RIGHT },		{ DNRT, UPLT }		},
{	{ RIGHT, DOWN}, 	{ DNLT, UPRT }		},
};

/*
 * BOUNCE: this is my hyper-esoteric bounce algorithm for wand bolts
 */

int
bounce(dir, d)
int			*dir;
register DUNGEON	**d;
{
	register	count = 0,
			i;
	DUNGEON		*du;

	/* if ray is not in diag direction, the bolt just rebounds in the
	   opposite direction */
	if (!(*dir % 2)) {
		*dir = oppdir(*dir);
		return YES;
	}

	/* this looks at the surrounding walls and bounces accordingly.  if
	   you can't figure it out, GOOD! (it's proprietary) */
	for (i = 0; i < 2; i++) {
		du = mvindir(bdata[*dir / 2].bd_loc[i], *d, 1);

		if (du->d_data & (D_STONE | D_DOOR))
			count += (i + 1);
	}

	/* bolts come straight out of corners */
	if (count > 2) {
		pline("It rebounds from a corner!");
		*dir = oppdir(*dir);

		return NO;
	}

	/* change dungeon location and direction of bolt */
	*d = mvindir(bdata[*dir / 2].bd_loc[!(count - 1)], *d, 1);
	*dir = bdata[*dir / 2].bd_newdir[count - 1];

	return NO;
}

/*
 * WAND_DAM: handle damage done to player or monsters when hit with a ray
 */

void
wand_dam(off, m, who)
register MONSTER	*m,
			*who;
{
	register	letter = (m ? m->m_perm->p_letter : 0);
	int		dam = 0;

	switch (off) {
	case DRAINLIFE:
		{
			/* organize ptrs to TO and FROM hit points */
			int     *zapper = (who ? &who->m_hp : &u.u_hp),
				*victim = (m ? &m->m_hp : &u.u_hp);

			/* wand has no effect if the zapper has less than
			   5 hps because it also takes away hps from him */
			if (*zapper < 5)
				return;

			/* some monsters are immune to drain life */
			if (m && strchr("ARGL", letter)) {
				if (!who)
					pline("It doesn't seen to be affected.");

				return;
			}

			dam = *victim;

			/* figure out how much damage to do to the victim.
			   but never do more than twice as much as the zapper
			   has because he has hps taken away too */
			if (dam / 2 >= *zapper)
				dam = (*zapper - 1) * 2;

			*zapper -= dam / 2;

			/* damage to player is handled at the end of this
			   procedure so we won't do it here */

			blineflg |= REDRAW_HP;
		}

		break;

	case SLOWMON:
		if (m)
			m->m_data |= M_SLOWED;
		else {
			MONSTER	*mo;

			pline("You feel slow and lethargic.");

			/* make the player seem slow by making mons fast! */
			for (mo = mons; mo < &mons[nummons]; mo++)
				mo->m_data |= M_SPEEDED;
		}

		break;

	case POLYMORPH:
		if (m) {
			if (letter == 'L') {
				if (!who)
					pline("The %s doesn't seem affected!",
						mname(m));
				return;
			}

			polymon(m);
		} else polymorph(50 + rnd(50));

		break;

	case MONINVIS:
		if (m)
			/* zap routine will take care of reprinting mon */
			m->m_data |= M_INVIS;
		else mkmeinvis(50 + rnd(100));

		break;

	case FEAR:
		if (m)
			m->m_data |= M_SCARED;
		else {
			/* since the player can't really get scared, we make
			   him sleepy and confused, instead */

			if (!(u.u_data & UD_SLEEPING))
				mkmesleep(1 + rnd(3));

			if (!confused())
				mkmeconfused(5 + rnd(7));
		}

		break;

	case STRIKING:
		dam = damage("6d4");
		break;

	case FIRE:
		dam = damage("9d4");

		if (m) {
			if (strchr("diqyDEV", letter)) {
				if (!who)
					pline("It doesn't seem to be affected.");
				return;
			}

			/* cold monsters take EXTRA damage! */
			if (letter == 'F' || letter == 'c' || letter == 'Y')
				dam *= 2;
		} else if (lrfinger(FIRERES)) {
			pline("You don't feel too hot!");
			dam /= 3;
		}

		break;

	case FROST:
		dam = damage("8d3");

		if (m) {
			if (letter == 'c' || letter == 'l')
				dam /= 2;
			else if (strchr("dvyEV", letter))
				dam *= 2;
			else if (letter == 'F' || letter == 'L') {
				if (!who)
					pline("It doesn't seem affected.");
				return;
			}
		} else if (lrfinger(WARMTH)) {
			dam /= 3;
			pline("You don't feel too cold!");
		}

		break;

	case TELEAWAY:
		teleaway(m);
		break;

	case TELETO:
		teleto(m);
		break;

	case CANCEL:
		if (m)
			m->m_data |= M_CANCELLED;

		break;

	case LIGHTNING:
		dam = damage("5d6");

		if (m) {
			if (letter == 'G') {	/* golems love lightning */
				if (!who)
					pline("The huge %s seem to grow even larger (and meaner too!)",
						mname(m));
				dam = -(dam/4);
			} else if (letter == 'L') {
				if (!who)
					pline("The %s doesn't seem affected!",
						mname(m));
				return;
			}
		}

		break;

	case HASTEMON:
		if (m)
			m->m_data |= M_SPEEDED;
		else mkmespeedy(4 + rnd(5));

		break;

	case MAGMIS:
		dam = damage("3d3");
		break;

	case WPARALYSIS:
		if (m)
			m->m_data |= M_PARALYZED;
		else mkmesleep(5 + rnd(5));

		break;

	default:
		return;
	}

	if (m) {
		m->m_hp -= dam;
		ck_mhp(m, NO);
	} else chg_hp(-dam);
}

/*
 * PR_WAND: executes all of the non-bolt wands like illumination
 */

int
pr_wand(off, m)
register		off;
register MONSTER	*m;
{
	if (m && (off == ILLUMINATION || off == CURING || off == TRAPDET ||
				off == WMAGDET))
		return NO;

	switch (off) {
	case ILLUMINATION:
		lightroom();
		break;

	case CURING:
		pline("You feel strange.");
		healme(NO, NO);

		break;

	case TRAPDET:
		pline("The rod vibrates.");
		trapdet();
		sdoordet();

		break;

	case WMAGDET:
		pline("The wand jumps up, spins around, and lands back in your hand!");
		detect(NO);

		break;

	default:
		return YES;
	}

	return NO;
}

/*
 * SDOORDET: detects all of the secret doors on the dungeon level and prints
 * them too!
 */

void
sdoordet()
{
	register DOOR	*d;

	for (d = doors; d < &doors[numdoors]; d++) {
		if ((d->dr_d->d_data & (D_DOOR | D_WALL))==(D_DOOR|D_WALL)) {
			d->dr_d->d_what = DOORC;
			d->dr_d->d_data &= ~(D_WALL | D_STONE);
			d->dr_d->d_data |= D_SEEN;
			putwhat(d->dr_d);
		}
	}
}

char	*wnames[NUMWANDS] = { "wisp of death", 0, 0, 0, "chromatic spray", 0,
		0, 0, 0, "force", "sizzling flame", "crystaline frost", 0, 0,
		0, "crackling bolt", 0, 0, 0, "phosphorescent globe", 0 };

/*
 * BNAME: returns the description of a bolt/ray
 */

char	*
bname(off)
register	off;
{
	if (wnames[off - DRAINLIFE])
		return wnames[off - DRAINLIFE];

	return "ray";
}

/*
 * LETSDIG: handles a staff of digging going thru walls and rock
 */

int
letsdig(d)
register DUNGEON *d;
{
	register ROOM	*r;

	/* if it hits a secret door, make it visible */
	if ((d->d_data & (D_DOOR | D_WALL)) == (D_DOOR | D_WALL)) {
		d->d_what = DOORC;
		d->d_data &= ~(D_WALL | D_STONE);
	} else if (d->d_data & D_WALL) {
		/* if ray hits a torch, WHAMO! the torch is gonzo */
		if (strchr(TORCHC, d->d_what) && (r = what_room(d))->r_data
				& R_TORCHED && !(r->r_data & R_MAGICLIT)) {
			if (insight(d))
				pline("The room goes dark!");

			dk_room(r);
			red_box(r->r_uline, r->r_ucol, r->r_lline,r->r_lcol);
			r->r_data &= ~R_TORCHED;
			know(u.u_d, YES);
		}

		if (!mk_door(d, 0))	/* can't make a door? */
			return YES;
	} else if (d->d_what == ROCK) {	/* tunnel thru rock */
		d->d_data &= ~D_STONE;
		d->d_what = CORRIDOR;
	}

	return NO;
}

/*
 * TELETO: makes a monster teleport adjacent to the player
 */

void
teleto(m)
register MONSTER *m;
{
	if (m)
		mteleport(m, near_loc(u.u_d));
	else teleport((DUNGEON *)0);	/* heh, secret ability */
}

/*
 * TELEAWAY: makes a monster teleport away from the player someplace
 */

void
teleaway(m)
register MONSTER *m;
{
	if (m)
		mteleport(m, (DUNGEON *)0);
	else teleport((DUNGEON *)0);	/* heh, yet another */
}
