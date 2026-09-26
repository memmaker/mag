/*
 *  		MAG - A Dungeon Adventuring Game
 *
 *	Copyright (C) 1986,87,88 by Michael J. Teixeira
 *
 *  General permission to copy or modify, but not for profit, is hereby
 *  granted, provided that the above copyright notice is included.
 *
 */

#include	<memory.h>
#include	<string.h>
#include	"mag.h"

/*
 * MK_EVENT: insert an event in the event list
 */

void
mk_event(when, where)
register unsigned	when;
 register		(*where)();
{
	register struct event	*e;

	/* look thru the event list to see if this new event is already in
	   it.  if it is, then just add the time instead of creating two */
	for (e = events; e < &events[numevents]; e++) {
		if (e->e_proc == where) {
			e->e_when += (long)when;
			return;
		}
	}

	if (numevents >= MAXEVENTS) {
		pline("Ran out of events!");
		return;
	}

	/* stick the new event at the end of the event list */
	events[numevents].e_when = u.u_moves + (long)when;
	events[numevents++].e_proc = where;
}

/*
 * UPDEVENT: go thru the event list and execute any ready events
 */

void
updevent()
{
	register struct event	*e;
	register		again;
	int			(*proc)(void);

	/* occasionally prolong till-hungry-time if wearing sustinance */
	if (!rnd(3) && lrfinger(SUSTINANCE))
		efood->e_when++;

	for (e = events; e < &events[numevents]; ) {
		if (e->e_when <= u.u_moves) {	/* event ready to go? */
			proc = e->e_proc;

			/* if the event proc returns non-zero, it means
			   the event will occur again in the returned number
			   of time units */
			if (again = (*proc)()) {
				e->e_when = u.u_moves + (long)again;
				e++;
				continue;
			}

			/* after event expires, remove it from list, but
			   make sure it's not the permanent events hungry
			   or regeneration */
			if (e > efood && --numevents > 2)
				memmove(e, &events[numevents], sizeof 
							(struct event));
		} else e++;
	}
}

/*
 * RM_EVENT: delete an event from the list and don't execute it
 */

void
rm_event(func)
 register	(*func)();
{
	register struct event   *e;

	for (e = events; e < &events[numevents]; ) {
		if (e->e_proc == func) {
			memmove(e, e+1, (numevents - ((e - events)+1)) *
				sizeof (struct event));
			numevents--;
		} else e++;
	}
}

/*
 * DOEVENT: look thru event list and force execution of any matching events
 * to occur next event update
 */

int
doevent(func)
 register	(*func)();
{
	register struct event	*e;

	for (e = events; e < &events[numevents]; e++) {
		if (e->e_proc == func) {
			e->e_when = u.u_moves;
			return YES;
		}
	}

	return NO;
}

/*
 * NOCONMON: discontinue confuse monster
 */

int
noconmon()
{
	pline("Your hands stop glowing.");
	u.u_data &= ~UD_CONMON;
	stop();

	return NO;
}

/*
 * NOCONFUSE: discontinue player confusion
 */

int
noconfuse()
{
	pline("You feel less confused now.");
	u.u_data &= ~UD_CONFUSED;
	stop();

	return NO;
}

/*
 * NOBLIND: discontinue player blindness
 */

int
noblind()
{
	pline("The veil of darkness lifts!");
	u.u_data &= ~UD_BLIND;
	know(u.u_d, YES);
	readyroom(u.u_r);
	stop();

	return NO;
}

/*
 * NOSLEEP: discontinue player unconsciousness
 */

int
nosleep()
{
	pline("You awaken.");
	u.u_data &= ~UD_SLEEPING;

	return NO;
}

/*
 * NOPARA: discontinue player paralysis
 */

int
nopara()
{
	pline("You can move again.");
	u.u_data &= ~UD_SLEEPING;

	return NO;
}

/*
 * NOSPEED: discontinue player haste self
 */

int
nospeed()
{
	pline("You feel yourself slowing down.");
	u.u_data &= ~UD_HASTED;

	return NO;
}

/*
 * NOSTUCK: handles the player getting out of a bear or weed trap
 */

int
nostuck()
{
	static	count = NO;	/* holds how long the player is stuck */

	/* stranglers take 1 hit point per time unit */
	if (u.u_data & UD_STRANGLE) {
		killer = "strangle weed trap";
		chg_hp(-1);
	}

	/* if count is zero (player just stepped on the trap), then it
	   needs to be reset to the length of the stuckage (stickage?) */
	if (!count)
		count = lrfinger(FREEACT) ? 2 : rnd(5) + 5;

	/* only motion commands will reduce the stuck count */
	if ((strchr("lkjhyubnLKJHYUBN", typed) || (typed >= 198 && typed <=
			209)) && --count == NO) {
		pline("You escape!");
		u.u_data &= ~(UD_STUCK | UD_STRANGLE);
	}

	/* this event continues every time unit until player escapes */
	return (count ? 1 : NO);
}

/*
 * MKMESPEEDY: makes the player hasted
 */

void
mkmespeedy(howlong)
register	howlong;
{
	u.u_data |= UD_HASTED;
	mk_event(howlong, nospeed);
}

/*
 * MKMEINVIS: make the player invisible
 */

void
mkmeinvis(howlong)
register	howlong;
{
	/* in order to prevent unusual problems, the player cannot be
	   polyphorphed while invisible */
	doevent(nopoly);
	updevent();

	pline("You can no longer see yourself!");
	u.u_data |= UD_INVIS;
	u.u_sym = IPLAYER;
	mk_event(howlong, noinvis);
}

/*
 * MKMEBLIND: make our hero blind as a bat
 */

void
mkmeblind(howlong)
register	howlong;
{
	u.u_data |= UD_BLIND;
	mk_event(howlong, noblind);

	if (u.u_r && (u.u_r->r_data & R_TORCHED) == NO) {
		know(u.u_d, NO);
		red_box(u.u_r->r_uline, u.u_r->r_ucol, u.u_r->r_lline,
				u.u_r->r_lcol);
	}
}

/*
 * MKMECONFUSED: make player befuddled
 */

void
mkmeconfused(howlong)
register	howlong;
{
	pline("You just don't know what's happening!");
	u.u_data |= UD_CONFUSED;
	mk_event(howlong, noconfuse);
}

/*
 * FIX_STR: all strength modifications go thru this procedure to make sure
 * that if current str goes above max str, then max str is raised and other
 * good stuff
 */

int
fix_str(amount)
register	amount;
{
	/* str doesn't go down if using magical protection */
	if (amount < 0 && lrfinger(SUSTAINSTR))
		return NO;

	u.u_str += amount;

	/* raise max str if str goes above it */
	if (u.u_str > u.u_maxstr)
		u.u_maxstr = u.u_str;

	blineflg = REDRAW_ALL;

	/* if str goes down, then make an event to raise it later */
	if (amount < 0) {
		rm_event(nopoison);
		mk_event(500 + rnd(500), nopoison);
	}

	return YES;
}

/*
 * NOPOISON: restores a strength unit if needed
 */

int
nopoison()
{
	if (u.u_str < u.u_maxstr) {
		u.u_str++;
		blineflg = REDRAW_ALL;

		/* if not at max, create another event */
		if (u.u_str < u.u_maxstr)
			return (rnd(500) + 500);
	}

	return NO;
}

/*
 * NOINVIS: discontinue player invisibility
 */

int
noinvis()
{
	pline("You can see yourself again.");
	u.u_data &= ~UD_INVIS;
	u.u_sym = PLAYER;
	stop();

	return NO;
}

/*
 * MKMESLEEP: make the player unconscious
 */

void
mkmesleep(howlong)
register	howlong;
{
	u.u_data |= UD_SLEEPING;
	mk_event(howlong, nosleep);
}

/*
 * MKMEPARA: paralyse the player
 */

void
mkmepara(howlong)
register	howlong;
{
	u.u_data |= UD_SLEEPING;
	mk_event(howlong, nopara);
}

/*
 * NOHERO: restore the player to his former wimpy self after the effects of
 * a heroism potion wear off
 */

int
nohero()
{
	pline("You feel kind of wimpy again now.");

	if ((u.u_hp -= 30) < 1)	/* so he won't die because of it */
		u.u_hp = 1;

	u.u_maxhp -= 30;
	u.u_elevel -= 3;

	/* make sure the player doesn't lose any experience */
	u.u_epoints -= (needexp(oldelev + 3) - oldexp);

	blineflg |= REDRAW_ALL;

	return NO;
}

/*
 * FASTSEARCH: search event used with a ring of searching
 */

int
fastsearch()
{
	dosearch(NO);
	return 3;
}

/*
 * NOPOLY: return the player to his normal human shape
 */

int
nopoly()
{
	pline("You feel normal again.");

	if (u.u_data & UD_INVIS)
		u.u_sym = IPLAYER;
	else u.u_sym = PLAYER;

	stop();

	return NO;
}

/*
 * REGEN: give the player back some hit points if needed
 */

int
regen()
{
	chg_hp(1);	/* always gets at least one */

	/* higher experience levels get more and more hp's back */
	if (u.u_elevel > 3)
		chg_hp(rnd(2));

	if (u.u_elevel > 6)
		chg_hp(rnd(u.u_elevel / 3));

	/* rings of regen cause food to be used quicker than normal */
	if (lrfinger(REGEN)) {
		if (efood->e_when > u.u_moves + 210L)
			efood->e_when--;

		return 1;
	}

	/* experience level also affects how often regen occurs */
	if (u.u_elevel < 11)
		return 16 - (u.u_elevel / 2) * (2 + rnd(2));

	return 11 - (u.u_elevel / 4) * 2;
}

/*
 * GETHUNGRY: do proper action for difference hunger conditions
 */

int
gethungry()
{
	if (u.u_data & UD_FAINTING) {
		pline("You faint from lack of food.");
		mkmesleep(rnd(5) + 2);
		efood->e_when += (long)(rnd(10) + rnd(10) + 1);
		return NO;
	}

	if (u.u_data & UD_HUNGRY) {
		pline("You are starting to feel faint from lack of food.");
		u.u_data |= UD_FAINTING;
		efood->e_when += 100L;
	} else {
		pline("You are starting to feel hungry.");
		u.u_data |= UD_HUNGRY;
		efood->e_when += 100L;
	}

	return NO;
}

/*
 * MKWANDMON: have a new monster wander onto the current dungeon level
 */

int
mkwandmon()
{
	register DUNGEON *d;
	
	if (!(u.u_data & UD_LABYRINTH)) {	/* no monsters in labs */
		do d = location(SFLOOR);
		while (what_room(d) == u.u_r);

		mk_mon(d, -1);
	}

	return 75 + rnd(100);
}

/*
 * ETELE: a teleportation event for horrible rings of teleportation
 */

int
etele()
{
	teleport(NO);
	return rnd(40) + rnd(40) + 5;
}

/*
 * POLYMORPH: temporarily change the shape of the player into a monster
 */

void
polymorph(howlong)
register	howlong;
{
	register	i = rnd(NUMMON);

	u.u_sym = pmon[i].p_letter;
	pline("You're starting to feel rather %s-ish.", pmon[i].p_name);
	mk_event(howlong, nopoly);
}

/*
 * NOSEEINV: discontinue see invisible power for player
 */

int
noseeinv()
{
	if (lrfinger(SEEINVIS) == NO)
		u.u_data &= ~UD_SEEINV;

	return NO;
}

/*
 * NOLEVIT: discontinue player levitation
 */

int
nolevit()
{
	if (rnd(2))
		pline("You gently sink to the ground.");
	else {
		pline("You crash to the ground!");
		chg_hp(-(rnd(4) + 1));
	}

	u.u_data &= ~UD_LEVITATE;
	pickup(u.u_d); /* grab anything on the ground that person lands on */

	return NO;
}

/*
 * CHANGEALL: simulates the player tripping on LSD
 */

int
changeall()
{
	register LEVOBJ	*l;
	register	c;

	if (!ntimes) {
		/* set all the objects to redisplay */
		for (l = lobjs; l < &lobjs[numlobjs]; l++) {
			if (insight(l->l_d)) {
				c = scrcol(l->l_d);
				set_red(scrline(l->l_d), c, c);
			}
		}
	}

	/* there is a 74/75 chance that the trip will continue */
	if (rnd(75))
		return 1;

	u.u_data &= ~UD_DELUSION;

 	/* when the trip is over, make sure everything return to normal */
	for (l = lobjs; l < &lobjs[numlobjs]; l++) {
 		if (insight(l->l_d)) {
			c = scrcol(l->l_d);
			set_red(scrline(l->l_d), c, c);
		}
	}
 
	if (!blind())
		pline("Everything seems so plain now.");

	return NO;
}

/*
 * OILEXPLODE: handle the explosion of an oil flask in player's pack
 */

int
oilexplode()
{
	register OBJECT	*o;
	register	flag = NO;

	/* go thru inventory and remove all burning oil flask */
	for (o = inv; o < &inv[numinv]; ) {
		if (o->o_offset == OILFLASK && o->o_data & O_IGNITED) {
			lessweight(o, o->o_quantity);
			rm_inv(o);
			flag++;
		} else o++;
	}

	/* only if there was still a flask burning do the explosion */
	if (flag) {
		stop();
		pline("A burning oil flask in your pack explodes!");
		bell();
		redraw();
		killer = "exploding oil flask";
		explode(u.u_d);
	}

	return NO;
}
