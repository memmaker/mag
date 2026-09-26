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
#include	"mag.h"

/*
 * MK_MON: create a monster at location D, of type KIND.  if KIND=-1 then
 * make a random type monster for the current level
 */

struct permmon	*
mk_mon(d, kind)
register DUNGEON	*d;
register		kind;
{
	MONSTER	m;

	if (kind == -1)			/* pick a random one? */
		m.m_perm = &pmon[pickmon()];
	else m.m_perm = &pmon[kind];

	m.m_r = what_room(d);
	m.m_ni = 0;
	m.m_d = d;
	m.m_od = m.m_dest = (DUNGEON *)0;
	m.m_maxhp = m.m_hp = roll_hp(m.m_perm->p_hitdice);
	m.m_data = 0;

	/* invisixplodes are always sleeping and invis */
	if (m.m_perm == INVISIXPLODE)
		m.m_data |= (M_INVIS | M_SLEEPING);

	/* quasits are sometimes invisible */
	if (m.m_perm == QUASIT && !rnd(4))
		m.m_data |= M_INVIS;

	if ((rnd(2) && kind == -1 && !lrfinger(AGGMON)) || m.m_perm ==
			IMPDRAGON || m.m_perm == SHRIEKER)
		m.m_data |= M_SLEEPING;

	if (lrfinger(AGGMON))
		m.m_data |= M_AGGRAVATED;

	if (m.m_perm == POLYMORPHER)
		m.m_data |= M_POLYPOWER;

	mstarto(&m);	/* give mons starting objects */

	/* these mons have zap abilities */
	if (strchr("cdDFMZ", m.m_perm->p_letter))
		m.m_data |= M_SPECIAL;

	add_mon(&m);

	return m.m_perm;
}

/*
 * ADD_MON: stick a monster in the monster array
 */

void
add_mon(m)
register MONSTER	*m;
{
	if (nummons < MAXMONS) {
		memmove(&mons[nummons++], m, sizeof (MONSTER));
		m->m_d->d_data |= D_MONSTER;
	} else pline("WARNING - I can't fit any more monsters!  Frak!");
}

/*
 * MOVE_MON: decides which monsters can move and how many times, and then has
 * them moved
 */

void
move_mon()
{
	register MONSTER	*m;
	register		move,
				moved;
	int			deadflg = NO;

	for (m = mons; m < &mons[nummons]; m++) {
		/* if the dead flag is set for a monster, don't move it and
		   make sure that it will be removed when done moving all
		   the other mons */
		if (m->m_data & M_DEAD) {
			deadflg++;
			continue;
		}

		if (m->m_data & M_SLEEPING) {
			/* notice that two rings of stealth are twice as
			   good as one */
			if ((m->m_r && m->m_r == u.u_r && rnd(100) < (5 + (u.
					u_r->r_data & R_TORCHED ? 20 : 0) -
					(20 * lrfinger(STEALTH))) && m->
					m_perm != SHRIEKER) || m->m_data
								& M_WAKEUP)
				wakemon(m);

			continue;
		}

		/* make sure that if the monster and the player are in the
		   same room that the mon forgets about its ordained dest-
		   ination and heads right for the player */
		if (m->m_r == u.u_r)
			m->m_dest = (DUNGEON *)0;

		if ((m->m_data & M_CONFUSED) && !rnd(15))
			m->m_data &= ~M_CONFUSED;

		/* if player is hasted, then the mons are SLOWED to make it
		   seem like the player is faster (cheap huh?) */
		move = m->m_perm->p_speed / (1 + ((u.u_data & UD_HASTED) ? 1
								: 0));

		/* this IF statement is only to speed execution up.  instead
		   of checking each one individually each time, we check for
		   any and then find out which ones were true */
		if (m->m_data & (M_PARALYZED|M_STUCK|M_SLOWED|M_SPEEDED)) {
			if (m->m_data & (M_PARALYZED | M_STUCK)) {
				/* mon become unparalyzed? make sure that
				   an imp drag is unpara after being hit */
				if ((!rnd(7) && !sapp_lev()) || (sapp_lev()
						&& m->m_hp < m->m_maxhp))
					m->m_data &= ~(M_PARALYZED|M_STUCK);

				continue;
			}

			if (m->m_data & M_SPEEDED)
				move *= 2;

			if (m->m_data & M_SLOWED)
				move /= 2;
		}

		moved = NO;
		m->m_data &= ~M_ATTACKED; /* reset mon attacked player flg */

		/* if move counter is greater than 5 then monster moves, but
		   if counter < 5 then there is only a chance for move */
		while (move >= 5 || rnd(5) < move) {
			moved++;
			inch_mon(m);

			/* make sure the mon does't move after it's killed if
			   it would get two moves in one round */
			if (m->m_data & M_DEAD)
				break;

			move -= 5;
		}

		if (m->m_data & M_DEAD)	{	/* the monster died */
			deadflg++;
			continue;
		}

		/* give a really slow monster a chance to defend itself if
		   it's right next to the player */
		if (!moved && inrange(u.u_d, m->m_d, 1) && !(u.u_data &
				UD_HASTED) && !(m->m_data & M_SLOWED) &&
				m->m_perm->p_speed > 0)
			inch_mon(m);

		if (moved && !rnd(8) && m->m_r == u.u_r && m->m_r && 
				(u.u_options & O_SPECMESS) && !(m->m_r->
				r_data & R_MAZE) && !insight(m->m_d))
			pline("You hear sounds from the %s.", dirdesc[
						what_dir(u.u_d, m->m_d)]);

		if ((m->m_data & M_POLYPOWER) && !rnd(7))
			polymon(m);

		/* check if there is a reason the mon shouldn't use its
		   special ability */
		if (!(m->m_data & (M_GOTBOW|M_GOTWAND|M_SPECIAL)) || rnd(2)
				|| !clearshot(m) || inrange(m->m_d, u.u_d, 1)
				|| (u.u_data & UD_INVIS) || u.u_sym !=PLAYER)
			continue;

		pr_mon();
		redraw();

		if (m->m_data & M_GOTBOW)
			archer(m);
		else if (m->m_data & M_GOTWAND)
			zapmeplease(m);
		else specact(m);
	}

	/* if there were any dead mons in the array, clear 'em out */
	if (deadflg) {
		for (m = mons; m < &mons[nummons]; ) {
			if (m->m_data & M_DEAD)
				rm_mon(m);
			else m++;
		}
	}
}

int	rot[2][5] = { { 0, 1, 6, 3, 4 }, { 0, 7, 2, 5, 4 } };

/*
 * INCH_MON: actually move the monster one space across the dungeon
 */

void
inch_mon(m)
register MONSTER *m;
{
	register	dir,
			i;
	DUNGEON		*d = m->m_d;
	int		which = rnd(2);
	
	/* find the prime direction to where the mon wants to go, whether it
	   be some pre-chosen location or toward the player */
	dir = what_dir(m->m_d, m->m_dest ? m->m_dest : u.u_d);

	/* move in a random direction? keep in mind that polymorphers can
	   see thru a self-polymorphed player and aggravated mons can too */
	if ((m->m_data & M_CONFUSED) || (u.u_data & UD_INVIS) || (u.u_sym !=
			PLAYER && m->m_perm != POLYMORPHER && !(m->m_data &
					M_AGGRAVATED)))
		dir = rnd(8);
	else if ((m->m_data & M_SCARED) && !m->m_dest)
		dir = oppdir(dir);

	/* try the primary dir and then two dirs on either side to find a
	   legal location for the mon to move */
	for (i = 0; i < 5; i++) {
		dir = (dir + rot[which][i]) % 8;
		m->m_d = mvindir(dir, d, 1);

		/* make sure it's a good location and if moving diag that
		   it's a legal diag motion (like not thru a door) */
		if (canmmove(m) && (dir % 2 == 0 || !nodiagmove(dir, d)))
			break;
	}

	/* was the dir selected the 4th or 5th or no choice? */
	if (i > 2) {
		m->m_dest = (DUNGEON *)0;

		/* found a dir on the 4th or 5th try */
		if (i != 5) {
			/* if mon in corridor, have it walk to one end */
			if (!m->m_r)
				m->m_dest = endofcor(d, dir);
			/* if in room, chance it might look for a door */
			else if (!rnd(4))
				i = 5;
		}

		/* if mon could not move in any direction */
		if (i == 5) {
			m->m_d = d;

			/* if a scared mon is cornered, it'll fight! */
			if ((m->m_data&M_SCARED) && inrange(m->m_d,u.u_d, 1))
				mattack(m);
			/* if mon in room, chance it will look for door */
			else if (m->m_r && !rnd(3)) {
				if (!lookfordoor(m, dir * 2 / 4)) {
					dir = get_cdir(-1, scrline(m->m_d),
						scrcol(m->m_d),scrline(u.u_d),
						scrcol(u.u_d));

					if (!lookfordoor(m, dir))
						lookfordoor(m, (dir + (rnd(2)
							 * 3 + 1)) % 8);
				}
			/* if mon couldn't move in a corridor, then it must
			   be caught in a corner, so pick a random non-diag
			   dir and make mon walk that way */
			} else if (!m->m_r && !rnd(3))
				m->m_dest = endofcor(d, rnd(4) * 2);

			return;
		}
	}

	if (m->m_d == u.u_d) {	/* if monster moves onto player, ATTACK! */
		m->m_d = d;
		mattack(m);

		return;
	}

	if (m->m_d->d_data & D_DOOR) {
		DOOR	*dr = what_door(m->m_d);

		if (dr->dr_stuck) {
			dr->dr_stuck -= rnd(2);	/* not full effect */

			if (inrange(m->m_d,u.u_d, 5) && (u.u_options &
					O_SPECMESS))
				pline("You hear a knocking sound.");

			m->m_d = d;

			return;
		}
	/* mon may decide to guard a staircase if it steps on one */
	} else if ((m->m_d->d_data&D_STAIRCASE) && rnd(2) && m->m_r != u.u_r)
		m->m_data |= (M_PARALYZED | M_SLEEPING);

	if (m == u.u_stuckmon) { /* make sure the stuck mon doesn't move */
		m->m_d = d;
		return;
	}

	if (m->m_d == m->m_dest)	/* arrived at destination */
		m->m_dest = (DUNGEON *)0;

	d->d_data &= ~D_MONSTER;

	/* if mon stepped on object and mon is a obj pick-up type mon */
	if ((m->m_d->d_data & D_OBJECT) && !(m->m_perm->p_data & NOGRAB)) {
		LEVOBJ	*l;

		/* loop and pick up all the objs at the mon's location that
		   the mon can carry */
		for (l=lobjs; l < &lobjs[numlobjs] && m->m_ni<MAXMONCARRY;) {
			if (l->l_d != m->m_d) {
				l++;
				continue;
			}

			if (insight(m->m_d) && (u.u_options & O_SPECMESS))
				pline("The %s snatched %s.", mname(m),
							obj_str(&l->l_o));

			/* if mon picks up a detected obj then mon becomes
			   detected (I really like this feature) */
			if (l->l_data & L_DETECTED)
				m->m_data |= M_DETECTED;

			add_minv(m, &l->l_o);

			if (l->l_o.o_type == WAND && (m->m_perm->p_data&ZAP))
				m->m_data |= M_GOTWAND;
			else if (l->l_o.o_offset == BOULDER && (m->m_perm->
					p_data & THROW))
				m->m_data |= M_SPECIAL;
			else if (l->l_o.o_offset == ARROW || l->l_o.o_offset
					== LONGBOW)
				is_archer(m);

			rm_lobj(l);
		}

		/* if mon picked up everything, turn off obj bit */
		if (!l)
			m->m_d->d_data &= ~D_OBJECT;

		putwhat(m->m_d);	/* just in case mon moves twice */
	}

	if (m->m_d->d_data & D_TRAPPED) {
		dotrap(m->m_d, m);
		putwhat(d); /* reprint in case mon dies or is teleported */

		if (m->m_data & M_DEAD)	/* awwh, too bad */
			return;
	}

	m->m_d->d_data |= D_MONSTER;

	/* if mon just entered or exited a door */
	if ((d->d_data & D_DOOR) || (m->m_d->d_data & D_DOOR)) {
		ROOM	*r = m->m_r;

		m->m_r = what_room(m->m_d);	/* what room is it in now? */

		if (!r && m->m_r == u.u_r && (u.u_options & O_SPECMESS) &&
				!inrange(m->m_d, u.u_d, 1)) {
			if (insight(m->m_d)) {
				pline("%s comes through a doorway!", article(
							mname(m), YES));
				stop();
			}
		}
	}

	/* set the old dungeon location (mon's last position) var */
	if (!m->m_od)
		m->m_od = d;
}

/*
 * CONMMOVE: decides whether a monster can move to the given location
 */

int
canmmove(m)
register MONSTER *m;
{
	register DUNGEON *d = m->m_d;

	/* is this impressive or what? */
	return (!(d->d_data & (D_STONE | D_MONSTER)) &&

		/* mons usually keep away from detected traps */
		(d == u.u_d || (!(d->d_data & D_TRAPPED) || (d->d_what !=
			TRAPC && rnd(2)))) &&

		/* mons can't attack player twice in one move-round */
		(d != u.u_d || !(m->m_data & M_ATTACKED)) &&

		/* mons can't move into magic-locked doors */
		(!(d->d_data & D_DOOR) || !(what_door(d)->dr_data &
			DR_WITHLOCK)) &&

		/* mons won't go into water if they don't like it unless they
		   have been aggravated */
		(d->d_what != POOL || !(m->m_perm->p_data & HATEWATER) ||
			(m->m_data & M_AGGRAVATED) || d == u.u_d) &&

		/* finally, they can't move onto a scroll of hold mon */
		(!(d->d_data & D_OBJECT) || what_obj(d)->l_o.o_offset !=
			HOLDMON));
}

/*
 * IS_ARCHER: redecide if monster has archer capability
 */

void
is_archer(m)
register MONSTER	*m;
{
	register OBJECT	*o;
	int		havebow = 0,
			havearrows = 0;

	/* make sure mon is capable of using a bow */
	if (!(m->m_perm->p_data & BOW))
		return;

	for (o = m->m_i; o < &m->m_i[m->m_ni]; o++)
		if (o->o_offset == LONGBOW)
			havebow++;
		else if (o->o_offset == ARROW)
			havearrows++;

	if (havebow && havearrows)
		m->m_data |= M_GOTBOW;
}

