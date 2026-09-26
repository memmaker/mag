/*
 *  		MAG - A Dungeon Adventuring Game
 *
 *	Copyright (C) 1986,87,88 by Michael J. Teixeira
 *
 *  General permission to copy or modify, but not for profit, is hereby
 *  granted, provided that the above copyright notice is included.
 *
 */

/* for a description of all these vars, look in the MAG.H file */

DUNGEON		dun[LINES][COLUMNS];
DOOR		doors[MAXDOORS];
OBJECT		inv[MAXINV];
LEVOBJ		lobjs[MAXLOBJS];
MONSTER		mons[MAXMONS];
ROOM		rooms[MAXROOMS];
TRAP		traps[MAXTRAPS];
struct event	events[MAXEVENTS];
struct you	u;

struct trapdata trapd[NUMTRAPS] = {
	"spider web",			spiderweb,
	"stun", 			stun,
	"swinging pendulum",		pendulum,
	"strangle weed",		weed,
	"poison needle",		poisneed,
	"sleeping gas", 		sleepgas,
	"bear", 			bear,
	"flying spear", 		flyingspear,
	"teleportation",		teletrap,
};

char	*titles[NUMTITLES] = {
		"Novice",	"Adventurer",	"Veteran",	"Warrior",
		"Swordsman",	"Warder",	"Myrmidon",	"Hero",
		"Swashbuckler",	"Protector",	"Defender",	"Champion",
		"Superhero",	"Chevalier",	"Knight",	"Knight Errant",
		"Lord",		"Lord Keeper",	"High Lord",	"Lord Noble"
	},

	*dirdesc[8] = {	"north", "northeast", "east", "southeast", "south",
		"southwest", "west", "northwest", };

unsigned char	out_attrib;

char	*screen,
	optscr[2][LINES][COLUMNS],
	lastmess[NUMPLINE][85],
	message[85],
	genostr[NUMMON + 1],
	*killer,
	*nameptr[NUMOBJ][2],
	ostats[NUMOBJ],
	*CITEMS = "%!?/=)][,\"*$",
	*wepdam[NUMWEAP] = { "1d6", "1d3", "1d6", "1d3", "1d5", "1d3", "2d3",
		"1d6", "2d4", "1d5", "2d5", "1d8", "3d10", "2d3", "4d4" },
	*obj_names[NUMTYPES] = { 0, "potion", "scroll", 0, "ring", 0, "armor",
					"shield", "key", 0, "gem", "piece" };

int     i_line[2],
	i_col[2],
	curline[2],
	curcol[2],
	typed,
	macro[NUMMACRO],
	*macroptr,
	red_data[LINES][2],
	circle[9] = {0,-(COLUMNS+1), 1, 1, COLUMNS, COLUMNS, -1, -1,-COLUMNS},
	armorac[NUMARMOR + NUMSHIELD] = { 6, 5, 5, 2, 1, 7, 3, 4, 6, 3, 1, 1,
								 2, 3 },
	objoff[NUMTYPES] = { FOODRAT, BLINDNESS, SAGGMON, DRAINLIFE, DECRDAM,
		ARROW, BANDED, BUCKLER, COPPER, SAPPHIRE, SPARKLING,PLATINUM},
	numobj[NUMTYPES] = { NUMFOOD, NUMPOTIONS, NUMSCROLLS, NUMWANDS,
		NUMRINGS, NUMWEAP, NUMARMOR, NUMSHIELD, 4, NUMMISC, 12, 4 },
	w_food[NUMFOOD] = { 454, 318, 227, 193, 181, 177, 109 },
	w_weapon[NUMWEAP] = { 91, 4536, 45, 2268, 91, 91, 454, 227, 6804,
				2268, 2722, 4536, 30000, 2268, 11340 },
	w_armor[NUMARMOR] = { 15876, 13608, 9072, 6804, 4536, 20412, 11340, 
							18144, 18144, 9072 },
	w_shield[NUMSHIELD] = { 1361, 2268, 4536, 3402 },
	w_misc[NUMMISC] = { 0, 363, 222, 227, 907, 91, 875, 227 },
	oweight[NUMTYPES] = { 0, 1134, 1225, 0, 91, 0, 0, 0, 227, 0, 113, 3 },
	*wtable[NUMTYPES] = { w_food, 0, 0, 0, 0, w_weapon, w_armor, w_shield,
							0, w_misc, 0, 0 },
	oprint,
	proceed,
	scrtype,
	eflg,
	blineflg,
	ntimes,
	oldelev,
	wizard,
	msearch,
	mrest,
	nummons,
	numrooms,
	numdoors,
	numevents,
	numlobjs,
	numinv,
	numtraps,
	npline,
	active_page,
	visual_page;

long	oldexp;
