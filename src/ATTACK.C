/*
 *  		MAG - A Dungeon Adventuring Game
 *
 *	Copyright (C) 1986,87,88 by Michael J. Teixeira
 *
 *  General permission to copy or modify, but not for profit, is hereby
 *  granted, provided that the above copyright notice is included.
 *
 */

#include	<time.h>
#include	<string.h>
#include	<stdio.h>
#include	<process.h>
#include	"mag.h"

/*
 * UATTACK: handles player attacking mons hand-to-hand
 */

void
uattack(m)
register MONSTER	*m;
{
	register	roll,
			dam;
	OBJECT		*o;
	int		need = needtohit(m->m_perm->p_ac, u.u_elevel),
			hits = 0,
			i;

	/* this is to wake up the imp drag which sits on the Sapphire */
	if (m->m_perm == IMPDRAGON && sapp_lev())
		m->m_data &= ~M_PARALYZED;

	stop();
	pr_mon();
	redraw();

	if (!leftweapon && !rightweapon) {
		pline("You can't attack without a weapon!");
		bell();

		return;
	}

	m->m_data |= M_AGGRAVATED;	/* ooh, it's mad now! */

	if (lrfinger(FUMBLING) && rightweapon && rightweapon->o_type ==
			WEAPON && !iscursed(rightweapon, NO) && !rnd(10)) {
		pline("Ack!  You accidentally threw your %s.",
						poname(rightweapon));
		bell();
		o = rightweapon;
		unwield(RIGHTHAND);
		dothrow((MONSTER *)0, u.u_d, o, rnd(8), 2 + rnd(5), 0, 0);
		rm_inv(o);
		lessweight(o, o->o_quantity);

		return;
	}

	for (i = LEFTHAND; i <= RIGHTHAND; i++) {
		if (!u.u_use[i])	/* no weapon in hand? */
			continue;

		/* fumbling enchantment is negative. mons asleep or para are
		   +4 to hit against since they can't dodge.  weps in left
		   hand are -3 to hit since our hero is right-handed */
		roll = rnd(20) + tfinger(FUMBLING) + tfinger(DEXTERITY) +
			(m->m_data & (M_SLEEPING | M_PARALYZED) ? 4 : 0) -
			(i == LEFTHAND ? 3 : 0);

		/* if player levitates, -5 against certain small monsters */
		if (u.u_data & UD_LEVITATE && strchr("fijqrsv", m->m_perm->
				p_letter))
			roll -= 5;

		if (u.u_use[i]->o_type == WEAPON)
			roll += u.u_use[i]->o_enchant;
		else if (u.u_use[i]->o_offset == STRIKING)
			roll += 2;	/* staff of striking is +2 to hit */

		if (roll >= need) {
			hits++;

			if (u.u_use[i]->o_type == WEAPON)
				dam = damage(wepdam[u.u_use[i]->o_offset -
					ARROW]) + u.u_use[i]->o_wepench;
			else {		/* wielding a staff */
				dam = rnd(2) + 2;

				if (u.u_use[i]->o_offset == STRIKING &&
						u.u_use[i]->o_charges > 0) {
					dam += 6 + rnd(4);

					/* chance staff uses a charge */
					if (!rnd(15)) {
						u.u_use[i]->o_charges--;
						pline("The tip of the staff glows as you bring it down!");
					}
				}
			}

			dam += tfinger(INCRDAM)+tfinger(DECRDAM)+strbonus();

			/* prevent GIVING hit points! */
			if (dam < 1)
				dam = 1;

			m->m_hp -= dam;
		}
	}

	if (hits) {
		pline("You hit the %s%s", mname(m), (hits==1?"!":" twice!"));

		/* if mon dead, we're finished here */
		if (ck_mhp(m, YES))
			return;

		if (u.u_data & UD_CONMON && !(m->m_data & M_CONFUSED)) {
			/* mon gets saving throw by # of hit dice */
			if (rnd(20) > m->m_perm->p_hitdice) {
				pline("The %s appears confused.", mname(m));
				m->m_data |= M_CONFUSED;
			} else pline("The %s looks confused for just a moment.",
						mname(m));
		}

		if (m->m_perm==GOLEM && !rnd(3) && !(m->m_data&M_SPEEDED)) {
			pline("The %s begins berzerking wildly!", mname(m));
			m->m_data |= M_SPEEDED;
		}

		/* monsters will stop running if they are getting hit */
		if (m->m_data & M_SCARED && rnd(2))
			m->m_data &= ~M_SCARED;
	} else pline("You missed.");

	m->m_data |= M_WAKEUP;  /* it begins to wake up */

	/* mons stop guarding stairs when bothered */
	if (m->m_data & M_PARALYZED && m->m_d->d_data & D_STAIRCASE)
		m->m_data &= ~(M_PARALYZED | M_SLEEPING | M_WAKEUP);
}

/*
 * NEEDTOHIT: calculates roll needed to hit the specified armor class,
 * given the # of hit dice of the attacker
 */

int
needtohit(ac, hd)
register	ac,
		hd;
{
	register	i;

	if (ac > 25)
		ac = 25;
	else if (ac < 0)
		ac = 0;

	i = 8 + ac - (hd > 5 ? (3 + hd / 2) : hd);  /* what a mess, eh? */

	if (i > 20)
		i = 20 + (i - 20) / 3;

	return i;
}

/*
 * DAMAGE: takes a string like '3d4' and calcs the total as 3 four-sided dice
 */

int
damage(str)
register char	*str;
{
	register	i = *str - '0',
			total = 0;

	while (i--)
		total += rnd(atoi(str + 2)) + 1;

	return total;
}

/*
 * DPARSE: picks a selected damage from a string like '3d4/2d6/1d4'
 * ex. damage 1 would be '2d6/1d4', damage 0 would be '3d4/2d6/1d4'
 */

char	*
dparse(which, str)
register	which;
register char	*str;
{
	while (which--)
		str = strchr(str, '/') + 1;

	return str;
}

/*
 * STRBONUS: calcs the strength bonus to damage for our hero
 */

int
strbonus()
{
	if (u.u_str > 16 && u.u_str < 21)	
		return u.u_str - 16;

	if (u.u_str > 20)
		return 5 + (u.u_str - 20) / 2;

	if (u.u_str < 12)
		return -((12 - u.u_str) / 2);

	return 0;
}

/*
 * CHG_HP: any change in the player's hp is done thru this procedure to
 * make sure the player is still alive and hp is not > than max hp
 */

void
chg_hp(amount)
register	amount;
{
	register	hp = u.u_hp;

	u.u_hp += amount;

	if (u.u_hp < 1) {
		pline("You died!");
		blineflg = REDRAW_ALL;
		pr_bline();
		more(0, 9, YES);
		die();
	} else if (u.u_hp > u.u_maxhp)
		u.u_hp = u.u_maxhp;

	if (u.u_hp != hp) {
		blineflg |= REDRAW_HP;

		/* make sure hp is updated on screen when sleeping */
		if (u.u_data & UD_SLEEPING)
			pr_bline();
	}
}

/*
 * DIE: handle the nasty business of the death of our hero
 */

void
die()
{
	char		line[100];
	register char	*cp;
	time_t		t;

	if (wizard == YES)	/* one of the benefits of being a wizard */
		return;

	if (lrfinger(RESURRECTION)) {
		pline("You are still alive!");
		bell();
		resurrect();
		return;
	}

	/* set up the tomb screen */
	set_act(1);

	color(YELLOW);
	prfile("pics\\tomb");

	time(&t);
	cp = ctime(&t);

	color(CYAN | INTENSE);
	sprintf(line, "%.3s", cp + 4);
	i_set(9, 36);
	swrite(line);
	sprintf(line, "%.4s", cp + 20);

	i_set(10, 36);
	swrite(line);
	sprintf(line, "%2d", u.u_dlevel);

	i_set(12, 7);
	swrite(line);

	i_set(11, 23 - ((strlen(u.u_name)+1) / 2));
	swrite(u.u_name);

	i_set(12, 23 - ((strlen(titles[u.u_elevel-1])+1) / 2));
	swrite(titles[u.u_elevel-1]);

	i_set(14, 18);
	swrite("killed by");

	cp = article(killer, NO);
	i_set(15, 23 - ((strlen(cp)+1) / 2));
	swrite(cp);
	sprintf(line, "%s points", commastr(getscore(YES)));

	i_set(17, 23 - ((strlen(line)+1) / 2));
	swrite(line);

#ifdef	dont
{
	register FILE	*fp;

	if ((fp = fopen("diedata", "a")) == NO)
		pline("Can't open diedata file!");
	else {
		fprintf(fp, "D%d:%s:E%d:%ld:%s:%s", u.u_dlevel, killer,
			u.u_elevel, getscore(YES), u.u_name, ctime(&t));
		fclose(fp);
	}
}
#endif

	set_vis(1);

	doexit();
}

/*
 * RESURRECT: takes care of player when wearing a ring-o-resurrection
 */

void
resurrect()
{
	register OBJECT	*o;

	/* could have been a ring of randomness that got us here so we
	   have to make sure (without l/rfinger()) which finger if any
	   the ring-o-res is on */
	if (rightring && rightring->o_offset == RESURRECTION)
		o = rightring;
	else if (leftring && leftring->o_offset == RESURRECTION)
		o = leftring;
	else o = (OBJECT *)0;

	/* if there is a ring-o-res, DESTROY IT! */
	if (o) {
		lessweight(o, o->o_quantity);
		rm_inv(o);
	}

	/* all possessions are dropped where the psuedo-death occurred.
	   any used or lit objects are unused/unlit */
	for (o = inv; o < &inv[numinv]; ) {
		o->o_data &= ~(O_INUSE | O_IGNITED);
		add_lobj(u.u_d, o);
		rm_inv(o);
	}

	rm_event(oilexplode);	/* no more oil flasks TO explode! */

	u.u_weight = 0L;
	u.u_hp = 1;
	u.u_ac = 0;
	leftring = rightring = armor = shield = leftweapon = rightweapon =
					secweap = (OBJECT *)0;
	blineflg = REDRAW_ALL;
	teleport((DUNGEON *)0);	/* good luck to the player (he'll need it) */
}
