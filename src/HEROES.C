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
#include	<memory.h>
#include	<stdio.h>
#include	<string.h>

struct hero	{
	long	h_score;
	int	h_level,
		h_elevel;
	char	h_name[25],
		h_killer[40];
};

char	*honornames[9] = { " bold", " brave", " courageous", " gallant",
				" valiant", " stouthearted", " fearless",
				" noble", "n honorable", };

/*
 * TOPTEN: actually it's a top five list handler for the greatest scores
 * achieved in the game by a mortal (not a wizard)
 */

void
topten(score, kill)
long	score;
char	*kill;
{
	struct hero	h[5];
	register FILE	*list;
	register	i = 0,
			j;
	int		modflag = NO;
	char		line[100];

	/* open the hero list and load <= 5 heroes from it */
	if (list = fopen("heroes.mag", "r")) {
		for ( ; i < 5 && fgets(line, 100, list); i++) {
			if (sscanf(line, "%[^|]|%ld|%[^|]|%d|%d\n", h[i].
					h_name,&h[i].h_score, h[i].h_killer,
					&h[i].h_level, &h[i].h_elevel) < 5) {
				pline("Problems with \"heroes.mag\".");
				break;
			}
		}

		fclose(list);
	}

	/* if less than 5 in file, or file doesn't exist, init the rest */
	for (j = i; j < 5; j++)
		h[j].h_score = 0L;

	cl_scr();
	set_vis(1);
	set_act(0);

	color(BROWN);
	prfile("pics\\herobox");
	color(MAGENTA | INTENSE);
	i_set(3, 28);
	swrite("- The MAG Hall Of Heroes -");

	for (i = 0; i < 5; i++) {
		/* if score hasn't been added to list yet and player's score
		   is higher than the current list score, add the score to
		   the hero list */
		if (!modflag && score && score >= h[i].h_score) {
			/* shift all the others over one record */
			memmove(&h[i + 1], &h[i], (4 - i) * sizeof
					(struct hero));
			h[i].h_score = score;

			/* since '|' is reserved as the field delimiter in
			   the hero file, it is replace with space if in the
			   hero's name */
			for (j = 0; u.u_name[j]; j++)
				if (u.u_name[j] == '|')
					u.u_name[j] = ' ';

			strcpy(h[i].h_name, u.u_name);
			strcpy(h[i].h_killer, kill);
			h[i].h_level = u.u_dlevel;
			h[i].h_elevel = u.u_elevel;
			modflag = YES;

			/* if player gets on list, print it diff color */
			color(CYAN);
		} else color(RED | INTENSE);

		/* zero scores are not printed */
		if (h[i].h_score == 0L)
			continue;

		sprintf(line, "%s - %s points", h[i].h_name, commastr(
				h[i].h_score));
		i_set(5 + (i * 3), 40 - (strlen(line) / 2));
		swrite(line);

		if (!strcmp(h[i].h_killer, "quit"))
			sprintf(line, "quit on level %d.", h[i].h_level);
		else if (!strcmp(h[i].h_killer, "escaped"))
			sprintf(line, "escaped the dungeon!");
		else sprintf(line, "A%s %s slain by %s on level %d.",
			honornames[rnd(9)], titles[h[i].h_elevel-1],
			article(h[i].h_killer, NO),h[i].h_level);

		i_set(6 + (i * 3), 40 - (strlen(line) / 2));
		swrite(line);
	}

	/* if the score has been added to the list, save the new list */
	if (modflag) {
		if (list = fopen("heroes.mag", "w")) {
			for (i = 0; i < 5 && h[i].h_score; i++)
				fprintf(list, "%s|%ld|%s|%d|%d\n", h[i].
					h_name, h[i].h_score, h[i].h_killer,
					h[i].h_level, h[i].h_elevel);

			fclose(list);
		} else pline("Ack!  Can't open \"heroes.mag\" for writing!");
	}

	set_vis(0);
	doset(22, 0);
}
