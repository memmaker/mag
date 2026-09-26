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
 * READSCRL: handle the result of reading a scroll
 */

int
readscrl(o)
register OBJECT	*o;
{
	register MONSTER	*m;
	register		flag = NO;

	switch (o->o_offset) {
	case SAGGMON:
		pline("You hear a high pitched shrieking sound.");
		allwakeup();

		break;

	case NEGATION:
		pline("This scroll appears to be blank.");
		doevent(noconmon);
		doevent(noconfuse);
		doevent(noblind);
		doevent(nospeed);	/* kill all active magic */
		doevent(noinvis);
		doevent(nohero);
		doevent(nopoly);
		doevent(nolevit);
		updevent();

		break;

	case CONMON:
		pline("Your hands begin to glow irridescently.");
		u.u_data |= UD_CONMON;
		mk_event(100 + rnd(100), noconmon);

		break;

	case CREATEMON:
		if (u.u_data & UD_LABYRINTH)	/* no mons in lab levs */
			pline("Nothing happens.");
		else {
			pline("Fwwoooosh...");
			mk_mon(near_loc(u.u_d), -1);
			redraw();
		}

		break;

	case TREASFIND:
		if (detect(YES))
			pline("You sense the presence of treasure!");
		else {
			pline("You feel a pull downward."); /* none on lev */
			flag++;
		}

		break;

	case ENCHARM:
		pline("This is a scroll of enchant armor!");

		/* fake keyboard and use the UTILIZE procedure */
		typed = ']';
		utilize();

		break;

	case ENCHWEP:
		pline("This is a scroll of enchant weapon!");

		/* fake keyboard and use the UTILIZE procedure */
		typed = ')';
		utilize();

		break;

	case GENOCIDE:
		pline("Behold! You have been granted the boon of genocide!");
		genocide();

		break;

	case HOLDMON:
		pline("You hear Tex's maniacal laughter in the distance.");
		break;

	case IDENTIFY:
		makeknown(o);
		pline("This is a scroll of identify!");

		/* fake keyboard and use the UTILIZE procedure */
		typed = '\011';
		utilize();

		break;

	case CONTLIGHT:
		if (u.u_r) {
			lightroom();

			/* chance that mons in room will be confused */
			for (m = mons; m < &mons[nummons]; m++)
				if (u.u_r == m->m_r && !rnd(4))
					m->m_data |= M_CONFUSED;
		} else pline("There is a momentary bright flash.");

		if (!rnd(4)) {	/* and the player too! */
			pline("You can't see!");
			mkmeblind(2 + rnd(2));
		}

		break;

	case MAGICMAP:
		pline("Oh!  This has a map on it.");
		mapping();

		break;

	case REMCURSE:
		pline("It feels like someone is helping you.");
		decurse();

		break;

	case RESTCHARGE:
		pline("This is a scroll of restore charges!");

		/* fake keyboard and use the UTILIZE procedure */
		typed = '/';
		utilize();

		break;

	case SCAREMON:
		pline("There is dead silence momentarily.");

		/* all mon within scrolls range are frightened */
		for (m = mons; m < &mons[nummons]; m++)
			if (inrange(m->m_d, u.u_d, 7))
				m->m_data |= M_SCARED;

		break;

	case SLEEP:
		pline("You are starting to feel very sleepy...");

		if (rnd(20) < u.u_elevel) /* player gets saving throw */
			pline("You manage to shake it off and remain awake.");
		else {
			pline("You fall asleep.");
			u.u_data |= UD_SLEEPING;
			mk_event(rnd(10) + 5, nosleep);
		}

		break;

	case TELEPORT:
		teleport((DUNGEON *)0);	/* 0 means randomly */
		break;

	case OBJATTRACT:
		pline("You feel pulled in all directions!");
		attract();

		break;

	case DETECTFOOD:
		if (detect(2))
			pline("You sniff the presence of food!");
		else {
			pline("Your nose tingles.");
			flag++;
		}

		break;

	case GALVANIZE:
		pline("You seem to see the image of a man with red hair.");
		pline("Wow!  This is a scroll of galvanization!");

		/* fake keyboard and use the UTILIZE procedure */
		typed = '[';
		utilize();

		break;

	case LABYRINTH:
		dolab();

		break;

	default:
		pline("readscrl: Unknown scroll!");
	}

	u.u_epoints += 10L;	/* yay! more experience */
	usedmagic(o, flag);
				
	return NO;
}

/*
 * ATTRACT: make random objects laying on the dungeon zot to the locations
 * adjacent to the player
 */

void
attract()
{
	register LEVOBJ		*l;
	register DUNGEON	*d;
	int			i = 3 + rnd(3);	/* # of objs to move */

	for (l = lobjs; i && l < &lobjs[numlobjs]; i--, l++) {
		/* move the object */
		d = l->l_d;
		l->l_d = near_loc(u.u_d);
		l->l_d->d_data |= D_OBJECT;

		/* if there are no more objs at the loc where it used to be,
		   then clear the obj flag there */
		if (!what_obj(d))
			d->d_data &= ~D_OBJECT;

		putwhat(d);		/* redraw both locations */
		putwhat(l->l_d);
	}
}

/*
 * GENOCIDE: lets the player pick a monster to kill forever
 */

void
genocide()
{
	register MONSTER	*m;
	register struct permmon *p;
	register		c;

	for (;;) {
		pline("@What monster do you want to genocide (symbol)? ");
		go_imaginary();
		c = ctgetch();

		if (!(p = l_to_p((char)c))) {
			pline("That's an unknown species to me.");
			bell();
			continue;
		}

		if (strchr(genostr, c)) {
			pline("You've already genocided that creature!");
			bell();
			continue;
		}

		/* any dragon is just too much for the genocide scroll */
		if (c == REDDRAGON->p_letter) {
			pline("A crimson flash fires out of the scroll!");
			mkmeblind(2 + rnd(3));
			return;
		}

		pline("Farewell to all %ss!", p->p_name);

		/* now remove the genocided mons from the level, if any */
		for (m = mons; m < &mons[nummons]; m++) {
			if (m->m_perm == p) {
				pline("You hear a cry of anguish!");
				dump_minv(m);
				m->m_hp = -1;
				ck_mhp(m, NO);
			}
		}

		sprintf(genostr + strlen(genostr), "%c", c);

		return;
	}
}

/*
 * WHATGENO: display all of the monsters that have been genocided
 */

int
whatgeno()
{
	char	*cp;

	if (!*genostr)
		pline("You have not genocided anything yet.");
	else
		for (cp = genostr; *cp; cp++)
			pline("You have genocided %ss%s", l_to_p(*cp)->
				p_name, *(cp+1) ? " and" : ".");

	return NO;
}

/*
 * L_TO_P: convert a letter to a ptr to a perm mon with that letter for a
 * symbol
 */

struct permmon	*
l_to_p(c)
register char	c;
{
	register struct permmon	*p;

	for (p = pmon; p < &pmon[NUMMON]; p++)
		if (p->p_letter == c)
			return p;

	return (struct permmon *)0;
}

/*
 * RECHARGE: give a wand some charges
 */

int
recharge(o)
register OBJECT	*o;
{
	o->o_charges += 2 + rnd(5);
	return YES;
}

/*
 * ENCHARM: enchant a suit of armor
 */

int
encharm(o)
register OBJECT	*o;
{
	pline("Your %s%s glows golden for a moment.", o->o_type == ARMOR
			? "suit of " : "", nameptr[o->o_offset][0]);

	o->o_enchant++;
	o->o_data &= ~(O_CURSED | O_SEECURSED);

	if (o == shield || o == armor) {
		u.u_ac++;
		blineflg = REDRAW_ALL;
	}

	return YES;
}

/*
 * ENCHWEP: enchant a weapon
 */

int
enchwep(o)
register OBJECT	*o;
{
	if (o->o_type == WAND)	/* only staves enchanted as a wep */
		pline("Your staff glows golden for a moment.");
	else pline("Your %s glow golden for a moment.", nameptr[o->o_offset]
							[0]);

	o->o_data &= ~(O_CURSED | O_SEECURSED);

	/* for weps, there is a 50-50 chance of enchanting to hit or to
	   damage...with staves, it's always to hit (charges) */
	if (rnd(2) || o->o_type == WAND)
		o->o_enchant++;
	else o->o_wepench++;

	return YES;
}

/*
 * GALVANIZE: make a suit of armor galvanized against corrosion
 */

int
galvanize(o)
register OBJECT	*o;
{
	/* check if armor has natural resistance to corrosion */
	if ((o->o_offset > CHAIN && o->o_offset < PLATE) || o->o_offset ==
					MITHRIL)
		pline("Not much happens to it.");
	else {
		pline("Your %s %s glows golden-red!", nameptr[o->o_offset]
						[0], typename(o));
		o->o_data |= O_GALVANIZED;
	}

	return YES;
}

/*
 * DOLAB: create a HUGE maze-room on a new level
 */

void
dolab()
{
	/* if player reads a scroll of lab IN a lab, then the walls
	   disintegrate and the level becomes a huge empty room */
	if (u.u_data & UD_LABYRINTH) {
		pline("There's a bright white flash!  You're blinded!");
		mkmeblind(2 + rnd(2));
		cl_room(u.u_r);
		u.u_r->r_data |= (R_MAGICLIT | R_TORCHED);
	} else {
		pline("Rumble....rumble....walls are thrusting up around you!");
		u.u_data |= UD_LABYRINTH;
		newlev(DOWN, FLOOR, YES);
	}
}
