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
#include	<stdio.h>
#include	"mag.h"

char	wizpword[8] =
	{ 'f'-'^','r'-'^','a'-'^','k'-'^','o'-'^','l'-'^','a'-'^',0 };

/*
 * WIZCHECK: check to see if the player in currently in wizard mode
 */

int
wizcheck()
{
	if (wizard != YES)
		illegal();

	return (wizard == YES);
}

/*
 * WEXP: raise the player by one exp level
 */

int
wexp()
{
	if (wizcheck())
		relevel();

	return NO;
}

/*
 * WDETECT: allow the wizard to detect monsters or objects
 */

int
wdetect()
{
	register LEVOBJ	*l;
	register	c;

	if (!wizcheck())
		return NO;

	pline("Detect what? (objects or monsters) ");
	go_imaginary();

	if ((c = getlower()) == 'm' || c == 'M')
		detmon();
	else if (c == 'o' || c == 'O') {
		for (l = lobjs; l < &lobjs[numlobjs]; l++) {
			l->l_data |= L_DETECTED;
			putwhat(l->l_d);
		}
	}

	erase();
	return NO;
}

/*
 * WMK_MON: allow the wizard to create the monster of his choice
 */

int
wmk_mon()
{
	register		i,
				c;
	register DUNGEON	*d;

	if (!wizcheck())
		return NO;

	if (u.u_data & UD_LABYRINTH) {
		pline("Sorry, this is a no-monster zone.");
		return NO;
	}

	pline("What type of monster: ");
	go_imaginary();
	c = ctgetch();

	/* figure out the monster for the letter typed */
	for (i = 0; i < NUMMON; i++) {
		if (c == pmon[i].p_letter) {
			mk_mon((d = near_loc(u.u_d)), i);
			putwhat(d);
			break;
		}
	}

	erase();

	return NO;
}

/*
 * WMODE: lets the player get in and out of wizard mode
 */

int
wmode()
{
	char		new[10],
			*red_herring;
	register	i;

	if (wizard == YES) {
		pline("You are no longer in wizard mode.");
		wizard = 2;	/* 2 means formerly in wiz mode */

		return NO;
	}

	for (i = 0; wizpword[i]; i++)	/* to make it a challenge! */
		new[i] = wizpword[i] + '^';

	new[i] = 0;

	pline("@Wizard password: ");
	red_herring = "tobsgcbh";	/* to throw off password seekers */

	if (!strcmp(get_str(NO), new)) {
		pline("You feel as smart as Michael J. Teixeira -- Welcome, wizard.");
		wizard = YES;

	} else pline("Sorry.");

	return (wizard != YES);
}

/*
 * WADVANCE: give the wizard some serious 'character enhancements'
 */

int
wadvance()
{
	if (!wizcheck())
		return NO;

	if (u.u_elevel < 18)			/* zot up to elev 18 */
		u.u_epoints = needexp(18);

	ck_exp();

	if (armor) {
		armor->o_enchant += 10;		/* mega-armor */
		u.u_ac += 10;
		ident(armor);
	}

	if (rightweapon) {
		rightweapon->o_enchant += 10;	/* mega-weapon */
		rightweapon->o_wepench += 10;
		ident(rightweapon);
	}

	fix_str(5);				/* mega-strength */
	blineflg = REDRAW_ALL;

	pline("Holy shit!  Whatever you did, it felt great!");

	return NO;
}

/*
 * WIDENT: let the wizard identify any object in inventory
 */

int
wident()
{
	if (wizcheck()) {
		typed = '\011';	/* fake out utilize to do an identify */
		utilize();
	}

	return NO;
}

/*
 * WTORCH: let the wiz magically illuminate a room
 */

int
wtorch()
{
	if (wizcheck())
		lightroom();

	return NO;
}

/*
 * WMAP: let the wiz map a dungeon level
 */

int
wmap()
{
	if (wizcheck())
		mapping();

	return NO;
}

/*
 * WTELE: let the wiz teleport randomly
 */

int
wtele()
{
	if (wizcheck())
		teleport((DUNGEON *)0);

	return NO;
}

/*
 * WDETTRAPS: let the wiz detect the traps on the dungeon level
 */

int
wdettraps()
{
	if (wizcheck())
		trapdet();

	return NO;
}

/*
 * TRAPDET: detect all of the traps on the level and print them
 */

void
trapdet()
{
	register TRAP	*t;
	
	for (t = traps; t < &traps[numtraps]; t++) {
		t->t_d->d_what = TRAPC;
		t->t_d->d_data |= D_SEEN;
		putwhat(t->t_d);
	}
}

/*
 * WMK_OBJ: let the wiz create any object he desires
 */

int
wmk_obj()
{
	int		a, b, c, d, e = 0;
	register	ch;
	char		line[90];

	if (!wizcheck())
		return NO;

	if (numinv >= MAXINV) {
		pline("Wizard, you're already carrying maximum items (%d).",
						numinv);
		return NO;
	}

	pline("@Object type? (symbol) ");
	go_imaginary();

	a = strchr(CITEMS, ctgetch()) - CITEMS;

	if (a < 0) {
		erase();
		return NO;
	}

	for (;;) {
		pline("@Which kind? <SPACE for list> ");
		go_imaginary();

		if ((ch = ctgetch()) == '\033') {	/* user aborted? */
			erase();
			return NO;
		}

		/* did the wiz type a legal letter for that obj type? */
		if ((b = ch - 'a') >= 0 && b < numobj[a]) {
			b += objoff[a];
			break;
		}

		/* if not, print a list of the possible types */
		cl_scr();

		for (c = 0; c < numobj[a]; c++) {
			i_set(c % 20 + 1, c / 20 * 40);
			sprintf(line, "%c) %s", 'a' + c, pobj[objoff[a] + c].
								po_name);
			swrite(line);
		}

		more(c % 20 + 1, c / 20 * 40, NO);
		remap();
	}

	if (a >= WAND && a <= SHIELD) {
		pline("@Enchantment%s? <0> ", a == WEAPON ? " to hit" : "");
		go_imaginary();
		c = atoi(get_str(YES));
	}

	if (a == WEAPON) {
		pline("@Enchantment to damage? <0> ");
		go_imaginary();
		e = atoi(get_str(YES));
	}

	pline("@Quantity? <1> ");
	go_imaginary();

	if ((d = atoi(get_str(YES))) < 0) {
		erase();
		return NO;
	}

	add_invent(mk_cursed(create_o(a, c, e, 0, b, (!d ? 1 : d))), YES);
}

/*
 * WMONSTAT: let the wiz do a Stat-o-Scan on a monster type to see just what
 * a monster can do...
 */

int
wmonstat()
{
	register		c;
	register struct permmon	*p;

	if (!wizcheck())
		return NO;

	pline("@Which monster (symbol): ");
	go_imaginary();
	c = ctgetch();

	if ((p = l_to_p((char)c)) == (struct permmon *)0) {
		erase();
		return NO;
	}

	pline("AL: %d Exp: %d HD: %d AC: %d Spd: %d Dam: %s Nm: %s",
		p->p_appear, p->p_bexp, p->p_hitdice, p->p_ac, p->p_speed,
						p->p_damage, p->p_name);
	return NO;
}

/*
 * WEXPSTAT: print out how many exp points the wizard has
 */

int
wexpstat()
{
	if (!wizcheck())
		return NO;

	pline("Wizard has %s experience points.", commastr(u.u_epoints));

	return NO;
}

/*
 * WMAKEWALL: let the wiz create a magic marble wall (can be dangerous!)
 */

int
wmakewall()
{
	if (!wizcheck())
		return NO;

	u.u_d->d_what = MARBLE;
	u.u_d->d_data |= D_STONE;

	return NO;
}

/*
 * WGENO: let the wiz genocide a monster
 */

int
wgeno()
{
	if (!wizcheck())
		return NO;

	genocide();

	return NO;
}
