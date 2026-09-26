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
 * QUAFF: handle the player drinking a magic potion
 */

int
quaff(o)
register OBJECT	*o;
{
	register	flag = NO;

	switch (o->o_offset) {
	case BLINDNESS:
		pline("A cloak of darkness falls around you!");
		mkmeblind(400 + rnd(400));

		break;

	case CONFUSION:
		mkmeconfused(10 + rnd(10));
		break;

	case EXTRAHEAL:
		healme(YES, YES);
		break;

	case GAINSTR:
		pline("Your muscles start growing before your very eyes!");
		fix_str(1);

		break;

	case HASTESELF:
		pline("You feel yourself speeding up...");
		mkmespeedy(rnd(5) + 10);

		break;

	case HEALING:
		healme(NO, YES);
		break;

	case HEROISM:
		/* make sure the player doesn't shoot up past max elevel */
		if (u.u_elevel > 15) {
			pline("You feel zonked-out for a moment.");
			break;
		}

		/* if player drinks a heroism before the previous one wears
		   off, then the player (permanently) goes up 1 elevel */
		if (doevent(nohero)) {
			updevent();
			relevel();
			return YES;
		}

		pline("Wow!  You feel extremely powerful!");

		oldexp = u.u_epoints;	/* save old info */
		oldelev = u.u_elevel;

		u.u_epoints = needexp(u.u_elevel + 3);
		u.u_elevel += 3;
		u.u_hp += 30;
		u.u_maxhp += 30;
		mk_event(40 + rnd(40), nohero);
		blineflg |= REDRAW_ALL;

		break;

	case INVISIBILITY:
		mkmeinvis(250 + rnd(250));
		break;

	case MAGICDET:
		if (detect(NO))
			pline("You sense the presence of magic!");
		else {
			pline("You feel a pull downward."); /* none on lev */
			flag++;
		}

		break;

	case MONDETECT:
		if (nummons) {
			detmon();
			pline("You sense the monsters around you!");
		} else {
			pline("You feel a pull downward."); /* none on lev */
			flag++;
		}

		break;

	case PARALYSIS:
		pline("Your feet are frozen to the ground!");
		mkmepara(rnd(5) + 10);

		break;

	case POISON:
		pline("Oh, yucko...poison!");

		if (!fix_str(-(rnd(2) + 1)))
			pline("But you feel fine!");

		break;

	case RAISELEV:
		relevel();
		break;

	case RESTORESTR:
		pline("You feel warm and tingly all over!");

		/* if player at max strength, chance max str will go up */
		if (u.u_str == u.u_maxstr && !rnd(3))
			fix_str(1);
		else fix_str(u.u_maxstr - u.u_str);

		break;

	case PSEEINV:
		pline("This tastes like fruit juice.");
		u.u_data |= UD_SEEINV;
		doevent(noblind);
		mk_event(5000, noseeinv);
		pr_mon();

		break;

	case SELFPOLY:
		/* can't morph when invis because there is no good symbols
		   for invis monsters like there is for the player */
		if (u.u_data & UD_INVIS)
			pline("It tastes yummy and magical, but nothing happens.");
		else polymorph(50 + rnd(50));

		break;

	case THIRST:
		pline("This tastes sort of dull.");

		/* full to the max! (bursting, that is) */
		efood->e_when = u.u_moves + 1900;
		u.u_data &= ~(UD_HUNGRY | UD_FAINTING);

		break;

	case DELUSION:
		pline("Everything seems so cosmic!");
		u.u_data |= UD_DELUSION;
		mk_event(1, changeall);

		break;

	case LEVITATION:
		pline("You slowly rise above the ground.");
		u.u_data |= UD_LEVITATE;
		mk_event(50 + rnd(100), nolevit);

		break;

	case ACID:
		pline("YAAAAHH!  The fluid burns your entrails!");
		killer = "potion of acid";
		chg_hp(-(rnd(u.u_hp+1)));	/* damn, I'm mean */

		break;

	default:
		pline("quaff: Unknown potion!");
	}

	u.u_epoints += 10L;
	usedmagic(o, flag);

	return NO;
}

/*
 * USEDMAGIC: change obj description after it is used.  for some it asks the
 * player to give the obj a name and some are obvious so it makes them
 * identified
 */

void
usedmagic(o, special)
register OBJECT	*o;
register	special; /* if set, force obj not to be identified */
{
	pr_bline();

	if (!(pobj[o->o_offset].po_flag & SID) || special) {
		if (ostats[o->o_offset] == UNKNOWN) {
			call(o);
			erase();
		}
	} else makeknown(o);
}

/*
 * HEALME: restore player's HPs in two ways (HOW): regular and extra
 */

void
healme(how, mode)
register	how,
		mode;
{
	register	i;

	i = (u.u_maxhp / 4) + (2 * rnd(u.u_maxhp / (how ? 2 : 4)));

	/* if heals past max hp, possible raises max hp one or two pts */
	if (u.u_hp + i > u.u_maxhp && mode) {
		u.u_maxhp += (how + 1);
		blineflg |= REDRAW_ALL;
	}

	chg_hp(i);

	doevent(noblind);	/* healing cures blindness and confusion */
	doevent(noconfuse);

	if (mode)
		pline("You feel %sbetter.", how ? "much " : "");
}

/*
 * DETECT: detect either treasure, magic, or food on the dungeon level
 */

int
detect(what)
register	what;	/* magic=0, treasure=1, food=2 */
{
	register LEVOBJ	*l;
	register	flag = NO;

	/* loop thru all of the level objects */
	for (l = lobjs; l < &lobjs[numlobjs]; l++) {
		if ((what == 1 && (l->l_o.o_type == TREASURE || l->l_o.
				o_type == GEM)) || (!what && (l->l_o.
				o_enchant || l->l_o.o_wepench)) || (what ==
					2 && l->l_o.o_type == FOOD)) {
			flag++;
			l->l_data |= L_DETECTED;
			putwhat(l->l_d);
		}
	}

	return flag;	/* returns whether anything was detected */
}
