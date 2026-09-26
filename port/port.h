/*
 * port.h - the DOS / Microsoft C 5 layer of MAG, for the RVIP port (2026)
 *
 * Force-included into every game file (-include port.h). MAG talks to the
 * PC through the BIOS (int86: video service 16, keyboard service 0x16),
 * conio (kbhit/getch), graph.h (_setvisualpage), dos.h (interrupt vectors,
 * _dos_findfirst) and DOS paths with backslashes. All of that ends up in
 * port/pcvideo.c: two 80x25 text pages of (attribute << 8 | CP437 char)
 * and a key queue, which a frontend (fe_web.c in the browser, fe_tty.c for
 * the native test build) shows and fills. The game sources stay as they are.
 */
#ifndef MAG_PORT_H
#define MAG_PORT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Microsoft C keywords */
#define far
#define near
#define huge
#define interrupt
#define cdecl

/* dos.h: BIOS register block */
struct WORDREGS { unsigned short ax, bx, cx, dx, si, di, cflag; };
struct BYTEREGS { unsigned char al, ah, bl, bh, cl, ch, dl, dh; };
union REGS { struct WORDREGS x; struct BYTEREGS h; };
int	int86(int intno, union REGS *in, union REGS *out);

/* interrupt vectors: nothing to hook (Ctrl-Break never reaches the game) */
typedef void (*port_vect)(void);
#define _dos_getvect(n)		((port_vect)0)
#define _dos_setvect(n, f)	((void)(f))

/* _dos_findfirst / _dos_findnext (SAVE.C clr_save: "save\maglevel.*") */
struct find_t { char name[64]; };
#define _A_NORMAL 0
int	_dos_findfirst(const char *pattern, unsigned attr, struct find_t *ft);
int	_dos_findnext(struct find_t *ft);

/* conio.h, graph.h */
int	kbhit(void);
int	getch(void);
short	_setvisualpage(short page);

/* process.h: no shell in the port ('~' says it can't find command.com) */
#define P_WAIT 0
#define spawnl(mode, path, ...)	(-1)

/* io.h */
#ifndef O_BINARY
#define O_BINARY 0
#endif
#ifndef S_IREAD
#define S_IREAD S_IRUSR
#define S_IWRITE S_IWUSR
#endif

/* DOS paths: "help\help.1", "save\maglevel.3" -> host paths */
const char *port_path(const char *dos);
#define fopen(p, m)	fopen(port_path(p), m)
#define open(p, ...)	open(port_path(p), __VA_ARGS__)
#define access(p, m)	access(port_path(p), m)
#define remove(p)	remove(port_path(p))

/* the BIOS tick counter at 0040:006C (THROW.C tick_tock) */
void	port_delay(int ticks);

/* program end goes through the frontend (web: sync saves, "Play again") */
void	port_exit(int code);
#define exit(c)		port_exit(c)

/* RVIP additions (port/rvip.c), called from src/ under #ifdef PORT */
void	port_idle(void);
int	port_save_ok(void);
void	port_remember(void);
int	port_auto(void);
int	port_command(int k);
int	port_item_prompt(int which, const char *verb);
/* port/menu.c */
void	port_push_key(int k);
int	port_pop_key(void);
int	port_menu(void);
void	port_msg(const char *m);	/* a top-line message (frontend: Messages window) */
int	port_auto_more(void);	/* skip the top-line =-More-= */
int	port_inventory(void);
extern int port_inv_again;

#endif
