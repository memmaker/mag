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
#include	<ctype.h>
#include	"mag.h"

/*
 * INVENTORY: list a player's complete invent on the screen
 */

int
inventory()
{
	register	line;

	if (!numinv) {
		pline("You are empty-handed.");
		return NO;
	}

	/* list all dropable (read all) items */
	if ((line = pr_obj(DROPABLE, NO)) != -1) {
		set_vis(1);
		more(line < 22 ? line : 21, 0, NO);
		cl_scr();
		set_act(0);
		set_vis(0);
	}

	return NO;
}

/*
 * PR_OBJ: this proc does the actual displaying of inv items and deciding
 * which items to print
 */

int
pr_obj(which, how)
register	which;
{
	register OBJECT	*o;
	register	line = 0;
	int		count = 0;
	char		*cp;

	/* count how many items are going to appear on this list.  this is
	   decided with the WHICH param which when negative means print all
	   items of obj type abs(WHICH)...ex) WHICH=-4 so it will print all
	   objects of type +4 (rings).  if WHICH is positive, then print
	   all items with WHICH bit(s) set in the obj flag...ex) WHICH=CREAD
	   (CREAD = 4) which will print all of the readable items. got it? */
	for (o = inv; o < &inv[numinv]; o++)
		if ((which > 0 && (pobj[o->o_offset].po_flag & which)) ||
				(which < 1 && o->o_type == (char)abs(which)))
			count++;

	/* nothing to print return special value */
	if (!count)
		return -2;

	/* if printing more than one and HOW=0 (how is flag to say if items
	   should be printed over level or on sep video page), prepare for
	   video page 1 */
	if (count > 1 && !how) {
		erase();
		set_act(1);
	}

	color(WHITE);

	/* go thru inv and print the proper ones */
	for (o = inv; o < &inv[numinv]; o++) {
		if ((which < 1 && o->o_type != (char)abs(which)) || (which >
				0 && !(pobj[o->o_offset].po_flag & which)))
			continue;

		/* if only printing one and on another video page, just print
		   it on the top line and forget another video page */
		if (count == 1 && !how) {
			pline(form(o, NO));
			return -1;
		}

		/* position cursor for two column printing */
		i_set((line % 20) + 1, (((line / 20) % 2) * 38));
		cp = form(o, NO);

		/* if another item is going to be printed to the right of
		   the current item, truncate the string so that the two
		   columns don't run together.  also truncate all items in
		   the right column so they don't go passed the end of the
		   line */
		if (count >= 20 + line + 1 || line >= 20)
			*(cp + 37) = 0;
		else *(cp + 77) = 0;

		/* if printing over the level, set redisplay to clean up */
		if (how)
			set_red((line%20) + 1, 0, (((line/20)%2) * 38) +
						strlen(cp));
		/* printing complete inv on sep video page, use different
		   color for each type if that option is on */
		else if ((u.u_options & O_COLORINV) && which == DROPABLE)
			color(o->o_type + 1 + (o->o_type > 6));

		swrite(cp);

		if (how)
			swrite(" ");

		line++;
	}

	return line + 1;
}

char	*blindobj[] = { 0, "potion", "scroll", "stick", "ring", 0, 0, 0,
						"key", 0, "gem", "piece" };

/*
 * OBJ_STR: create a string that describes any object
 */

char	*
obj_str(o)
register OBJECT	*o;
{
	static char	rstr[120];
	char		tmp[120],
			dud[120];
	register char	*name = nameptr[o->o_offset][0];

	/* if player is blind, things are very ambiguous */
	if (blind() && blindobj[o->o_type])
		sprintf(tmp, "%s%s", blindobj[o->o_type], o->o_quantity > 1 ?
								"s" : "");
	else if (isknown(o)) {
		sprintf(tmp, "%s of %s", typename(o), name);

		if (o->o_type == WAND && (o->o_data & O_IDENT)) {
			sprintf(dud, " (%d charge%s left)", o->o_charges,
				o->o_charges != 1 ? "s" : "");
			strcat(tmp, dud);
		}

		if ((o->o_data & O_IDENT) && (pobj[o->o_offset].po_flag &
							CENCH) && !blind()) {
			sprintf(rstr, "%s%d %s", o->o_enchant < 0 ? "" : "+",
							o->o_enchant, tmp);
			strcpy(tmp, rstr);
		}
	} else if (isunknown(o)) {
		if (o->o_type == SCROLL)
			sprintf(tmp, "scroll titled `%s'", name);
		else if ((pobj[o->o_offset].po_flag & (CDRINK | CWEAR | CZAP
				| CHAND | CUNLOCK)) || o->o_type >= GEM)
			sprintf(tmp, "%s%s %s", (o->o_type != ARMOR ? "" :
					"suit of "), name, typename(o));
		else {
			strcpy(tmp, name);

			if (o->o_quantity > 1)
				strcat(tmp, "s");
		}

		if ((o->o_data & O_IDENT) && (pobj[o->o_offset].po_flag &
							CENCH) && !blind()) {
			if (o->o_type == WEAPON)
				sprintf(rstr, "%s%d,%s%d %s", (o->o_enchant <
				    0 ? "" : "+"), o->o_enchant,(o->o_wepench
				    < 0 ? "" : "+"), o->o_wepench, tmp);
			else sprintf(rstr, "%s%d %s", (o->o_enchant < 0 ? "":
						"+"), o->o_enchant, tmp);

			strcpy(tmp, rstr);
		}
	} else sprintf(tmp, "%s called `%s'", typename(o), name);

	if (!isunknown(o) && !(pobj[o->o_offset].po_flag&CREAD)&& !blind()) {
		sprintf(dud, " [%s]", nameptr[o->o_offset][1]);
		strcat(tmp, dud);
	}

	if (o->o_quantity > 1)
		sprintf(rstr, "%s %s", commastr((long)o->o_quantity), tmp);
	else if (o->o_offset != SAPPHIRE)
		strcpy(rstr, article(tmp, NO));
	else strcpy(rstr, tmp);

	tmp[0] = 0;

	/* whew!  now we have to add all of the parenthetic messages */

	if (secweap && o == secweap)
		tackon(tmp, "secondary weapon");

	if (o->o_data & O_INUSE) {
		if (shield && o == shield)
			tackon(tmp, "strapped on left arm");
		else if (armor && o == armor)
			tackon(tmp, "being worn");
		else if (o->o_offset == TWOHAND)
			tackon(tmp, "in both hands");
		else if (leftweapon && o == leftweapon)
			tackon(tmp, "in left hand");
		else if (rightweapon && o == rightweapon)
			tackon(tmp, "in right hand");
		else if (leftring && o == leftring)
			tackon(tmp, "on left hand");
		else if (rightring && o == rightring)
			tackon(tmp, "on right hand");
	}

	if (o->o_data & O_IGNITED)
		tackon(tmp, "burning");

	if (o->o_data & O_SEECURSED)
		tackon(tmp, "cursed");

	if (o->o_data & O_GALVANIZED)
		tackon(tmp, "galvanized");

	if (o->o_data & O_ETHEREAL)
		tackon(tmp, "ethereal");

	if (tmp[0]) {
		strcat(rstr, tmp);
		strcat(rstr, ")");
	}

	return rstr;
}

/*
 * TACKON: adds to string of messages separated with commas
 */

void
tackon(str, on)
register char	*str,
		*on;
{
	if (!*str)
		sprintf(str, " (%s", on);
	else sprintf(&str[strlen(str)], ", %s", on);
}

/*
 * ARTICLE: puts an 'a' or 'an' in front of a word and will also capitalize
 * if cap is true
 */

char	*
article(str, cap)
register char	*str;
register	cap;
{
	static char	line[100];
	
	if (strchr("aeiouAEIOU", *str) || !strncmp(str, "uni", 3))
		sprintf(line, "an %s", str);
	else sprintf(line, "a %s", str);

	if (cap)
		capital(line);

	return line;
}

/*
 * CAPITAL: capitalizes the first letter of a string
 */

char	*
capital(str)
register char	*str;
{
	if (islower(*str))
		*str -= 'a' - 'A';

	return str;
}

/*
 * ADD_INVENT: add a copy of the given obj into the proper location of the
 * player inv
 */

OBJECT	*
add_invent(o, print)
register OBJECT	*o;
{
	register OBJECT	*i,
			*j;

	if (numinv >= MAXINV) {
		pline("Warning!  Attempting to add too much to inv...");
		bell();
		return inv;
	}

	/* picking up the sapphire creates the down staircase on the
	   sapphire level */
	if (o->o_offset == SAPPHIRE && sapp_lev()) {
		dun[18][4].d_what = DNSTAIR;
		dun[18][4].d_data |= D_STAIRCASE;
	}

	blineflg |= REDRAW_SCR;

	/* should the new item go at the beginning of the list? */
	if (!numinv || inv[0].o_type > o->o_type) {
		/* shift everying over and add new item */
		memmove(&inv[1], inv, numinv * sizeof (OBJECT));
		memmove(inv, o, sizeof (OBJECT));
		numinv++;

		inc_iptrs(inv);		/* fixup inuse items */

		if (print)
			pline(form(inv, NO));

		u.u_weight += weight(o->o_type, o->o_offset, o->o_quantity,
					o->o_data);
		return inv;
	}

	/* not at beg. so find out where */
	for (i = inv; i < &inv[numinv]; i++) {
		/* can the new object be grouped with one already in inv? */
		if (i->o_offset == o->o_offset && i->o_data == o->o_data &&
				i->o_enchant == o->o_enchant && i->o_wepench
				== o->o_wepench && (pobj[o->o_offset].
				po_flag & CGROUP)) {
			i->o_quantity += o->o_quantity;

			if (print)
				pline("%s (You added %d)", form(i, NO), o->
								o_quantity);
			u.u_weight += weight(o->o_type, o->o_offset, o->
						o_quantity, o->o_data);

			return i;
		}

		/* should it go after the current one? */
		if (i == &inv[numinv-1] || (i+1)->o_type > o->o_type) {
			/* shove over the remaining (if any) and add the
			   new one */
			j = &inv[numinv];
			memmove(i+2, i+1, ((j - i) - 1) * sizeof (OBJECT));
			memmove(i+1, o, sizeof (OBJECT));
			numinv++;

			inc_iptrs(i+1);

			if (print)
				pline(form(i+1, NO));

			u.u_weight += weight(o->o_type, o->o_offset, o->
						o_quantity, o->o_data);
			return i+1;
		}
	}

	return inv;	/* should never get here */
}

/*
 * INC_IPTRS: move the inuse ptrs like RIGHTWEP and SHIELD one lower in the
 * inv list if they are AFTER the object just added since all of the items
 * were shifted one
 */

void
inc_iptrs(o)
register OBJECT	*o;
{
	register	i;

	for (i = 0; i < 7; i++)
		if (u.u_use[i] && u.u_use[i] >= o)
			u.u_use[i]++;
}

/*
 * DEC_IPTRS: same as INC_IPTRS except move em up one because an item was
 * removed from inv
 */

void
dec_iptrs(o)
register OBJECT	*o;
{
	register	i;

	for (i = 0; i < 7; i++)
		if (u.u_use[i] && u.u_use[i] >= o)
			u.u_use[i]--;
}

