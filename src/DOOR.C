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
 * WHAT_DOOR: returns a ptr to the door at the given dungeon location
 */

DOOR	*
what_door(d)
register DUNGEON	*d;
{
	register DOOR	*dr;

	/* loop thru door array */
	for (dr = doors; dr < &doors[numdoors]; dr++)
		if (dr->dr_d == d)
			return dr;

	pline("what_door: can't find door");

	return doors;		/* so at least it won't munge */
}

/*
 * MK_DOOR: create a door at the given dungeon location
 */

int
mk_door(d, secret)
register DUNGEON	*d;
{
	register ROOM	*r;
	register DOOR	*dr;

	if (d->d_data & D_DOOR)		/* already a door here? */
		return YES;

	if (numdoors == MAXDOORS) {
		pline("mk_door: Out of doors!");
		return NO;
	}

	r = what_room(d);
	dr = &doors[numdoors];

	dr->dr_d = d;
	dr->dr_r = r;
	dr->dr_data = 0;
	dr->dr_appear = d->d_what;
	dr->dr_stuck = !rnd(15) * (2 + rnd(5)) + !rnd(100) * rnd(50);

	r->r_numdoors++;	/* one more door in the room */

	if (!secret) {
		d->d_data &= ~(D_WALL | D_STONE);
		d->d_what = DOORC;
	}

	d->d_data |= D_DOOR;
	numdoors++;

	return YES;
}

/*
 * WALL_LOC: 
 */

DUNGEON	*
wall_loc(side, r, mode)
register	side;
register ROOM	*r;
{
	register	i;
	int		line,
			col;

	for (i = 0; i < 200; i++) {
		line = r->r_uline,
		col = r->r_ucol;

		switch (side) {
		case DOWN:
			line += rheight(r);
		case UP:
			col += rnd(rwidth(r) - 2) + 1;
			break;

		case RIGHT:
			col += rwidth(r);
		case LEFT:
			line += rnd(rheight(r) - 2) + 1;
			break;
		}

		/* if mode is true, we're making a torch */
		if (mode) {
			if ((dun[line][col].d_data & (D_WALL | D_DOOR)) ==
							D_WALL)
				break;

			/* not a good location, so try another random one */
			side = rnd(4) * 2;

			/* out of tries */
			if (i > 198)
				return (DUNGEON *)0;

		/* when placing a door, don't put it next to another one */
		} else if ((side%4 == 0 && !(dun[line][col+1].d_data&D_DOOR)
				&& !(dun[line][col-1].d_data&D_DOOR)) ||
				(side%4 && !(dun[line+1][col].d_data&D_DOOR)
				&& !(dun[line-1][col].d_data&D_DOOR)))
			break;
	}

	return &dun[line][col];
}

char	*lockcol[] = {
	" copper",
	" silver",
	0,
	" golden",
};

/*
 * ENTERDOOR: return whether it's possible to enter a door
 */

int
enterdoor(d)
register DOOR	*d;
{
	if (d->dr_data & DR_WITHLOCK) {
		pline("The door is sealed with magical%s lock.", (blind() ?
			"-feeling" : lockcol[(d->dr_data & DR_WITHLOCK)-1]));
		return NO;
	}

	if (d->dr_stuck) {
		if (!--d->dr_stuck)
			pline("It crashes open!");
		else pline("It's stuck.");

		return NO;
	}

	return YES;
}
