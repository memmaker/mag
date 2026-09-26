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

char	*mhtimes[] = { "!", " twice!", " three times!", " four times!",
			" five times!" };

/*
 * MATTACK: handles all monster attacks on player
 */

void
mattack(m)
register MONSTER *m;
{
	register	i,
			dam = 0;
	int		hits = 0,
			roll;

	/* stop player if doing a contiual move */
	stop();
	pr_mon();
	redraw();
	m->m_data |= M_ATTACKED;

	/* if not cancelled, let mons do some special abilities */
	if (!(m->m_data & M_CANCELLED))
		firstatt(m);

	/* if mons don't do damage, just return */
	if (!m->m_perm->p_damage)
		return;

	killer = mname(m);

	for (i = 0; i < numattacks(m->m_perm->p_damage); i++) {
		roll = rnd(20);
			
		/* small mons -5 to hit when attacking levitating player */
		if ((u.u_data & UD_LEVITATE) && strchr("fijqrsv", m->m_perm->
				p_letter))
			roll -= 5;

		/* if mon stuck to player, it always hits! */
		if (rnd(20) >= needtohit(u.u_ac, m->m_perm->p_hitdice) ||
							m == u.u_stuckmon) {
			hits++;
			dam += damage(dparse(i, m->m_perm->p_damage));
		}
	}

	u.u_score += dam;	/* player gets points for being hit! */

	/* do more special abilities if not cancelled */
	if (!(m->m_data & M_CANCELLED) && middleatt(m)) {
		chg_hp(-dam);

		return;
	}

	if (blind())
		pline("You are attacked from the %s!", dirdesc[what_dir(
							u.u_d, m->m_d)]);

	if (hits) {
		pline("The %s %s you%s", mname(m), (m->m_perm->p_attack ?
			m->m_perm->p_attack : "hit"), mhtimes[hits - 1]);

		/* yet more special abilities if not cancelled */
		if (!(m->m_data & M_CANCELLED))
			afteratt(m);

	} else pline("The %s missed you.", mname(m));

	chg_hp(-dam);
}

/*
 * NUMATTACKS: count the number of attacks given the damage string, for ex)
 * "2d4/1d8" will return 2
 */

int
numattacks(str)
register char	*str;
{
	register	i = 1;
	
	while (str = strchr(str, '/'))
		i++, str++;

	return i;
}

/*
 * FIRSTATT: some mons use special abilities without having to hit the
 * player...this proc handles those
 */

void
firstatt(m)
register MONSTER *m;
{
	switch (m->m_perm->p_letter) {
	case 'b':
		if (!rnd(5) && !confused()) {
			pline("The %s's babbling confuses you!", mname(m));
			mkmeconfused(5 + rnd(5));
		}

		break;

	case 'g':
		if (!rnd(5) && !confused() && !blind()) {
			pline("The %s's gaze confuses you!", mname(m));
			mkmeconfused(5 + rnd(7));
		}

		break;

	case 'i':
		dump_minv(m);
		m->m_data |= M_DEAD;
		m->m_d->d_data &= ~D_MONSTER;
		putwhat(m->m_d);
		pline("Aack!  An invisixplode detonates!");
		killer = "invisixplode";
		explode(m->m_d);

		break;

	case 's':
		pline("The %s exhales a piercing shriek!", mname(m));
		allwakeup();

		break;

	case 'U':
		if (!rnd(4) && !confused() && !blind()) {
			pline("The %s's gaze confuses you!", mname(m));
			mkmeconfused(5 + rnd(10));
		}
	}
}

/*
 * MIDDLEATT: this is for all the mons that need to hit to do special
 * ability, but the game doesn't tell the player that he was hit before the
 * ability happens
 */

int
middleatt(m)
register MONSTER *m;
{
	switch (m->m_perm->p_letter) {
	case 'f':
		if (!rnd(7) && steal(m) && !blind()) {
			pline("The %s disappears in a puff of smoke!",
								mname(m));
			return YES;
		}

		return NO;

	case 'u':
		if (!rnd(6)) {
			pline("The %s's horn impales you!", mname(m));
			chg_hp(-(2 + rnd(2)));
		} else return NO;

		break;

	case 'Q':
		if (!rnd(4) && !u.u_stuckmon) {
			pline("The %s coils around your body!", mname(m));

			if (lrfinger(FREEACT))
				pline("You easily slip out.");
			else u.u_stuckmon = m;

			break;
		}

	default:
		return NO;
	}

	return YES;
}

/*
 * AFTERATT: handle mons special ability after the mon hit and the hit
 * message was printed
 */

void
afteratt(m)
register MONSTER *m;
{
	register	i;

	switch (m->m_perm->p_letter) {
	case 'v':
	case 't':
		if (!rnd(5) && fix_str(-1)) {
			if (m->m_perm->p_letter == 'v')
				pline("The %s stings you!", mname(m));
			else pline("The %s's stench weakens you!", mname(m));
		}

		break;

	case 'z':
		if (!rnd(3)) {
			pline("The %s's speeding winds suffocate you!",
								mname(m));
			chg_hp(-(rnd(6) + rnd(6) + 1));
		}

		break;

	case 'A':
		if (!rnd(5)) {
			if (fix_str(-1))
				pline("The %s's icy fingers make you feel weaker!",
						mname(m));
			else pline("The %s's fingers have no effect!",
			       			mname(m));
		}

		break;

	case 'B':
		if (rnd(2) && steal(m) && !blind())
			pline("The %s cackles and vanishes!", mname(m));

		break;

	case 'I':
		if (!rnd(4))
			physdam();

		break;

	case 'K':
		if (rnd(3) && steal(m) && !blind())
		     pline("The %s phases through a wall and vanishes!",
								mname(m));
		break;

	case 'L':
		if (!rnd(4) && (u.u_data & UD_SLEEPING) == NO) {
			pline("The %s's scratch paralyzes you!", mname(m));
			mkmesleep(2 + rnd(3));
		}

		break;

	case 'P':
		polymorph(1 + rnd(3));
		break;

	case 'R':
		if (!rnd(5)) {
			pline("The %s's scythe slices an experience level from you!",
					mname(m));
			u.u_hp -= (i = rnd(9) + 2);
			u.u_maxhp -= i;
			u.u_epoints = needexp(u.u_elevel - 1);	/* nasty. */
			u.u_elevel--;
			blineflg = REDRAW_ALL;
		}

		break;

	case 'S':
		if (armor && (armor->o_offset < COCONUT || armor->o_offset >
				PADDED) && what_ac(armor) > 0 && !(armor->
				o_data & O_GALVANIZED)) {
			pline("Your armor is dissolving in the acid!");
			armor->o_enchant--;
		} else if (shield && shield->o_offset != MITHRIL && what_ac(
				shield) > 0 && !(shield->o_data &
				O_GALVANIZED)) {
			pline("Your shield is dissolving in the acid!");
			shield->o_enchant--;
		} else break;

		bell();
		u.u_ac--;
		blineflg = REDRAW_ALL;

		break;

	case 'T':
		/* both the player and the monster teleport together! */
		teleport((DUNGEON *)0);
		mteleport(m, near_loc(u.u_d));
	}
}

/*
 * STEAL: a mon picks an obj from player in and then steals it
 */

int
steal(m)
register MONSTER *m;
{
	register	j;
	register OBJECT *o;

	if (m->m_ni >= MAXMONCARRY) /* if mons load is full, don't steal */
		return NO;

	if (!numinv || lrfinger(FREEACT)) {	/* feature of free action */
		if (!blind())
			pline("The %s looks furious for a moment.",mname(m));
		else pline("The monster screams in frustration!");

		return NO;
	}

	/* randomly pick the obj to steal */
	if (numinv > 1)
		for (o = inv, j = rnd(numinv); j; o++, j--)
			;
	else o = inv;

	if ((o->o_data & O_CURSED) || o->o_offset == SAPPHIRE) {
		pline("The %s says, \"Yucko, it's cursed!\"", mname(m));

		if (!rnd(4)) {	/* if the monster is from New York */
			pline("And then the %s proceeds to give you a lecture on how",
						mname(m));
			pline("difficult it is to make a living in a dungeon when");
			pline("adventurer's not unlike yourself carry around crummy,");
			pline("cursed magic items.");
		}

		return NO;
	}

	if (o == leftweapon)
		unwield(LEFTHAND);
	else if (o == rightweapon)
		unwield(RIGHTHAND);
	else if (o == armor)
		unwear(NO);
	else if (o == shield)
		unstrap();
	else if (o == leftring)
		unring(LEFTHAND);
	else if (o == rightring)
		unring(RIGHTHAND);

	blineflg |= REDRAW_SCR;
	pline("The %s stole %s!", mname(m), obj_str(o));
	add_minv(m, o);
	lessweight(o, o->o_quantity);
	rm_inv(o);

	mteleport(m, (DUNGEON *)0);	/* this monster's outta here! */
	m->m_data |= M_SCARED;

	return YES;
}

/*
 * CLEARSHOT: decide if a monster has a clear shot at the player with an
 * arrow or wand
 */

int
clearshot(m)
register MONSTER *m;
{
	register		dir = what_dir(m->m_d, u.u_d);
	register DUNGEON	*d = m->m_d;

	/* keep moving in dir of player until get to player or wall */
	do {
		if ((d = mvindir(dir, d, 1)) == u.u_d)
			return YES;
	} while (!(d->d_data & (D_STONE | D_DOOR)));

	return NO;
}

/*
 * ARCHER: handles monsters shooting arrows at the player
 */

void
archer(m)
register MONSTER *m;
{
	register OBJECT	*o;

	/* find the arrows in the monster's invent */
	for (o = m->m_i; o < &m->m_i[m->m_ni]; o++)
		if (o->o_offset == ARROW)
			break;

	if (o == &m->m_i[m->m_ni]) {		/* outta ammo */
		m->m_data &= ~M_GOTBOW;
		return;
	}

	killer = "monster's arrow";

	if (insight(m->m_d))
		pline("The %s shoots an arrow at you!", mname(m));

	dothrow(m, m->m_d, copy_obj(o, 1), what_dir(m->m_d, u.u_d), 20, 0,0);

	if (--o->o_quantity < 1)	/* monster's last arrow */
		rm_minv(m, o);
}

/*
 * ZAPMEPLEASE: handles monsters zapping the player with wands
 */

void
zapmeplease(m)
register MONSTER *m;
{
	register OBJECT	*o;

	/* find the first wand in mon's inv with charges */
	for (o = m->m_i; o < &m->m_i[m->m_ni]; o++)
		if (o->o_type == WAND && o->o_charges)
			break;

	if (o == &m->m_i[m->m_ni]) {		/* no usable wands */
		m->m_data &= ~M_GOTWAND;
		return;
	}

	if (insight(m->m_d))
		pline("The %s aims a %s at you!", mname(m), typename(o));

	if (pr_wand(o->o_offset, m))
		dozap(what_dir(m->m_d, u.u_d), m->m_d, o->o_offset, m);

	o->o_charges--;
}

/*
 * SPECACT: handle any other special monster missile abilities such as
 * throwing boulder and dragon breath
 */

void
specact(m)
register MONSTER *m;
{
	register OBJECT	*o;
	register	off;

	if (m->m_perm == OGRE || m->m_perm == HILLGIANT) {
		/* find the boulders in mon inv */
		for (o = m->m_i; o < &m->m_i[m->m_ni]; o++)
			if (o->o_offset == BOULDER)
				break;

		if (o == &m->m_i[m->m_ni]) {	/* out of boulders */
			m->m_data &= ~M_SPECIAL;
			return;
		}

		killer = "thrown boulder";

		if (insight(m->m_d))
		       pline("The %s throws a huge boulder at you!",mname(m));

		dothrow(m, m->m_d, copy_obj(o, 1), what_dir(m->m_d, u.u_d),
					15, 0, 0);

		if (--o->o_quantity < 1)	/* just threw last rock */
			rm_minv(m, o);

		return;
	}

	/* now handle all of the breath weapons */

	if ((m->m_perm == FIREDRAKE && rnd(5)) || (m->m_perm == ZABLORON &&
								rnd(2)))
		return;

	if (m->m_perm == CRYSTALIZER || m->m_perm == FROSTFIEND || m->m_perm
							== WHITEDRAGON)
		off = FROST;
	else if (m->m_perm == FIREDRAKE || m->m_perm == REDDRAGON ||
			m->m_perm == IMPDRAGON)
		off = FIRE;
	else if (m->m_perm == ZABLORON)
		off = MAGMIS;
	else if (m->m_perm == BLUEDRAGON)
		off = LIGHTNING;
	else off = DRAINLIFE + rnd(NUMWANDS);

	if (pr_wand(off, m))
		dozap(what_dir(m->m_d, u.u_d), m->m_d, off, m);
}
