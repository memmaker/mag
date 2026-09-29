/*
 * menu.c - the Enter command menu and the inventory with a cursor (RVIP
 * steps 3b and 3c) for MAG.
 *
 * Both are pop-up boxes drawn straight onto the visual BIOS page over the
 * map and taken away again (the game's own shadow screen never sees them).
 * Every box is sized to its content. Choosing a command hands its key back
 * to dispatch(); an item action is the game's own command key with the item
 * letter queued behind it (port_push_key), so the game asks and answers as
 * usual.
 */

#include "mag.h"
#include "fe.h"
#include <ctype.h>

#define CTRL(c)	((c) & 0x1f)
#define FKEY(n)	(128 + 0x3a + (n))	/* F1..F10 as tgetch() returns them */

int port_inv_again;

/* ---- key queue in front of the keyboard (pcvideo.c reads it first) ---- */
static int kq[16], kq_n;

void
port_push_key(int k)
{
	if (kq_n < 16)
		kq[kq_n++] = k;
}

int
port_pop_key(void)
{
	int k;

	if (!kq_n)
		return -1;
	k = kq[0];
	memmove(kq, kq + 1, --kq_n * sizeof *kq);
	return k;
}

/* ---- pop-up boxes on the visual page ------------------------------------ */
#define A_BOX	0x0b	/* bright cyan frame */
#define A_TEXT	0x07
#define A_KEY	0x0e	/* yellow keys */
#define A_TITLE	0x0f
#define A_SEL	0x70	/* black on white */

static unsigned short under[25][80];
static int box_r0, box_c0, box_r1, box_c1;

static void
put(int r, int c, int ch, int a)
{
	if (r >= 0 && r < 25 && c >= 0 && c < 80)
		vram[vis_page][r][c] = a << 8 | (unsigned char)ch;
}

static void
puts_at(int r, int c, const char *s, int a, int w)
{
	for (; w > 0; w--, c++)
		put(r, c, *s ? *s++ : ' ', a);
}

/* a box of rows x cols inside the frame, centred over the map */
static void
box_open(int rows, int cols, const char *title)
{
	int r, c, h = rows + 2, w = cols + 2;

	memcpy(under, vram[vis_page], sizeof under);
	if (h > 22) h = 22;	/* rows 1-22: the message and status lines stay */
	if (w > 80) w = 80;
	box_r0 = 1 + (22 - h) / 2;
	if (box_r0 < 1) box_r0 = 1;
	box_c0 = (80 - w) / 2;
	box_r1 = box_r0 + h - 1;
	box_c1 = box_c0 + w - 1;
	for (r = box_r0; r <= box_r1; r++)
		for (c = box_c0; c <= box_c1; c++)
			put(r, c, ' ', A_TEXT);
	for (c = box_c0 + 1; c < box_c1; c++) {
		put(box_r0, c, 0xcd, A_BOX);
		put(box_r1, c, 0xcd, A_BOX);
	}
	for (r = box_r0 + 1; r < box_r1; r++) {
		put(r, box_c0, 0xba, A_BOX);
		put(r, box_c1, 0xba, A_BOX);
	}
	put(box_r0, box_c0, 0xc9, A_BOX);
	put(box_r0, box_c1, 0xbb, A_BOX);
	put(box_r1, box_c0, 0xc8, A_BOX);
	put(box_r1, box_c1, 0xbc, A_BOX);
	if (title && (int)strlen(title) + 2 <= w - 2)
		puts_at(box_r0, box_c0 + (w - strlen(title) - 2) / 2, " ", A_BOX, 1),
		puts_at(box_r0, box_c0 + (w - strlen(title)) / 2, title, A_TITLE, strlen(title)),
		puts_at(box_r0, box_c0 + (w + strlen(title)) / 2, " ", A_BOX, 1);
	fe_popup(box_r0, box_c0, box_r1, box_c1);
}

static void
box_close(void)
{
	memcpy(vram[vis_page], under, sizeof under);
	fe_popup(-1, 0, 0, 0);
	fe_present();
}

/* keys for lists: returns 'U' up, 'D' down, 'L' left/back, 'R' right,
 * '\r' choose, 27 close, a click row as 1000 + row, or the key itself */
static int
list_key(void)
{
	int k = fe_getkey(-1);

	switch (k) {
	case FK_UP: case FK_KP8:	return 'U' | 0x100;
	case FK_DOWN: case FK_KP2:	return 'D' | 0x100;
	case FK_LEFT: case FK_KP4:	return 'L' | 0x100;
	case FK_RIGHT: case FK_KP6:	return 'R' | 0x100;
	case FK_PGUP: case FK_KP9:	return 'P' | 0x100;
	case FK_PGDN: case FK_KP3:	return 'N' | 0x100;
	case FK_KP5: case FK_KPENTER: case '\n': case '\r': return '\r';
	case FK_KP0: case FK_KPDOT: case 27: return 27;
	case FK_KPPLUS:			return '+' | 0x100;
	case FK_KPMINUS:		return '-' | 0x100;
	case FK_KPSTAR:			return '*' | 0x100;
	case FK_CLICK:			return 1000 + fe_click_row;
	}
	if (k >= FK_F1 && k <= FK_F10)
		return FKEY(k - FK_F1 + 1);
	return k;
}

/* ---- the command menu (RVIP 3b), grouped like the help screens ---------- */
struct cmd { int key; const char *desc; };

static const struct cmd g_move[] = {
	{'Z', "Explore (walk to what you haven't seen)"},
	{'>', "Go down / walk to the down stairs"},
	{'<', "Go up / walk to the up stairs"},
	{'.', "Rest one turn"},
	{':', "Rest several turns"},
	{'s', "Search once"},
	{';', "Search several turns"},
	{0, 0}
};
static const struct cmd g_items[] = {
	{'i', "Inventory"},
	{'g', "Get what is lying here"},
	{'d', "Drop"},
	{'e', "Eat"},
	{'q', "Quaff a potion"},
	{'r', "Read a scroll"},
	{'z', "Zap a wand, staff or rod"},
	{'w', "Wield a weapon"},
	{'t', "Throw / fire"},
	{'W', "Wear or take off armor"},
	{'S', "Strap on or unstrap a shield"},
	{'p', "Put on or remove a ring"},
	{'x', "Swap to the secondary weapon"},
	{'X', "Choose the secondary weapon"},
	{CTRL('U'), "Use a miscellaneous item"},
	{'I', "Ignite a torch"},
	{'E', "Extinguish a torch"},
	{CTRL('K'), "Unlock a door with a key"},
	{'c', "Call (name) a kind of object"},
	{0, 0}
};
static const struct cmd g_lists[] = {
	{'%', "List food"},
	{'!', "List potions"},
	{'?', "List scrolls"},
	{'/', "List wands, staffs and rods"},
	{'=', "List rings"},
	{')', "List weapons"},
	{']', "List armor"},
	{'[', "List shields"},
	{',', "List keys"},
	{'"', "List miscellaneous items"},
	{'*', "List gems"},
	{'$', "List money"},
	{0, 0}
};
static const struct cmd g_info[] = {
	{'v', "View: what you can see here"},
	{'a', "Objects you already know"},
	{'C', "How full your pack is"},
	{CTRL('H'), "What is which symbol"},
	{'F', "How your stomach feels"},
	{'T', "Your title"},
	{'A', "How long you have been down here"},
	{CTRL('R'), "Previous messages"},
	{CTRL('L'), "Redraw the screen"},
	{'o', "Options"},
	{'V', "Version"},
	{FKEY(1), "Help (the game's own screens)"},
	{0, 0}
};
static const struct cmd g_game[] = {
	{'{', "Start recording a macro"},
	{'}', "End the macro"},
	{CTRL('E'), "Play the macro"},
	{'R', "Remember (save) the game and leave"},
	{'Q', "Quit"},
	{0, 0}
};
static const struct { const char *name; const struct cmd *cmds; } groups[] = {
	{"Moving", g_move},
	{"Things you carry", g_items},
	{"Listing what you carry", g_lists},
	{"Information", g_info},
	{"Macros and the game", g_game},
};
#define NGROUPS	(int)(sizeof groups / sizeof *groups)

static const char *
keyname(int k)
{
	static char buf[8];

	if (k >= FKEY(1) && k <= FKEY(10))
		snprintf(buf, sizeof buf, "F%d", k - FKEY(1) + 1);
	else if (k < 32)
		snprintf(buf, sizeof buf, "^%c", k + '@');
	else
		snprintf(buf, sizeof buf, "%c", k);
	return buf;
}

/* one group's commands; returns the key or 0 (back) or 27 (close) */
static int
group_menu(int g)
{
	const struct cmd *cmds = groups[g].cmds;
	int n, w = 0, i, sel = 0, k, len;

	for (n = 0; cmds[n].key; n++)
		if ((len = 4 + strlen(cmds[n].desc)) > w)
			w = len;
	if ((len = strlen(groups[g].name) + 2) > w)
		w = len;
	box_open(n, w + 2, groups[g].name);
	for (;;) {
		for (i = 0; i < n; i++) {
			int sa = i == sel ? A_SEL : A_TEXT;
			puts_at(box_r0 + 1 + i, box_c0 + 1, " ", sa, 1);
			puts_at(box_r0 + 1 + i, box_c0 + 2, keyname(cmds[i].key), i == sel ? A_SEL : A_KEY, 3);
			puts_at(box_r0 + 1 + i, box_c0 + 5, cmds[i].desc, sa, w - 3);
		}
		k = list_key();
		if (k == ('U' | 0x100))
			sel = (sel + n - 1) % n;
		else if (k == ('D' | 0x100))
			sel = (sel + 1) % n;
		else if (k == '\r' || k == ('R' | 0x100)) {
			box_close();
			return cmds[sel].key;
		} else if (k == ('L' | 0x100)) {
			box_close();
			return 0;
		} else if (k == 27) {
			box_close();
			return 27;
		} else if (k >= 1000) {
			i = k - 1000 - box_r0 - 1;
			if (i >= 0 && i < n) {
				box_close();
				return cmds[i].key;
			}
		} else
			for (i = 0; i < n; i++)
				if (cmds[i].key == k) {	/* the command's own key */
					box_close();
					return k;
				}
	}
}

/* Enter at the command prompt: returns the chosen command key, 0 = none */
int
port_menu(void)
{
	static int sel;
	int i, k, w = 0, len;

	for (i = 0; i < NGROUPS; i++)
		if ((len = 4 + strlen(groups[i].name)) > w)
			w = len;
	for (;;) {
		box_open(NGROUPS, w + 2, "Commands");
		for (;;) {
			for (i = 0; i < NGROUPS; i++) {
				char num[4];
				int sa = i == sel ? A_SEL : A_TEXT;

				snprintf(num, sizeof num, "%d", i + 1);
				puts_at(box_r0 + 1 + i, box_c0 + 1, " ", sa, 1);
				puts_at(box_r0 + 1 + i, box_c0 + 2, num, i == sel ? A_SEL : A_KEY, 2);
				puts_at(box_r0 + 1 + i, box_c0 + 4, groups[i].name, sa, w - 2);
			}
			k = list_key();
			if (k == ('U' | 0x100))
				sel = (sel + NGROUPS - 1) % NGROUPS;
			else if (k == ('D' | 0x100))
				sel = (sel + 1) % NGROUPS;
			else if (k >= '1' && k < '1' + NGROUPS) {
				sel = k - '1';
				break;
			} else if (k >= 1000) {
				i = k - 1000 - box_r0 - 1;
				if (i >= 0 && i < NGROUPS) {
					sel = i;
					break;
				}
			} else if (k == '\r' || k == ('R' | 0x100))
				break;
			else if (k == 27 || k == ('L' | 0x100)) {
				box_close();
				return 0;
			}
		}
		box_close();
		if ((k = group_menu(sel)) == 27)
			return 0;
		if (k)
			return k;
	}
}

/* ---- inventory with a cursor and item menus (RVIP 3c) ------------------- */
struct act { int key; int flag; const char *name; };

/* the util[] commands of COMMAND1.C that act on one item, with the pobj
 * flag that says an item fits (same test as utilize()) */
static const struct act acts[] = {
	{'e', CEAT, "Eat"},
	{'q', CDRINK, "Quaff"},
	{'r', CREAD, "Read"},
	{'z', CZAP, "Zap"},
	{'p', CHAND, "Put on / remove"},
	{'W', CWEAR, "Wear / take off"},
	{'S', CSTRAP, "Strap on / unstrap"},
	{'w', CWIELD, "Wield"},
	{'X', CWIELD, "Make secondary weapon"},
	{'t', CTHROW, "Throw"},
	{CTRL('U'), CUSE, "Use"},
	{'I', CUSE, "Ignite a torch with it"},
	{CTRL('K'), CUNLOCK, "Unlock a door with it"},
	{'c', CALLABLE, "Call (name)"},
	{'d', DROPABLE, "Drop"},
	{0, 0, 0}
};

static int
fits(OBJECT *o, int flag)
{
	return (pobj[o->o_offset].po_flag & flag) != 0;
}

/* the item's main action: eat, quaff, read, zap, put on, wear, strap,
 * wield, use, unlock; arrows and oil throw; anything else is examined (0) */
static int
main_action(OBJECT *o)
{
	static const int order[] = { CEAT, CDRINK, CREAD, CZAP, CHAND, CWEAR,
		CSTRAP, CWIELD, CUSE, CUNLOCK, CTHROW, 0 };
	int i, j;

	for (i = 0; order[i]; i++)
		if (fits(o, order[i]))
			for (j = 0; acts[j].key; j++)
				if (acts[j].flag == order[i])
					return acts[j].key;
	return 0;
}

/* the game's own inventory colours (O_COLORINV in pr_obj) */
static int
item_attr(OBJECT *o)
{
	return o->o_type == FOOD ? GREEN | INTENSE : o->o_type + 1 + (o->o_type > 6);	/* food: green, the blue was too dark */
}

static void
examine(OBJECT *o)
{
	pline("%s", form(o, YES));
}

/* run action key on item o: the command with the letter queued behind it */
static int
do_action(int key, OBJECT *o)
{
	port_push_key(i_to_l(o));
	port_inv_again = 1;
	return key;
}

/* the menu of every action that fits item o; returns a command key or 0 */
static int
item_menu(OBJECT *o)
{
	int list[20], n = 0, i, k, sel = 0, w, len;
	const char *name = obj_str(o);

	for (i = 0; acts[i].key; i++)
		if (fits(o, acts[i].flag))
			list[n++] = i;
	list[n++] = -1;			/* examine */
	w = strlen(name) + 2;
	if (w > 74) w = 74;
	for (i = 0; i < n; i++)
		if ((len = 5 + (list[i] < 0 ? 7 : strlen(acts[list[i]].name))) > w)
			w = len;
	box_open(n, w + 2, NULL);
	puts_at(box_r0, box_c0 + 2, name, A_TITLE, strlen(name) < (size_t)w - 2 ? strlen(name) : w - 2);
	for (;;) {
		for (i = 0; i < n; i++) {
			int sa = i == sel ? A_SEL : A_TEXT;
			puts_at(box_r0 + 1 + i, box_c0 + 1, " ", sa, 1);
			puts_at(box_r0 + 1 + i, box_c0 + 2, list[i] < 0 ? "*" : keyname(acts[list[i]].key),
				i == sel ? A_SEL : A_KEY, 3);
			puts_at(box_r0 + 1 + i, box_c0 + 5, list[i] < 0 ? "Examine" : acts[list[i]].name, sa, w - 3);
		}
		k = list_key();
		if (k == ('U' | 0x100))
			sel = (sel + n - 1) % n;
		else if (k == ('D' | 0x100))
			sel = (sel + 1) % n;
		else if (k == 27 || k == ('L' | 0x100)) {
			box_close();
			return 0;
		} else if (k >= 1000 || k == '\r' || k == ('R' | 0x100)) {
			if (k >= 1000 && ((i = k - 1000 - box_r0 - 1) < 0 || i >= n))
				continue;
			if (k >= 1000)
				sel = i;
			box_close();
			if (list[sel] < 0) {
				examine(o);
				return 0;
			}
			return do_action(acts[list[sel]].key, o);
		} else if (k == ('*' | 0x100) || k == '*') {
			box_close();
			examine(o);
			return 0;
		} else
			for (i = 0; i < n; i++)
				if (list[i] >= 0 && acts[list[i]].key == k) {
					box_close();
					return do_action(k, o);
				}
	}
}

/*
 * The item list with a cursor. which: pobj flags of the items to show
 * (DROPABLE = everything); prompt: the game's question (item prompts) or
 * NULL (the inventory). Returns
 *  - inventory: a command key to run (0 = none)
 *  - prompt: the chosen item letter, 27 = cancelled, or another key
 */
static int
item_list(int which, const char *prompt)
{
	OBJECT *items[MAXINV], *o;
	int n = 0, i, k, w, len, top = 0, rows, sel = 0;

	for (o = inv; o < &inv[numinv]; o++)
		if (fits(o, which))
			items[n++] = o;
	if (!n)
		return prompt ? 0 : 0;
	w = prompt ? strlen(prompt) : 9;
	for (i = 0; i < n; i++)
		if ((len = strlen(form(items[i], NO))) > w)
			w = len;
	if (w > 76) w = 76;
	rows = n > 20 ? 20 : n;
	box_open(rows, w + 2, NULL);
	puts_at(box_r0, box_c0 + 2, prompt ? prompt : "Inventory", A_TITLE,
		strlen(prompt ? prompt : "Inventory"));
	for (;;) {
		if (sel < top) top = sel;
		if (sel >= top + rows) top = sel - rows + 1;
		for (i = 0; i < rows; i++) {
			o = items[top + i];
			int sa = top + i == sel ? A_SEL : item_attr(o);
			puts_at(box_r0 + 1 + i, box_c0 + 1, " ", sa, 1);
			puts_at(box_r0 + 1 + i, box_c0 + 2, form(o, NO), sa, w);
		}
		fe_present();
		k = list_key();
		o = items[sel];
		if (k == ('U' | 0x100))
			sel = (sel + n - 1) % n;
		else if (k == ('D' | 0x100))
			sel = (sel + 1) % n;
		else if (k == ('P' | 0x100))
			sel = sel > rows ? sel - rows : 0;
		else if (k == ('N' | 0x100))
			sel = sel + rows < n ? sel + rows : n - 1;
		else if (k == 27) {
			box_close();
			return prompt ? 27 : 0;
		} else if (k >= 1000) {
			i = k - 1000 - box_r0 - 1;
			if (i < 0 || i >= rows)
				continue;
			sel = top + i;
			o = items[sel];
			box_close();
			if (prompt)
				return i_to_l(o);
			if ((k = item_menu(o)))
				return k;
			return 0;
		} else if (prompt) {
			if (k == '\r' || k == ('R' | 0x100)) {
				box_close();
				return i_to_l(o);
			}
			if (k >= 0x100)
				continue;
			box_close();
			return k;	/* a letter or anything else: the game decides */
		} else if (k == '\r' || k == ' ' || k == ('R' | 0x100)) {
			box_close();
			return item_menu(o);
		} else if (k == ('+' | 0x100)) {
			box_close();
			if ((k = main_action(o)))
				return do_action(k, o);
			examine(o);
			return 0;
		} else if (k == ('-' | 0x100)) {
			box_close();
			return do_action('d', o);
		} else if (k == ('*' | 0x100)) {
			box_close();
			examine(o);
			return 0;
		} else if (k == ('L' | 0x100))
			continue;
		else if (k > 0 && k < 0x100 && isalpha(k) && (o = l_to_i(tolower(k))) != NULL
				&& isupper(k) && i_to_l(o) != k) {
			/* Shift+letter: drop (letters past z are A-Z in MAG) */
			box_close();
			return do_action('d', o);
		} else if (k > 0 && k < 0x100 && isalpha(k) && (o = l_to_i(k)) != NULL) {
			/* the item's letter: its main action */
			box_close();
			if ((k = main_action(o)))
				return do_action(k, o);
			examine(o);
			return 0;
		} else if (k > 0 && k < 27 && (o = l_to_i(k + 'a' - 1)) != NULL) {
			/* Ctrl+letter: examine */
			box_close();
			examine(o);
			return 0;
		} else {
			/* any other key is a normal command */
			box_close();
			return k < 0x100 ? k : 0;
		}
	}
}

int
port_inventory(void)
{
	if (!numinv) {
		pline("You are empty-handed.");
		return 0;
	}
	return item_list(DROPABLE, NULL);
}

/* utilize()'s "What do you want to ...?" (COMMAND1.C): the fitting items
 * with a cursor; returns the key the game should see */
int
port_item_prompt(int which, const char *verb)
{
	char q[80];
	int k;

	if ((k = port_pop_key()) >= 0)
		return k;	/* the inventory chose the item already */
	snprintf(q, sizeof q, "What do you want to %s?", verb);
	k = item_list(which, q);
	return k ? k : 27;
}
