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
#include	<string.h>
#include	<stdio.h>

#define         NUMOPTS         8
#define		STROPTS		1
#define		INTOPTS		2
#define         BITOPTS         5

char	*optname[NUMOPTS] = {
	"Adventurer's name",
	"Follow corridors",
	"Audible bell",
	"Auto-pickup objects",
	"Print special messages",
	"Color inventory",
	"Multi-search",
	"Multi-sleep",
};
 
char	*optstr[STROPTS] = { u.u_name };

int	*optnum[INTOPTS] = { &msearch, &mrest },
	optbits[BITOPTS] = {O_FOLLOW,O_BELL,O_AUTOGET,O_SPECMESS,O_COLORINV};

/*
 * OPTION: displays and allows the player to change options
 */

int
option()
{
	register	c,
			i;
	register char   *cp;

	set_act(1);
	printopts();
	set_vis(1);

	for (;;) {
		i_set(0, 0);
		swrite("Set which option? ");
		go_imaginary();
		c = ctgetch();

		if (c == '\033' || c == '\015')	/* ESC and ENTER */
			break;

		c -= '1';

		if (c < 0 || c >= NUMOPTS) {
			bell();
			continue;
		}

		/* will the player have to enter a string or number? */
		if (c < STROPTS || c >= STROPTS + BITOPTS) {
			i_set(c+1, 30);
			cl_eol();
			i_set(c+1, 30);
			go_imaginary();
		}

		if (c < STROPTS) {	/* string options */
			if (atoi(cp = get_str(YES)) != -1 && *cp != 0)
				strncpy(optstr[c], cp, 24);
		/* no/yes options */
		} else if (c < STROPTS + BITOPTS) {
			if (u.u_options & optbits[c - STROPTS])
				u.u_options &= ~(optbits[c - STROPTS]);
			else u.u_options |= optbits[c - STROPTS];
		/* numeric options */
		} else {
			if ((i = atoi(get_str(YES))) < 1 && i != -1)
				i = 1;
			else if (i > 999)
				i = 999;

			/* if player didn't press ESC, set option value */
			if (i != -1)
				*(optnum[c - (STROPTS + BITOPTS)]) = i;
		}

		printopts();
	}

	cl_scr();
	set_act(0);
	set_vis(0);

	return NO;
}

/*
 * PRINTOPTS: display all options current settings on the screen
 */

void
printopts()
{
	register	i;
	char		*dots = "..............................",
			str[60];

	color(WHITE);

	for (i = 0; i < NUMOPTS; i++) {
		i_set(i + 1, 0);
		sprintf(str, "%d) %s", i + 1, optname[i]);
		strcat(str, &dots[strlen(str)]); /* variable # of elipses */
		swrite(str);

		if (i < STROPTS)
			swrite(optstr[i]);
		else if (i < STROPTS + BITOPTS) {
			if (u.u_options & optbits[i - STROPTS])
				swrite("YES");
			else swrite("NO");
		} else {
			sprintf(str, "%d", *(optnum[i - (STROPTS+BITOPTS)]));
			swrite(str);
		}

		cl_eol();
	}
}

/*
 * OPTFILE: detect and read in a MAG options file
 */

void
optfile()
{
	register FILE	*opts;
	char		line[100];
	register char	*cp;
	register	i;
	int		value;

	/* find the options file if it exists */
	if ((opts = fopen("options.mag", "r")) == NULL)
		return;

	while (fgets(line, 100, opts)) {
		if (cp = strchr(line, '\n'))	/* get rid of \n if one */
			*cp = 0;

		if (!line[0])		/* ignore blank lines */
			continue;

		/* convert 1st char to upper and rest before '=' to lower */
		line[0] = (char)toupper(line[0]);

		for (cp = line+1; *cp && *cp != '='; cp++)
			*cp = (char)tolower(*cp);

		if (!strncmp(line, "Screen mode=", 12)) {
			for (cp = line+12; *cp; cp++)
				*cp = (char)tolower(*cp);

			if (!strcmp(line+12, "monochrome"))
				scrtype = MONOCHROME;
			else if (!strcmp(line+12, "mono color"))
				scrtype = MONOCOLOR;

			/* otherwise default to color */

			continue;
		}
		
		if (!strncmp(line, "Save directory=", 15)) {
			strncpy(u.u_savedir, line+15, 24);

			continue;
		}

		/* see if line matches any standard options */
		for (i = 0; i < NUMOPTS; i++) {
			if (strncmp(line, optname[i], strlen(optname[i])))
				continue;

			cp = &line[strlen(optname[i]) + 1];

			if (i < STROPTS)
				strncpy(optstr[i], cp, 24);
			else if (i < STROPTS + BITOPTS) {
				if (*cp == 'y' || *cp == 'Y')
					u.u_options |= optbits[i - STROPTS];
				else if (*cp == 'n' || *cp == 'N')
					u.u_options &= ~(optbits[i-STROPTS]);
			} else {
				if ((value = atoi(cp)) < 1)
					value = 1;
				else if (value > 999)
					value = 999;

				*(optnum[i - (STROPTS + BITOPTS)]) = value;
			}

			break;
		}

		if (i >= NUMOPTS) /* tell player that something's wrong */
			bell();
	}
}
