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
#include	<ctype.h>
#include	"mag.h"

/*
 * TYPENAME: returns the object type name for an item
 */

char	*
typename(o)
register OBJECT	*o;
{
	static char	tname[50];

	if (obj_names[o->o_type]) {
		strcpy(tname, obj_names[o->o_type]);

		if (o->o_quantity > 1)
			strcat(tname, "s");

		return tname;
	}

	if (o->o_type == WAND) {
		if (o->o_offset < DIGGING)
			return "rod";

		if (o->o_offset < CANCEL)
			return "staff";

		return "wand";
	}

	return poname(o);
}

/*
 * FORM: create a string with an item from invent, showing letter.  HOW
 * decides which of 2 forms to use
 */

char	*
form(o, how)
register OBJECT	*o;
register	how;
{	
	static char	line[120];

	if (how)
		sprintf(line, "%s (%c)", obj_str(o), i_to_l(o));
	else sprintf(line, "%c%c %s", i_to_l(o), (o->o_data & O_INUSE ?
					']' : ')'), obj_str(o));
	return line;
}

/*
 * I_TO_L: convert an inv ptr to the corresponding inv letter
 */

int
i_to_l(o)
register OBJECT	*o;
{
	register OBJECT	*c;
	register	letter = 'a';
	
	for (c = inv; c < &inv[numinv]; c++) {
		if (c == o)
			return letter;

		if (letter == '?')	/* once a ?, always a ? */
			continue;

		/* after lower case letters, go to upper case */
		if (++letter == 'z' + 1)
			letter = 'A';

		/* after upper case letters, go to all ?'s */
		if (letter == 'Z' + 1)
			letter = '?';
	}

	return letter;
}

/*
 * RM_INV: remove an item from player's inventory
 */

void
rm_inv(o)
register OBJECT	*o;
{
	register OBJECT	*ni;

	if (o == secweap)
		secweap = (OBJECT *)0;

	memmove(o, o+1, (numinv - ((o - inv)+1)) * sizeof (OBJECT));
	numinv--;

	/* if the item was ignited, that means that there is an explosion
	   event set to happen.  if there is nothing else in the pack that
	   is ignited, we have to remove the event */

	if (o->o_data & O_IGNITED) {
		for (ni = inv; ni < &inv[numinv]; ni++)
			if (ni->o_data & O_IGNITED)
				break;

		/* if nothing else burning, remove explosion event */
		if (ni == &inv[numinv])
			rm_event(oilexplode);
	}

	/* fixup all of the inuse inv ptrs */
	dec_iptrs(o);
}

/*
 * LESSWEIGHT: remove the weight of the given obj and quant
 */

void
lessweight(o, quan)
register OBJECT	*o;
register	quan;
{
	u.u_weight -= weight(o->o_type, o->o_offset, quan, o->o_data);
}

/*
 * CARRY: print the % max weight carried and the overall max
 */

int
carry()
{
	long	p = (long)((float)u.u_weight / (float)cancarry() * 100.0);

	pline("Your pack is %ld%% full (%s grams).", p,commastr(u.u_weight));

	return NO;
}

/*
 * GET: have the player snag any objects under him
 */

int
get()
{
	if (u.u_d->d_data & D_OBJECT) {
		pickup(u.u_d);
		return YES;
	}

	pline("There's nothing here for you to get.");

	return NO;
}

/*
 * POSSITEMS: create a string with the available inv letters in a nice
 * compact format such as 'a,d-f,i' for items a, d thru f, and i.
 */

char	*
possitems(which)
{
	register	blet = -1,
			elet = -1;
	register OBJECT	*o;
	static char	final[60];

	final[0] = 0;

	for (o = inv; o < &inv[numinv]; o++) {
		if (pobj[o->o_offset].po_flag & which) {
			elet = i_to_l(o);	/* mark end letter */

			if (blet == -1)		/* if not set, mark beg */
				blet = elet;
		} else if (blet != -1) {	/* end of sublist, close */
			sealup(final, blet, elet);
			elet = blet = -1;	/* restart */
		}
	}

	/* at end close up if needed */
	if (blet != -1)
		sealup(final, blet, elet);

	if (final[0])		/* nothing fits: final[-1] was written (RVIP port) */
		final[strlen(final) - 1] = 0;

	return final;
}

/*
 * SEALUP: adds letters to the POSSITEMS str in appropriate fashion
 */

void
sealup(final, beg, end)
register char	*final;
register	beg,
		end;
{
	char	foo[8];
	
	if (beg == end)
		sprintf(foo, "%c,", beg);
	else if (beg == end - 1)
		sprintf(foo, "%c,%c,", beg, end);
	else sprintf(foo, "%c-%c,", beg, end);

	strcat(final, foo);
}

/*
 * L_TO_I: convert a letter to the corresponding inv item
 */

OBJECT	*
l_to_i(l)
char	l;
{
	register OBJECT	*o;
	
	if (!isalpha(l))
		return (OBJECT *)0;

	for (o = inv; o < &inv[numinv]; o++) {
		if (l == 'a')
			return o;

		/* make sure to wrap from upper to lower case*/
		if (--l == 'A' - 1)
			l = 'z';
	}

	return (OBJECT *)0;
}

/*
 * IN_INV: look thru inv and see if given obj offset is in the list
 */

int
in_inv(off)
register	off;
{
	register OBJECT	*o;

	for (o = inv; o < &inv[numinv]; o++)
		if (o->o_offset == off)
			return YES;

	return NO;
}

/*
 * MINUSONE: handle removing one quantity from an inv item
 */

void
minusone(o)
register OBJECT	*o;
{
	lessweight(o, 1);
	blineflg |= REDRAW_SCR;

	/* if there was only one, we must remove the inv item */
	if (--o->o_quantity < 1)
		rm_inv(o);
}
