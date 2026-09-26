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
 * LOOKFORDOOR: have a monster look for a door on the DIR side of the room
 */

int
lookfordoor(m, dir)
register MONSTER	*m;
{
	register DOOR	*d;

	/* look for the first door that in the same room, not magical and
	   is on the proper side of the room */
	for (d = doors; d < &doors[numdoors]; d++)
		if (d->dr_r == m->m_r && !(d->dr_data & DR_WITHLOCK) &&
				room_side(scrline(d->dr_d), scrcol(d->dr_d),
				d->dr_r) == dir) {
			/* set mons destination to the end of the corr that
			   leads out of the door */
			m->m_dest = endofcor(mvindir(dir, d->dr_d, 1), dir);
			return YES;
		}

	return NO;	/* no door available */
}

/*
 * ENDOFCOR: follow a corridor until the end is reached
 */

DUNGEON	*
endofcor(d, dir)
register DUNGEON	*d;
register		dir;
{
	while (d->d_what == CORRIDOR || d->d_what == DOORC)
		d = mvindir(dir, d, 1);

	/* allow one step into room if that's the case */
	if (d->d_what == FLOOR || d->d_what == POOL)
		return d;

	return mvindir(dir, d, -1);	/* at end, back up one */
}

/*
 * RM_MON: remove a monster from the mon array
 */

void
rm_mon(m)
register MONSTER	*m;
{
	/* shift all mons after M on top of M */
	memmove(m, m+1, (nummons - ((m - mons)+1)) * sizeof (MONSTER));
	nummons--;

	/* if the mon stuck to player moves up, so must the pointer to it */
	if (u.u_stuckmon && u.u_stuckmon > m)
		u.u_stuckmon--;
}

/*
 * ADD_MINV: put an object into the monster's inventory array
 */

void
add_minv(m, o)
register MONSTER	*m;
register OBJECT		*o;
{
	if (m->m_ni < MAXMONCARRY) {
		memmove(&m->m_i[m->m_ni], o, sizeof (OBJECT));
		m->m_ni++;
	}
}

/*
 * RM_MINV: remove an object from the monster's inv array
 */

void
rm_minv(m, o)
register MONSTER	*m;
register OBJECT		*o;
{
	/* shift all objs after O onto O */
	memmove(o, o+1, (m->m_ni - ((o - m->m_i)+1)) * sizeof (OBJECT));
	m->m_ni--;
}

/*
 * WHAT_DIR: pick the best direction to move from location SD to loc DD
 */

int
what_dir(sd, dd)
register DUNGEON	*sd,
			*dd;
{
	if (scrline(dd) > scrline(sd)) {
		if (scrcol(dd) > scrcol(sd))
			return DNRT;
		else if (scrcol(dd) < scrcol(sd))
			return DNLT;
		else return DOWN;
	} else if (scrline(dd) < scrline(sd)) {
		if (scrcol(dd) > scrcol(sd))
			return UPRT;
		else if (scrcol(dd) < scrcol(sd))
			return UPLT;
		else return UP;
	} else if (scrcol(dd) > scrcol(sd))
		return RIGHT;

	return LEFT;
}

/*
 * WAKEMON: prepare monster to wake up next round
 */

void
wakemon(m)
register MONSTER	*m;
{
	if (insight(m->m_d) && (m->m_data & M_SLEEPING) && (u.u_options &
			O_SPECMESS))
		pline("The %s awakens.", mname(m));

	m->m_data &= ~(M_SLEEPING | M_WAKEUP);
}

/*
 * WHAT_MON: return which monster is at the given location
 */

MONSTER	*
what_mon(d)
register DUNGEON	*d;
{
	register MONSTER	*m;

	for (m = mons; m < &mons[nummons]; m++)
		if (m->m_d == d && !(m->m_data & M_DEAD))
			return m;

	pline("what_mon: Can't find monster!");

	return mons;	/* to prevent munging */
}

/*
 * MSTARTO: possibly give the new monster some starting objects
 */

void
mstarto(m)
register MONSTER *m;
{
	register	letter = m->m_perm->p_letter;
	
	/* these monsters sometimes start with a bow and arrows to use on
	   the (pincushion) player */
	if ((m->m_perm->p_data & BOW) && !rnd(5)) {
		add_minv(m, create_o(WEAPON, 0, 0, O_USEDUP, LONGBOW, 1));
		add_minv(m, create_o(WEAPON,0,0, O_USEDUP, ARROW,5+rnd(10)));
		m->m_data |= M_GOTBOW;
	}

	/* give some monsters some treasure */
	if ((strchr("cefghjknouvAEFGHKLMNRUVW", letter) && !rnd(5)) ||
				(strchr("mBD", letter) && rnd(5) < 4))
		add_minv(m, create_o(TREASURE, 0, 0, 0, PLATINUM + rnd(3) +
						!rnd(3), muchtreas()));

	/* give monsters food, but NEVER mushrooms (these monsters don't
	   do drugs) */
	if (strchr("abehjklmnoprtyBHJNOQUXYZ", letter) && !rnd(8))
		add_minv(m, create_o(FOOD, 0,0,0,rnd(NUMFOOD-1), 1+!rnd(4)));

	/* give monsters a random item */
	if ((strchr("ceghmuBCMRW", letter) && !rnd(5)) || letter == 'D')
		add_minv(m, mk_obj());

	/* some monster have wands (and know how to use them!) but never
	   on the first level to give the player a chance */
	if (strchr("efqBCDEV", letter) && !rnd(15) && u.u_dlevel > 1) {
		add_minv(m, create_o(WAND, 1 + rnd(7), 0, 0, MAGMIS, 1));
		m->m_data |= M_GOTWAND;
	}

	/* ogres and hill giants throw boulders at the player! */
	if ((letter == 'o' || letter == 'H') && rnd(2)) {
		add_minv(m, create_o(WEAPON, 0, 0, 0, BOULDER, 1 + rnd(2)));
		m->m_data |= M_SPECIAL;
	}
}

/*
 * PICKMON: choose a random type of monster that is allowed to appear on
 * the current dungeon level
 */

int
pickmon()
{
	register	i,
			lev = (u.u_dlevel < 1 ? 30 : u.u_dlevel),
			tries = 0;

	/* if the player genocided too many monsters, give 'em a treat */
	if (strlen(genostr) > NUMMON / 2)
		return 53;	/* Imperial Dragon! */

	do {
		/* if we have a hard time picking a mon, give 'em a treat */
		if (++tries >= 500)
			return 53;	/* Imp. Drag. ! */

		i = rnd(NUMMON);
	} while (strchr(genostr, pmon[i].p_letter) || (int)pmon[i].p_appear >
		lev || ((int)pmon[i].p_appear != lev && rnd(2)) || (lev > 20
		&& pmon[i].p_appear < 9));

	return i;
}

/*
 * LEV_MON: create the starting mons on a dungeon level
 */

void
lev_mon()
{
	register        i = rnd(3) + 4,
			j,
			k;
	int             pack_or_lair;
	ROOM            *r;
	DUNGEON		*d;

	while (i--) {
		/* on sapphire lev, all mons are in first room (maze) */
		if (sapp_lev()) {
			mk_mon(room_loc(rooms), -1);
			continue;
		}

		k = pickmon();

		if ((pmon[k].p_data & PACK) && !rnd(25)) {
			r = what_room(location(SPUTMON));
			pack_or_lair = rnd(2);

			/* create a nice pack or lair of mons! */
			for (j = 0; j < rnd(5) + 5; j++) {
				do d = room_loc(r);
				while ((d->d_data & D_MONSTER) && rnd(500));

				/* don't make a mon on a mon */
				if (d->d_data & D_MONSTER)
					continue;

				mk_mon(d, k);

				if (pack_or_lair)
					mons[nummons-1].m_data |= M_SLEEPING;
				else mons[nummons-1].m_data &= ~M_SLEEPING;
			}
		} else {
			do d = location(SPUTMON);
			while (d->d_data & D_MONSTER);

			mk_mon(d, k);
		}
	}
}

/*
 * ROLL_HP: calculate a hit point value given the number of hit dice
 */

int
roll_hp(hd)
register	hd;
{
	register	hp = 0;

	while (hd--)
		hp += (rnd(7) + 2);

	return hp;
}

/*
 * PR_MON: force all the monster's to be redisplayed the next screen update
 */

void
pr_mon()
{
	register MONSTER	*m;
	register		c;

	for (m = mons; m < &mons[nummons]; m++) {
		if (m->m_od) {	/* redraw their last location too */
			c = scrcol(m->m_od);
			set_red(scrline(m->m_od), c, c);
			m->m_od = (DUNGEON *)0;
		}

		c = scrcol(m->m_d);
		set_red(scrline(m->m_d), c, c);
	}
}

/*
 * DETMON: make all monster's detected and print them
 */

void
detmon()
{
	register MONSTER	*m;

	for (m = mons; m < &mons[nummons]; m++) {
		m->m_data |= M_DETECTED;
		putwhat(m->m_d);
	}
}

/*
 * MTELEPORT: teleport a monster someplace in the dungeon
 */

void
mteleport(m, wd)
register MONSTER	*m;
register DUNGEON	*wd;
{
	if (!m->m_od)
		m->m_od = m->m_d;

	m->m_d->d_data &= ~D_MONSTER;
	putwhat(m->m_d);

	if (wd)
		m->m_d = wd;
	else m->m_d = location(SFLOOR);	/* if no WD, get a random loc */

	m->m_dest = (DUNGEON *)0;
	m->m_d->d_data |= D_MONSTER;
	m->m_r = what_room(m->m_d);

	if (u.u_stuckmon == m)	/* not stuck any more! */
		u.u_stuckmon = (MONSTER *)0;
}

/*
 * CK_MHP: check and see if a monster is still alive
 */

int
ck_mhp(m, how)
register MONSTER *m;
register	how;
{
	if (m->m_data & M_DEAD)	/* don't bother checking, har har */
		return YES;

	/* make sure the mon doesn't have too many hit points */
	if (m->m_hp > m->m_maxhp)
		m->m_hp = m->m_maxhp;

	if (m->m_hp < 1) {
		keep();

		if (how) {
			pline("You killed the %s!", m->m_data & M_POLYPOWER ?
						"polymorpher" : mname(m));
			u.u_epoints += m->m_perm->p_bexp + m->m_maxhp * m->
							m_perm->p_hitdice;
			ck_exp();
		} else if (insight(m->m_d))
			pline("The %s was killed!", m->m_data & M_POLYPOWER ?
						"polymorpher" : mname(m));

		dump_minv(m);
		m->m_data |= M_DEAD;
		m->m_d->d_data &= ~D_MONSTER;
		putwhat(m->m_d);

		if (m->m_od)	/* in case it happens before redisplay */
			putwhat(m->m_od);

		if (m == u.u_stuckmon)	/* not stuck any more */
			u.u_stuckmon = (MONSTER *)0;

		return YES;
	}

	return NO;
}

/*
 * DUMP_MINV: drop all the monster's possession right where it is
 */

void
dump_minv(m)
register MONSTER *m;
{
	register OBJECT	*o;

	for (o = m->m_i; o < &m->m_i[m->m_ni]; o++)
		add_lobj(m->m_d, o);

	m->m_ni = 0;
}

/*
 * ALLWAKEUP: make all the monsters on the dungeon level wake up
 */

void
allwakeup()
{
	register MONSTER	*m;
	
	for (m = mons; m < &mons[nummons]; m++) {
		wakemon(m);
		m->m_data |= M_AGGRAVATED;
	}
}

/*
 * MNAME: return the current description of a monster
 */

char	*
mname(m)
register MONSTER	*m;
{
	/* sometimes the player just can't see the monster */
	if (!(m->m_data & M_DETECTED) && (blind() || ((m->m_data & M_INVIS)
			&& !(u.u_data & UD_SEEINV))))
		return "monster";

	/* when on acid, the player see's ALL kind of monsters! */
	if (u.u_data & UD_DELUSION)
		return (rnd(20) ? pmon[rnd(NUMMON)].p_name :"pink elephant");

	return m->m_perm->p_name;
}

/*
 * POLYMON: change the monster into another monster
 */

void
polymon(m)
register MONSTER	*m;
{
	m->m_perm = &pmon[rnd(NUMMON)];
	m->m_hp = m->m_maxhp = roll_hp(m->m_perm->p_hitdice);
	putwhat(m->m_d);
}

