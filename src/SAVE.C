/*
 *  		MAG - A Dungeon Adventuring Game
 *
 *	Copyright (C) 1986,87,88 by Michael J. Teixeira
 *
 *  General permission to copy or modify, but not for profit, is hereby
 *  granted, provided that the above copyright notice is included.
 *
 */

#include	<process.h>
#include	<stdio.h>
#include	<io.h>
#include	<string.h>
#include	<fcntl.h>
#include	<sys\types.h>
#include	<sys\stat.h>
#include	<dos.h>
#include	"mag.h"

int	openfile;

/*
 * SAVE_LEV: save all the info about the current dungeon level to a file
 */

int
save_lev()
{
	char	path[50];

	sprintf(path, "%s\\maglevel.%d", u.u_savedir, u.u_dlevel);

	if ((openfile = open(path, (O_CREAT | O_TRUNC | O_WRONLY | O_BINARY),
			(S_IREAD | S_IWRITE))) == -1) {
		pline("Can't save level <%s>.", path);

		return NO;
	}

	dowrite(&u.u_d, sizeof (u.u_d));
	dowrite(dun, sizeof (dun));
	dowrite(&numdoors, sizeof (numdoors));
	dowrite(doors, sizeof (DOOR) * numdoors);
	dowrite(&numlobjs, sizeof (numlobjs));
	dowrite(lobjs, sizeof (LEVOBJ) * numlobjs);
	dowrite(&nummons, sizeof (nummons));
	dowrite(mons, sizeof (MONSTER) * nummons);
	dowrite(&numrooms, sizeof (numrooms));
	dowrite(rooms, sizeof (ROOM) * numrooms);
	dowrite(&numtraps, sizeof (numtraps));
	dowrite(traps, sizeof (TRAP) * numtraps);

	close(openfile);

	return YES;
}

/*
 * DOWRITE: performs a WRITE and checks for errors
 */

void
dowrite(ptr, size)
void			*ptr;
register unsigned	size;
{
	register unsigned	did;

	if ((did = write(openfile, ptr, size)) < size)
		pline("Error writing %u (%u)!", did, size);
}

/*
 * DOREAD: performs a READ and checks for errors
 */

void
doread(ptr, size)
void			*ptr;
register unsigned	size;
{
	register unsigned	did;

	if ((did = read(openfile, ptr, size)) < size)
		pline("Error reading %u (%u)!", did, size);
}

/*
 * RESTORE_LEV: reads in all info about a dungeon level from a file
 */

int
restore_lev(lev)
{
	char		path[50];
	MONSTER		*m;

	sprintf(path, "%s\\maglevel.%d", u.u_savedir, lev);

	if ((openfile = open(path, (O_RDONLY | O_BINARY))) == -1)
		return NO;

	doread(&u.u_d, sizeof (u.u_d));
	doread(dun, sizeof (dun));
	doread(&numdoors, sizeof (numdoors));
	doread(doors, sizeof (DOOR) * numdoors);
	doread(&numlobjs, sizeof (numlobjs));
	doread(lobjs, sizeof (LEVOBJ) * numlobjs);
	doread(&nummons, sizeof (nummons));
	doread(mons, sizeof (MONSTER) * nummons);
	doread(&numrooms, sizeof (numrooms));
	doread(rooms, sizeof (ROOM) * numrooms);
	doread(&numtraps, sizeof (numtraps));
	doread(traps, sizeof (TRAP) * numtraps);

	close(openfile);

	/* get rid of any left-over monsters that have been genocided */
	for (m = mons; m < &mons[nummons]; m++) {
		if (strchr(genostr, m->m_perm->p_letter)) {
			m->m_hp = -1;
			ck_mhp(m, NO);
		}
	}


	return YES;
}

/*
 * REMEMBER: save the game to a file
 */

int
remember()
{
	char		path[50];
	int		len;
	register	i;

	/* to prevent any unforseeable problems */
	if (u.u_stuckmon) {
		pline("Sorry, not in the middle of this exciting battle.");
		return NO;
	}

	pline("@Do you want to remember this game? ");
	go_imaginary();

	if (getlower() != 'y') {
		erase();
		return NO;
	}

	pline("Remembering game...");
	save_lev();
	sprintf(path, "%s\\savefile.mag", u.u_savedir);

	if ((openfile = open(path, (O_CREAT | O_TRUNC | O_WRONLY | O_BINARY),
			(S_IREAD | S_IWRITE))) == -1) {
		pline("Sorry, Tex's helpers are on strike.");
		bell();
		return NO;
	}

	dowrite(&u, sizeof (struct you));
	dowrite(&numevents, sizeof (numevents));
	dowrite(events, sizeof (struct event) * numevents);
	dowrite(genostr, sizeof (genostr));
	dowrite(&oldexp, sizeof (oldexp));
	dowrite(&oldelev, sizeof (oldelev));
	dowrite(&wizard, sizeof (wizard));
	dowrite(ostats, sizeof (ostats));
	dowrite(nameptr, sizeof (nameptr));

	/* we've got to save all the names given to objs too */
	for (i = 0; i < NUMOBJ; i++) {
		if (ostats[i] == CALLED) {
			len = strlen(nameptr[i][0]) + 1;
			dowrite(&len, sizeof (len));
			dowrite(nameptr[i][0], len);
		}
	}

	dowrite(&numinv, sizeof (numinv));
	dowrite(inv, sizeof (OBJECT) * numinv);

	close(openfile);

	cl_scr();
	pline("Farewell, %s.", u.u_name);

	doset(2, 0);
	cursor(YES);
	reset_kb();	/* reset interrupts */
	exit(0);
}

/*
 * REST_GAME: restore a game from a file
 */

int
rest_game()
{
	char		path[50];
	register	i;
	int		len;

	sprintf(path, "%s\\savefile.mag", u.u_savedir);

	if ((openfile = open(path, (O_RDONLY|O_BINARY))) == -1)
		return NO;	/* no saved game on disk */

	cl_scr();
	pline("Recalling remembered game...");

	doread(&u, sizeof (struct you));
	doread(&numevents, sizeof (numevents));
	doread(events, sizeof (struct event) * numevents);
	doread(genostr, sizeof (genostr));
	doread(&oldexp, sizeof (oldexp));
	doread(&oldelev, sizeof (oldelev));
	doread(&wizard, sizeof (wizard));
	doread(ostats, sizeof (ostats));
	doread(nameptr, sizeof (nameptr));

	/* restore all the objs called names */
	for (i = 0; i < NUMOBJ; i++)
		if (ostats[i] == CALLED) {
			doread(&len, sizeof (len));
			nameptr[i][0] = tex_alloc(len);
			doread(nameptr[i][0], len);
		}

	doread(&numinv, sizeof (numinv));
	doread(inv, sizeof (OBJECT) * numinv);

	close(openfile);
	remove(path);

	return YES;
}

/*
 * CLR_SAVE: remove any level save files that can be found
 */

void
clr_save()
{
	struct find_t	ft;
	char		path[50];

	sprintf(path, "%s\\maglevel.*", u.u_savedir);

	/* keep zotting files until DOS can't find any more */
	if (!_dos_findfirst(path, _A_NORMAL, &ft))
		do {
			sprintf(path, "%s\\%s", u.u_savedir, ft.name);
			remove(path);
		} while (!_dos_findnext(&ft));
}
