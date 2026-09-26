/*
 * fe.h - frontend interface of the MAG port (RVIP 2026)
 */
#ifndef FE_H
#define FE_H

/* keys that are not plain characters (fe_getkey) */
enum {
	FK_UP = 0x100, FK_DOWN, FK_LEFT, FK_RIGHT, FK_HOME, FK_END,
	FK_PGUP, FK_PGDN, FK_INS, FK_DEL,
	FK_KP0, FK_KP1, FK_KP2, FK_KP3, FK_KP4, FK_KP5, FK_KP6, FK_KP7,
	FK_KP8, FK_KP9, FK_KPDOT, FK_KPENTER, FK_KPPLUS, FK_KPMINUS,
	FK_KPSTAR, FK_KPSLASH,
	FK_F1, FK_F2, FK_F3, FK_F4, FK_F5, FK_F6, FK_F7, FK_F8, FK_F9,
	FK_F10,
	FK_CLICK,		/* mouse click on the text grid: fe_click_row/col */
	FK_WHEELUP, FK_WHEELDOWN
};

/* the PC text screen (pcvideo.c): two BIOS pages */
extern unsigned short vram[2][25][80];
extern int vis_page, cur_on, cur_row[2], cur_col[2];

void	fe_init(void);
void	fe_present(void);
int	fe_getkey(int msdelay);		/* -1 on timeout */
int	fe_kbhit(void);
void	fe_flush(void);
void	fe_sleep(int ms);
void	fe_exit(int code);
void	fe_sync(void);			/* files changed: write them back (web) */
double	fe_now(void);			/* milliseconds */
extern int fe_click_row, fe_click_col;
void	fe_popup(int r0, int c0, int r1, int c1);	/* a port pop-up box (r0 < 0: none) */

#endif
