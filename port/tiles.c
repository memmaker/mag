/*
 * tiles.c - which DawnLike sprite shows a map cell (RVIP R4, W0)
 *
 * The game decides: the character MAG drew at a map cell (its page-0
 * shadow screen optscr[0]) is matched against the game's own lists at that
 * location (player, monster, level object, door, trap, terrain), so a
 * monster gets the sprite of its pmon entry, an object the sprite of its
 * kind (potions, scrolls, wands and rings: of their random appearance, so
 * sprites never give away what an unknown item is). Delusion and invisible
 * monsters show what MAG shows. Slot tables: tiles.h (mkdawn.py).
 */

#include "mag.h"
#include "tiles.h"

static int
fake_index(char *p, char **fk, int n)
{
	int i;

	for (i = 0; i < n; i++)
		if (fk[i] == p)
			return i;
	return 0;
}

/* sprite of an object kind as the player sees it */
int
obj_sprite(OBJECT *o)
{
	unsigned off = o->o_offset;

	if (off >= NUMOBJ)
		return obj_tile[0];
	switch (o->o_type) {
	case POTION:
		return pot_tile[fake_index(nameptr[off][1], fpotions, NUMPOTIONS)];
	case SCROLL:
		return scroll_tile[fake_index(nameptr[off][1], fscrolls, NUMSCROLLS)];
	case RING:
		return ring_tile[fake_index(nameptr[off][1], frings, NUMRINGS)];
	case WAND:
		if (off >= DIGGING && off < CANCEL)
			return TL_STAFF;
		return wand_tile[fake_index(nameptr[off][1], fwands, NUMWANDS)];
	}
	return obj_tile[off];
}

/* a generic sprite for an object class character (delusion) */
static int
class_sprite(int ch)
{
	static const short first[NUMTYPES] = { FOODRAT, BLINDNESS, SAGGMON, DRAINLIFE,
		DECRDAM, ARROW, BANDED, BUCKLER, COPPER, SAPPHIRE, SPARKLING, 147 };
	char *p = strchr(CITEMS, ch);

	return p && *p ? obj_tile[first[p - CITEMS]] : -1;
}

static int
terrain(int ch, DUNGEON *d)
{
	TRAP *t;
	DOOR *dr;

	switch (ch) {
	case HWALL:	return TL_HWALL;
	case VWALL:	return TL_VWALL;
	case ULCORNER:	return TL_ULWALL;
	case URCORNER:	return TL_URWALL;
	case LLCORNER:	return TL_LLWALL;
	case LRCORNER:	return TL_LRWALL;
	case 0xd1:	return TL_TTORCH;	/* TORCHC: wall torches */
	case 0xcf:	return TL_BTORCH;
	case 0xc7:	return TL_LTORCH;
	case 0xb6:	return TL_RTORCH;
	case MARBLE:	return TL_MARBLE;
	case CORRIDOR:	return TL_CORRIDOR;
	case FLOOR:	return TL_FLOOR;
	case UPSTAIR:	return TL_UPSTAIR;
	case DNSTAIR:	return TL_DNSTAIR;
	case POOL:	return TL_POOL;
	case DOORC:
		for (dr = doors; dr < &doors[numdoors]; dr++)
			if (dr->dr_d == d)
				return (dr->dr_data & DR_WITHLOCK) ? TL_LOCKED : TL_DOOR;
		return TL_DOOR;
	case TRAPC:
		for (t = traps; t < &traps[numtraps]; t++)
			if (t->t_d == d && t->t_t >= trapd && t->t_t < &trapd[NUMTRAPS])
				return trap_tile[t->t_t - trapd];
		return TL_TRAP;
	}
	return -1;
}

/* what a cell with a thing on it stands on */
static int
ground(DUNGEON *d)
{
	int t;

	if (d->d_data & D_STONE && !(d->d_data & D_DOOR))
		return -1;
	if (d->d_data & D_TRAPPED && d->d_what == TRAPC && (d->d_data & D_SEEN))
		return terrain(TRAPC, d);
	t = terrain(d->d_what, d);
	if (t == TL_TRAP || t < 0)
		return (d->d_data & D_SEEN) ? TL_FLOOR : -1;
	if (!(d->d_data & D_SEEN) && t == TL_FLOOR)
		return -1;	/* a dark room floor the player hasn't seen */
	return t;
}

static struct permmon *
by_letter(int ch)
{
	int i;

	for (i = 0; i < NUMMON; i++)
		if ((unsigned char)pmon[i].p_letter == ch)
			return &pmon[i];
	return NULL;
}

/*
 * Sprite for map line l, column c (1..LINES-2); *under gets the terrain
 * to draw first (or -1). Returns -2 for an empty (black) cell and -1 for
 * a cell drawn as text.
 */
int
tile_for(int l, int c, int *under)
{
	DUNGEON *d = &dun[l][c];
	int ch = (unsigned char)optscr[0][l][c], t;
	MONSTER *m;
	LEVOBJ *o;
	struct permmon *p;

	*under = -1;
	if (ch == ' ' || ch == 0)
		return -2;
	if (d == u.u_d && (ch == PLAYER || ch == IPLAYER || ch == (unsigned char)u.u_sym)) {
		*under = ground(d);
		return TL_PLAYER;
	}
	if (d->d_data & D_MONSTER)
		for (m = mons; m < &mons[nummons]; m++)
			if (m->m_d == d && !(m->m_data & M_DEAD)
			  && (unsigned char)m->m_perm->p_letter == ch) {
				*under = ground(d);
				return mon_tile[m->m_perm - pmon];
			}
	if (d->d_data & D_OBJECT)
		for (o = lobjs; o < &lobjs[numlobjs]; o++)
			if (o->l_d == d && (unsigned char)CITEMS[o->l_o.o_type] == ch) {
				*under = ground(d);
				return obj_sprite(&o->l_o);
			}
	if ((t = terrain(ch, d)) >= 0) {
		/* sprites with see-through parts stand on the room floor */
		if (t == TL_DOOR || t == TL_LOCKED || t == TL_UPSTAIR || t == TL_DNSTAIR || ch == TRAPC)
			*under = TL_FLOOR;
		return t;
	}
	/* delusion, detected or remembered things: the class or letter */
	if ((t = class_sprite(ch)) >= 0 || ((p = by_letter(ch)) && (t = mon_tile[p - pmon]) >= 0)) {
		*under = ground(d);
		return t;
	}
	return -1;
}
