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
 * LIGHTROOM: illuminate the player's current room
 */

void
lightroom()
{
	if (u.u_r) {
		pline("There is a flash of indigo light!");
		pline("The room is now lit by a shimmering blue light.");

		u.u_r->r_data |= (R_TORCHED | R_MAGICLIT);
		readyroom(u.u_r);
	}
}

/*
 * FIT_ROOMS: place all the rooms down on the dungeon at the beginning of
 * mklev.
 */

void
fit_rooms()
{
		/* make tries larger for more rooms */
	register	tries = 300 - !rnd(3) * 100;
	register ROOM	*r;

	for (numrooms = 0; numrooms < MAXROOMS && tries; tries--) {
		r = &rooms[numrooms];
		r->r_uline = (char)rnd(LINES - 7) + 2;
		r->r_ucol = (char)rnd(COLUMNS - 17) + 2;
		r->r_lline = r->r_uline + 4 + rnd(5);
		r->r_lcol = r->r_ucol + 5 + rnd(9);

		/* certain chance that the room will be BIGGER */
		if (rnd(5) < 2) {
			r->r_lline += (char)(4 + rnd(10));
			r->r_lcol += (char)(4 + rnd(15));
		}

		/* make sure the room will fit at this location */
		if (!rinbounds(r) || !ck_room(r))
			continue;

		r->r_data = r->r_numdoors = 0;

		/* decide if the room will be lit with torches */
		if (rnd(20) > u.u_dlevel - 1 || !rnd(100))
			r->r_data |= R_TORCHED;

		mk_room(r);
		numrooms++;
	}
}

/*
 * RINBOUNDS: make sure the rooms corners are within dungeon boundaries
 */

int
rinbounds(r)
register ROOM	*r;
{
	return (r->r_uline > 2 && r->r_lline < 21 && r->r_ucol > 1 && r->
								r_lcol < 76);
}

/*
 * MK_POOL: slap a pseudo-oval pool of water on a room floor
 */

void
mk_pool(r, rad)
register ROOM	*r;
{
	DUNGEON		*d = room_loc(r),
			*nd;
	register	i,
			j;
	int		lr = (int)(rad * 0.6);

	/* this is the pseudo part...making an oval shaped pool without
	   using trig */
	for (i = -lr; i <= lr; i++) {
		for (j = (scrcol(d) - rad) + (i == -lr || i == lr); j <=
			     (scrcol(d) + rad) - (i == -lr || i == lr);j++) {
			nd = &dun[scrline(d) + i][j];

			if (what_room(nd) == r && nd->d_what == FLOOR)
				nd->d_what = POOL;
		}
	}
}

/*
 * WHAT_ROOM: returns a ptr the room that the location is in
 */

ROOM	*
what_room(d)
DUNGEON	*d;
{
	register ROOM	*r;
	register	line = scrline(d),
			col = scrcol(d);

	for (r = rooms; r < &rooms[numrooms]; r++)
		if (line >= r->r_uline && line <= r->r_lline && col >= 
					r->r_ucol && col <= r->r_lcol)
			return r;

	return (ROOM *)0;
}

/*
 * DOROOM: a generic procedure to do something to every location in a room.
 * if MODE=0 then make the room visible, MODE=1 then make sure the room is
 * on rock, MODE=2: put in the symbols for a room (walls, floor, corners),
 * MODE=3: fill a room with marble walls, MODE=4: fill the room with trove
 * obj and treasure, MODE=5: make a room dark, MODE=6: clear a room of all
 * marble walls
 */

int
doroom(r, mode)
ROOM	*r;
{
	register DUNGEON	*d;
	register		l,
				c;
	int			el = r->r_lline,
				ec = r->r_lcol,
				sl = r->r_uline,
				sc = r->r_ucol;

	if (mode == 1) {
		sl -= 1;
		sc -= 1;
		el += 1;
		ec += 1;
	} else if (mode >= 3) {
		sl += 1;
		sc += 1;
		el -= 1;
		ec -= 1;
	}

	for (l = sl; l <= el; l++) {
		for (c = sc; c <= ec; c++) {
			d = &dun[l][c];

			switch (mode) {
			case 0: 			/* pr_room */
				d->d_data |= D_SEEN;
				continue;

			case 1: 			/* ck_room */
				if (d->d_what != ROCK)
					return NO;

				continue;

			case 2: 			/* mk_room */
				d->d_data |= D_WALL;

				if (l == sl && c == sc)
					d->d_what = ULCORNER;
				else if (l == el && c == sc)
					d->d_what = LLCORNER;
				else if (l == sl && c == ec)
					d->d_what = URCORNER;
				else if (l == el && c == ec)
					d->d_what = LRCORNER;
				else if (l == sl || l == el)
					d->d_what = HWALL;
				else if (c == sc || c == ec)
					d->d_what = VWALL;
				else {
					d->d_what = FLOOR;
					d->d_data &= ~(D_WALL | D_STONE);
				}

				continue;

			case 3: 			/* wl_room */
				d->d_what = MARBLE;
				d->d_data |= (D_WALL | D_STONE);
				continue;

			case 4: 			/* tr_room */
				if (rnd(2) || sapp_lev())
					add_lobj(d, rnd(2) ? mk_obj() : 
							mk_treasure());

				continue;

			case 5: 			/* dk_room */
				if (d->d_what == FLOOR) 
					d->d_data &= ~D_SEEN;

				continue;
			case 6:				/* cl_room */
				if (d->d_what == MARBLE) {
					d->d_what = FLOOR;
					d->d_data &= ~(D_WALL | D_STONE);
				}

				d->d_data &= ~D_SEEN;

				continue;
			}
		}
	}

	return YES;
}

/*
 * SPEC_ROOM: handle creating some of the special rooms
 */

void
spec_room()
{
	register ROOM		*r;
	register		nt = 0;
	int			h,
				w;
	
	for (r = rooms; r < &rooms[numrooms]; r++) {
		h = rheight(r);
		w = rwidth(r);

		/* should the room have pools of water in it? */
		if (h > 6 && w > 7 && rnd(2)) {
			do mk_pool(r, rnd(2) + 3);
			while (rnd(9) < 4 || (w > 12 & !rnd(3)));

			continue;
		}

		/* should the room be a treasure trove?!? */
		if (r->r_numdoors == 1 && nt < 3 && !rnd(8)) {
			mk_trove(r);
			nt++;

			continue;
		}

		if (h < 5 || w < 6 || h % 2 || w % 2 || rnd(3))
			continue;

		r_to_maze(r);	/* and lastly a maze-room */
	}
}

/*
 * ROOM_LOC: return a random location withing the given room
 */

DUNGEON	*
room_loc(r)
register ROOM	*r;
{
	register DUNGEON	*d;

	do d = &dun[r->r_uline + rnd(rheight(r))][r->r_ucol +rnd(rwidth(r))];
	while (what_room(d) != r || d->d_data & (D_WALL | D_DOOR));

	return d;
}

/*
 * ROOM_SIDE: return the direction of the wall that the given is on
 */

int
room_side(line, col, r)
register	line,
		col;
register ROOM	*r;
{
	if (line == r->r_uline)
		return UP;

	if (line == r->r_lline)
		return DOWN;

	if (col == r->r_ucol)
		return LEFT;

	if (col == r->r_lcol)
		return RIGHT;

	return -1;
}

/*
 * READYROOM: prepare a room to be displayed on the screen
 */

void
readyroom(r)
register ROOM	*r;
{
	if (r && (r->r_data & R_TORCHED) && !blind()) {
		pr_room(u.u_r = what_room(u.u_d));
		red_box(r->r_uline, r->r_ucol, r->r_lline, r->r_lcol);
	}
}

/*
 * MK_TROVE: make a room into a treasure trove
 */

void
mk_trove(r)
register ROOM	*r;
{
	register DOOR	*d;
	
	/* put a magic lock on the door into the trove.  make sure that if
	   it is the trove on the sapphire level that the lock is golden */
	for (d = doors; d < &doors[numdoors]; d++)
		if (d->dr_r == r)
			d->dr_data |= (sapp_lev() ? 1 << 2 : (1 << rnd(3)));

	tr_room(r);
	r->r_data |= R_TROVE;
}
