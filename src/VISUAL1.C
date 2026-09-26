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
#include	<stdarg.h>
#include	<string.h>
#include	<dos.h>
#include	<graph.h>
#include	"mag.h"

/*
 * DI_SET: set the imaginary cursor using a dungeon location
 */

void
di_set(d)
register DUNGEON *d;
{
	i_line[active_page] = scrline(d);
	i_col[active_page] = scrcol(d);
}

/*
 * SWRITE: write a string to the active video page, but only print the chars
 * that aren't already there
 */

void
swrite(str)
register char	*str;
{
	union REGS	r,
			or;
	register char   *cp;
	int		*line = &i_line[active_page],
			*col = &i_col[active_page],
			*cline = &curline[active_page],
			*ccol = &curcol[active_page];

	/* use BIOS video service 9: write char and attribute */
	r.h.ah = 9;
	r.x.bx = (active_page << 8) + out_attrib;
	r.x.cx = 1;

	for (cp = screen+(*line * 80)+*col; *str; str++, (*col)++, cp++) {
		if (*cp == *str) /* if char already in place, ignore */
			continue;

		/* get the cursor in the right place */
		if (*cline != *line || *ccol != *col)
			doset(*line, *col);

		r.h.al = *str;
		int86(16, &r, &or);

		*cp = *str;
	}
}

/*
 * DOSET: set the active page cursor to the given line and col
 */

void
doset(line, col)
register        line,
		col;
{
	union REGS      r;

	/* BIOS video service 2: set cursor position */
	r.h.ah = 2;
	r.h.bh = (char)active_page;
	r.x.dx = (line << 8) + col;
	int86(16, &r, &r);

	curline[active_page] = line;
	curcol[active_page] = col;
}

/*
 * CL_SCR: clear the screen and the MAG internal screen array
 */

void
cl_scr()
{
	union REGS	r;

	memset(screen, (int)' ', (size_t)(LINES * COLUMNS));

	/* BIOS video service 6: scroll window up */
	r.x.ax = (6 << 8);
	r.h.bh = 7;
	r.x.cx = 0;
	r.x.dx = (24 << 8) + 79;
	int86(16, &r, &r);

	doset(0, 0);
	i_set(0, 0);
	erase();
}

/*
 * AT: put a char at the given location
 */

void
at(line, col, c)
register	line,
		col;
char	c;
{
	i_set(line, col);
	oputc(c);
}

/*
 * DAT: put a char at the given dungeon location
 */

void
dat(d, c)
register DUNGEON	*d;
unsigned char		c;
{
	i_set(scrline(d), scrcol(d));
	oputc(c);
}

/*
 * OPUTC: actually put a char at the current imaginary cursor
 */

void
oputc(c)
unsigned char	c;
{
	union REGS	r;

	/* check and see if the char is already at the given location */
	if (*(screen + i_line[active_page] * 80 + i_col[active_page]) == c)
		return;

	go_imaginary();

	/* BIOS video service 9: write char and attribute */
	r.x.ax = (9 << 8) + c;
	r.x.bx = (active_page << 8) + out_attrib;
	r.x.cx = 1;
	int86(16, &r, &r);

	/* set the MAG internal screen to the new char */
	*(screen + i_line[active_page] * 80 + i_col[active_page]) = c;
}

/*
 * CL_EOL: clears from the current imaginary cursor to the end of the line
 */

void
cl_eol()
{
	char            dud[85];
	register        line = i_line[active_page],
			col = i_col[active_page];

	/* create a string of spaces to print */
	memset((void *)dud, (int)' ', (size_t)(COLUMNS - col));
	dud[COLUMNS - col] = '\0';
	swrite(dud);
}

/*
 * PLINE: format a string and print it on the top line of the screen
 */

void
pline(str, ...)
char	*str;
{
	register char	*cp;
	char		buf[140];
	va_list		args;

	va_start(args, str);		/* format the variable # of args */
	vsprintf(buf, str, args);
	va_end(args);

	cp = str = buf;
	buf[70] = 0;	/* truncate anything past the 70th char */

	if (*cp == '@')	/* special char which means overprint */
		cp++;

	eflg = 0;

	if (!strcmp(cp, message)) /* if same as last message, ignore */
		return;

	if (message[0]) {
		/* if overprint flag is not set, print the more message */
		if (!oprint)
			more(0, strlen(message), 1);

		/* store away the message in recall buffer */
		strcpy(lastmess[npline], message);
		npline = (npline + 1) % NUMPLINE;
	}

	if (u.u_data & (UD_HUNGRY | UD_FAINTING))
		sprintf(message, "%c %s", u.u_data & UD_HUNGRY ? '*':'!',cp);
	else strcpy(message, cp);

	i_set(0, 0);
	color(WHITE);
	swrite(message);
	cl_eol();
	i_set(0, strlen(message));
	oprint = (*str == '@');
}

/*
 * MORE: print one of two MORE messages and wait for pressed key
 */

void
more(line, col, how)
register	line,
		col,
		how;
{
	if (proceed)	/* if proceed flag set, don't wait for MORE */
		return;

	i_set(line, col);
	color(WHITE | INTENSE);
	swrite(how ? " =-More-= " : " =-Press space to continue-= ");

	/* if the more message is in the middle of the screen, make sure that
	   it gets cleaned up with the next redisplay */
	if (line && !active_page)
		set_red(line, col, col + 35);

	domore(how);

	/* if it was on the top line, erase it manually */
	if (!line && how) {
		i_set(line, col + 1);
		color(WHITE);
		swrite("        ");
	}
}

/*
 * DOMORE: waits for the player to type one of the special MORE keys
 */

void
domore(allow_p)
register	allow_p;
{
	register	c;

	for (;;) {
		c = tgetch();

		switch (c) {
		case ' ':
		case '\015':	/* ^M */
			return;

		case '\033':	/* ESC */
			ntimes = 0;		/* abort multi commands */
			return;

		case '\003':	/* ^C */
			erase();
			doquit();
			return;

		case 'p':
		case 'P':
			if (allow_p)
				proceed = YES;

			return;
		}
	}
}

/*
 * REPRINT: recalls the previous top line messages from the buffer
 */

int
reprint()
{
	/* loop indefinately through the list */
	npline = (npline + NUMPLINE - 1) % NUMPLINE;

	i_set(0, 0);
	color(WHITE);

	/* if there was a hungry symbol, don't print it */
	if (lastmess[npline][0] == '*' || lastmess[npline][0] == '!')
		swrite(&lastmess[npline][2]);
	else swrite(lastmess[npline]);

	cl_eol();
	eflg = 0;

	/* set the special flag which tells ERASE not to save this in the
	   message buffer */
	message[84] = '!';

	return NO;
}

void
erase()
{			
	if (visual_page || active_page)
		return;

	/* decide if we want to save the message in the old message buffer */
	if (message[0] || message[84] == '!') {
		i_set(0, 0);
		cl_eol();

		if (message[84] != '!') {	/* store the message */
			strcpy(lastmess[npline], message);
			npline = (npline + 1) % NUMPLINE;
		} else message[84] = '\0';

		message[0] = '\0';
	}
}

/*
 * PR_BLINE: print all or portions of the bottom status line
 */

void
pr_bline()
{
	char	str[90];

	color(WHITE);

	if (blineflg & REDRAW_ALL) {
		sprintf(str, "Level: %-4dHp: %-3d{%d}  Str: %-3d{%d}  Ac:%3d  ELevel:%3d  Scr:",
			u.u_dlevel, u.u_hp, u.u_maxhp, u.u_str, u.u_maxstr,
						10 - u.u_ac, u.u_elevel);
		i_set(23, 0);
		swrite(str);

		sprintf(str, "%-11s", commastr(getscore(NO)));
		i_set(23, 64);
		swrite(str);
	} else {
		if (blineflg & REDRAW_HP) {
			sprintf(str, "%-3d", u.u_hp);
			i_set(23, 15);
			swrite(str);
		}

		if (blineflg & REDRAW_SCR) {
			sprintf(str, "%-11s", commastr(getscore(NO)));
			i_set(23, 64);
			swrite(str);
		}
	}

	blineflg = 0;
}

/*
 * SET_ACT: set the active page variables
 */

void
set_act(page)
register	page;
{
	active_page = page;
	screen = &optscr[page][0][0];
}

/*
 * SET_VIS: set the visual page variables
 */

void
set_vis(page)
register	page;
{
	visual_page = page;
	_setvisualpage(page);
}

