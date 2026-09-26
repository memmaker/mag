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
#include        <string.h>

/*
 * PICKUP: have the player attempt to pick up all objs under him
 */

void
pickup(d)
register DUNGEON *d;
{
	register LEVOBJ	*l;
	register	howmany;
	long		singlewt;

	for (l = lobjs; l < &lobjs[numlobjs]; ) {
		if (l->l_d != d) {
			l++;
			continue;
		}

		if (u.u_data & UD_LEVITATE) {
			pline("You are hovering above %s.",obj_str(&l->l_o));
			l++;
			continue;
		}

		if (!(u.u_options & O_AUTOGET)) {
			redraw();
			pline("@Do you want to pick up %s? ", obj_str(&l->
								l_o));
			go_imaginary();

			if (getlower() != 'y') {
				erase();
				l++;
				continue;
			}
		}

		if (l->l_o.o_data & O_USEDUP) {
			pline("Whoa! %s turned to dust!", capital(obj_str(
								&l->l_o)));
			rm_lobj(l);
			continue;
		}

		singlewt = weight(l->l_o.o_type, l->l_o.o_offset, 1,
						l->l_o.o_data);

		if (numinv < MAXINV && singlewt * (long)l->l_o.o_quantity <=
							carryleft()) {
			add_invent(&l->l_o, YES);
			rm_lobj(l);
			continue;
		}

		/* figure out how many can be picked up */
		howmany = numinv < MAXINV ? (int)(carryleft()/singlewt) : 0;

		if (howmany < 1) {
			pline("You can't carry %s.", obj_str(&l->l_o));
			l++;
			continue;
		}

		pline("You can only pick up %d of %s.", howmany, obj_str(
								&l->l_o));
		l->l_o.o_quantity -= howmany;
		add_invent(copy_obj(&l->l_o, howmany), NO);
		l++;
	}
}

/*
 * SETUPOBJ: set up the initial config of object descriptions
 */

void
setupobj()
{
	register	i;

	for (i = 0; i < NUMOBJ; i++) {
		nameptr[i][0] = pobj[i].po_name;
		ostats[i] = UNKNOWN;
	}

	scramble(fpotions, objoff[POTION], NUMPOTIONS);
	scramble(fscrolls, objoff[SCROLL], NUMSCROLLS);
	scramble(fwands, objoff[WAND], NUMWANDS);
	scramble(frings, objoff[RING], NUMRINGS);
}

/*
 * SCRAMBLE: shuffle the random scroll, potion, ring, and wand names
 */

void
scramble(fakes, where, num)
char		*fakes[];
register	num;
{
	char		keeptrack[30];	/* max of 30 diff descriptions */
	register	i,
			j;

	for (i = 0; i < num; i++)	/* set all to not used */
		keeptrack[i] = NO;

	for (i = 0; i < num; i++) {
		do j = rnd(num);	/* pick an unused description */
		while (keeptrack[j]);

		keeptrack[j]++;
		nameptr[where + i][0] = nameptr[where + i][1] = fakes[j];
	}
}

/*
 * CREATE_O: setup a static object structure as described
 */

OBJECT	*
create_o(type, ench, wench, data, off, quan)
register	type,
		ench;
{
	static OBJECT	o;

	o.o_type = (char)type;
	o.o_enchant = ench;
	o.o_wepench = wench;
	o.o_data = data;
	o.o_offset = off;
	o.o_quantity = quan;

	return &o;
}


/*
 * MK_CURSED: set an object to cursed if it meets the requirements
 */

OBJECT *
mk_cursed(o)
register OBJECT	*o;
{
	if (o->o_enchant < 0 || o->o_wepench < 0 || (o->o_type == RING &&
			o->o_offset <= RTELEPORT))
		o->o_data |= O_CURSED;

	return o;
}

/*
 * MK_LEVOBJ: plop down the starting objs on a new level
 */

void
mk_levobj()
{
	register ROOM	*r;
	register TRAP	*t;

	if (sapp_lev()) {	/* only 2 objs on the sapphire level! */
		add_lobj(&dun[11][64], create_o(KEY, 0, 0, 0, GOLDEN, 1));
		add_lobj(&dun[11][64], create_o(MISC, 1+rnd(5),0,0, SAPPHIRE,
									1));
		return;
	}		

	for (r = rooms; r < &rooms[numrooms]; r++) {
		/* trove treasure is handled separately */
		if (r->r_data & R_TROVE)
			continue;

		if (rnd(2))
			do add_lobj(room_loc(r), mk_treasure());
			while (!rnd(4));

		if (rnd(10) < 4)
			do add_lobj(room_loc(r), mk_obj());
			while (!rnd(8));
	}

	/* all strangle-weed traps have treasure to lure their prey! */
	for (t = traps; t < &traps[numtraps]; t++)
		if (t->t_t->t_func == weed)
			add_lobj(t->t_d, mk_treasure());
}

/*
 * WHAT_OBJ: return the first obj on the level found at location D
 */

LEVOBJ *
what_obj(d)
register DUNGEON	*d;
{
	register LEVOBJ	*l;

	for (l = lobjs; l < &lobjs[numlobjs]; l++)
		if (l->l_d == d)
			return l;

	return (LEVOBJ *)0;
}

/*
 * Remove an object from the level obj array
 */

void
rm_lobj(l)
register LEVOBJ	*l;
{
	register DUNGEON *d = l->l_d;

	/* shift all obj after L forward one */
	memmove(l, l+1, (numlobjs - ((l - lobjs)+1)) * sizeof (LEVOBJ));
	numlobjs--;

	/* if there are no more obj at L's location, clear object flag */
	if (!what_obj(d))
		d->d_data &= ~D_OBJECT;
}

/*
 * COPY_OBJ: create a copy of the given obj
 */

OBJECT	*
copy_obj(o, quan)
register OBJECT	*o;
register	quan;
{
	return create_o(o->o_type, o->o_enchant, o->o_wepench, o->o_data,
							o->o_offset, quan);
}

/*
 * MK_TREASURE: create a random treasure type
 */

OBJECT	*
mk_treasure()
{
	return create_o(TREASURE, 0, 0, 0, PLATINUM + (rnd(3) ?
			rnd(NUMTREAS - 1) + 1 : rnd(NUMTREAS)), muchtreas());
}

/*
 * MUCHTREAS: calculate the amount of treasure to create for current level
 */

int
muchtreas()
{
	return (1 + rnd((((u.u_dlevel / 4) + 1) * 50) + 2));
}

/*
 * MK_OBJ: create a random object
 */

OBJECT	*
mk_obj()
{
	register	type = gettype(),
			off = getoff(type);

	return mk_cursed(create_o(type, getench(off), getwench(type), 0, off,
		getquan(off)));
}

/*
 * GETENCH: return the appropriate enchantment for and object offset
 */

int
getench(off)
register	off;
{
	register	i;

	switch (obj_type(off)) {
	case MISC:
		return (off == SAPPHIRE) * (rnd(3) + 1);

	case POTION:
	case SCROLL:
	case KEY:
		return YES;
	
	case WAND:
		if (rnd(5)) {
			if (off == ILLUMINATION || off == TRAPDET)
				return 10 + rnd(20);

			return 1 + rnd(10);
		} else return NO;

	case RING:
		if (off == INCRHP || off == DECRHP)
			i = rnd(7) + 1;
		else i = rnd(3) + 1;

		if (off <= RTELEPORT)	/* cursed rings */
			return -i;

		return i;

	case WEAPON:
		if (!rnd(4))
			return rnd(7) - 3;

		return NO;

	case ARMOR:
	case SHIELD:
		if (!rnd(3))
			return rnd(7) - 3;
	}

	return NO;
}

/*
 * GETWENCH: get a weapon enchantment
 */

int
getwench(type)
register	type;
{
	if (type == WEAPON && !rnd(5))
		return (rnd(7) - 3);

	return NO;
}

/*
 * GETQUAN: figure out the quantity of a new object
 */

int
getquan(off)
register	off;
{
	if (off <= MUSHROOM)	/* foodstuffs */
		return 1 + !rnd(4);

	if (off == OILFLASK)
		return rnd(4) + 2 + !rnd(4) * 4;

	if (off == ARROW || off == BOLT || off == BULLET || off == DART) 
		return rnd(10) + 2 + !rnd(5) * rnd(5);

	return 1;
}

/*
 * ADD_LOBJ: add an object to the level obj array
 */

void
add_lobj(d, o)
register DUNGEON	*d;
register OBJECT		*o;
{
	register LEVOBJ	*l = &lobjs[numlobjs];

	if (numlobjs < MAXLOBJS) {
		l->l_d = d;
		l->l_data = 0;
		memmove(&l->l_o, o, sizeof (OBJECT));
		d->d_data |= D_OBJECT;
		numlobjs++;
	} else pline("Warning - not enough level object space!");
}

/*
 * OBJ_TYPE: return object TYPE given its OFFSET
 */

int
obj_type(off)
register	off;
{
	register	i;
	
	for (i = NUMTYPES - 1; i >= 0; i--)
		if (off >= objoff[i])
			return i;

	return NO;
}

/*
 * GETOFF: return a random offset given an object type
 */

int
getoff(type)
register	type;
{
	register	i = objoff[type] + rnd(numobj[type]);

	switch (type) {
	case FOOD:
	case WAND:
	case RING:
	case GEM:
	case KEY:
		return i;

	case POTION:
		/* some potions are more rare than others */
		if ((i == HEROISM || i == RAISELEV) && rnd(2))
			return (rnd(2) ? HEALING : RESTORESTR);

		return i;

	case SCROLL:
		/* some scrls are more common and some are more rare */
		if (!rnd(10) || ((i == GENOCIDE || i == HOLDMON) && rnd(2)))
			return (rnd(2) ? IDENTIFY : MAGICMAP);

		return i;

	case WEAPON:
		if (i == TWOHAND && rnd(2))	/* rare 2-handers */
			return DAGGER;

		return i;

	case ARMOR:	
		if (i == PLATE && rnd(2))	/* rare plate armor */
			return PADDED;

		return i;

	case SHIELD:
		if (i == MITHRIL && rnd(2))	/* rare mithril */
			return MEDIUM;

		return i;

	case MISC:
		if (i == SAPPHIRE)	/* NEVER the Sapphire */
			return objoff[type] + rnd(numobj[MISC]-1) + 1;

		return i;
	}
}

int	typechance[NUMTYPES-1] = { 5,30,55,59,62,77,84,89,92,96,99 };

/*
 * GETTYPE: get a random obj type using the chance array above
 */

int
gettype()
{
	register	i,
			pick = rnd(100);

	for (i = 0; i < NUMTYPES - 1; i++)
		if (pick <= typechance[i])
			return i;
}

/*
 * IDENT: identify and object
 */

int
ident(o)
register OBJECT	*o;
{
	if (o->o_type >= WAND && o->o_type <= SHIELD)
		o->o_data |= O_IDENT;

	if (iscursed(o, NO))
		o->o_data |= O_SEECURSED;

	makeknown(o);

	return YES;
}

/*
 * MAKEKNOWN: change the name of an object to its real name
 */

void
makeknown(o)
register OBJECT	*o;
{
	if (ostats[o->o_offset] == CALLED)
		tex_free(nameptr[o->o_offset][0]);

	nameptr[o->o_offset][0] = poname(o);

	if (o->o_type >= POTION && o->o_type <= RING)
		ostats[o->o_offset] = KNOWN;
}

/*
 * DECURSE: remove the curses from any object in use by player
 */

void
decurse()
{
	register	i;

	for (i = 0; i < 6; i++)
		if (u.u_use[i])
			u.u_use[i]->o_data &= ~(O_CURSED | O_SEECURSED);
}

char	*al_data[4] = {"potions","scrolls","wands, staves, or rods","rings"};

/*
 * ALREADY: list ring,wand,scrls,and potions that have been experienced
 */

int
already()
{
	register	type,
			i,
			line = 0;
	OBJECT		*o = create_o(0, 0, 0, 0, 0, 1);
	int		count,
			any;
	char		done[30];	/* max of 30 per obj type */

	set_act(1);
	set_vis(1);

	/* each group-type of objects must be printed in random order so that
	   the player can't figure out what each one is */
	for (type = POTION; type <= RING; type++) {
		color(CYAN | INTENSE);

		for (i = 0; i < 30; i++)	/* reset used flags */
			done[i] = NO;

		count = any = 0;
		o->o_type = (char)type;

		while (count < numobj[type]) {
			do i = rnd(numobj[type]); /* find unprinted offset */
			while (done[i]);

			done[i] = YES;

			/* has obj been experienced? print it */
			if (ostats[objoff[type] + i] != UNKNOWN) {
				if (line > 20) {
					more(22, 1, 0);
					color(CYAN | INTENSE);
					cl_scr();
					line = 0;
				}

				any++;
				i_set(line++, 0);
				o->o_offset = objoff[type] + i;
				swrite(capital(obj_str(o)));
			}

			count++;
		}

		/* if none of a type have been experience, print message */
		if (!any) {
			if (line > 20) {
				more(22, 1, 0);
				color(CYAN | INTENSE);
				cl_scr();
				line = 0;
			}

			i_set(line++, 0);
			swrite("You don't know anything about any ");
			swrite(al_data[type - POTION]);
			swrite(".");
		}

		line++;
	}

	more(22, 1, 0);

	cl_scr();
	set_act(0);
	set_vis(0);

	return NO;
}

