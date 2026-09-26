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
#include	<ctype.h>
#include	<string.h>
#include	"mag.h"

/*
 * DISPATCH: this is the main MAG loop for all activity
 */

void
dispatch()
{
	register	update = YES;
	int		(*proc)(void);

    for (;;) {

	if (!(u.u_data & UD_SLEEPING)) {
		if (!ntimes || isdigit(typed)) { /* need to get input? */
			proceed = NO;
			oprint = YES;

			if (eflg > 2) {	/* erase the top line after 3 mvs */
				erase();
				eflg = 0;

			}

			if (update)
				pr_mon();

			redraw();

			if (blineflg)
				pr_bline();

#ifdef PORT
			port_idle();	/* autosave, RVIP additions */
			if ((typed = port_auto()) != 0)
				;	/* explore / walk to stairs: next step */
			else {
#endif
			/* the cursor appear when player is polymorphed */
			if (u.u_sym != PLAYER && u.u_sym != IPLAYER) {
				doset(scrline(u.u_d), scrcol(u.u_d));
				typed = ctgetch();
			} else typed = tgetch();
#ifdef PORT
			if (!(u.u_data & UD_FIND) &&
					(typed = port_command(typed)) == 0)
				continue;	/* the port handled it */
			}
#endif

			if ((u.u_data & UD_FIND) && typed != '\033') {
				/* make sure a legal 'f' direction */
				if ((u.u_dir = todir(typed)) == -1) {
					pline("@I don't know that direction.");
					u.u_data &= ~UD_FIND;
				} else ntimes = 80;

				continue;
			}
		}

		/* keep doing same thing while NTIMES */
		if (!isdigit(typed) && ntimes)
			ntimes--;

		if (u.u_dir != -1)
			update = move();
		else {
			proc = cmd[typed];
			update = (*proc)();
		}
	} else update = YES;

	/* only some of the commands use time, so check before moving mons */
	if (update) {
		u.u_moves++;
		move_mon();
		updevent();
	}

	if (!ntimes)	/* if not looping, prep to erase top line */
		eflg++;

    }
}

/*
 * MULTI: handle player entering digits for multi-commands
 */

int
multi()
{
	ntimes = ntimes * 10 + (typed - '0');

	if (ntimes > 999)
		ntimes = 999;

	if (u.u_options & O_SPECMESS) {
		pline("@Multi-command <%d>", ntimes);
		eflg = 2;	/* force erasure of top line */
	}

	return NO;
}

/*
 * ILLEGAL: print a message about using an illegal command
 */

int
illegal()
{
	ntimes = 0;	/* in case they typed 999 <illegal command> */
	pline("@Press F1 for a list of commands.");

	return NO;
}

/*
 * MOVE: handles moving the player around the dungeon
 */

int
move()
{
	register DUNGEON *d = u.u_d;

	if (u.u_data & (UD_STUCK | UD_STRANGLE)) {
		pline("You're stuck!");
		stop();

		return YES;
	}

	if (confused() && rnd(4))
		u.u_dir = rnd(8);	/* random direction when confused */

	red_box(scrline(u.u_d)-1, scrcol(u.u_d)-1, scrline(u.u_d)+1, 
							scrcol(u.u_d)+1);
	u.u_d = mvindir(u.u_dir, u.u_d, 1);

	/* make sure the player can move to the new location */
	if ((u.u_d->d_data&D_STONE) || (u.u_dir%2 &&nodiagmove(u.u_dir,d))) {
		u.u_d = d;
		stop();

		return NO;
	}

	/* if player stuck to monster then can only move to attack mon */
	if (u.u_stuckmon && (!(u.u_d->d_data & D_MONSTER) || what_mon(u.u_d)
			!= u.u_stuckmon)) {
		pline("You can't escape the %s!", mname(u.u_stuckmon));
		bell();
		u.u_d = d;
		stop();

		return YES;
	}

	if (u.u_d->d_data & D_MONSTER) {	/* attacking a monster! */
		MONSTER	*m = what_mon(u.u_d);

		u.u_d = d;
		stop();
		redraw();
		uattack(m);

		return YES;
	}

	/* clean up torchlight around player from last location */
	if (u.u_r && !(u.u_r->r_data & R_TORCHED) && !blind())
		know(d, NO);

	if (u.u_d->d_data & D_DOOR) {
		DOOR	*dr = what_door(u.u_d);
	
		if (enterdoor(dr)) {
			if (u.u_r != dr->dr_r) { /* entering room */
				if ((dr->dr_r->r_data & R_MAGICLIT) &&
						!blind() && (u.u_options &
						O_SPECMESS))
					pline("This room is enshrouded in a bluish aura.");

				u.u_r = dr->dr_r;
				readyroom(u.u_r);
			}
		} else {		/* can't open door */
			u.u_d = d;

			if (!blind())	/* redo torchlight */
				know(u.u_d, YES);

			stop();

			return YES;
		}
	/* if leaving a room, make sure it gets redrawn to clean it up */
	} else if ((d->d_data & D_DOOR) && !(u.u_r = what_room(u.u_d))) {
		ROOM	*r = what_room(d);

		red_box(r->r_uline, r->r_ucol, r->r_lline, r->r_lcol);
	}

	if (!blind())			/* put torchlight */
		know(u.u_d, YES);
	else d->d_data |= D_SEEN;	/* when blind, put one location */

	if ((u.u_d->d_data & (D_STAIRCASE | D_DOOR | D_OBJECT)) ||
			shouldstop() || !ntimes)
		stop();

	if (u.u_d->d_data & D_OBJECT)
		pickup(u.u_d);

	/* traps don't get sprung when levitating */
	if ((u.u_d->d_data & D_TRAPPED) && !(u.u_data & UD_LEVITATE)) {
		u.u_d->d_data |= D_SEEN;
		redraw();
		stop();
		dotrap(u.u_d, (MONSTER *)0);
	}

	if ((u.u_data & UD_FIND) && (u.u_options & O_FOLLOW))
		dofollow();

	return YES;
}

/*
 * STOP: discontinue and multiple move commands
 */

void
stop()
{
	u.u_dir = -1;
	ntimes = 0;
	u.u_data &= ~(UD_FORCE | UD_FIND);
	red_box(scrline(u.u_d)-1, scrcol(u.u_d)-1, scrline(u.u_d)+1, 
							scrcol(u.u_d)+1);
	pr_mon();
}

/*
 * WALK: general procedure to handle moving one space in any direction
 */

int
walk()
{
	ntimes++;
	u.u_data |= UD_FORCE;
	u.u_dir = todir(typed);

	return NO;
}

/*
 * TODIR: convert and type of direction entry into internal dir values
 */

int
todir(c)
register	c;
{
	switch (c) {
	case 'k':
	case 'K':
	case 200:	/* up arrow */
		return UP;

	case 'u':
	case 'U':
	case 201:	/* pgup */
		return UPRT;

	case 'l':
	case 'L':
	case 205:	/* rt arrow */
		return RIGHT;

	case 'n':
	case 'N':
	case 209:	/* pgdn */
		return DNRT;

	case 'j':
	case 'J':
	case 208:	/* dn arrow */
		return DOWN;

	case 'b':
	case 'B':
	case 207:	/* end */
		return DNLT;

	case 'h':
	case 'H':
	case 203:	/* lt arrow */
		return LEFT;

	case 'y':
	case 'Y':
	case 199:	/* home */
		return UPLT;

	default:
		return -1;
	}
}

/*
 * RUN: general proc to handle moving as far as possible in any direction
 */

int
run()
{
	ntimes = 80;
	u.u_data |= UD_FORCE;
	u.u_dir = todir(typed);

	return NO;
}

/*
 * ABRT: will abort any multi-command or follow ('f') prefix
 */

int
abrt()
{
	if (ntimes || (u.u_data & UD_FIND)) {
		pline("@Abort.");
		u.u_data &= ~UD_FIND;
		u.u_dir = -1;
	}

	ntimes = 0;

	return NO;
}

/*
 * FIND: set the find move flag before getting a direction
 */

int
find()
{
	u.u_data |= UD_FIND;
	return NO;
}

/*
 * GETDIR: ask for a direction either silently or with a prompt
 */

int
getdir(mode)
register	mode;
{
	register	c;
	
	if (mode) {
		pline("@In what direction? ");
		go_imaginary();
		c = ctgetch();
	} else c = tgetch();

	if (c == '\033')
		return -2;	/* -2 because todir return -1 for bad dir */

	return todir(c);
}

/*
 * DOFOLLOW: handle the player following the turns of a corridor
 */

int
dofollow()
{
	register		dir,
				foll = 0;
	register DUNGEON	*d;
	int			newdir;

	if (u.u_r)		/* not active in a room */
		return NO;

	/* loop thru all of the non-diag directions */
	for (dir = UP; dir <= UPLT; dir += 2) {
		/* don't look in opposite dir that player in moving in */
		if (dir == oppdir(u.u_dir))
			continue;

		d = mvindir(dir, u.u_d, 1);

		/* add up possible directions to move in */
		if (d->d_what == CORRIDOR) {
			foll++;
			newdir = dir;
		} else if (d->d_what == DOORC) { /* force a stop at a door */
			foll += 2;
			break;
		}
	}

	if (foll == 1) {	/* if only one dir, follow it */
		u.u_dir = newdir;
		ntimes = 80;
	} else stop();

	return YES;
}

/*
 * NODIAGMOVE: figure out if given move would be illegal because of corner
 */

int
nodiagmove(dir, d)
register		dir;
register DUNGEON	*d;
{
	return (mvindir((dir + 7) % 8, d, 1)->d_data & (D_DOOR | D_STONE) ||
		mvindir((dir + 1) % 8, d, 1)->d_data & (D_DOOR | D_STONE));
}

 int	motdirs[8] = {   -COLUMNS, -(COLUMNS - 1),  1,	(COLUMNS + 1),
			  COLUMNS,  (COLUMNS - 1), -1, -(COLUMNS + 1)	 };

/*
 * MVINDOR: move a dungeon ptr AMOUNT times in DIR direction
 */

DUNGEON	*
mvindir(dir, d, amount)
register		dir;
register DUNGEON	*d;
register		amount;
{
	return (d + (amount * motdirs[dir]));
}

/*
 * SHOULDSTOP: decide if the player should stop his multi-move command
 */

int
shouldstop()
{
	register DUNGEON	*d;
	register		dir;

	if (blind())	/* when blind, keep moving until you hit something */
		return NO;

	/* loop from 2 dirs counter-clockwise thru 2 dirs CW from current
	   player direction */
	for (dir = (u.u_dir+6)%8; dir != (u.u_dir+3) % 8; dir = (dir+1) % 8) {
		d = mvindir(dir, u.u_d, 1);

		/* simplified: decide if player should stop */
		if ((d->d_data & D_MONSTER) || ((u.u_data & UD_FIND) &&	((d->
				d_data & (D_OBJECT | D_STAIRCASE)) || d->
				d_what == TRAPC)) || ((d->d_data & (D_DOOR |
				D_WALL)) == D_DOOR && ((dir == (u.u_dir + 6)
				% 8 || dir == (u.u_dir + 2) % 8) || (u.u_data
				& UD_FIND && dir == u.u_dir))))
			return YES;
	}

	return NO;
}
