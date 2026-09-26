/*
 *  		MAG - A Dungeon Adventuring Game
 *
 *	Copyright (C) 1986,87,88 by Michael J. Teixeira
 *
 *  General permission to copy or modify, but not for profit, is hereby
 *  granted, provided that the above copyright notice is included.
 *
 */

#include	<stdlib.h>
#include	"objdef.h"

#define		COLOR		0	/* screen types */
#define		MONOCHROME	1
#define		MONOCOLOR	2

#define		BLUE			01	/* screen attributes */
#define		GREEN			02
#define		RED			04
#define		INTENSE		       010
#define		BLINKING 	(char)0200

#define		CYAN		(BLUE | GREEN)
#define		MAGENTA		(BLUE | RED)
#define		BROWN		(GREEN | RED)
#define		WHITE		(BLUE | GREEN | RED)
#define		YELLOW		(BROWN | INTENSE)

#define	color(c)	(out_attrib=(scrtype==COLOR?(char)(c):(char)WHITE))

#define		CRYSTALIZER	(&pmon[2])	/* quick ref to perm mon */
#define		FIREDRAKE	(&pmon[3])	/* stru for certain mons */
#define		INVISIXPLODE	(&pmon[8])
#define		OGRE		(&pmon[14])
#define		QUASIT		(&pmon[16])
#define         SHRIEKER        (&pmon[18])
#define		REDDRAGON	(&pmon[29])
#define		WHITEDRAGON	(&pmon[30])
#define		BLUEDRAGON	(&pmon[31])
#define		IMPDRAGON	(&pmon[32])
#define         FROSTFIEND      (&pmon[34])
#define         GOLEM           (&pmon[35])
#define		HILLGIANT	(&pmon[36])
#define		POLYMORPHER	(&pmon[44])
#define		ZABLORON	(&pmon[54])

/* these are the monster data bits for the m_data element */

#define 	M_DETECTED	     01L
#define 	M_INVIS 	     02L
#define 	M_SLEEPING	     04L
#define 	M_AGGRAVATED 	    010L
#define 	M_STUCK 	    020L	/* mon stuck in trap */
#define 	M_CONFUSED	    040L
#define 	M_PARALYZED	   0100L
#define 	M_SLOWED	   0200L
#define 	M_SPEEDED	   0400L
#define 	M_SCARED	  01000L
#define 	M_CANCELLED	  02000L	/* can't use magic attacks */
#define 	M_GOTBOW	  04000L	/* has a bow & arrows */
#define 	M_GOTWAND	 010000L	/* has a wand */
#define 	M_SPECIAL	 020000L	/* has special ability */
#define 	M_POLYPOWER	 040000L	/* can polymorph */
#define 	M_ATTACKED	0100000L	/* has attacked this round */
#define		M_WAKEUP	0200000L	/* is waking up */
#define		M_DEAD		0400000L	/* RIP */

/* these are the bit flags for the permanent object structure (po_flags) */

#define 	CDRINK		     01	/* object is drinkable */
#define 	CEAT		     02	/* etc. */
#define 	CREAD		     04
#define 	CZAP		    010
#define 	CHAND		    020
#define 	CWIELD		    040
#define 	CTHROW		   0100
#define 	CWEAR		   0200
#define 	CUSE		   0400
#define 	CLIGHT		  01000
#define 	CUNLOCK 	  02000
#define 	CSTRAP		  04000
#define 	CMWENCH		 010000
#define 	CENCH		 020000
#define 	CGROUP		 040000	/* objects can be grouped in inv */
#define 	SID	   (int)0100000	/* object is self-identifying */
#define 	CALLABLE	    035
#define 	IDENTABLE	  04375
#define 	DROPABLE	 077777

/* these are the bit flags for any given object (o_data) */

#define 	O_IDENT 	  01	/* obj is identified */
#define 	O_IGNITED	  02
#define 	O_CURSED	  04
#define 	O_SEECURSED	 010	/* player knows about curse */
#define 	O_USEDUP	 020	/* obj has been used up */
#define		O_INUSE		 040	/* obj is being used by player */
#define		O_GALVANIZED	0100
#define		O_ETHEREAL	0200

#define		LEFTHAND	0
#define		RIGHTHAND	1

#define		leftweapon	(u.u_use[0])	/* the inuse ptrs */
#define		rightweapon	(u.u_use[1])
#define		armor		(u.u_use[2])
#define		shield		(u.u_use[3])
#define		leftring	(u.u_use[4])
#define		rightring	(u.u_use[5])
#define		secweap		(u.u_use[6])

#define 	L_DETECTED	01	/* the only level obj flag */

#define 	REDRAW_HP	01	/* bottom line redraw flags */
#define 	REDRAW_SCR	02
#define 	REDRAW_ALL	04

#define 	NO		0
#define 	YES		1

#define 	LINES		24	/* dimensions of the screen */
#define 	COLUMNS 	80
#define 	MAXROOMS	15	/* max room on dungeon level */
#define 	MAXDOORS	(MAXROOMS * 4) /* 4 per room should be good */
#define 	MAXTRAPS	20	/* max traps on dungeon level */
#define 	MAXEVENTS	10	/* max events at one time */
#define         MAXMONS         100	/* max mons on dungeon level */
#define		MAXMONCARRY	3	/* max obj that a mon can carry */
#define		MAXINV		40	/* max obj the player can carry */
#define		MAXLOBJS	200	/* max obj on dungeon level */

#define		NUMTITLES	20	/* num of experience titles */
#define 	NUMTRAPS	9	/* num types of traps */
#define 	NUMOBJ		151	/* num types of objects */
#define 	NUMTYPES	12	/* num types of types of objs */
#define 	NUMPOTIONS	20	/* num types of potions, etc. */
#define 	NUMSCROLLS	21
#define 	NUMWANDS	21
#define 	NUMRINGS	25
#define 	NUMWEAP 	15
#define 	NUMFOOD 	7
#define 	NUMARMOR	10
#define 	NUMSHIELD	4
#define 	NUMMISC 	8
#define 	NUMTREAS	4
#define 	NUMMON		55	/* num types of monsters */
#define		NUMMACRO	21	/* max num chars in macro */
#define		NUMPLINE	3	/* num of stores top lines */

/* these are the status types for each type of object */

#define 	UNKNOWN 	0	/* players doesn't know the obj */
#define 	KNOWN		1	/* player knows the obj */
#define 	CALLED		2	/* player has called the obj */

/* these are the special perm monster flags.  note that the first four
   bits (01 thru 010) are color attributes defined above */

#define 	BOW		 020	/* mon can use a bow & arrows */
#define 	ZAP		 040	/* mon can use a wand */
#define 	THROW		0100	/* mon can throw boulders */
#define		NOGRAB		0200	/* mon doesn't pick up objs */
#define		HATEWATER	0400	/* mon hates water */
#define         PACK            01000	/* mon can come in packs/lairs */

/* force the next top line message to print a MORE first */
#define	keep()		(oprint = NO)

/* convert a dungeon ptr to a line and col */
#define	scrline(d)	(int)(((d) - (DUNGEON *)dun) / COLUMNS)
#define	scrcol(d)	(int)(((d) - (DUNGEON *)dun) % COLUMNS)

/* make all the calls to the DOROOM procedure easier */
#define	pr_room(r)	(doroom((r), 0))		/* print */
#define	ck_room(r)	(doroom((r), 1))		/* check */
#define	mk_room(r)	(doroom((r), 2))		/* make */
#define	wl_room(r)	(doroom((r), 3))		/* wall */
#define	tr_room(r)	(doroom((r), 4))		/* treasure */
#define	dk_room(r)	(doroom((r), 5))		/* darken */
#define	cl_room(r)	(doroom((r), 6))		/* clear */

/* calc the height or width of a given room */
#define	rheight(r)	((r)->r_lline - (r)->r_uline)
#define	rwidth(r)	((r)->r_lcol - (r)->r_ucol)

/* return the given object's permanent name */
#define	poname(o)	((char *)pobj[(o)->o_offset].po_name)

#define	isknown(o)	(ostats[(o)->o_offset] == KNOWN)
#define	isunknown(o)	(ostats[(o)->o_offset] == UNKNOWN) /* quick tests */
#define	iscalled(o)	(ostats[(o)->o_offset] == CALLED)

/* return the total amount the player can carry in grams */
#define	cancarry()	((long)(u.u_str * 4000L))

/* return how many more grams the player can carry */
#define	carryleft()	((long)(cancarry() - u.u_weight))

/* return an armor class for a given suit of armor or shield */
#define	what_ac(o)	(armorac[(o)->o_offset-BANDED]+(o)->o_enchant)

#define	rnd(i)		(rand()%(i))

/* set the imaginary cursor for the active video page to the given l & c */
#define	i_set(l, c)	(i_line[active_page] = (l), i_col[active_page] = (c))

/* force the cursor to the imaginary cursor location */
#define go_imaginary()  (doset(i_line[active_page], i_col[active_page]))

/* quick ref to the two permanent events (regeneration & hungry) */
#define 	eregen		((struct event *)(&events[0]))
#define 	efood		((struct event *)(&events[1]))

#define 	oppdir(dir)	(((dir) + 4) % 8)
#define 	UP		0
#define 	UPRT		1
#define 	RIGHT		2
#define 	DNRT		3
#define 	DOWN		4
#define 	DNLT		5
#define 	LEFT		6
#define 	UPLT		7
#define		NUMDIRS		8

#define 	FOOD		0
#define 	POTION		1
#define 	SCROLL		2
#define 	WAND		3
#define 	RING		4
#define 	WEAPON		5
#define 	ARMOR		6
#define 	SHIELD		7
#define 	KEY		8
#define 	MISC		9
#define 	GEM		10
#define 	TREASURE	11

/* these are the bit flags for the dungeon structure (d_data) */

#define 	D_DOOR		(unsigned char)  01 /* door here */
#define 	D_MONSTER	(unsigned char)  02 /* mon here */
#define 	D_OBJECT	(unsigned char)  04 /* obj here */
#define 	D_TRAPPED	(unsigned char) 010 /* trap here */
#define 	D_SEEN		(unsigned char) 020 /* loc has been seen */
#define 	D_STAIRCASE	(unsigned char) 040 /* stairs here */
#define 	D_WALL		(unsigned char)0100 /* wall here */
#define 	D_STONE 	(unsigned char)0200 /* solid rock here */

/* these are the bit flags for the room struct (r_data) */

#define 	R_MAZE		 01	/* room is a maze room */
#define 	R_TORCHED	 02	/* is currently lit by a torch */
#define 	R_STAIRCASE	 04	/* there are stairs in the room */
#define 	R_TROVE 	010	/* room is a treasure trove */
#define 	R_MAGICLIT	020	/* room is magically lit */

/* these are the bit flags for the door struct (d_data) */

#define 	DR_COPPER	01
#define 	DR_SILVER	02
#define 	DR_GOLDEN	04
#define 	DR_WITHLOCK	07	/* first three */

/* these are the bit flags for the you struct (u_data) */

#define 	UD_FORCE	     01L	/* in force-move mode */
#define 	UD_FIND 	     02L	/* in find-move mode */
#define 	UD_BLIND	     04L	/* player is blind */
#define 	UD_SLEEPING	    010L	/* etc. */
#define 	UD_CONFUSED	    020L
#define 	UD_STUCK	    040L	/* stuck in trap */
#define 	UD_STRANGLE	   0100L	/* being strangled */
#define 	UD_CONMON	   0200L	/* confusing mon (hands) */
#define 	UD_HASTED	   0400L
#define 	UD_FAINTING	  01000L
#define 	UD_HUNGRY	  02000L
#define 	UD_INVIS	  04000L
#define 	UD_SEEINV	 010000L	/* can see invisible */
#define		UD_BEENSAPP	 020000L	/* been to the sapphre lev */
#define		UD_LEVITATE	 040000L
#define		UD_DELUSION	0100000L
#define		UD_LABYRINTH	0200000L	/* is in labyrinth */

#define 	blind() 	(u.u_data & UD_BLIND)
#define		confused()	(u.u_data & UD_CONFUSED)

/* these are the bit flags for the you options element (u_options) */

#define         O_FOLLOW          01	/* follow corridors is on */
#define         O_MAKEMACRO       02	/* recording macro */
#define         O_DOMACRO         04	/* executing a macro */
#define         O_BELL           010	/* audible bell is on */
#define         O_AUTOGET        020	/* auto-get is on */
#define         O_SPECMESS       040	/* special messages are on */
#define         O_COLORINV      0100	/* color inventories is on */

/* these are all of the different dungeon symbols (most anyway) */

#define 	ROCK		' '
#define 	HWALL		(unsigned char)0xcd
#define 	VWALL		(unsigned char)0xba
#define 	ULCORNER	(unsigned char)0xc9
#define 	URCORNER	(unsigned char)0xbb
#define 	LLCORNER	(unsigned char)0xc8
#define 	LRCORNER	(unsigned char)0xbc
#define 	CORRIDOR	(unsigned char)0xb2
#define 	FLOOR		(unsigned char)0xfa
#define 	DOORC		(unsigned char)0xce
#define 	UPSTAIR 	(unsigned char)0xae
#define 	DNSTAIR 	(unsigned char)0xaf
#define 	POOL		(unsigned char)0xf7
#define 	MARBLE		(unsigned char)0xdb
#define 	TRAPC		(unsigned char)0xe8
#define 	PLAYER		(unsigned char)0x02
#define		IPLAYER		(unsigned char)0x01
#define		TORCHC		"\321\266\317\307"
#define		CORNERS		"\311\273\274\310"
#define		SFLOOR		"\372"
#define		STELETO		"\372\257\256\367"
#define		SPUTMON		"\372\367"

extern int	(*cmd[261])();		/* the key/command table */

typedef struct door	{
	struct dungeon	*dr_d;		/* the location of the door */
	struct room	*dr_r;		/* the room that the door is in */
	char		dr_data,	/* door data flags */
			dr_appear;	/* what the door looks like */
	int		dr_stuck;	/* if and how long door is stuck */
} DOOR;
extern DOOR	doors[MAXDOORS];

typedef struct dungeon	{
	unsigned char	d_what,		/* the symbol at that location */
			d_data;		/* dungeon data flags */
} DUNGEON;
extern DUNGEON	dun[LINES][COLUMNS];
	       
struct event	{
	long	e_when;			/* when the event will occur */
	int	(*e_proc)();		/* ptr to the event procedure */
};
extern struct event	events[MAXEVENTS];

typedef struct object	{
	char		o_type;		/* the type of the object */
	int		o_wepench,	/* enchantment to damage */
			o_enchant;	/* enchantment to hit (charges) */
	unsigned	o_data,		/* object data flags */
			o_offset,	/* offset into perm object table */
			o_quantity;	/* the number of objects */
} OBJECT;
extern OBJECT	inv[MAXINV];

typedef struct levelobj {
	struct dungeon	*l_d;		/* location of obj on level */
	struct object	l_o;		/* a ptr to the obj stru */
	int		l_data;		/* level obj data flags */
} LEVOBJ;
extern LEVOBJ	lobjs[MAXLOBJS];

typedef struct monster	{
	struct permmon	*m_perm;	/* a ptr into the perm mon table */
	struct room	*m_r;		/* current room that mon is in */
	struct dungeon	*m_d,		/* dungeon location where mon is */
			*m_od,		/* last location mon was */
			*m_dest;	/* location where mon want to go */
	struct object	m_i[MAXMONCARRY]; /* mon's inventory */
	int		m_ni,		/* num of objs in mon's inventory */
			m_hp,		/* mon's current hit points */
			m_maxhp;	/* mon's max hps */
	long		m_data;		/* mons data flags */
} MONSTER;
extern MONSTER	mons[MAXMONS];

#define 	o_charges	o_enchant

struct permmon	{
	char		*p_name,	/* mons name */
			p_letter,	/* symbol on screen */
			p_appear;	/* dun level mon appears on first */
	unsigned	p_bexp;		/* base experience value */
	char		p_hitdice,	/* # of hit dice mon gets */
			p_ac,		/* mon's armor class */
			*p_damage,	/* damage in form of "1d4/2d8" */
			p_speed,	/* speed (player = 5) */
			*p_attack;	/* how it attacks "claws","bites" */
	int		p_data;		/* random attributes */
};
extern struct permmon	pmon[NUMMON];

struct permobj	{
	char	*po_name;		/* permanent object name */
	int	po_flag;		/* perm obj data flags */
};
extern struct permobj	pobj[NUMOBJ];

typedef struct room	{
	char	r_uline,		/* room's upper left line */
		r_ucol,			/* upper left column */
		r_lline,		/* lower right line */
		r_lcol,			/* lower right col */
		r_data,			/* room data flags */
		r_numdoors;		/* num door in room */
} ROOM;
extern ROOM	rooms[MAXROOMS];

typedef struct trap	{
	struct dungeon	*t_d;		/* location of trap */
	struct trapdata *t_t;		/* ptr to trap info stru */
} TRAP;
extern TRAP	traps[MAXTRAPS];

struct trapdata {
	char	*t_name;		/* name of the trap */
	int	(*t_func)();		/* trap proc called when sprung */
};
extern struct trapdata	trapd[NUMTRAPS];

struct you	{
	struct dungeon	*u_d;		/* location of player */
	struct room	*u_r;		/* current room player is in */
	struct object	*u_use[7];	/* inuse items */
	struct monster	*u_stuckmon;	/* ptr to mon if one stuck to plyr */
	int		u_ac,		/* armor class */
			u_dir,		/* direction moving in */
			u_dlevel,	/* dungeon level */
			u_elevel,	/* experience level */
			u_hp,		/* current hit points */
			u_maxhp,	/* maximum hps */
			u_maxstr,	/* max strength */
			u_options,	/* player options flags */
			u_str,		/* current strength */
			u_lo_level,	/* highest dlev player's been to */
			u_hi_level;	/* deepest dlev player's been to */
	long		u_epoints,	/* experience points */
			u_score,	/* player's score */
			u_data,		/* players data flags */
			u_weight,	/* how much player is carrying */
			u_moves;	/* how many moves player's made */
	char		u_sym,		/* the symbol for the player */
			u_savedir[25],	/* directory for save files */
			u_name[25];	/* player's name */
};
extern struct you	u;

extern unsigned char	out_attrib;	/* current output attribute */

extern char	*screen,		/* ptr to active screen buffer */
		optscr[2][LINES][COLUMNS], /* the 2 screen buffers */
		lastmess[NUMPLINE][85],	/* prev mesages buffer */
		message[85],		/* copy of last message */
		*CITEMS,		/* symbol string for objs */
		*dirdesc[NUMDIRS],	/* description of directions */
		*titles[NUMTITLES],	/* experience level titles */
		*wepdam[NUMWEAP],	/* damage done by weapons */
		*obj_names[NUMTYPES],	/* name of obj types */
		*nameptr[NUMOBJ][2],	/* name ptrs for objects */
		*fpotions[NUMPOTIONS],	/* fake potions names */
		*fscrolls[NUMSCROLLS],	/* etc. */
		*fwands[NUMWANDS],
		*frings[NUMRINGS],
		*killer,		/* name of thing that killed plyr */
		ostats[NUMOBJ],		/* status of all the objects */
		genostr[NUMMON + 1];	/* string of genocided monsters */

extern  i_line[2],		/* imaginary cursor locs for scrn bufs */
	i_col[2],
	curline[2],		/* actual cursor locs for screen bufs */
	curcol[2],
	typed,			/* last command key typed */
	macro[NUMMACRO],	/* macro buffer */
	*macroptr,		/* ptr into the macro buffer */
	objoff[NUMTYPES],	/* offset table for objects */
	numobj[NUMTYPES],	/* num of each type of obj table */

	w_food[NUMFOOD],	/* weight tables */
	w_weapon[NUMWEAP],
	w_armor[NUMARMOR],
	w_shield[NUMSHIELD],
	w_misc[NUMMISC],
	oweight[NUMTYPES],
	*wtable[NUMTYPES],	/* table with ptrs to tables! */

	armorac[NUMARMOR + NUMSHIELD],	/* armor class table */
	red_data[LINES][2],	/* redisplay flags */
	circle[9],		/* circle traversal table */
	oprint,		/* overprint top message flag */
	proceed,	/* ignore MOREs flag */
	scrtype,	/* type of video being used */
	eflg,		/* erase top line flag */
	ntimes,		/* multi-command counter */
	blineflg,	/* redraw bottom line flag */
	oldelev,	/* old elev storage for HEROISM potion */
	numrooms,	/* number of rooms on the dungeon level */
	numdoors,	/* num of doors on the dungeon level */
	numevents,	/* num of events currently */
	numtraps,	/* num of traps on the dungeon level */
	nummons,	/* current num of mons on the dungeon level */
	numlobjs,	/* num of objs currently on the dungeon level */
	numinv,		/* num of objs in players inventory */
	wizard,		/* wiz mode flag (0=never,1=in wiz,2=was in wiz) */
	msearch,	/* multi-search counter */
	mrest,		/* multi-rest counter */
	npline,		/* num top line messages in top line buffer */
	active_page,	/* currently active video page */
	visual_page;	/* currently viewed video page */

extern long	oldexp;	/* old epts storage for HEROISM potion */

#include	"defs.h"
