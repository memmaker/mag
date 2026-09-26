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
 * WHAT_TRAP: returns a ptr to the trap at the given location
 */

TRAP	*
what_trap(d)
register DUNGEON *d;
{
	register TRAP	*t;

	for (t = traps; t < &traps[numtraps]; t++)
		if (t->t_d == d)
			return t;

	pline("what_trap: Can't find trap!");

	return traps;	/* to prevent munging */
}

/*
 * DOTRAP: make a trap happen either to the player or a monster
 */

void
dotrap(d, m)
register DUNGEON *d;
register MONSTER *m;
{
	register TRAP	*t;
	void		(*proc)(MONSTER *);
	
	t = what_trap(d);	/* find which trap sprung */
	killer = "trap";
	proc = t->t_t->t_func;
	(*proc)(m);		/* call the trap procedure */

	/* traps only appear if the player steps on it, or a monster steps
	   on one IN SIGHT of the player */
	if (!m || insight(d))
		d->d_what = TRAPC;

	putwhat(d);
}

/*
 * MK_TRAPS: place all the traps on a newly created dungeon level
 */

void
mk_traps()
{
	register TRAP	*t;

	/* there are more traps deeping the dungeon */
	numtraps = 1 + rnd(2) + ((u.u_dlevel > 10) * rnd(4)) + ((u.u_dlevel >
							20) * rnd(4));

	/* make sure we don't try to make too many traps */
	if (numtraps > MAXTRAPS)
		numtraps = MAXTRAPS;

	/* fill the traps array one by one */
	for (t = traps; t < &traps[numtraps]; t++) {
		t->t_t = &trapd[rnd(NUMTRAPS)];	/* pick random trap type */

		do t->t_d = location(SFLOOR);	/* find a good location */
		while (t->t_d->d_data & D_STAIRCASE);

		t->t_d->d_data |= D_TRAPPED;
	}
}

/*
 * AVOID: handle when the player steps on a trap but doesn't set it off
 */

int
avoid()
{
	if ((u.u_d->d_what == TRAPC && rnd(2)) || !rnd(20)) {
		pline("You avoid a %s trap.", what_trap(u.u_d)->t_t->t_name);
		return YES;
	}

	bell();

	return NO;
}

/*
 * SPIDERWEB: spring a spider web trap
 */

void
spiderweb(m)
register MONSTER *m;
{
	if (!m) {
		if (avoid())
			return;

		pline("You are caught in a sticky, strandy spider web!");

		if (lrfinger(FREEACT))
			pline("Wait! The web melts and you pass through it.");
		else {
			u.u_data |= UD_STUCK;
			mk_event(1, nostuck);
		}
	} else {
		m->m_data |= M_STUCK;

		if (insight(m->m_d))
			pline("The %s gets caught in a spider web!",
				mname(m));
	}
}

/*
 * STUN: spring a stun trap
 */

void
stun(m)
register MONSTER *m;
{
	if (m) {
		pline("You hear a zapping sound.");

		if (rnd(2))
			m->m_data |= M_STUCK;
		else m->m_data |= M_CONFUSED;

		m->m_hp--;	/* so it'll die eventually */
		ck_mhp(m, NO);
	} else if (!avoid()) {
	  	pline("<<ZZRRRRAAAPP>>  An electric burning rips through your body!");
		physdam();
	}
}

/*
 * PHYSDAM: do one of three types of physical damage to the player
 */

void
physdam()
{
	switch (rnd(3)) {
	case 0:
		if (!confused())
			mkmeconfused(10 + rnd(10));

		break;

	case 1:
		if (!blind()) {
			pline("You can't see anything!");
			mkmeblind(rnd(10) + 5);
		}

		break;

	case 2:
		if (!(u.u_data & UD_SLEEPING)) {
			pline("You go unconsious!");
			mkmesleep(rnd(5) + 5);
		}
	}

	chg_hp(-(rnd(5) + 1));
}

/*
 * PENDULUM: spring a pendulum trap
 */

void
pendulum(m)
register MONSTER *m;
{
	if (m) {
		if (insight(m->m_d))
		       pline("A pendulum swings down and slices into the %s!",
							      mname(m));
		m->m_hp -= (rnd(5) + 5 + u.u_dlevel);
		ck_mhp(m, NO);
	} else {
		pline("A huge sharp pendulum blade swings down from above...");

		if (rnd(20) > u.u_elevel && (u.u_d->d_what != TRAPC ||
								rnd(2))) {
			pline("It slices into you!");
			killer = "pendulum trap";
			chg_hp(-(rnd(5) + 5 + u.u_dlevel));
		} else pline("It swishes past your head!");
	}
}

/*
 * WEED: spring a strangle weed trap
 */

void
weed(m)
register MONSTER *m;
{
	if (m) {
		if (insight(m->m_d))
			pline("A weed snakes out and wraps around the %s!",
								mname(m));
		m->m_data |= M_STUCK;
		m->m_hp -= rnd(4) + 1;
		ck_mhp(m, NO);
	} else if (!avoid()) {
		pline("A weed snakes out and wraps around your neck!");

		if (lrfinger(FREEACT))
			pline("You manage to slip from its grasp!");
		else {
			u.u_data |= UD_STRANGLE;
			mk_event(1, nostuck);
		}
	}
}

/*
 * POISNEED: spring a poison needle trap
 */

void
poisneed(m)
register MONSTER *m;
{
	if (m) {
		if (insight(m->m_d))
			pline("A needle jabs the %s.", mname(m));

		m->m_hp -= rnd(5) + 5;
		ck_mhp(m, NO);
	} else if (!avoid()) {
		pline("Ouch! A poison needle stabbed you in the foot.");
		killer = "poison needle";
		chg_hp(-(rnd(5) + 5));

		if (!fix_str(-(rnd(2) + 1)))
			pline("You feel fine!");
	}
}

/*
 * SLEEPGAS: spring a sleeping gas trap
 */

void
sleepgas(m)
register MONSTER *m;
{
	if (m) {
		if (insight(m->m_d))
			pline("A faint white wisp of mist encircles the %s.",
								mname(m));
		m->m_data |= M_SLEEPING;
	} else if (!avoid()) {
		pline("A curious white mist encircles you...you fall asleep.");
		u.u_data |= UD_SLEEPING;
		mk_event(rnd(5) + 5, nosleep);
	}
}

/*
 * BEAR: spring a bear trap
 */

void
bear(m)
register MONSTER *m;
{
	if (m) {
		if (insight(m->m_d))
			pline("A bear trap snaps shut on the %s.", mname(m));

		m->m_data |= M_STUCK;
		m->m_hp -= rnd(6) + 1;
		ck_mhp(m, NO);
	} else if (!avoid()) {
		pline("A bear trap snaps shut on your foot!");
		killer = "bear trap";
		chg_hp(-(rnd(6) + 1));
		u.u_data |= UD_STUCK;
		mk_event(lrfinger(FREEACT) ? 2 : rnd(5) + 5, nostuck);
	}
}

/*
 * FLYINGSPEAR: spring a flying spear trap
 */

void
flyingspear(m)
register MONSTER *m;
{
	int			dir = rnd(4) * 2; /* non-diag direction */
	register DUNGEON	*d = (m ? m->m_d : u.u_d);
	register ROOM		*r = (m ? m->m_r : u.u_r);

	/* spear traps fail in maze rooms.  I test R here even though traps
	   should always be in a room just to be safe */
	if (r && (r->r_data & R_MAZE)) {
		pline("You step on a trap trigger-stone but nothing happens.");
		return;
	}

	/* move to the wall in the chosen direction */
	while (what_room(d) == r)
		d = mvindir(dir, d, 1);

	d = mvindir(dir, d, -1);	/* back one space out of the wall */

	if (insight(d))
		pline("A spear shoots out from a hole in the wall!");

	killer = "flying spear";
	dothrow((MONSTER *)0, d, create_o(WEAPON, 0, 0, 0, SPEAR, 1),
						 oppdir(dir), 80, 1, 1);
}

/*
 * TELETRAP: spring a teleportation trap
 */

void
teletrap(m)
register MONSTER *m;
{
	register DUNGEON	*d;

	if (m)
		mteleport(m, (DUNGEON *)0);
	else if (!avoid()) {
		mkmeconfused(1 + rnd(3));	/* disorientation factor */
		d = u.u_d;
		teleport((DUNGEON *)0);
		d->d_data |= D_SEEN;
	}
}

