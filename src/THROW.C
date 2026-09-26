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
 * THROW: handles the player throwing something from inventory
 */

int
throw(o)
register OBJECT	*o;
{
	register	hf = howfar(o->o_offset),
			wo = (rightweapon ? (int)rightweapon->o_offset : -1);
	int		dir,
			oo = o->o_offset,
			th = 0,
			td = 0;

	if (o->o_data & O_INUSE) {
		pline("You are still using that!");
		bell();

		return -1;
	}

	/* you can throw your secondary weapon */
	if (o == secweap)
		secweap = (OBJECT *)0;

	dir = getdir(YES);
	erase();

	if (dir < 0)
		return -1;

	/* add weapon-in-hand enchantments to the missile weapon only if
	   the wep-in-hand is the corresponding wep for the missile wep and
	   it's in the player's right hand */
	if ((wo == LONGBOW && oo == ARROW) || (wo == CROSSBOW && oo ==
			BOLT) || (wo == SLING && oo == BULLET)) {
		th += rightweapon->o_enchant;
		td += rightweapon->o_wepench;
	}

	/* only add missile wep enchantments for weps (not potions) */
	if (o->o_type == WEAPON) {
		th += o->o_enchant;
		td += o->o_wepench;
	}

	if (dothrow((MONSTER *)0, u.u_d, copy_obj(o, 1), dir, hf, (th +
			tfinger(DEXTERITY) + tfinger(FUMBLING)), (td +
			tfinger(INCRDAM) + tfinger(DECRDAM))))
		minusone(o);

	return NO;
}

/*
 * DOTHROW: this is the generic procedure which will throw any object across
 * the dungeon
 */

int
dothrow(m, sd, o, dir, len, pthit, ptdam)
register MONSTER	*m;
register DUNGEON	*sd;
register OBJECT		*o;
{
	int    	i,
	       	oc = obj_color(o);
	DUNGEON	*d = sd;

	/* LEN (distance) can't be 0 for monsters */
	if (!len) {
		pline("You're not strong enough to throw it!");
		bell();

		return NO;
	}

	for (i = 1; i < len; i++) {
		d = mvindir(dir, d, 1);

		/* if obj hits a wall, back the obj 1 space from wall */
		if (d->d_data & D_STONE) {
			d = mvindir(dir, d, -1);
			break;
		}

		if (d->d_data & D_MONSTER) {
			MONSTER	*mo = what_mon(d);

			if (rnd(20) + pthit >= needtohit(mo->m_perm->p_ac,
					(m ? (int)m->m_perm->p_hitdice :
					u.u_elevel))) {
				mo->m_data |= (M_WAKEUP | M_AGGRAVATED);

				/* mons stop guarding stairs when bothered */
				if ((mo->m_data & M_PARALYZED) && (mo->m_d->
						d_data & D_STAIRCASE))
					mo->m_data &= ~M_PARALYZED;

				/* sometimes mons become unscared when hit */
				if ((mo->m_data & M_SCARED) && rnd(2))
					mo->m_data &= ~M_SCARED;

				/* erase old missile location */
				if (i > 1)
					putwhat(mvindir(dir, d, -1));

				if (insight(d))
					pline("The %s hit the %s!",
						typename(o), mname(mo));

				if (!m)
					mo->m_hp -= strbonus() +
						tfinger(DECRDAM) +
						tfinger(INCRDAM);

				if (!throwdam(mo, o, d) && rnd(5))
					add_minv(mo, o);

				/* if mon still alive, it wakes up! */
				if (!ck_mhp(mo, (m ? NO : YES)))
					mo->m_data |= M_WAKEUP;

				return YES;
			}

			if (insight(d))
				pline("The %s missed the %s.", typename(o),
					mname(mo));

			if (rnd(2)) {	/* it sproings off somewhere */
				dir = rnd(8);
				len = i + 2;
				mo->m_data |= (M_WAKEUP | M_AGGRAVATED);
			}
		} else if (d == u.u_d) {
			stop();

			if (!insight(sd))
				pline("You hear a swish from the %s.",
					dirdesc[what_dir(u.u_d, sd)]);

			if (rnd(20) + pthit >= needtohit(u.u_ac, (m ?
					m->m_perm->p_hitdice : 7))) {
				pline("The %s hits you!", poname(o));
				throwdam((MONSTER *)0, o, d);
				chg_hp(-ptdam);

				if (i > 1) /* erase last location */
					putwhat(mvindir(dir, d, -1));

				return YES;
			} else {
				pline("The %s missed you!", poname(o));

				if (rnd(2)) {	 /* the sproing effect */
					dir = rnd(8);
					len = i + 2;
				}
			}
		}

		if (i > 1)	/* erase last location */
			putwhat(mvindir(dir, d, -1));

		if (insight(d)) {
			color(oc);
			dat(d, CITEMS[o->o_type]);
		}

		if (!m)
			tick_tock(1);

		if (d->d_data & D_DOOR)	/* missile stops when hits door */
			break;
	}

	if (o->o_data & O_IGNITED) {
		killer = "exploding oil flask";
		explode(d);
	} else if ((o->o_type == POTION || o->o_offset == OILFLASK) &&
					rnd(2)) {
		pline("%s smashes on the floor.", capital(obj_str(o)));
		putwhat(d);
	} else add_lobj(d, o);

	return YES;
}

/*
 * HOWFAR: returns how far a missile weapon should travel
 */

int
howfar(off)
register	off;
{
	register	o = (rightweapon ? (int)rightweapon->o_offset : -1);

	if (off == BOULDER && u.u_str < 21)
		return 0;	/* can't throw boulder until STRONG! */

	if ((off == ARROW && o == LONGBOW) || (off == BULLET && o == SLING)
			|| (off == BOLT && o == CROSSBOW) || off == DART ||
			off == DAGGER || off == HANDAXE || off == SPEAR ||
							off == OILFLASK)
		return 10 + rnd(6);

	return 3 + rnd(3);
}

/*
 * THROWDAM: do damage to whatever is hit by a missile weapon
 */

int
throwdam(m, o, d)
register MONSTER	*m;
register OBJECT		*o;
DUNGEON			*d;
{
	register	i = o->o_offset;

	if (o->o_type == WEAPON) {
		i = damage(wepdam[i - ARROW]) + o->o_wepench;

		if (m)
			m->m_hp -= i;
		else chg_hp(-i);

		return NO;
	}

	/* player isn't affected by thrown potions, so we return here */
	if (!m)
		return NO;

	switch (o->o_offset) {	/* the potions and oil flasks */
	case BLINDNESS:
	case CONFUSION:
	case DELUSION:
		if (insight(m->m_d) && !(m->m_data & M_CONFUSED)) {
			pline("The %s appear to be confused.", mname(m));
			m->m_data |= M_CONFUSED;
		}

		break;

	case EXTRAHEAL:
	case HEALING:
		m->m_hp += rnd(8) + 2;	/* heal it! */
		break;

	case INVISIBILITY:
		if (insight(m->m_d))
			pline("Wow!  The %s just vanished!", mname(m));

		m->m_data |= M_INVIS;

		break;

	case PARALYSIS:
		m->m_data |= M_PARALYZED;
		break;

	case POISON:
	case ACID:
		if (insight(m->m_d))
			pline("The %s screeches in anguish!", mname(m));

		m->m_hp -= 2 + rnd(3);

		break;

	case SELFPOLY:
		polymon(m);
		break;

	case OILFLASK:
		if (o->o_data & O_IGNITED) {
			killer = "exploding oil flask";
			explode(d);
			return YES;
		} else if (insight(d))
			pline("The oil splatters all over the %s.",mname(m));

		break;
	}

	if (o->o_type == POTION)
		return YES;

	return NO;
}

/*
 * EXPLODE: create a (quite deadly) explosion
 */

void
explode(sd)
register DUNGEON	*sd;
{
	register		i;
	register DUNGEON	*d;
	MONSTER			*m;
	char			*look = "\xec\\\x1e/\x10\\\x1f/\x11";
	int			j;

	bell();

	for (j = 0; j < 10; j++) {
		color(rnd(2) ? (RED | INTENSE) : YELLOW);
		d = sd;

		for (i = 0; i < 9; i++) {
			d += circle[i];

			/* every other loop erases the explosion */
			if (j % 2)
				putwhat(d);
			else dat(d, look[i]);

			if ((d->d_data & D_MONSTER)) {
				m = what_mon(d);

				if (!strchr("diqyEV", m->m_perm->p_letter)) {
					m->m_hp -= (2 + rnd(2));
					ck_mhp(m, NO);
				}
			} else if (d == u.u_d) {
				if (lrfinger(FIRERES))
					chg_hp(-rnd(2));
				else chg_hp(-(2 + rnd(2)));
			}

		}

		tick_tock(1);
	}
}

/*
 * TICK_TOCK: use clock interrupt tick count to pause in 18ths of a second
 */

void
tick_tock(ticks)
register	ticks;
{
	long	dud = *(long far *)0x0040006c;

	while (*(long far *)0x0040006c < dud + ticks)
		;
}


