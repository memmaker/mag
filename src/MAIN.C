/*
 *  		MAG - A Dungeon Adventuring Game
 *
 *	Copyright (C) 1986,87,88 by Michael J. Teixeira
 *
 *  General permission to copy or modify, but not for profit, is hereby
 *  granted, provided that the above copyright notice is included.
 *
 */

#include        <stdlib.h>
#include        <string.h>
#include	<stdio.h>
#include	<dos.h>
#include	<process.h>
#include	<ctype.h>
#include	<malloc.h>
#include	<time.h>
#include	<conio.h>
#include	<io.h>
#include	"mag.h"
#include	"vars.h"
#include	"command.h"
#include	"object.h"
#include	"monster.h"

/* interrupt handlers for preventing Ctrl-Breaking */
extern void interrupt far newkb(void);
extern void interrupt far newdb(void);

/* ptrs to old interrupt handlers */
void (interrupt far *oldkb)(void);
void (interrupt far *olddb)(void);

void
main(argc, argv)
char	*argv[];
{
	long		t;
	register char	*cp;

	if (access("help\\help.1", 0) || access("pics\\header", 0)) {
		puts("MAG doesn't seem to be setup properly.");
		puts("Have you set up the HELP, PICS, and SAVE dirs?");
		exit(1);
	}

	time(&t);
	srand((unsigned)t);

	/* store the old keyboard and dos break interrupts and replace them
	   with the new ones */
        oldkb = _dos_getvect(0x09);
	olddb = _dos_getvect(0x23);
	_dos_setvect(0x09, newkb);
	_dos_setvect(0x23, newdb);

	init();

	if (argc > 1) {
		/* undocumented way to start on any level */
		if (cp = strchr(argv[1], 'l'))
			u.u_lo_level = u.u_hi_level = u.u_dlevel =
							atoi(cp + 1) - 1;
		/* undocumented way to seed the random num generator */
		if (cp = strchr(argv[1], 's'))
			srand((unsigned)atoi(cp + 1));
	}

	get_game();
	dispatch();
}

/*
 * NEWKB: the keyboard interrupt handler
 */

void interrupt far
newkb()
{
	(*oldkb)();
	*((char far *)0x00400071) = 0;	/* clear the BREAK flag if set */
}

/*
 * NEWDB: the DOS break interrupt handler
 */

void interrupt far
newdb()
{
	/* this is a dummy procedure */
}

/*
 * RESET_KB: set the two interrupt vectors back to normal
 */

void
reset_kb()
{
	_dos_setvect(0x09, oldkb);
	_dos_setvect(0x23, olddb);
}

/*
 * CTGETCH: get a character with the cursor on
 */

int
ctgetch()
{
	register	c;

	cursor(YES);
	c = tgetch();
	cursor(NO);

	return c;
}

/*
 * TGETCH: return the next character typed (or next in a macro)
 */

int
tgetch()
{
	register	c;
	union REGS	r;

	/* if in macro, return the next char */
	if (u.u_options & O_DOMACRO) {
		if (!*macroptr)
			u.u_options &= ~O_DOMACRO;
		else return *macroptr++;
	}
	
	r.h.ah = 0;
	int86(0x16, &r, &r);	/* BIOS get next char service */

	if (!r.h.al)	/* if special char, offset by 128 */
		c = 128 + r.h.ah;
	else c = r.h.al;

	/* if recording a macro, store in the macro buffer */
	if (u.u_options & O_MAKEMACRO) {
		*macroptr++ = c;

		/* if macro full, close it up and beep */
		if (macroptr - macro == NUMMACRO-1) {
			u.u_options &= ~O_MAKEMACRO;
			bell();
		}
	}

	return c;
}

/*
 * DOQUIT: let a person prematurely quit the game
 */

int
doquit()
{
	pline("Do you really want to quit? ");
	go_imaginary();

	if (getlower() != 'y') {
		erase();
		ntimes = 0;
		return NO;
	}

	/* sorry, if used wizard mode at all means no hero */
	topten((wizard ? 0L : getscore(YES)), "quit");

	clr_save();

	cursor(YES);
	reset_kb();
	exit(0);
}

/*
 * GETLOWER: get an alpha char and return the upper case version of it
 */

int
getlower()
{
	register	c;

	for (;;) {
		if ((c = ctgetch()) == '\033')	/* pressed ESC? */
			return -1;

		if (isalpha(c))
			return tolower(c);
	}
}

/*
 * TEX_ALLOC: use MALLOC and make sure it does not fail
 */

void	*
tex_alloc(size)
register unsigned	size;
{
	register char	*ptr;

	if ((ptr = malloc(size)) == 0)
		pline("Memory?");

	return ptr;
}

/*
 * TEX_FREE: make sure ptr is not NULL and then use FREE
 */

void
tex_free(ptr)
register void	*ptr;
{
	if (!ptr)
		pline("Error!  Attempting to free a pointer to zero!");
	else free(ptr);
}

/*
 * COMMASTR: take a LONG and return the string version of the number with
 * thousand commas inserted
 */

char	*
commastr(l)
long	l;
{
	static char	str[25];
	char		ls[20];
	register char	*cp,
			*cp1 = str;

	/* if L is neg, put hyphen at beg of string and then negate L */
	if (l < 0) {
		*cp1++ = '-';
		l = -l;
	}

	sprintf(ls, "%ld", l);

	/* go thru LS string and insert comma in the final string every 3 */
	for (cp = ls; *cp; cp++, cp1++) {
		if (cp != ls && (strlen(ls) - (cp - ls)) % 3 == 0)
			*cp1++ = ',';

		*cp1 = *cp;
	}

	*cp1 = '\0';

	return str;
}

/*
 * PRFILE: read a file from disk and print it on the screen
 */

int
prfile(filename)
register char	*filename;
{
	FILE		*fp;
	char		line[100];
	register	count = 0;

	cl_scr();

	if ((fp = fopen(filename, "r")) == 0) {
		pline("Can't open <%s>!", filename);

		/* force a more so user can see message */
		more(i_line[active_page], i_col[active_page], YES);

		return NO;
	}

	while (fgets(line, 100, fp) && count < 22) {
		line[strlen(line) - 1] = 0;	/* get rid of '\n' */
		i_set(count++, 0);
		swrite(line);
	}

	fclose(fp);

	return YES;
}

/*
 * DOEXIT: generic procedure to call when exiting program
 */

void
doexit()
{
	emptybuf();
	i_set(22, 0);
	swrite("Press any key...");
	tgetch();

	topten((wizard ? 0L : getscore(YES)), killer);

	clr_save();

	cursor(YES);
	emptybuf();

	reset_kb();
	exit(0);
}

/*
 * STARTOBJ: set up the player's initial inventory
 */

void
startobj()
{
	add_invent(create_o(FOOD, 0, 0, 0, FOODRAT, 2 + !rnd(5)), NO);

	rightweapon = add_invent(create_o(WEAPON, 1, 1, (O_IDENT | O_INUSE),
			(rnd(2) ? FLAIL : (rnd(2) ? DAGGER : MACE)), 1), NO);

	secweap = add_invent(create_o(WEAPON, 1, 1, O_IDENT, LONGBOW, 1), NO);

	add_invent(create_o(WEAPON, 0, 0, O_IDENT, ARROW, 20 + rnd(10)), NO);

	add_invent(create_o(MISC, 0, 0, 0, TINDERBOX, 1), NO);

	shield = add_invent(create_o(SHIELD, 0, 0, (O_IDENT | O_INUSE),
							MEDIUM, 1), NO);

	armor = add_invent(create_o(ARMOR, 0, 0, (O_IDENT | O_INUSE), (rnd(3)
					? STUDLEATHER : SCALE), 1), NO);

	u.u_ac += what_ac(armor) + 1;
}

/*
 * SETUPGAME: print the title page when starting the game and get name
 * and set up a few initial vars
 */

void
setupgame()
{
	register char	*cp;

	if (!u.u_name[0]) {
		set_vis(1);
		set_act(0);

		color(YELLOW);
		prfile("pics\\header");

		color(CYAN | INTENSE);
		i_set(6, 36);
		swrite("MAG");

		color(RED | INTENSE);
		i_set(8, 25);
		swrite("A Dungeon Adventuring Game");

		color(RED | INTENSE);
		i_set(10, 27);
		swrite("By Michael J. Teixeira");

		color(BROWN);
		i_set(13, 8);
		swrite("What is your name, Brave One? ");

		color(CYAN | INTENSE);

		set_vis(0);

		cp = get_str(YES);
		cp[25] = 0;

		if (*cp == '-' || !*cp)  /* '-' means they pressed ESC */
			strcpy(u.u_name, "Bozo the Clown");
		else strcpy(u.u_name, cp);

		color(BROWN);
		i_set(15, 8);
		swrite("Hold on warrior while I prepare your destiny...");
	}

	clr_save();
	u.u_elevel = 1;

	/* there's a 5% chance that the player is of abnormal strength */
	if (rnd(20))
		u.u_str = u.u_maxstr = 16;
	else u.u_str = u.u_maxstr = 14 + rnd(5);

	/* there's a 5% chance that the player is of abnormal hit points */
	if (rnd(20))
		u.u_hp = u.u_maxhp = 12;
	else u.u_hp = u.u_maxhp = 10 + rnd(5);

	u.u_dir = -1;
	u.u_weight = u.u_score = u.u_moves = 0L;
	u.u_stuckmon = (MONSTER *)0;
	leftweapon = (OBJECT *)0;
	setupobj();
	startobj();

	mk_event(5, regen);		/* create the 2 perm events */
	mk_event(1000, gethungry);

	mk_event(50, mkwandmon);
	newlev(DOWN, UPSTAIR, NO);

	version();
}

/*
 * EMPTYBUF: clear the keyboard buffer
 */

void
emptybuf()
{
	while (kbhit())
		getch();
}

/*
 * INIT: do a bunch of game inits
 */

void
init()
{
	register	i;

	/* make sure that both video pages are clear */
	set_act(1);	/* side effect: inits SCREEN variable */
	set_vis(1);
	cl_scr();	/* side effect: inits image curs & cur curs */
	set_act(0);
	set_vis(0);
	cl_scr();

	/* initialize any you structure stuff */
	u.u_options = (O_FOLLOW|O_BELL|O_AUTOGET|O_SPECMESS|O_COLORINV);
	u.u_name[0] = '\0';
	strcpy(u.u_savedir, "save");
	u.u_sym = PLAYER;
	u.u_lo_level = u.u_hi_level = 1;
	u.u_dlevel = 0;

	/* init the misc stuff */
	msearch = mrest = 5;
	scrtype = COLOR;
	typed = message[0] = '\0';
	ntimes = numevents = nummons = numlobjs = numinv = npline = 0;
	macro[0] = 0;
	wizard = proceed = NO;
	oprint = YES;

	/* clear the top line multi-buffer */
	for (i = 0; i < NUMPLINE; i++)
		lastmess[i][0] = '\0';

	/* init the redisplay data */
	for (i = 0; i < LINES; i++)		
		red_data[i][0] = -1;

	optfile();
	setupscr();
	cursor(NO);
}

/*
 * GET_GAME: check for saved game and do appropriate thing
 */

void
get_game()
{
	if (!rest_game())
		setupgame();
	else {
		cl_scr();
		u.u_dlevel--;
		newlev(DOWN, UPSTAIR, NO);
		pline("Welcome back, %s.", u.u_name);
	}
}
