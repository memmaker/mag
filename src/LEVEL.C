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

/*
 * MK_LEV: the proc that manages the creation of a dungeon level
 */

void
mk_lev()
{
	/* the sapphire level is always 3 predefined rooms */
	if (sapp_lev()) {
		numrooms = 3;

		rooms[0].r_uline = rooms[0].r_ucol = 3;
		rooms[0].r_lline = 19;
		rooms[0].r_lcol = 41;
		rooms[0].r_data = 0;
		rooms[1].r_uline = 5;
		rooms[1].r_lline = 17;
		rooms[1].r_data = rooms[2].r_data = (R_MAGICLIT | R_TORCHED);
		rooms[1].r_ucol = 44;
		rooms[1].r_lcol = 65;
		rooms[2].r_uline = 7;
		rooms[2].r_lline = 15;
		rooms[2].r_ucol = 68;
		rooms[2].r_lcol = 75;

		mk_room(rooms);
		mk_room(&rooms[1]);
		mk_room(&rooms[2]);

		/* create the marble wall on either side of the sapphire */
		dun[10][64].d_what = dun[12][64].d_what = MARBLE;
		dun[10][64].d_data = dun[12][64].d_data = (D_WALL | D_STONE);

		/* mk the 2 passages to connect the 3 rooms */
		dig(rooms, &rooms[1], NO);
		dig(&rooms[1], &rooms[2], YES);

		mk_mon(room_loc(&rooms[1]), 32); /* imperial dragons */
		mk_mon(room_loc(&rooms[1]), 32);

		/* now create a drag ON the sapphire which won't move until
		   bothered */
		mk_mon(&dun[11][64], 32);
		mons[2].m_data |= M_PARALYZED;

		r_to_maze(rooms);	/* first room is a big maze */
		fix_maze();
		mk_trove(&rooms[2]);	/* third room is a nice trove */

		/* plop an up staircase in the corner of the 1st room */
		dun[4][4].d_what = UPSTAIR;
		dun[4][4].d_data |= D_STAIRCASE;
		what_room(&dun[4][4])->r_data |= R_STAIRCASE;
	} else {
		fit_rooms();
		corridor();
		spec_room();
		fix_maze();
		mk_stair();
		mk_torch();
	}

	mk_traps();
	mk_levobj();
}

/*
 * FIX_MAZE: make sure no doors are blocked with marble walls from mazes
 */

void
fix_maze()
{
	register DOOR		*dr;
	register DUNGEON	*d;
	register		rs;

	/* go thru the doors array */
	for (dr = doors; dr < &doors[numdoors]; dr++) {
		/* get the direction of the wall that the door is on */
		if ((rs = room_side(scrline(dr->dr_d), scrcol(dr->dr_d),
							dr->dr_r)) != -1) {
			/* move in the opposite dir 1 space (which is one
			   move into the room */
			d = mvindir(rs, dr->dr_d, -1);

			/* fix it if it's blocked! */
			if (d->d_data & D_WALL) {
				d->d_what = FLOOR;
				d->d_data &= ~(D_WALL | D_STONE);
			}
		}
	}
}

/*
 * MK_TORCH: add torches to the walls of some rooms
 */

void
mk_torch()
{
	register ROOM		*r;
	register DUNGEON	*d;

	for (r = rooms; r < &rooms[numrooms]; r++) {
		d = wall_loc(rnd(4) * 2, r, YES);

		/* after picking a location on the room wall, make sure that
		   the room is not already lit and is not a maze */
		if (((r->r_data & R_TORCHED) || !rnd(3)) && !(r->r_data &
				R_MAZE) && d)
			d->d_what = TORCHC[room_side(scrline(d), scrcol(d),
								r) / 2];
	}
}

/*
 * CL_DUN: clear the dungeon structure to all solid rock
 */

void
cl_dun()
{
	register DUNGEON	*d;

	/* this loop is trickier than it seems...MODIFY AT YOUR OWN RISK! */
	for (d = &dun[0][0]; d < &dun[LINES-1][COLUMNS]; d++) {
		d->d_what = ROCK;
		d->d_data = D_STONE;
	}
}

/*
 * MK_STAIR: find some good locations for stairs and put them there
 */

void
mk_stair()
{
	register DUNGEON	*d;
	register ROOM		*r;
	register		i;

	/* find a location on a floor that's not in a treasure trove */
	do if (!(r = what_room(d = location(SFLOOR))))
		pline("mk_stair: fatal error! location not in room");
	while (r->r_data & R_TROVE);

	r->r_data |= R_STAIRCASE;

	d->d_what = UPSTAIR;
	d->d_data |= D_STAIRCASE;

	/* now do the same thing for 1 to 3 down staircases */
	for (i = 0; i < (1 + !rnd(3) + !rnd(20)); i++) {
		do if (!(r = what_room(d = location(SFLOOR))))
			pline("mk_stair: fatal error! location not in room");
		while (r->r_data & R_TROVE);

		r->r_data |= R_STAIRCASE;

		d->d_what = DNSTAIR;
		d->d_data |= D_STAIRCASE;
	}
}

/*
 * LOCATION: find a location on the dungeon level where it looks like one of
 * the characters in STR.  ex) if STR=".~", it will find a random place on
 * the dungeon that is floor or water (or puke!)
 */

DUNGEON	*
location(str)
register char	*str;
{
	register DUNGEON	*d;
	register		count = 1000;
	
	/* keep looking until we find a good place where there isn't a mon.
	   if count runs out, then give up */
	do d = &dun[rnd(LINES)][rnd(COLUMNS)];
	while (!(strchr(str, d->d_what) || (d->d_data & D_MONSTER)) &&
								count--);

	/* prevent munging by returning an existing location */
	if (count < 1) {
		pline("location: can't find <%s>", str);
		return &dun[3][3];
	}

	return d;
}

/*
 * FIND_LOC: similar to LOCATION, but this looks for one char and looks in
 * order for it (not random)
 */

DUNGEON	*
find_loc(c)
unsigned char	c;
{
	register DUNGEON	*d;

	for (d = &dun[0][0]; d < &dun[LINES-1][COLUMNS]; d++)
		if (d->d_what == c)
			return d;

	pline("find_loc: can't find '%c'", c);

	return location(SFLOOR);
}

/*
 * CORRIDOR: dig corridors interconnecting ALL of the rooms.  this algorithm
 * assures that there is a passage from any one room to any other.  each
 * room is represented by a single bit in a 32-bit longword.  at the start,
 * each room is consider a one-room group of rooms and each time two rooms
 * are connected, their bits are ORed together and they become a bigger
 * group of rooms.  this continues until a group exists with all of the room
 * bits set which means all the rooms are now one big group
 */

void
corridor()
{
	register		i,
				bit;
	ROOM			*r;
	int			numgroups = 0;
	long			groups[MAXROOMS + 1],
				*lp1,
				*lp2,
				*gmax;

	/* create a group for all the rooms with a different bit set in each
	   one */
	for (lp1 = groups, i = 0; i < numrooms; lp1++, i++, numgroups++)
		*lp1 = 1L << i;

	*lp1 = 0;	/* set the trailing group to 0 */

	gmax = &groups[numrooms]; /* set a ptr to the end of the groups */

	/* we'll loop until we have one big group */
	for (;;) {
		/* pick two different random groups */
		do {
			lp1 = &groups[rnd(numgroups)];
			lp2 = &groups[rnd(numgroups)];
		} while (lp1 == lp2);

		/* pick a random room from each of the two groups and try
		   to dig a tunnel interconnecting them */
		r = dig(&rooms[pickbit(*lp1)], &rooms[pickbit(*lp2)], NO);

		/* since we don't always hit the room we wanted, get the bit
		   of the one we did hit */
		bit = r - rooms;

		/* now find out which group contains that bit (room) */
		for (lp2 = groups; lp2 < gmax; lp2++)
			if ((*lp2) & (1 << bit))
				break;

		/* if it's not in the group that we started from, join the
		   groups into one */
		if (lp1 != lp2) {
			/* combine the ending group bits into the starting
			   group */
			*lp1 |= *lp2;

			/* replace the old ending group with the last group
			   in the list */
			*lp2 = *(--gmax);

			/* zero the trailing group */
			*gmax = 0;

			/* if the number of groups is now one then we're
			   done */
			if (--numgroups == 1)
				break;
		}
	}

	/* now we make some dead-end corridors to irritate the player */
	do dig(&rooms[rnd(numrooms)], (DUNGEON *)0, NO);
	while (rnd(3));
}

/*
 * CINBOUNDS: returns whether the location is a legal corridor location
 */

int
cinbounds(line, col)
register	line,
		col;
{
	return (line > 1 && line < 22 && col > 0 && col < 77);
}

/*
 * PICKBIT: keep picking a random room bit until it is in the longword given
 */

int
pickbit(l)
long	l;
{
	register	i;

	do i = rnd(MAXROOMS);
	while (!(l & (1L << i)));

	return i;
}

/*
 * DIG: this proc attempts to dig a tunnel from the start room to the dest
 * room
 */

ROOM	*
dig(sr, dr, flag)
ROOM	*sr,
	*dr;
{
	DUNGEON			*dd,
				*od;
	register DUNGEON	*d;
	register		dir,
				len;

	/* if a dest room was specified, set the dest location to the middle
	   of the room */
	if (dr)
		dd = &dun[dr->r_uline + 3][dr->r_ucol + 3];
	/* if no dest room given, pick a random location (dead-end?) */
	else {
		do dd = &dun[rnd(LINES)][rnd(COLUMNS)];
		while (!cinbounds(scrline(dd), scrcol(dd)) || what_room(dd)
								== sr);
	}

	/* flag means that it is a special sapphire level corridor */
	if (flag) {
		d = &dun[11][65];
		dir = RIGHT;
	} else {
		/* find a location for the door from the start room */
		dir = get_cdir(-1, sr->r_uline + 3, sr->r_ucol + 3,
						scrline(dd), scrcol(dd));
		if (!(d = wall_loc(dir, sr, NO)))
			pline("Can't find location for door!");
	}

	/* if flag then force the door to be secret */
	mk_door(d, (flag ? YES : !rnd(9)));
	od = d = mvindir(dir, d, 1);

	for (;;) {
		if (!(dir % 4))
			len = abs(scrline(d) - scrline(dd)) + rnd(3);
		else len = abs(scrcol(d) - scrcol(dd)) + rnd(3);

		while (len--) {
			/* does this dead-end end? */
			if (!dr && !rnd(5))
				return (ROOM *)0;

			d->d_what = CORRIDOR;
			d->d_data &= ~D_STONE;
			od = d;
			d = mvindir(dir, d, 1);

			/* if we're heading out of bounds, change dir */
			if (!cinbounds(scrline(d), scrcol(d)))
				break;

			/* hit the door of a room? */
			if (d->d_data & D_DOOR)
				return what_room(d);

			/* a-ha we hit a room */
			if (d->d_data & D_WALL) {
				/* if hit corner, try again */
				if (strchr(CORNERS, d->d_what))
					break;

				mk_door(d, !rnd(9));

				return what_room(d);
			}
		}

		d = od;
		dir = get_cdir(dir, scrline(d), scrcol(d), scrline(dd),
								scrcol(dd));
	}
}

/*
 * GET_CDIR: pick a new direction to dig in
 */

int
get_cdir(curdir, sline, scol, dline, dcol)
register	curdir,
		sline,
		scol;
{
	int	dir[2];
	
	dir[0] = (sline <= dline) ? DOWN : UP;
	dir[1] = (scol <= dcol) ? RIGHT : LEFT;

	if (dir[0] == oppdir(curdir))
		return dir[1];

	if (dir[1] == oppdir(curdir))
		return dir[0];

	return dir[rnd(2)];
}

/*
 * R_TO_MAZE: the proc manages the construction of a maze room
 */

void
r_to_maze(r)
register ROOM	*r;
{
	register DUNGEON	*d;

	wl_room(r);	/* fill the room with walls */

	r->r_data |= R_MAZE;
	r->r_data &= ~R_TORCHED;

	/* start in the upper left corner */
	d = &dun[r->r_uline + 1][r->r_ucol + 1];
	d->d_what = FLOOR;
	d->d_data &= ~(D_WALL | D_STONE);

	make_maze(r, d);
}

/*
 * MAKE_MAZE: recursive procedure to create a maze in a room
 */

void
make_maze(r, sd)
register ROOM	*r;
register DUNGEON *sd;
{
	register	dir;
	int		count = 0;
	DUNGEON		*d,
			*od;

	/* pick a random non-diag dir and loop clockwise thru other 3 */
	for (dir = rnd(4) * 2; count < 4; dir = (dir + 2) % 8, count++) {
		d = od = sd;
		d = mvindir(dir, d, 2);

		/* if not an ok new location loop */
		if (what_room(d) != r || d->d_what == FLOOR)
			continue;

		/* dig 2 spaces in new location and then recursify */
		d->d_what = FLOOR;
		d->d_data &= ~(D_WALL | D_STONE);

		od = mvindir(dir, od, 1);
		od->d_what = FLOOR;
		od->d_data &= ~(D_WALL | D_STONE);

		make_maze(r, d);
	}
}
