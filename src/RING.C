/*
 *  		MAG - A Dungeon Adventuring Game
 *
 *	Copyright (C) 1986,87,88 by Michael J. Teixeira
 *
 *  General permission to copy or modify, but not for profit, is hereby
 *  granted, provided that the above copyright notice is included.
 *
 */

#include	"mag.h"

/*
 * PUTON: have the player put a magic ring on a hand
 */

int
puton(o)
register OBJECT	*o;
{
	register	hand;

	if (o->o_data & O_INUSE) {	/* wants to remove ring */
		if (!iscursed(o, YES))
			unring(o == rightring);

		return NO;
	}

	if ((hand = gethand(leftring, rightring)) == -1)
		return -1;

	/* 4 & 5 are the ring locations in the u_use array */
	if (u.u_use[4+hand]) {
		if (iscursed(u.u_use[4+hand], NO)) {
			pline("You can't.  The ring that you already have on is stuck!");
			bell();

			return NO;
		}

		unring(hand);
	}

	if (iscursed(o, NO)) {
		pline("The ring adheres to your hand!");
		bell();
		o->o_data |= O_SEECURSED;
	}

	doring(o, 1);
	o->o_data |= O_INUSE;
	u.u_use[4+hand] = o;

	if (pobj[o->o_offset].po_flag & SID)	/* self identiying? */
		ident(o);

	return YES;
}

/*
 * UNRING: takes a ring off the given hand
 */

void
unring(hand)
register	hand;
{
	u.u_use[4+hand]->o_data &= ~O_INUSE;
	doring(u.u_use[4+hand], -1);
	pline("You are no longer wearing %s.", form(u.u_use[4+hand], YES));
	u.u_use[4+hand] = (OBJECT *)0;
}

/*
 * DORING: do whatever happens when player puts on or removes a ring.  HOW
 * equals 1 when putting on and -1 when removing
 */

void
doring(o, how)
register OBJECT	*o;
{
	register		enchant = o->o_enchant * how;
	register MONSTER	*m;

	switch (o->o_offset) {
	case DECRHP:
		u.u_hp += enchant;
		u.u_maxhp += enchant;
		killer = "ring of decrease hit points";
		chg_hp(0);	/* check for death */

		break;

	case VULNERABILITY:
		u.u_ac += enchant;
		break;

	case WEAKNESS:
		u.u_str += enchant;
		u.u_maxstr += enchant;

		break;

	case AGGMON:
		if (how == YES) {
			/* stir up those monsters! */
			for (m = mons; m < &mons[nummons]; m++) {
				m->m_data &= ~M_SLEEPING;
				m->m_data |= M_AGGRAVATED;
			}
		}

		break;

	case RTELEPORT:
		if (how == YES)
			mk_event(50 + rnd(25), etele);
		else rm_event(etele);

		break;

	case SEARCHING:
		if (how == YES)
			mk_event(2, fastsearch);
		else rm_event(fastsearch);

		break;

	case SEEINVIS:
		if (how == YES) {
			u.u_data |= UD_SEEINV;
			pr_mon();
		} else u.u_data &= ~UD_SEEINV;

		break;

	case ADDSTR:
		u.u_str += enchant;
		u.u_maxstr += enchant;

		break;

	case INCRHP:
		u.u_hp += enchant;
		u.u_maxhp += enchant;
		killer = "ring of increase hit points";
		chg_hp(0);	/* check for death */

		break;

	case PROTECTION:
		u.u_ac += enchant;
		break;

	default:
		return;
	}

	blineflg = REDRAW_ALL;
}

/*
 * TFINGER: add up enchantments of all worn rings of type OFF
 */

int
tfinger(off)
register	off;
{
	register	ench = 0;

	if (lfinger(off))
		ench += leftring->o_enchant;

	if (rfinger(off))
		ench += rightring->o_enchant;

	return ench;
}

/*
 * LFINGER: returns whether ring type OFF is being worn on left hand
 */

int
lfinger(off)
register	off;
{
	return (leftring && (leftring->o_offset == off || (leftring->
			o_offset == RANDOMNESS && !rnd(20))));
}

/*
 * RFINGER: returns whether ring type OFF is being worn on right hand
 */

int
rfinger(off)
register	off;
{
	return (rightring && (rightring->o_offset == off || (rightring->
			o_offset == RANDOMNESS && !rnd(20))));
}

/*
 * LRFINGER: returns how many rings of type OFF are currently being worn
 */

int
lrfinger(off)
register	off;
{
	return (lfinger(off) + rfinger(off));
}
