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

char	*foodmes[5] = {
	"Yucko!  That was a rotten",
	"Ahh...what a delicious",
	"Yum!  That sure was a good",
	"Oh.  That sure was a wierd tasting",
	"Mmmmmm-mmmmmm.  That was a terrific",
};

/*
 * EAT: handles all consumption of foodstuffs
 */

int
eat(o)
register OBJECT	*o;
{
			/* 'shrooms are never rotten (being magical) */
	register        j = (o->o_offset == MUSHROOM ? 1 : rnd(5));

	/* the earlier foods in the obj array are more filling, also,
	   when rotten, all food is 1/8 as filling */
	efood->e_when += (long)((MUSHROOM+1-o->o_offset) * (j ? 200 : 25));
	u.u_data &= ~UD_FAINTING;

	if (o->o_offset == MUSHROOM) {
		pline("Whoa!  That was a real potent mushroom!");

		switch (rnd(4)) {
		case 0:
			healme(NO, NO);	/* heals player */

			if (rnd(3))	/* chance to fall through */
				break;

		case 1:
			fix_str(1);	/* raise strength by 1 */

			if (rnd(3))	/* chance to fall through */
				break;

		case 2:
			relevel();	/* raise experience level */
			break;

		case 3:
			/* sometimes a placebo-shroom */
			pline("It tastes like dirty fungus.");
		}

		/* make sure player isn't still hungry */
		if (efood->e_when < 200L)
			efood->e_when = 250L;

		u.u_data &= ~UD_HUNGRY;
	} else if (efood->e_when < 200L)
		pline("You still feel a little hungry.");
	else {
		u.u_data &= ~UD_HUNGRY;
		pline("%s %s.", foodmes[j], poname(o));
	}

	/* this is to prevent bonk-head players from using our poor hero's
	   stomach as a backpack for food */
	if (efood->e_when - u.u_moves > 2000L) {
		pline("You ate too much and vomited!");
		bell();
		efood->e_when = u.u_moves + 2L;	/* hungry in 2 moves! */

		if (u.u_d->d_what == FLOOR)
			u.u_d->d_what = POOL;	/* yuk!  don't step in it! */
	}

	return NO;
}

/*
 * WIELD: handles using/unusing weapons and staves
 */

int
wield(o)
register OBJECT	*o;
{
	register	hand;

	/* if wep/staff in current use, unwield it */
	if (o->o_data & O_INUSE) {
		unwield(o == rightweapon);
		return NO;
	/* if using 2-handed, must remove it to wield anything else */
	} else if (rightweapon && rightweapon->o_offset == TWOHAND &&
							!unwield(RIGHTHAND))
		return NO;

	/* 2-handed always held in right hand (internally) */
	if (o->o_offset == TWOHAND)
		hand = RIGHTHAND;
	else hand = gethand(leftweapon, rightweapon);

	if (hand == -1)		/* user pressed ESC */
		return -1;

	/* if using shield (but not a buckler) and left hand needed for wep,
	   remove shield */
	if (shield && (hand == LEFTHAND || (shield->o_offset != BUCKLER
				&& o->o_offset == TWOHAND)) && !unstrap())
		return NO;

	/* unuse a wep if in hand that's needed for new wep */
	if (u.u_use[hand] && !unwield(hand))
		return NO;

	/* unuse left wep if in use and new wep is 2-handed */
	if (o->o_offset == TWOHAND && leftweapon && !unwield(LEFTHAND))
		return NO;

	if (iscursed(o, NO)) {
		pline("The weapon adheres to your hand!");
		bell();
		o->o_data |= O_SEECURSED;
	}

	/* if player wield the secondary wep, it's no longer the sec wep */
	if (o == secweap)
		secweap = (OBJECT *)0;

	u.u_use[hand] = o;
	o->o_data |= O_INUSE;

	return YES;
}

/*
 * UNWIELD: remove a wep/staff from player's hand
 */

int
unwield(hand)
register	hand;
{
	if (iscursed(u.u_use[hand], YES))
		return NO;

	pline("You are no longer using %s.", form(u.u_use[hand], YES));
	u.u_use[hand]->o_data &= ~O_INUSE;
	u.u_use[hand] = (OBJECT *)0;

	return YES;
}

/*
 * WEAR: handles slapping on or removing a suit of armor
 */

int
wear(o)
register OBJECT	*o;
{
	register OBJECT	*oa = armor;

	/* try to take off armor and return if cursed or the user isn't
	   putting a new suit on */
	if ((oa && !unwear(YES)) || o == oa)
		return NO;

	armor = o;
	o->o_data |= O_INUSE;

	if (iscursed(o, NO)) {
		pline("The armor adheres to you!");
		bell();
		o->o_data |= O_SEECURSED;
	}

	ident(o);		/* armor is auto-identifying */
	u.u_ac += what_ac(o);
	blineflg = REDRAW_ALL;

	move_mon();	/* putting on armor is a slow process! */

	return YES;
}

/*
 * UNWEAR: actually take off a suit of armor if possible
 */

int
unwear(ifmovemon)
register        ifmovemon;	/* klepto-monsters can remove instantly */
{
	if (iscursed(armor, YES))
		return NO;

	if (ifmovemon)		/* removing armor is slow */
		move_mon();

	armor->o_data &= ~O_INUSE;
	pline("You are no longer using %s.", form(armor, YES));
	u.u_ac -= what_ac(armor);
	armor = (OBJECT *)0;
	blineflg = REDRAW_ALL;

	if (ifmovemon)		/* REALLY slow! */
		move_mon();

	return YES;
}

/*
 * CALL: gives a name to an object for future reference
 */

int
call(o)
register OBJECT	*o;
{
	register char	*cp;

	pline("@Call %s: ", obj_str(o));
	cursor(YES);

	/* make sure user didn't type ESC or RETURN */
	if (strcmp((cp = get_str(YES)), "-1") && *cp) {
		if (iscalled(o))	/* already called? free space */
			tex_free(nameptr[o->o_offset][0]);

		ostats[o->o_offset] = CALLED;
		nameptr[o->o_offset][0] = tex_alloc(strlen(cp) + 1);
		strcpy(nameptr[o->o_offset][0], cp);
	}

	cursor(NO);

	return YES;
}

/*
 * DROP: removes stuff from inventory and puts it on the dungeon floor
 */

int
drop(o)
register OBJECT	*o;
{
	register OBJECT	*no;
	register	howmany;
	char		*cp;

	if (o->o_data & O_INUSE) {
		pline("You are still using that!");
		bell();

		return -1;
	}

	if (o->o_quantity > 1) {
		pline("@Drop how many of the %s? ", obj_str(o));
		go_imaginary();

		/* if quant is > 9, then user must enter a string #, else
		  the user can enter a single digit without CR */
		if (o->o_quantity > 9) {
			/* check for ESC */
			if (!strcmp((cp = get_str(YES)), "-1")) {
				erase();
				return -1; 
			}

			/* CR means all the items */
			howmany = (!*cp ? o->o_quantity : atoi(cp));
		} else {
			howmany = ctgetch() - '0';

			/* check for ESC */
			if (howmany == '\033' - '0') {
				erase();
				return -1;
			}

			/* CR means all the items */
			if (howmany == '\015' - '0')
				howmany = o->o_quantity;
		}

		if (howmany < 1) {
			pline("Make up your mind!");
			return NO;
		}

		if (howmany > o->o_quantity) {
			pline("You don't have that many!");
			bell();
			return NO;
		}

		lessweight(o, howmany);
		no = copy_obj(o, howmany);

		if (howmany < o->o_quantity)
			o->o_quantity -= howmany;
		else rm_inv(o);
	} else {
		no = copy_obj(o, 1);
		lessweight(o, 1);	/* only one quant, just remove */
		rm_inv(o);
	}

	blineflg |= REDRAW_SCR;

	/* once you drop a scroll of hold mon, it's dust. wands do the same
	   thing sometimes if they are out of charges */
	if (no->o_offset == HOLDMON || (no->o_type == WAND && !no->o_charges
							&& rnd(2)))
		no->o_data |= O_USEDUP;

	no->o_data &= ~O_IGNITED;	/* extinguished when dropped */
	add_lobj(u.u_d, no);
	pline("You dropped %s.", obj_str(no));

	return NO;
}

struct	{
	char	*c_comname;
	int	(*c_pr)(),
		c_types;
} util[22] = {
	"quaff",		quaff,		CDRINK,
	"eat",			eat,		CEAT,
	"zap",			zap,		CZAP,
	"put on/remove",	puton,		CHAND,
	"read", 		readscrl,	CREAD,
	"wield",		wield,		CWIELD,
	"throw",		throw,		CTHROW,
	"wear/take off",	wear,		CWEAR,
	"call", 		call,		CALLABLE,
	"drop", 		drop,		DROPABLE,
	"use",			use,		CUSE,
	"ignite",		light,		CLIGHT,
	"unlock with",		unlock, 	CUNLOCK,
	"strap on/unstrap",	strap,		CSTRAP,
	"ignite a torch with",	ignite, 	CUSE,
	"recharge",		recharge,	CZAP,
	"identify",		ident,		IDENTABLE,
	"enchant",		encharm,	(CWEAR | CSTRAP),
	"enchant",		enchwep,	(CWIELD | CMWENCH),
	"galvanize",		galvanize,	(CWEAR | CSTRAP),
	"coat with oil",	smear,		DROPABLE,
   "use as a secondary weapon",	setsecwep,	CWIELD,
};

/*
 * UTILIZE: this procedure is called when any of the commands that require
 * a selection from inventory are executed.  After picking an item from
 * inventory, it calls the appropriate procedure to handle the action
 */
 
int
utilize()
{
	register OBJECT	*o;
	register char	*cp;
	char		*cletter = "qezprwtWcd\025L\013SI/\011])[sX";
	register	cmnd;
	int		red_flg = NO,
			c,
			last = 0,
			where,
			(*proc)(OBJECT *);


	/* find out which command by looking at the character typed */
	cmnd = strchr(cletter, typed) - cletter;

	/* get a compact string of the available inv items */
	cp = possitems(util[cmnd].c_types);

	if (*cp == 0) {
		pline("You don't have anything to %s.",util[cmnd].c_comname);
		return NO;
	}

	for (;;) {
		pline("@What do you want to %s? ([%s] or SPACE) ",util[cmnd].
							 c_comname, cp);
		doset(0, strlen(message));
		c = ctgetch();

		/* if spacebar hit, list all available.  if item symbol
		   typed, list ALL items of that type in inv */
		if (c == ' ' || (where = (strchr(CITEMS, c)-CITEMS)) >= 0) {
			/* we don't want to save this type of command to
			   a macro, so we zap it if recording */
			zapmacro();

			/* ignore since list is already on the screen */
			if (c == last)
				continue;

			last = c;

			if (red_flg)	/* clean up screen if needed */
				redraw();

			/* print the new list */
			if (pr_obj((c == ' ' ? util[cmnd].c_types : -where),
								YES) != -2)
				red_flg++;

			continue;
		}

		/* if user typed a letter, make sure it's a valid item */
		if ((o = l_to_i((char)c)) == (OBJECT *)0 || (pobj[o->
				o_offset].po_flag & util[cmnd].c_types)== NO) {
			if (c == '\033') {
				ntimes = 0;	/* prevent 999 q accident */
				erase();
				break;
			} else {
				pline("You can't %s that!", util[cmnd].
								c_comname);
				bell();
			}

			continue;
		}

		if (red_flg)
			redraw();

		/* this variable prevents a compiler warning that I'd get
		   since I can't figure out how to type-cast it */
		proc = util[cmnd].c_pr;

		if ((where = (*proc)(o)) == YES) /* exec the proper func */
			pline(form(o, NO));
		else if (where == -1)	/* -1 means func aborted */
			break;

		/* eating, drinking, and usually scrolls disappear after
		   being used */
		if (cmnd < 2 || where == -2 || (cmnd == 4 && rnd(15))) {
			minusone(o);

			if (cmnd == 4)
				pline("The scroll turns to dust.");
		}

		return YES;
	}

	if (red_flg)
		redraw();

	return NO;
}

/*
 * SMEAR: handles the coating of something with oil
 */
 
int
smear(o)
register OBJECT	*o;
{
	if (!(o->o_data & O_ETHEREAL)) {
		/* remove item from invent, make it ethereal, and then add
		   it back into invent (all behind the scenes) to reduce
		   the weight carried properly */
		lessweight(o, o->o_quantity);
		o->o_data |= O_ETHEREAL;
		u.u_weight += weight(o->o_type, o->o_offset, o->o_quantity,
					o->o_data);
	}

	return YES;
}
