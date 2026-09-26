/*
 *  		MAG - A Dungeon Adventuring Game
 *
 *	Copyright (C) 1986,87,88 by Michael J. Teixeira
 *
 *  General permission to copy or modify, but not for profit, is hereby
 *  granted, provided that the above copyright notice is included.
 *
 */

/* this file contains all of the procedure prototypes organized by file */

/* attack.c */
extern void	uattack(MONSTER *);
extern int	needtohit(int, int);
extern int	damage(char *);
extern char	*dparse(int, char *);
extern int	strbonus(void);
extern void	chg_hp(int);
extern void	die(void);
extern void	resurrect(void);

/* command1.c */
extern int	eat(OBJECT *);
extern int	wield(OBJECT *);
extern int	unwield(int);
extern int	wear(OBJECT *);
extern int      unwear(int);
extern int	call(OBJECT *);
extern int	drop(OBJECT *);
extern int	utilize(void);
extern int	smear(OBJECT *);

/* command2.c */
extern int	use(OBJECT *);
extern void	docross(void);
extern void	domirror(void);
extern int	light(OBJECT *);
extern int	unlock(OBJECT *);
extern int	strap(OBJECT *);
extern int	unstrap(void);
extern int	ignite(OBJECT *);
extern int	helpcmd(void);
extern int	helpsym(void);
extern void	domega(OBJECT *);
extern void	docompass(void);

/* command3.c */
extern int	version(void);
extern int	search(void);
extern void	dosearch(int);
extern int	stepup(void);
extern int	stepdown(void);
extern int	steps(int);
extern int	rest(void);
extern int	nothin(void);
extern int	howmoves(void);
extern int	stomach(void);
extern int	extinguish(void);
extern int	startmacro(void);
extern int	endmacro(void);
extern void	zapmacro(void);
extern int	execute(void);
extern int	exchange(void);
extern int	setsecwep(OBJECT *);
extern int	multirest(void);
extern int	multisearch(void);
extern int	saytitle(void);
extern int	getshell(void);
extern int	logbug(void);

/* door.c */
extern DOOR	*what_door(DUNGEON *);
extern int	mk_door(DUNGEON *, int);
extern DUNGEON	*wall_loc(int, ROOM *, int);
extern int	enterdoor(DOOR *);

/* event.c */
extern void	mk_event(unsigned int, int (*)());
extern void	updevent(void);
extern void	rm_event(int (*)());
extern int	doevent(int (*)());
extern int	noconmon(void);
extern int	noconfuse(void);
extern int	noblind(void);
extern int	nosleep(void);
extern int	nopara(void);
extern int	nospeed(void);
extern int	nostuck(void);
extern void	mkmespeedy(int);
extern void	mkmeinvis(int);
extern void	mkmeblind(int);
extern void	mkmeconfused(int);
extern int	fix_str(int);
extern int	nopoison(void);
extern int	noinvis(void);
extern void	mkmesleep(int);
extern void	mkmepara(int);
extern int	nohero(void);
extern int	fastsearch(void);
extern int	nopoly(void);
extern int	regen(void);
extern int	gethungry(void);
extern int	mkwandmon(void);
extern int	etele(void);
extern void	polymorph(int);
extern int	noseeinv(void);
extern int	nolevit(void);
extern int	changeall(void);
extern int	oilexplode(void);

/* invent1.c */
extern int	inventory(void);
extern int	pr_obj(int, int);
extern char	*obj_str(OBJECT *);
extern void	tackon(char *, char *);
extern char	*article(char *, int);
extern char	*capital(char *);
extern OBJECT	*add_invent(OBJECT *, int);
extern void	inc_iptrs(OBJECT *);
extern void	dec_iptrs(OBJECT *);

/* invent2.c */
extern char	*typename(OBJECT *);
extern char	*form(OBJECT *, int);
extern int	i_to_l(OBJECT *);
extern void	rm_inv(OBJECT *);
extern void	lessweight(OBJECT *, int);
extern int	carry(void);
extern int	get(void);
extern char	*possitems(int);
extern void	sealup(char *, int, int);
extern OBJECT	*l_to_i(char);
extern int	in_inv(int);
extern void	minusone(OBJECT *);

/* level.c */
extern void	mk_lev(void);
extern void	fix_maze(void);
extern void	mk_torch(void);
extern void	cl_dun(void);
extern void	mk_stair(void);
extern DUNGEON	*location(char *);
extern DUNGEON	*find_loc(unsigned char);
extern void	corridor(void);
extern int	cinbounds(int, int);
extern int	pickbit(long);
extern ROOM	*dig(ROOM *, ROOM *, int);
extern int	get_cdir(int, int, int, int, int);
extern void	r_to_maze(ROOM *);
extern void	make_maze(ROOM *, DUNGEON *);

/* main.c */
extern void	main(int, char **);
extern void	reset_kb(void);
extern int	ctgetch(void);
extern int	tgetch(void);
extern int	doquit(void);
extern int	getlower(void);
extern void	*tex_alloc(unsigned int);
extern void	tex_free(void *);
extern char	*commastr(long);
extern int	prfile(char *);
extern void	doexit(void);
extern void	startobj(void);
extern void	setupgame(void);
extern void	emptybuf(void);
extern void	init(void);
extern void	get_game(void);

/* mattack.c */
extern void	mattack(MONSTER *);
extern int	numattacks(char *);
extern void	firstatt(MONSTER *);
extern int	middleatt(MONSTER *);
extern void	afteratt(MONSTER *);
extern int	steal(MONSTER *);
extern int	clearshot(MONSTER *);
extern void	archer(MONSTER *);
extern void	zapmeplease(MONSTER *);
extern void	specact(MONSTER *);

/* misc.c */
extern void	newlev(int, unsigned char, int);
extern int	teleport(DUNGEON *);
extern void	mapping(void);
extern long	weight(int, int, int, int);
extern long	needexp(int);
extern void	ck_exp(void);
extern DUNGEON	*near_loc(DUNGEON *);
extern int	list_obj(void);
extern int	iscursed(OBJECT *, int);
extern int	gethand(OBJECT *, OBJECT *);
extern void	relevel(void);
extern long	getscore(int);
extern int	sapp_lev(void);
extern char	*get_str(int);
extern void	bell(void);

/* monster1.c */
extern struct permmon *mk_mon(DUNGEON *, int);
extern void	add_mon(MONSTER *);
extern void	move_mon(void);
extern void	inch_mon(MONSTER *);
extern int	canmmove(MONSTER *);
extern void	is_archer(MONSTER *);

/* monster2.c */
extern int	lookfordoor(MONSTER *, int);
extern DUNGEON	*endofcor(DUNGEON *, int);
extern void	rm_mon(MONSTER *);
extern void	add_minv(MONSTER *, OBJECT *);
extern void	rm_minv(MONSTER *, OBJECT *);
extern int	what_dir(DUNGEON *, DUNGEON *);
extern void	wakemon(MONSTER *);
extern MONSTER	*what_mon(DUNGEON *);
extern void	mstarto(MONSTER *);
extern int	pickmon(void);
extern void	lev_mon(void);
extern int	roll_hp(int);
extern void	pr_mon(void);
extern void	detmon(void);
extern void	mteleport(MONSTER *, DUNGEON *);
extern int	ck_mhp(MONSTER *, int);
extern void	dump_minv(MONSTER *);
extern void	allwakeup(void);
extern char	*mname(MONSTER *);
extern void	polymon(MONSTER *);

/* move.c */
extern void	dispatch(void);
extern int	multi(void);
extern int	illegal(void);
extern int	move(void);
extern void	stop(void);
extern int	walk(void);
extern int	todir(int);
extern int	run(void);
extern int	abrt(void);
extern int	find(void);
extern int	getdir(int);
extern int	dofollow(void);
extern int	nodiagmove(int, DUNGEON *);
extern DUNGEON	*mvindir(int, DUNGEON *, int);
extern int	shouldstop(void);

/* object.c */
extern void	pickup(DUNGEON *);
extern void	setupobj(void);
extern void	scramble(char **, int, int);
extern OBJECT	*create_o(int, int, int, int, int, int);
extern OBJECT	*mk_cursed(OBJECT *);
extern void	mk_levobj(void);
extern LEVOBJ	*what_obj(DUNGEON *);
extern void	rm_lobj(LEVOBJ *);
extern OBJECT	*copy_obj(OBJECT *, int);
extern OBJECT	*mk_treasure(void);
extern int	muchtreas(void);
extern OBJECT	*mk_obj(void);
extern int	getench(int);
extern int	getwench(int);
extern int	getquan(int);
extern void	add_lobj(DUNGEON *, OBJECT *);
extern int	obj_type(int);
extern int	getoff(int);
extern int	gettype(void);
extern int	ident(OBJECT *);
extern void	makeknown(OBJECT *);
extern void	decurse(void);
extern int	already(void);

/* option.c */
extern int	option(void);
extern void	printopts(void);
extern void	optfile(void);

/* potion.c */
extern int	quaff(OBJECT *);
extern void	usedmagic(OBJECT *, int);
extern void	healme(int, int);
extern int	detect(int);

/* ring.c */
extern int	puton(OBJECT *);
extern void	unring(int);
extern void	doring(OBJECT *, int);
extern int	tfinger(int);
extern int	lfinger(int);
extern int	rfinger(int);
extern int	lrfinger(int);

/* room.c */
extern void	lightroom(void);
extern void	fit_rooms(void);
extern int	rinbounds(ROOM *);
extern void	mk_pool(ROOM *, int);
extern ROOM	*what_room(DUNGEON *);
extern int	doroom(ROOM *, int);
extern void	spec_room(void);
extern DUNGEON	*room_loc(ROOM *);
extern int	room_side(int, int, ROOM *);
extern void	readyroom(ROOM *);
extern void	mk_trove(ROOM *);

/* save.c */
extern int	save_lev(void);
extern void	dowrite(void *, unsigned int);
extern void	doread(void *, unsigned int);
extern int	restore_lev(int);
extern int	remember(void);
extern int	rest_game(void);
extern void	clr_save(void);

/* scroll.c */
extern int	readscrl(OBJECT *);
extern void	attract(void);
extern void	genocide(void);
extern int	whatgeno(void);
extern struct permmon *l_to_p(char);
extern int	recharge(OBJECT *);
extern int	encharm(OBJECT *);
extern int	enchwep(OBJECT *);
extern int	galvanize(OBJECT *);
extern void	dolab(void);

/* throw.c */
extern int	throw(OBJECT *);
extern int	dothrow(MONSTER *, DUNGEON *, OBJECT *, int, int, int, int);
extern int	howfar(int);
extern int	throwdam(MONSTER *, OBJECT *, DUNGEON *);
extern void	explode(DUNGEON *);
extern void	tick_tock(int);

/* topten.c */
extern void	topten(long, char *);

/* trap.c */
extern TRAP	*what_trap(DUNGEON *);
extern void	dotrap(DUNGEON *, MONSTER *);
extern void	mk_traps(void);
extern int	avoid(void);
extern void	spiderweb(MONSTER *);
extern void	stun(MONSTER *);
extern void	physdam(void);
extern void	pendulum(MONSTER *);
extern void	weed(MONSTER *);
extern void	poisneed(MONSTER *);
extern void	sleepgas(MONSTER *);
extern void	bear(MONSTER *);
extern void	flyingspear(MONSTER *);
extern void	teletrap(MONSTER *);

/* visual1.c */
extern void	di_set(DUNGEON *);
extern void	swrite(char *);
extern void	doset(int, int);
extern void	cl_scr(void);
extern void	at(int, int, char);
extern void	dat(DUNGEON *, unsigned char);
extern void	oputc(unsigned char);
extern void	cl_eol(void);
extern void	pline(char *, ...);
extern void	more(int, int, int);
extern void	domore(int);
extern int	reprint(void);
extern void	erase(void);
extern void	pr_bline(void);
extern void	set_act(int);
extern void	set_vis(int);

/* visual2.c */
extern void	putwhat(DUNGEON *);
extern int	lookmon(MONSTER *);
extern void	redraw(void);
extern void	set_red(int, int, int);
extern void	red_box(int, int, int, int);
extern int	remap(void);
extern void	doremap(void);
extern int	view(void);
extern int	vinsight(DUNGEON *);
extern void	viewwait(DUNGEON *);
extern void	know(DUNGEON *, int);
extern int	insight(DUNGEON *);
extern int	inrange(DUNGEON *, DUNGEON *, int);
extern void	cursor(int);
extern void	setupscr(void);
extern char	obj_color(OBJECT *);

/* wand.c */
extern int	zap(OBJECT *);
extern void	dozap(int, DUNGEON *, int, MONSTER *);
extern int	bounce(int *, DUNGEON **);
extern void	wand_dam(int, MONSTER *, MONSTER *);
extern int	pr_wand(int, MONSTER *);
extern void	sdoordet(void);
extern char	*bname(int);
extern int	letsdig(DUNGEON *);
extern void	teleto(MONSTER *);
extern void	teleaway(MONSTER *);

/* wizard.c */
extern int	wizcheck(void);
extern int	wexp(void);
extern int	wdetect(void);
extern int	wmk_mon(void);
extern int	wmode(void);
extern int	wadvance(void);
extern int	wident(void);
extern int	wtorch(void);
extern int	wmap(void);
extern int	wtele(void);
extern int	wdettraps(void);
extern void	trapdet(void);
extern int	wmk_obj(void);
extern int	wmonstat(void);
extern int	wexpstat(void);
extern int	wmakewall(void);
extern int	wgeno(void);
