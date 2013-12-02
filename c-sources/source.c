/*
 * TITLE:       tetris.cpp
 *
 * TOPIC:       A Tribute to TETRIS
 *		release v1.0
 *
 * METHOD:
 *
 * WRITTEN BY:  Matthew Versluys
 *
 * ORIGINAL:    10 July, 1995
 * CURRENT:     28 September, 1995
 */

#include <dos.h>
#include <time.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>

#include "xlib.hpp"
#include "flclib.hpp"
#include "tetris.hpp"
#include "joystick.hpp"
#include "sound.h"

short	inputstatus_0	= 0x3DA;

#define PREVIEW_FRAMES	5
#define PLAYER_1	-2
#define PLAYER_2	202

#define KEYBOARD	0
#define JOYSTICK1	1
#define JOYSTICK2	2
#define MOUSE		3

#define LEFT 		0x01
#define RIGHT 		0x02
#define DROP 		0x04
#define CLOCKWISE 	0x08
#define ANTICLOCKWISE 	0x10
#define END_GAME 	0x20
#define QUIT_TO_OS 	0x40


/*
 * Assembler Routines
 */
void WaitVsyncStart(void);
#pragma aux WaitVsyncStart =		\
	"mov     dx, inputstatus_0"	\
	"WaitNotVsync:"			\
	"in      al, dx"		\
	"test    al, 08h"		\
	"jnz     WaitNotVsync"		\
	"WaitVsync:"			\
	"in      al, dx"		\
	"test    al, 08h"		\
	"jz      WaitVsync"		\
	modify [ax dx];

char TestVsync(void);
#pragma aux TestVsync =			\
	"mov	dx, inputstatus_0"	\
	"in	al, dx"			\
	modify [ax dx]			\
	value [al];

short GrabTick(void);
#pragma aux GrabTick =			\
	"mov	ax, 0"			\
	"int	01Ah"			\
	modify [ax]			\
	value [dx];

#pragma aux setoverscan = 		\
	"mov	ax, 1001h"		\
	"mov	bh, 80h"		\
	"int	10h"			\
	modify [ax bx];

#pragma aux MODE13 =			\
	"mov	ax, 13h"		\
	"int	010h"			\
	modify [ax];

void SETPAL(char);
#pragma aux SETPAL =			\
	"mov	ax, 1000h"		\
	"mov	bh, bl"			\
	"int	010h"			\
	parm[bl]			\
	modify[ax bx];

#pragma aux ClearKeyBuffer =		\
	"StartTest:"			\
	"mov	ax, 01h"		\
	"int	016h"			\
	"jnz	BufferClear"		\
	"mov	ax, 00h"		\
	"int	016h"			\
	"jmp	StartTest"		\
	"BufferClear:"			\
	modify[ax];


/*
 * CopyVideoDown
 *
 * in  -> pointer to source
 *        pointer to destination
 *        number of lines to copy
 * out <- void
 *
 * Copies a column (100 wide) of video memory
 * using the latches in the VGA to copy 4-pixels
 * at a time.  This copy stats at the top and
 * works its way down.  Use it when the source
 * is lower than the destination if they overlap.
 */
void CopyVideoDown(char near *, char near *, short);
#pragma aux CopyVideoDown =		\
	"mov	dx, 03CEh"		\
	"mov	ax, 0008h"		\
	"out	dx, ax"			\
	"mov	dx, 03C4h"		\
	"mov	ax, 0F02h"		\
	"out	dx, ax"			\
	"mov	dx, cx"			\
	"CopyLine:"			\
	"mov	ecx, 25"		\
	"cld"				\
	"rep	movsb"			\
	"add	esi, 55"		\
	"add	edi, 55"		\
	"dec	dx"			\
	"jnz	CopyLine"		\
	"mov	dx, 03CFh"		\
	"mov	al, 0FFh"		\
	"out	dx, al"			\
	parm[esi] [edi] [ecx]		\
	modify[eax ecx edx esi edi];


/*
 * CopyVideoUp
 *
 * in  -> pointer to source
 *        pointer to destination
 *        number of lines to copy
 * out <- void
 *
 * Copies a column (100 wide) of video memory
 * using the latches in the VGA to copy 4-pixels
 * at a time.  This copy starts at the bottom and
 * works its way up.  Use it when the source
 * is above than the destination if they overlap.
 */
void CopyVideoUp(char near *, char near *, short);
#pragma aux CopyVideoUp =		\
	"mov	dx, 03CEh"		\
	"mov	ax, 0008h"		\
	"out	dx, ax"			\
	"mov	dx, 03C4h"		\
	"mov	ax, 0F02h"		\
	"out	dx, ax"			\
	"mov	dx, cx"			\
	"CopyLine:"			\
	"mov	ecx, 25"		\
	"cld"				\
	"rep	movsb"			\
	"sub	esi, 105"		\
	"sub	edi, 105"		\
	"dec	dx"			\
	"jnz	CopyLine"		\
	"mov	dx, 03CFh"		\
	"mov	al, 0FFh"		\
	"out	dx, al"			\
	parm[esi] [edi] [ecx]		\
	modify[eax ecx edx esi edi];


/*
 * ScreenCopy
 *
 * in  -> pointer to source
 *        pointer to destination
 * out <- void
 *
 * Copies a screen full of info from src to
 * dest very quickly.
 */
void ScreenCopy(char near *, char near *);
#pragma aux ScreenCopy =		\
	"mov	dx, 03CEh"		\
	"mov	ax, 0008h"		\
	"out	dx, ax"			\
	"mov	dx, 03C4h"		\
	"mov	ax, 0F02h"		\
	"out	dx, ax"			\
	"mov	dx, cx"			\
	"mov	ecx, 19200"		\
	"cld"				\
	"rep	movsb"			\
	"mov	dx, 03CFh"		\
	"mov	al, 0FFh"		\
	"out	dx, al"			\
	parm[esi] [edi] 		\
	modify[eax ecx edx esi edi];


/*
 * C Prototypes
 */
void	init(void);
void	load_menu(char);
void 	config_menu(void);
void	pause_game(void);
void	drawpiece(char, char, char, char);
int	testpiece(pit *, char, char, char, char);
int	testlines(pit *, char);
int	testline(pit *, char);
int	two_testline(pit *, char);
int	checkline(pit *, char);
void	splitpiece(char, char, char, char);
void	removepiece(char, char, char, char);
void	removeline(pit *, char);
void	two_removeline(pit *, char);
int	gimmepiece(void);
void	adjust_movie(char, char);
void	preview_pal(void);
void	plr1_preview(char);
void	plr2_preview(char);
void	centrestring(short, short, char *);
void	printstring(short, short, char *);
void	printstring_slow(short, short, char *);
void 	printnum(short, short, char *);
void	plr1_score(void);
void	plr2_score(void);
void	clear_stats(void);
void	update_stats(char);
void	empty_pit(pit);
void 	block(short, short, char, char, char);
void 	row(short, char, char, char);
void 	init_pit(char, char);
void 	collect_bonus(char *, char);
void	display_highscores(struct highscore_table *);
void 	game_over(void);
void	stygian_tetris(void);
void	arcade_tetris(void);
void	traditional_tetris(void);
void	competition_tetris(void);
void 	__interrupt __far	( *oldhandler)();
void 	__interrupt __far 	kbhandler(void);
void 				kbinst(void);
void 				kbdeinst(void);

/*
 * External Data
 */
extern unsigned short activeStart;
extern unsigned short visibleStart;
extern unsigned short pageSize;


/*
 * Global Data
 */
struct styxmovie_struct	piece_preview[7];
struct blitbuf		tetris;
struct palette		tetris_pal[256];
struct palette		tetris_faded_pal[256];
struct blitbuf		plrscr;
struct palette		plrscr_pal[256];
struct blitbuf		high_score_scrn;
struct palette		high_score_pal[256];
struct blitbuf		name_buf;
struct palette		pal[256];
struct blitbuf		joystick_scrn;
struct palette		joystick_scrn_pal[256];
struct blitbuf		keyboard_scrn;
struct palette		keyboard_scrn_pal[256];
char			lpal[768];
FILE		 	*data;
short			random[200];
signed short		player;
long			bonus[4];
char			font[8640];

long			p1score;
long			p2score;
short			p1lines;
short			p2lines;
short			p1level;
short			p2level;

char			volume;
char			keys[128];

char			anykey;
char			pause;
char			endgame;
char			quit;
char			quit_flag = 0;

char			plr1_device;
char			plr1_left;
char			plr1_right;
char			plr1_clock;
char			plr1_aclock;
char			plr1_drop;

char			plr2_device;
char			plr2_left;
char			plr2_right;
char			plr2_clock;
char			plr2_aclock;
char			plr2_drop;

unsigned short		jstick1_xmax;
unsigned short		jstick1_xmin;
unsigned short		jstick1_ymax;
unsigned short		jstick1_ymin;

unsigned short		jstick2_xmax;
unsigned short		jstick2_xmin;
unsigned short		jstick2_ymax;
unsigned short		jstick2_ymin;




/*
 * main -
 *
 *
 */
void main(int argc, char *argv[])
{
	char	menu_new;
	char	menu_old;
	char	flag_up = 0;
	char	flag_down = 0;
	char	flag_left = 0;
	char	flag_right = 0;

	kbinst();
	init();
	load_menu(0);
	load_menu(1);

	menu_new = 0;
	menu_old = 0;
	flag_up = 0;
	flag_down = 0;
	set_paletteX((unsigned char *) tetris_faded_pal, 0);
	boxX(50, 40, 269, 59, 255);

	for (;;) {
		if (keys[quit] && keys[56]) {
			goto EXIT;
		}
		if (keys[0x48] && (flag_up == 0)) {
			flag_up = 1;
			menu_old = menu_new;
			if (menu_new == 0) {
				menu_new = 6;
			} else {
				menu_new--;
			}
			WaitVsyncStart();
			ScreenCopy((char *) 0xA4B00, (char *) 0xA0000);
			boxX(50, 40 + menu_new * 20, 269, 59 + menu_new * 20, 255);
		}
		if (keys[0x48] == 0) {
			flag_up = 0;
		}
		if (keys[0x50] && (flag_down == 0)) {
			flag_down = 1;
			menu_old = menu_new;
			if (menu_new == 6) {
				menu_new = 0;
			} else {
				menu_new++;
			}
			WaitVsyncStart();
			ScreenCopy((char *) 0xA4B00, (char *) 0xA0000);
			boxX(50, 40 + menu_new * 20, 269, 59 + menu_new * 20, 255);
		}
		if (keys[0x50] == 0) {
			flag_down = 0;
		}
		if (keys[0x1C]) {
			keys[0x1C] = 0;
			switch(menu_new) {
				case 0:
					fade_to_black(tetris_faded_pal);
					SOUND_modsfx(volume);
					stygian_tetris();
					if (quit_flag)
						goto EXIT;
					load_menu(0);
					load_menu(1);
					set_paletteX((unsigned char *) tetris_faded_pal, 0);
					boxX(50, 40 + menu_new * 20, 269, 59 + menu_new * 20, 255);
					break;
				case 1:
					fade_to_black(tetris_faded_pal);
					SOUND_modsfx(volume);
					arcade_tetris();
					if (quit_flag)
						goto EXIT;
					load_menu(0);
					load_menu(1);
					set_paletteX((unsigned char *) tetris_faded_pal, 0);
					boxX(50, 40 + menu_new * 20, 269, 59 + menu_new * 20, 255);
					break;
				case 2:
					fade_to_black(tetris_faded_pal);
					SOUND_modsfx(volume);
					traditional_tetris();
					if (quit_flag)
						goto EXIT;
					load_menu(0);
					load_menu(1);
					set_paletteX((unsigned char *) tetris_faded_pal, 0);
					boxX(50, 40 + menu_new * 20, 269, 59 + menu_new * 20, 255);
					break;
				case 3:
					fade_to_black(tetris_faded_pal);
					SOUND_modsfx(volume);
					competition_tetris();
					if (quit_flag)
						goto EXIT;
					load_menu(0);
					load_menu(1);
					set_paletteX((unsigned char *) tetris_faded_pal, 0);
					boxX(50, 40 + menu_new * 20, 269, 59 + menu_new * 20, 255);
					break;
				case 4:
					fade_to_black(tetris_faded_pal);
					wide_bitblitX(0, &tetris);
					display_highscores(&stygian);
					fade_from_black(tetris_faded_pal);
					sleep(2);
					fade_to_black(tetris_faded_pal);
					wide_bitblitX(0, &tetris);
					display_highscores(&arcade);
					fade_from_black(tetris_faded_pal);
					sleep(2);
					fade_to_black(tetris_faded_pal);
					wide_bitblitX(0, &tetris);
					display_highscores(&traditional);
					fade_from_black(tetris_faded_pal);
					sleep(2);
					fade_to_black(tetris_faded_pal);
					load_menu(0);
					load_menu(1);
					set_paletteX((unsigned char *) tetris_faded_pal, 0);
					boxX(50, 40 + menu_new * 20, 269, 59 + menu_new * 20, 255);
					break;
				case 5:
					config_menu();
					load_menu(0);
					load_menu(1);
					boxX(50, 40 + menu_new * 20, 269, 59 + menu_new * 20, 255);
					break;

				case 6:
					fade_to_black(tetris_faded_pal);
					goto EXIT;
					break;
			}
		}
	}


	EXIT:

	SOUND_end();
	kbdeinst();
	set80x25();

	/*
	 * Save the high scores
	 */
	fseek(data, 609040, 0);
	fwrite(&arcade, sizeof(arcade), 1, data);
	fwrite(&traditional, sizeof(traditional), 1, data);
	fwrite(&stygian, sizeof(stygian), 1, data);
	fwrite(&competitive, sizeof(competitive), 1, data);

	/*
	 * Save the user settings
	 */
	fseek(data, 609960, 0);
	putc(plr1_device, data);
	putc(plr1_left, data);
	putc(plr1_right, data);
	putc(plr1_drop, data);
	putc(plr1_clock, data);
	putc(plr1_aclock, data);
	putc(plr2_device, data);
	putc(plr2_left, data);
	putc(plr2_right, data);
	putc(plr2_drop, data);
	putc(plr2_clock, data);
	putc(plr2_aclock, data);
	fwrite(&jstick1_xmax, 2, 1, data);
	fwrite(&jstick1_xmin, 2, 1, data);
	fwrite(&jstick1_ymax, 2, 1, data);
	fwrite(&jstick1_ymin, 2, 1, data);
	fwrite(&jstick2_xmax, 2, 1, data);
	fwrite(&jstick2_xmin, 2, 1, data);
	fwrite(&jstick2_ymax, 2, 1, data);
	fwrite(&jstick2_ymin, 2, 1, data);
	putc(volume, data);
	fclose(data);

	fprintf(stderr,"\nTETRIS\nv1.0\n\nby Matthew Versluys\nStygian Software\nCopyright 1995\n");
}


/*
 * init -
 *
 * in  -> void
 * out <- void
 *
 *
 */
void init(void)
{
	signed long	i;
	signed long	a;
	short		t;
	short		rot;
	char		*ptr;
	char		*ptr2;
	long		counter;

	SOUND_preinit();
	SOUND_init(volume);
	SOUND_modsfx(volume);
	fprintf(stderr,"\nTETRIS\nv1.0\n\nby Matthew Versluys\nStygian Software\nCopyright 1995\n");

	srand(GrabTick());

	/*
	 * Lets get some RANDOM number streams for latter usage
	 */
	for (i = 0; i < 200; i++) {
		t = rand() % 200;
		for (a = 0; a < i; a++) {
			if (t == random[a]) {
				t = rand() % 200;
				a = -1;
			}
		}
		random[i] = t;
	}

	data = fopen("tetris.dat", "rb+");


	/*
	 * Load the piece previews from the flic files
	 */
	piece_preview[0].type = PLANE_4;
	fseek(data, 310272, 0);
	load_flc(data, &piece_preview[0]);

	piece_preview[1].type = PLANE_4;
	fseek(data, 339100, 0);
	load_flc(data, &piece_preview[1]);
	adjust_movie(1, 32);

	piece_preview[2].type = PLANE_4;
	fseek(data, 370958, 0);
	load_flc(data, &piece_preview[2]);
	adjust_movie(2, 64);

	piece_preview[3].type = PLANE_4;
	fseek(data, 406476, 0);
	load_flc(data, &piece_preview[3]);
	adjust_movie(3, 96);

	piece_preview[4].type = PLANE_4;
	fseek(data, 445200, 0);
	load_flc(data, &piece_preview[4]);
	adjust_movie(4, 128);

	piece_preview[5].type = PLANE_4;
	fseek(data, 481892, 0);
	load_flc(data, &piece_preview[5]);
	adjust_movie(5, 160);

	piece_preview[6].type = PLANE_4;
	fseek(data, 517984, 0);
	load_flc(data, &piece_preview[6]);
	adjust_movie(6, 192);

	fseek(data, 555600, 0);
	for (i = 0; i < 7; i++) {
		for (rot = 0; rot < 4; rot++) {
			piece_map[i].bitmap[rot] = (char *) malloc(6400);
			fread(piece_map[i].bitmap[rot], 1600, 1, data);
		}
	}

	/*
	 * Load in the font
	 */
	fseek(data, 600400, 0);
	fread(font, sizeof(font), 1, data);

	/*
	 * Load in the high score tables
	 */
	fseek(data, 609040, 0);
	fread(&arcade, sizeof(arcade), 1, data);
	fread(&traditional, sizeof(traditional), 1, data);
	fread(&stygian, sizeof(stygian), 1, data);
	fread(&competitive, sizeof(competitive), 1, data);

	/*
	 * Load in the user settings
	 */
	fseek(data, 609960, 0);
	plr1_device = getc(data);
	plr1_left = getc(data);
	plr1_right = getc(data);
	plr1_drop = getc(data);
	plr1_clock = getc(data);
	plr1_aclock = getc(data);

	plr2_device = getc(data);
	plr2_left = getc(data);
	plr2_right = getc(data);
	plr2_drop = getc(data);
	plr2_clock = getc(data);
	plr2_aclock = getc(data);

	fread(&jstick1_xmax, 2, 1, data);
	fread(&jstick1_xmin, 2, 1, data);
	fread(&jstick1_ymax, 2, 1, data);
	fread(&jstick1_ymin, 2, 1, data);

	fread(&jstick2_xmax, 2, 1, data);
	fread(&jstick2_xmin, 2, 1, data);
	fread(&jstick2_ymax, 2, 1, data);
	fread(&jstick2_ymin, 2, 1, data);

	volume = getc(data);


	/*
	 * Allocate Memory for Screens
	 */
	alloc_blitbuf(&tetris, 320, 240);
	alloc_blitbuf(&plrscr, 320, 240);
	alloc_blitbuf(&name_buf, 148, 15);
	alloc_blitbuf(&high_score_scrn, 320, 240);
	alloc_blitbuf(&joystick_scrn, 320, 240);
	alloc_blitbuf(&keyboard_scrn, 320, 240);

	/*
	 * Load into mem plr1screen
	 */
	fseek(data, 155136, 0);
	fread(plrscr_pal, 768, 1, data);
	fread(plrscr.image, 76800, 1, data);

	/*
	 * Load the High Score Screen
	 */
	fread(high_score_pal, 768, 1, data);
	fread(high_score_scrn.image, 76800, 1, data);

	/*
	 * Load the Joystick and Keyboard Screens
	 */
	fseek(data, 609989, 0);
	fread(joystick_scrn_pal, 768, 1, data);
	fread(joystick_scrn.image, 76800, 1, data);
	fread(keyboard_scrn_pal, 768, 1, data);
	fread(keyboard_scrn.image, 76800, 1, data);

	/*
	 * Start the Sound
	 */
	SOUND_sfxmod(volume);

	/*
	 * Switching to GFX mode
	 */
	set320x240x256_X();

	/*
	 * Stygian Software Logo
	 */
	fseek(data, 0, 0);
	fread(tetris_pal, 768, 1, data);
	fread(tetris.image, 76800, 1, data);
	black_pal();
	wide_bitblitX(0, &tetris);
	fade_from_black(tetris_pal);
	sleep(2);

	/*
	 * Tetris title screen
	 */
	fade_to_white(tetris_pal);
	fread(tetris_pal, 768, 1, data);
	fread(tetris.image, 76800, 1, data);
	setDrawPage(0);
	wide_bitblitX(0, &tetris);
	setDrawPage(1);
	wide_bitblitX(0, &tetris);

	ptr = (char *) tetris_faded_pal;
	ptr2 = (char *) tetris_pal;
	for (i = 0; i < 768; i++)
		*(ptr++) = *(ptr2++) >> 1;
	tetris_faded_pal[255].red = 0x3F;
	tetris_faded_pal[255].green = 0x3F;
	tetris_faded_pal[255].blue = 0x3F;

	tetris_pal[255].red = 0x3F;
	tetris_pal[255].green = 0x3F;
	tetris_pal[255].blue = 0x3F;

	fade_from_white(tetris_pal);
	setDrawPage(0);

	/*
	 * Run through some well deserved credits
	 */

	anykey = 0;

	sleep(2);
	if (anykey)
		goto QUIT_IT;

	printstring_slow(12, 200, " A TRIBUTE TO TETRIS");
	sleep(2);
	if (anykey)
		goto QUIT_IT;

	WaitVsyncStart();
	ScreenCopy((char *) 0xA4B00, (char *) 0xA0000);
	sleep(1);
	printstring_slow(12, 200, " RELEASE");
	printstring_slow(12, 212, " VERSION 1.0");
	sleep(2);
	if (anykey)
		goto QUIT_IT;

	WaitVsyncStart();
	ScreenCopy((char *) 0xA4B00, (char *) 0xA0000);
	sleep(1);
	printstring_slow(12, 200, " BY");
	printstring_slow(12, 212, " MATTHEW VERSLUYS");
	sleep(2);
	if (anykey)
		goto QUIT_IT;

	WaitVsyncStart();
	ScreenCopy((char *) 0xA4B00, (char *) 0xA0000);
	sleep(1);
	printstring_slow(12, 200, " ORIGINAL CONCEPT");
	printstring_slow(12, 212, " ALEXY PAZHITNOV");
	sleep(2);
	if (anykey)
		goto QUIT_IT;

	WaitVsyncStart();
	ScreenCopy((char *) 0xA4B00, (char *) 0xA0000);
	sleep(1);
	printstring_slow(12, 200, " ORIGINAL PROGRAMMER");
	printstring_slow(12, 212, " VADIM GERASIMOV");
	sleep(2);
	if (anykey)
		goto QUIT_IT;

	WaitVsyncStart();
	ScreenCopy((char *) 0xA4B00, (char *) 0xA0000);
	sleep(1);
	printstring_slow(12, 200, " ARCADE VERSION");
	printstring_slow(12, 212, " ATARI GAMES CORPORATION");
	sleep(2);
	if (anykey)
		goto QUIT_IT;

	WaitVsyncStart();
	ScreenCopy((char *) 0xA4B00, (char *) 0xA0000);
	sleep(1);
	printstring_slow(12, 200, " COMPILED WITH");
	printstring_slow(12, 212, " WATCOM C 10.0");
	sleep(2);
	if (anykey)
		goto QUIT_IT;

	WaitVsyncStart();
	ScreenCopy((char *) 0xA4B00, (char *) 0xA0000);
	sleep(1);
	printstring_slow(12, 200, " PMODE 1.20");
	printstring_slow(12, 212, " T.PYTEL & C.SCHEFFOLD");
	sleep(2);
	if (anykey)
		goto QUIT_IT;

	WaitVsyncStart();
	ScreenCopy((char *) 0xA4B00, (char *) 0xA0000);
	sleep(1);
	printstring_slow(12, 200, " MIKMOD 2.03");
	printstring_slow(12, 212, " HARDCORE '95");
	sleep(2);
	if (anykey)
		goto QUIT_IT;

	WaitVsyncStart();
	ScreenCopy((char *) 0xA4B00, (char *) 0xA0000);
	sleep(1);
	printstring_slow(12, 200, " FLILIB");
	printstring_slow(12, 212, " IVO BOSTICKY");
	sleep(2);
	if (anykey)
		goto QUIT_IT;

	WaitVsyncStart();
	ScreenCopy((char *) 0xA4B00, (char *) 0xA0000);
	sleep(1);
	printstring_slow(12, 200, " POV-RAY 2.2");
	printstring_slow(12, 212, " THE POV-TEAM");
	sleep(2);
	if (anykey)
		goto QUIT_IT;

	WaitVsyncStart();
	ScreenCopy((char *) 0xA4B00, (char *) 0xA0000);
	sleep(1);
	printstring_slow(12, 200, " DISPLAY 1.87");
	printstring_slow(12, 212, " JIH-SHIN HO");
	sleep(2);
	if (anykey)
		goto QUIT_IT;



	WaitVsyncStart();
	ScreenCopy((char *) 0xA4B00, (char *) 0xA0000);
	sleep(1);
	printstring_slow(12, 200, " YOU ARE LISTENING TO");
	printstring_slow(12, 212, " ICE FRONTIER");
	sleep(2);
	if (anykey)
		goto QUIT_IT;

	WaitVsyncStart();
	ScreenCopy((char *) 0xA4B00, (char *) 0xA0000);
	sleep(1);
	printstring_slow(12, 200, " COMPOSED AND TRACKED BY");
	printstring_slow(12, 212, " SKAVEN - FC");
	sleep(2);

QUIT_IT:
	WaitVsyncStart();
	ScreenCopy((char *) 0xA4B00, (char *) 0xA0000);
	fade_to_black(tetris_pal);
}


/*
 * load_menu -
 *
 * in  -> menu number
 * out <- void
 *
 */
void load_menu(char m)
{
	char	*ptr;
	char	*ptr2;
	short	i;

	switch(m) {
		case 0:
			setDrawPage(0);
			wide_bitblitX(0, &tetris);
			setDrawPage(1);
			wide_bitblitX(0, &tetris);
			setDrawPage(2);
			wide_bitblitX(0, &tetris);
			setDrawPage(0);
			break;
		case 1:
			for(i = 0; i < 2; i++) {
				setDrawPage(i);
				centrestring(160,  45, "STYGIAN TETRIS");
				centrestring(160,  65, "ARCADE TETRIS");
				centrestring(160,  85, "TRADITIONAL TETRIS");
				centrestring(160, 105, "COMPETITIVE TETRIS");
				centrestring(160, 125, "HIGH SCORES");
				centrestring(160, 145, "CONFIGURATION");
				centrestring(160, 165, "QUIT TO OS");
			}
			setDrawPage(0);
			break;
		case 2:
			for(i = 0; i < 3; i++) {
				setDrawPage(i);
				centrestring(160,  15, "PLAYER ONE DEVICE");
				centrestring( 80,  35, "KEYBOARD");
				centrestring(160,  35, "JOYSTICK1");
				centrestring(240,  35, "JOYSTICK2");
				centrestring(160,  55, "CALIBRATE");
				centrestring(160,  85, "PLAYER TWO DEVICE");
				centrestring( 80, 105, "KEYBOARD");
				centrestring(160, 105, "JOYSTICK1");
				centrestring(240, 105, "JOYSTICK2");
				centrestring(160, 125, "CALIBRATE");
				centrestring(160, 155, "VOLUME");
				lineX(50, 185, 269, 185, 255);
				centrestring(160, 205, "RETURN TO MAIN MENU");
			}
			setDrawPage(0);

			break;
		default:
			break;
	}
}


/*
 * gimme_key -
 *
 * in  -> void
 * out <- scan code of key
 *
 * � Firstly waits until no keys are pressed
 * � Then gets the first key pressed
 * � Return the scancode of this key to the caller
 * � If SHIFT (left or right) is also down, then
 *   set the highest bit of the scan code to 1.
 */
char gimme_key(void)
{
	char 	i;
	long	l;
	char	shift;

UMP_UM:
	for (i = 0; i < 128; i++) {
		if ((keys[i] != 0) && (i != 0x2A) && (i != 0x36))
			goto UMP_UM;
	}

	anykey = 0;
	while (anykey == 0)
		;
	shift = 0;
DOH:
	if (keys[0x2A] || keys[0x36])
		shift = 0x80;

	for (i = 0; i < 128; i++)
		if ((i != 0x2A) && (i != 0x36) && keys[i])
			return(i | shift);
	goto DOH;
}


/*
 * wait_for_empty -
 *
 * in  -> void
 * out <- void
 *
 * � wait till there are not keys pressed !
 */
void wait_for_empty(void)
{
	char	i;

UMP_UM:
	for (i = 0; i < 128; i++) {
		if (keys[i] != 0)
			goto UMP_UM;
	}
}


/*
 * config_menu -
 *
 */
void config_menu(void)
{
	char	i;
	long	l;
	char 	menu_new;
	char 	menu_old;
	char 	flag_up;
	char 	flag_down;
	char	flag_left;
	char	flag_right;
	char	flag_enter;
	short	menu_pos[6] = { 10, 50, 80, 120, 150, 200 };
	short	device_pos[3] = { 39, 117, 197 };
	short	volume_cursor;

	unsigned short	jvalxmax;
	unsigned short	jvalxmin;
	unsigned short	jvalxcen;
	unsigned short	jvalymax;
	unsigned short	jvalymin;
	unsigned short	jvalycen;

	load_menu(0);
	load_menu(2);

	menu_new = 0;
	menu_old = 0;
	flag_up = 0;
	flag_down = 0;
	flag_left = 0;
	flag_right = 0;
	flag_enter = 0;

	boxX(50, menu_pos[menu_new], 269, menu_pos[menu_new] + 19, 255);

	for (i = 0; i < 2; i++) {
		setDrawPage(i);
		filledboxX(device_pos[plr1_device], 36, device_pos[plr1_device] + 6, 42, 255);
		filledboxX(device_pos[plr2_device], 106, device_pos[plr2_device] + 6, 112, 255);
	}

	volume_cursor = (volume << 1) + 58;
	setDrawPage(1);
	filledboxX(volume_cursor, 181, volume_cursor + 4, 189, 0xFF);
	setDrawPage(0);
	filledboxX(volume_cursor, 181, volume_cursor + 4, 189, 0xFF);

	setDrawPage(0);
	for (;;) {
		if (keys[0x48] && (flag_up == 0)) {
			flag_up = 1;
			menu_old = menu_new;
			if (menu_new == 0) {
				menu_new = 5;
			} else {
				menu_new--;
			}
			WaitVsyncStart();
			ScreenCopy((char *) 0xA4B00, (char *) 0xA0000);
			boxX(50, menu_pos[menu_new], 269, menu_pos[menu_new] + 19, 255);
		}
		if (keys[0x48] == 0) {
			flag_up = 0;
		}
		if (keys[0x50] && (flag_down == 0)) {
			flag_down = 1;
			menu_old = menu_new;
			if (menu_new == 5) {
				menu_new = 0;
			} else {
				menu_new++;
			}
			WaitVsyncStart();
			ScreenCopy((char *) 0xA4B00, (char *) 0xA0000);
			boxX(50, menu_pos[menu_new], 269, menu_pos[menu_new] + 19, 255);
		}
		if (keys[0x50] == 0) {
			flag_down = 0;
		}
		if (keys[0x4B] && (flag_left == 0)) {
			flag_left = 1;
			if (menu_new == 0) {
				setDrawPage(2);
				aligned_getblitX(device_pos[plr1_device], 36, &name_buf);
				setDrawPage(1);
				aligned_bitblitX(device_pos[plr1_device], 36, &name_buf);
				setDrawPage(0);
				aligned_bitblitX(device_pos[plr1_device], 36, &name_buf);
				switch(plr1_device) {
					case KEYBOARD:
						if (plr2_device == JOYSTICK2)
							plr1_device = JOYSTICK1;
						else
							plr1_device = JOYSTICK2;
						break;
					case JOYSTICK1:
						plr1_device = KEYBOARD;
						break;
					case JOYSTICK2:
						if (plr2_device == JOYSTICK1)
							plr1_device = KEYBOARD;
						else
							plr1_device = JOYSTICK1;
						break;
				}
				setDrawPage(1);
				filledboxX(device_pos[plr1_device], 36, device_pos[plr1_device] + 6, 42, 255);
				setDrawPage(0);
				filledboxX(device_pos[plr1_device], 36, device_pos[plr1_device] + 6, 42, 255);
			}
			if (menu_new == 2) {
				setDrawPage(2);
				aligned_getblitX(device_pos[plr2_device], 106, &name_buf);
				setDrawPage(1);
				aligned_bitblitX(device_pos[plr2_device], 106, &name_buf);
				setDrawPage(0);
				aligned_bitblitX(device_pos[plr2_device], 106, &name_buf);
				switch(plr2_device) {
					case KEYBOARD:
						if (plr1_device == JOYSTICK2)
							plr2_device = JOYSTICK1;
						else
							plr2_device = JOYSTICK2;
						break;
					case JOYSTICK1:
						plr2_device = KEYBOARD;
						break;
					case JOYSTICK2:
						if (plr1_device == JOYSTICK1)
							plr2_device = KEYBOARD;
						else
							plr2_device = JOYSTICK1;
						break;
				}
				setDrawPage(1);
				filledboxX(device_pos[plr2_device], 106, device_pos[plr2_device] + 6, 112, 255);
				setDrawPage(0);
				filledboxX(device_pos[plr2_device], 106, device_pos[plr2_device] + 6, 112, 255);
			}
			if (menu_new == 4) {
				flag_left = 0;
				if (volume != 0) {
					volume_cursor = (volume << 1) + 58;
					setDrawPage(2);
					aligned_getblitX(volume_cursor, 181, &name_buf);
					setDrawPage(1);
					aligned_bitblitX(volume_cursor, 181, &name_buf);
					setDrawPage(0);
					volume--;
					SOUND_modvol(volume);
					volume_cursor = (volume << 1) + 58;
					WaitVsyncStart();
					aligned_bitblitX(volume_cursor + 2, 181, &name_buf);
					filledboxX(volume_cursor, 181, volume_cursor + 4, 189, 0xFF);
					setDrawPage(1);
					filledboxX(volume_cursor, 181, volume_cursor + 4, 189, 0xFF);
					setDrawPage(0);
				}
			}
		}
		if (keys[0x4B] == 0) {
			flag_left = 0;
		}
		if (keys[0x4D] && (flag_right == 0)) {
			flag_right = 1;
			if (menu_new == 0) {
				setDrawPage(2);
				aligned_getblitX(device_pos[plr1_device], 36, &name_buf);
				setDrawPage(1);
				aligned_bitblitX(device_pos[plr1_device], 36, &name_buf);
				setDrawPage(0);
				aligned_bitblitX(device_pos[plr1_device], 36, &name_buf);
				switch(plr1_device) {
					case KEYBOARD:
						if (plr2_device == JOYSTICK1)
							plr1_device = JOYSTICK2;
						else
							plr1_device = JOYSTICK1;
						break;
					case JOYSTICK2:
						plr1_device = KEYBOARD;
						break;
					case JOYSTICK1:
						if (plr2_device == JOYSTICK2)
							plr1_device = KEYBOARD;
						else
							plr1_device = JOYSTICK2;
						break;
				}
				setDrawPage(1);
				filledboxX(device_pos[plr1_device], 36, device_pos[plr1_device] + 6, 42, 255);
				setDrawPage(0);
				filledboxX(device_pos[plr1_device], 36, device_pos[plr1_device] + 6, 42, 255);
			}
			if (menu_new == 2) {
				setDrawPage(2);
				aligned_getblitX(device_pos[plr2_device], 106, &name_buf);
				setDrawPage(1);
				aligned_bitblitX(device_pos[plr2_device], 106, &name_buf);
				setDrawPage(0);
				aligned_bitblitX(device_pos[plr2_device], 106, &name_buf);
				switch(plr2_device) {
					case KEYBOARD:
						if (plr1_device == JOYSTICK1)
							plr2_device = JOYSTICK2;
						else
							plr2_device = JOYSTICK1;
						break;
					case JOYSTICK2:
						plr2_device = KEYBOARD;
						break;
					case JOYSTICK1:
						if (plr1_device == JOYSTICK2)
							plr2_device = KEYBOARD;
						else
							plr2_device = JOYSTICK2;
						break;
				}
				setDrawPage(1);
				filledboxX(device_pos[plr2_device], 106, device_pos[plr2_device] + 6, 112, 255);
				setDrawPage(0);
				filledboxX(device_pos[plr2_device], 106, device_pos[plr2_device] + 6, 112, 255);
			}
			if (menu_new == 4) {
				flag_right = 0;
				if (volume < 100) {
					volume_cursor = (volume << 1) + 58;
					setDrawPage(2);
					aligned_getblitX(volume_cursor - 2, 181, &name_buf);
					setDrawPage(1);
					aligned_bitblitX(volume_cursor - 2, 181, &name_buf);
					setDrawPage(0);
					volume++;
					SOUND_modvol(volume);
					volume_cursor = (volume << 1) + 58;
					WaitVsyncStart();
					aligned_bitblitX(volume_cursor - 4, 181, &name_buf);
					filledboxX(volume_cursor, 181, volume_cursor + 4, 189, 0xFF);
					setDrawPage(1);
					filledboxX(volume_cursor, 181, volume_cursor + 4, 189, 0xFF);
					setDrawPage(0);
				}
			}
		}
		if (keys[0x4D] == 0) {
			flag_right = 0;
		}
		if (keys[0x1C] && (flag_enter == 0)) {
			flag_enter = 1;
			switch(menu_new) {
				case 0:
					break;
				case 1:
					WaitVsyncStart();
					fade_to_black(tetris_faded_pal);
					switch(plr1_device) {
						case KEYBOARD:
							wide_bitblitX(0, &keyboard_scrn);
							set_paletteX((unsigned char *) keyboard_scrn_pal, 0);
							centrestring(160, 20, "PLAYER ONE");

							centrestring(160, 40, "KEYBOARD SELCTION");
							centrestring(160, 60, "USE DELIBERATE AND SUSTAINED PRESSES");
							wait_for_empty();
							centrestring(160, 100, "PRESS KEY FOR LEFT");
							plr1_left = gimme_key();
							centrestring(160, 120, "PRESS KEY FOR RIGHT");
							plr1_right = gimme_key();
							centrestring(160, 140, "PRESS KEY FOR DROP");
							plr1_drop = gimme_key();
							centrestring(160, 160, "PRESS KEY FOR CLOCKWISE");
							plr1_clock = gimme_key();
							centrestring(160, 180, "PRESS KEY FOR ANTI-CLOCKWISE");
							plr1_aclock = gimme_key();

							fade_to_black(keyboard_scrn_pal);
							break;
						case JOYSTICK1:
							wide_bitblitX(0, &joystick_scrn);
							set_paletteX((unsigned char *) joystick_scrn_pal, 0);
							centrestring(160, 20, "PLAYER ONE");

							centrestring(160, 40, "JOYSTICK1 CALIBRATION");
							if (time_ja_x() == 0xFFFF) {
								centrestring(160, 80, "SORRY, JOYSTICK 1 NOT FOUND");
								centrestring(160, 100, "CHECK CONNECTIONS AND TRY AGAIN");
								sleep(4);
								fade_to_black(joystick_scrn_pal);
								set_paletteX((unsigned char *) tetris_faded_pal, 0);
								break;
							}
							centrestring(160,  80, "MOVE JOYSTICK TO TOP-LEFT");
							centrestring(160,  95, "AND PRESS ANY BUTTON");
							for (l = 0; l < 100; l++) {
								while (~inp(0x201) & 0x30)
									;
							}
							for (l = 0; l < 100; l++) {
							while ((inp(0x201) & 0x10) && (inp(0x201) & 0x20))
								;
							}
							pretest();
							jvalxmin = time_ja_x();
							pretest();
							jvalymin = time_ja_y();
							centrestring(160, 130, "MOVE JOYSTICK TO BOTTOM-RIGHT");
							centrestring(160, 145, "AND PRESS ANY BUTTON");
							for (l = 0; l < 100; l++) {
								while (~inp(0x201) & 0x30)
									;
							}
							for (l = 0; l < 100; l++) {
							while ((inp(0x201) & 0x10) && (inp(0x201) & 0x20))
								;
							}
							pretest();
							jvalxmax = time_ja_x();
							pretest();
							jvalymax = time_ja_y();
							centrestring(160, 180, "CENTRE THE JOYSTICK");
							centrestring(160, 195, "AND PRESS ANY BUTTON");
							for (l = 0; l < 100; l++) {
								while (~inp(0x201) & 0x30)
									;
							}
							for (l = 0; l < 100; l++) {
							while ((inp(0x201) & 0x10) && (inp(0x201) & 0x20))
								;
							}
							pretest();
							jvalxcen = time_ja_x();
							pretest();
							jvalycen = time_ja_y();
							jstick1_xmax = (jvalxmax - jvalxcen) / 2 + jvalxcen;
							jstick1_xmin = (jvalxcen - jvalxmin) / 2 + jvalxmin;
							jstick1_ymax = (jvalymax - jvalycen) / 2 + jvalycen;
							jstick1_ymin = (jvalycen - jvalymin) / 2 + jvalymin;

							fade_to_black(joystick_scrn_pal);
							break;
						case JOYSTICK2:
							wide_bitblitX(0, &joystick_scrn);
							set_paletteX((unsigned char *) joystick_scrn_pal, 0);
							centrestring(160, 20, "PLAYER ONE");

							centrestring(160, 40, "JOYSTICK2 CALIBRATION");
							if (time_jb_x() == 0xFFFF) {
								centrestring(160, 80, "SORRY, JOYSTICK 2 NOT FOUND");
								centrestring(160, 100, "CHECK CONNECTIONS AND TRY AGAIN");
								sleep(4);
								fade_to_black(joystick_scrn_pal);
								set_paletteX((unsigned char *) tetris_faded_pal, 0);
								break;
							}
							centrestring(160,  80, "MOVE JOYSTICK TO TOP-LEFT");
							centrestring(160,  95, "AND PRESS ANY BUTTON");
							for (l = 0; l < 100; l++) {
								while (~inp(0x201) & 0xC0)
									;
							}
							for (l = 0; l < 100; l++) {
							while ((inp(0x201) & 0x40) && (inp(0x201) & 0x80))
								;
							}
							pretest();
							jvalxmin = time_jb_x();
							pretest();
							jvalymin = time_jb_y();
							centrestring(160, 130, "MOVE JOYSTICK TO BOTTOM-RIGHT");
							centrestring(160, 145, "AND PRESS ANY BUTTON");
							for (l = 0; l < 100; l++) {
								while (~inp(0x201) & 0xC0)
									;
							}
							for (l = 0; l < 100; l++) {
							while ((inp(0x201) & 0x40) && (inp(0x201) & 0x80))
								;
							}
							pretest();
							jvalxmax = time_jb_x();
							pretest();
							jvalymax = time_jb_y();
							centrestring(160, 180, "CENTRE THE JOYSTICK");
							centrestring(160, 195, "AND PRESS ANY BUTTON");
							for (l = 0; l < 100; l++) {
								while (~inp(0x201) & 0xC0)
									;
							}
							for (l = 0; l < 100; l++) {
							while ((inp(0x201) & 0x40) && (inp(0x201) & 0x80))
								;
							}
							pretest();
							jvalxcen = time_jb_x();
							pretest();
							jvalycen = time_jb_y();
							jstick2_xmax = (jvalxmax - jvalxcen) / 2 + jvalxcen;
							jstick2_xmin = (jvalxcen - jvalxmin) / 2 + jvalxmin;
							jstick2_ymax = (jvalymax - jvalycen) / 2 + jvalycen;
							jstick2_ymin = (jvalycen - jvalymin) / 2 + jvalymin;

							fade_to_black(joystick_scrn_pal);
							break;
					}
					WaitVsyncStart();
					ScreenCopy((char *) 0xA4B00, (char *) 0xA0000);
					set_paletteX((unsigned char *) tetris_faded_pal, 0);
					boxX(50, menu_pos[menu_new], 269, menu_pos[menu_new] + 19, 255);
					break;
				case 2:
					break;
				case 3:
					WaitVsyncStart();
					fade_to_black(tetris_faded_pal);
					switch(plr2_device) {
						case KEYBOARD:
							wide_bitblitX(0, &keyboard_scrn);
							set_paletteX((unsigned char *) keyboard_scrn_pal, 0);
							centrestring(160, 20, "PLAYER TWO");

							centrestring(160, 40, "KEYBOARD SELCTION");
							centrestring(160, 60, "USE DELIBERATE AND SUSTAINED PRESSES");
							wait_for_empty();
							centrestring(160, 100, "PRESS KEY FOR LEFT");
							plr2_left = gimme_key();
							centrestring(160, 120, "PRESS KEY FOR RIGHT");
							plr2_right = gimme_key();
							centrestring(160, 140, "PRESS KEY FOR DROP");
							plr2_drop = gimme_key();
							centrestring(160, 160, "PRESS KEY FOR CLOCKWISE");
							plr2_clock = gimme_key();
							centrestring(160, 180, "PRESS KEY FOR ANTI-CLOCKWISE");
							plr2_aclock = gimme_key();

							fade_to_black(keyboard_scrn_pal);
							break;
						case JOYSTICK1:
							wide_bitblitX(0, &joystick_scrn);
							set_paletteX((unsigned char *) joystick_scrn_pal, 0);
							centrestring(160, 20, "PLAYER TWO");

							centrestring(160, 40, "JOYSTICK1 CALIBRATION");
							if (time_ja_x() == 0xFFFF) {
								centrestring(160, 80, "SORRY, JOYSTICK 1 NOT FOUND");
								centrestring(160, 100, "CHECK CONNECTIONS AND TRY AGAIN");
								sleep(4);
								fade_to_black(joystick_scrn_pal);
								set_paletteX((unsigned char *) tetris_faded_pal, 0);
								break;
							}
							centrestring(160,  80, "MOVE JOYSTICK TO TOP-LEFT");
							centrestring(160,  95, "AND PRESS ANY BUTTON");
							for (l = 0; l < 100; l++) {
								while (~inp(0x201) & 0x30)
									;
							}
							for (l = 0; l < 100; l++) {
							while ((inp(0x201) & 0x10) && (inp(0x201) & 0x20))
								;
							}
							pretest();
							jvalxmin = time_ja_x();
							pretest();
							jvalymin = time_ja_y();
							centrestring(160, 130, "MOVE JOYSTICK TO BOTTOM-RIGHT");
							centrestring(160, 145, "AND PRESS ANY BUTTON");
							for (l = 0; l < 100; l++) {
								while (~inp(0x201) & 0x30)
									;
							}
							for (l = 0; l < 100; l++) {
							while ((inp(0x201) & 0x10) && (inp(0x201) & 0x20))
								;
							}
							pretest();
							jvalxmax = time_ja_x();
							pretest();
							jvalymax = time_ja_y();
							centrestring(160, 180, "CENTRE THE JOYSTICK");
							centrestring(160, 195, "AND PRESS ANY BUTTON");
							for (l = 0; l < 100; l++) {
								while (~inp(0x201) & 0x30)
									;
							}
							for (l = 0; l < 100; l++) {
							while ((inp(0x201) & 0x10) && (inp(0x201) & 0x20))
								;
							}
							pretest();
							jvalxcen = time_ja_x();
							pretest();
							jvalycen = time_ja_y();
							jstick1_xmax = (jvalxmax - jvalxcen) / 2 + jvalxcen;
							jstick1_xmin = (jvalxcen - jvalxmin) / 2 + jvalxmin;
							jstick1_ymax = (jvalymax - jvalycen) / 2 + jvalycen;
							jstick1_ymin = (jvalycen - jvalymin) / 2 + jvalymin;

							fade_to_black(joystick_scrn_pal);
							break;
						case JOYSTICK2:
							wide_bitblitX(0, &joystick_scrn);
							set_paletteX((unsigned char *) joystick_scrn_pal, 0);
							centrestring(160, 20, "PLAYER TWO");

							centrestring(160, 40, "JOYSTICK2 CALIBRATION");
							if (time_jb_x() == 0xFFFF) {
								centrestring(160, 80, "SORRY, JOYSTICK 2 NOT FOUND");
								centrestring(160, 100, "CHECK CONNECTIONS AND TRY AGAIN");
								sleep(4);
								fade_to_black(joystick_scrn_pal);
								set_paletteX((unsigned char *) tetris_faded_pal, 0);
								break;
							}
							centrestring(160,  80, "MOVE JOYSTICK TO TOP-LEFT");
							centrestring(160,  95, "AND PRESS ANY BUTTON");
							for (l = 0; l < 100; l++) {
								while (~inp(0x201) & 0xC0)
									;
							}
							for (l = 0; l < 100; l++) {
							while ((inp(0x201) & 0x40) && (inp(0x201) & 0x80))
								;
							}
							pretest();
							jvalxmin = time_jb_x();
							pretest();
							jvalymin = time_jb_y();
							centrestring(160, 130, "MOVE JOYSTICK TO BOTTOM-RIGHT");
							centrestring(160, 145, "AND PRESS ANY BUTTON");
							for (l = 0; l < 100; l++) {
								while (~inp(0x201) & 0xC0)
									;
							}
							for (l = 0; l < 100; l++) {
							while ((inp(0x201) & 0x40) && (inp(0x201) & 0x80))
								;
							}
							pretest();
							jvalxmax = time_jb_x();
							pretest();
							jvalymax = time_jb_y();
							centrestring(160, 180, "CENTRE THE JOYSTICK");
							centrestring(160, 195, "AND PRESS ANY BUTTON");
							for (l = 0; l < 100; l++) {
								while (~inp(0x201) & 0xC0)
									;
							}
							for (l = 0; l < 100; l++) {
							while ((inp(0x201) & 0x40) && (inp(0x201) & 0x80))
								;
							}
							pretest();
							jvalxcen = time_jb_x();
							pretest();
							jvalycen = time_jb_y();
							jstick2_xmax = (jvalxmax - jvalxcen) / 2 + jvalxcen;
							jstick2_xmin = (jvalxcen - jvalxmin) / 2 + jvalxmin;
							jstick2_ymax = (jvalymax - jvalycen) / 2 + jvalycen;
							jstick2_ymin = (jvalycen - jvalymin) / 2 + jvalymin;

							fade_to_black(joystick_scrn_pal);
							break;
					}
					WaitVsyncStart();
					ScreenCopy((char *) 0xA4B00, (char *) 0xA0000);
					set_paletteX((unsigned char *) tetris_faded_pal, 0);
					boxX(50, menu_pos[menu_new], 269, menu_pos[menu_new] + 19, 255);
					break;
				case 4:
					break;
				case 5:
					keys[0x1C] = 0;
					return;
					break;
			}
		}
		if (keys[0x1C] == 0) {
			flag_enter = 0;
		}
	}
}


/*
 * pause_game -
 *
 * in  -> void
 * out <- void
 *
 * Fade to black, wait for pause to be pressed
 * and then fade back in.
 *
 */
void pause_game(void)
{
	fade_to_black(plrscr_pal);
	while(!keys[pause])
		;
	fade_from_black(plrscr_pal);
}


/*
 * drawpiece -
 *
 * in  -> co-ordinates of piece
 *        piece number
 *        rotation of the piece
 * out <- void
 *
 * Draws the piece where it is asked for
 *
 */
void	drawpiece(char xp, char yp, char pn, char rot)
{
	short	sx;
	short	sy;
	short	x;
	short	y;
	char	*ptr;

	ptr = piece_map[pn].bitmap[rot];

	sx = xp * 10 + player;
	sy = yp * 10;

	for (y = sy; y < (sy + 40); y++) {
		for (x = sx; x < (sx + 40); x++) {
			if (*ptr != 0)
				putpixelX(x, y, *ptr);
			ptr++;
		}
	}
}


/*
 * testpiece -
 *
 * in  -> pointer to the pit
 *        co-ordinates of the piece
 *        piece number
 *        rotation of piece
 * out <- 0 then the proposed piece is OK
 *	  1 then the piece is not OK
 *
 */
int	testpiece(pit *p, char xp, char yp, char pn, char rot)
{
	char	*ptr1;
	char	*ptr2;
	char	x;
	char	y;

	rot &= 0x03;
	ptr1 = piece_map[pn].map[rot];
	ptr2 = p + (yp * 16) + xp + 2;

	for (y = 0; y < 4; y++) {
		for (x = 0; x < 4; x++) {
			if (*(ptr1) & *(ptr2))
				return (1);
			ptr1++;
			ptr2++;
		}
		ptr2 += 12;
	}
	return (0);
}


/*
 * testline -
 *
 * in  -> pit
 *	  starting line to scan
 * out <- number of lines dropped
 */
int testline(pit *p, char y)
{
	char		*ptr;

	ptr = p + (y * 16);
	if (*(ptr + 3) & *(ptr + 4) & *(ptr + 5) &
	    *(ptr + 6) & *(ptr + 7) & *(ptr + 8) &
	    *(ptr + 9) & *(ptr + 10) & *(ptr + 11) &
	    *(ptr + 12))
		return (1);

	return (0);
}


/*
 * checkline -
 *
 * in  -> pit
 *        line number to check
 * out <- 1 if the line has any blocks in it
 *        0 other wise
 */
int checkline(pit *p, char y)
{
	char		*ptr;

	ptr = p + (y * 16);
	if (*(ptr + 3) | *(ptr + 4) | *(ptr + 5) |
	    *(ptr + 6) | *(ptr + 7) | *(ptr + 8) |
	    *(ptr + 9) | *(ptr + 10) | *(ptr + 11) |
	    *(ptr + 12))
		return (1);

	return (0);
}



/*
 * testlines -
 *
 * in  -> pit
 *	  starting line to scan
 * out <- number of lines dropped
 */
int testlines(pit *p, char y)
{
	char		*ptr;
	char		rem[3];
	char		acc;
	signed char	i;

	rem[0] = 0;
	rem[1] = 0;
	rem[2] = 0;
	rem[3] = 0;
	acc = 0;

	switch (y) {
		case 21:
			ptr = p + (y * 16);
			if (*(ptr + 3) & *(ptr + 4) & *(ptr + 5) &
			    *(ptr + 6) & *(ptr + 7) & *(ptr + 8) &
			    *(ptr + 9) & *(ptr + 10) & *(ptr + 11) &
			    *(ptr + 12))
				rem[0] = 1;
			break;
		case 20:
			ptr = p + (y * 16);
			if (*(ptr + 19) & *(ptr + 20) & *(ptr + 21) &
			    *(ptr + 22) & *(ptr + 23) & *(ptr + 24) &
			    *(ptr + 25) & *(ptr + 26) & *(ptr + 27) &
			    *(ptr + 28))
				rem[1] = 1;
			if (*(ptr + 3) & *(ptr + 4) & *(ptr + 5) &
			    *(ptr + 6) & *(ptr + 7) & *(ptr + 8) &
			    *(ptr + 9) & *(ptr + 10) & *(ptr + 11) &
			    *(ptr + 12))
				rem[0] = 1;
			break;
		case 19:
			ptr = p + (y * 16);
			if (*(ptr + 35) & *(ptr + 36) & *(ptr + 37) &
			    *(ptr + 38) & *(ptr + 39) & *(ptr + 40) &
			    *(ptr + 41) & *(ptr + 42) & *(ptr + 43) &
			    *(ptr + 44))
				rem[2] = 1;
			if (*(ptr + 19) & *(ptr + 20) & *(ptr + 21) &
			    *(ptr + 22) & *(ptr + 23) & *(ptr + 24) &
			    *(ptr + 25) & *(ptr + 26) & *(ptr + 27) &
			    *(ptr + 28))
				rem[1] = 1;
			if (*(ptr + 3) & *(ptr + 4) & *(ptr + 5) &
			    *(ptr + 6) & *(ptr + 7) & *(ptr + 8) &
			    *(ptr + 9) & *(ptr + 10) & *(ptr + 11) &
			    *(ptr + 12))
				rem[0] = 1;
			break;
		default:
			ptr = p + (y * 16);
			if (*(ptr + 51) & *(ptr + 52) & *(ptr + 53) &
			    *(ptr + 54) & *(ptr + 55) & *(ptr + 56) &
			    *(ptr + 57) & *(ptr + 58) & *(ptr + 59) &
			    *(ptr + 60))
				rem[3] = 1;
			if (*(ptr + 35) & *(ptr + 36) & *(ptr + 37) &
			    *(ptr + 38) & *(ptr + 39) & *(ptr + 40) &
			    *(ptr + 41) & *(ptr + 42) & *(ptr + 43) &
			    *(ptr + 44))
				rem[2] = 1;
			if (*(ptr + 19) & *(ptr + 20) & *(ptr + 21) &
			    *(ptr + 22) & *(ptr + 23) & *(ptr + 24) &
			    *(ptr + 25) & *(ptr + 26) & *(ptr + 27) &
			    *(ptr + 28))
				rem[1] = 1;
			if (*(ptr + 3) & *(ptr + 4) & *(ptr + 5) &
			    *(ptr + 6) & *(ptr + 7) & *(ptr + 8) &
			    *(ptr + 9) & *(ptr + 10) & *(ptr + 11) &
			    *(ptr + 12))
				rem[0] = 1;
			break;
	}

	y += 3;

	for (i = 3; i >= 0; i--) {
		if (rem[i]) {
			acc++;
		}
	}

	if (acc > 0)
		SOUND_sfx(acc);

	for (i = 3; i >= 0; i--) {
		if (rem[i]) {
			removeline(p, y);
		} else {
			y--;
		}
	}
	return (acc);
}


/*
 * two_testlines -
 *
 * in  -> pit
 *	  starting line to scan
 * out <- number of lines dropped
 *
 * For two player games, we can't animate the falling pieces,
 * we just plonk them down to prevent interrupting the other
 * player whilst they are putting pieces into their pit.
 */
int two_testlines(pit *p, char y)
{
	char		*ptr;
	char		rem[3];
	char		acc;
	signed char	i;

	rem[0] = 0;
	rem[1] = 0;
	rem[2] = 0;
	rem[3] = 0;
	acc = 0;

	switch (y) {
		case 21:
			ptr = p + (y * 16);
			if (*(ptr + 3) & *(ptr + 4) & *(ptr + 5) &
			    *(ptr + 6) & *(ptr + 7) & *(ptr + 8) &
			    *(ptr + 9) & *(ptr + 10) & *(ptr + 11) &
			    *(ptr + 12))
				rem[0] = 1;
			break;
		case 20:
			ptr = p + (y * 16);
			if (*(ptr + 19) & *(ptr + 20) & *(ptr + 21) &
			    *(ptr + 22) & *(ptr + 23) & *(ptr + 24) &
			    *(ptr + 25) & *(ptr + 26) & *(ptr + 27) &
			    *(ptr + 28))
				rem[1] = 1;
			if (*(ptr + 3) & *(ptr + 4) & *(ptr + 5) &
			    *(ptr + 6) & *(ptr + 7) & *(ptr + 8) &
			    *(ptr + 9) & *(ptr + 10) & *(ptr + 11) &
			    *(ptr + 12))
				rem[0] = 1;
			break;
		case 19:
			ptr = p + (y * 16);
			if (*(ptr + 35) & *(ptr + 36) & *(ptr + 37) &
			    *(ptr + 38) & *(ptr + 39) & *(ptr + 40) &
			    *(ptr + 41) & *(ptr + 42) & *(ptr + 43) &
			    *(ptr + 44))
				rem[2] = 1;
			if (*(ptr + 19) & *(ptr + 20) & *(ptr + 21) &
			    *(ptr + 22) & *(ptr + 23) & *(ptr + 24) &
			    *(ptr + 25) & *(ptr + 26) & *(ptr + 27) &
			    *(ptr + 28))
				rem[1] = 1;
			if (*(ptr + 3) & *(ptr + 4) & *(ptr + 5) &
			    *(ptr + 6) & *(ptr + 7) & *(ptr + 8) &
			    *(ptr + 9) & *(ptr + 10) & *(ptr + 11) &
			    *(ptr + 12))
				rem[0] = 1;
			break;
		default:
			ptr = p + (y * 16);
			if (*(ptr + 51) & *(ptr + 52) & *(ptr + 53) &
			    *(ptr + 54) & *(ptr + 55) & *(ptr + 56) &
			    *(ptr + 57) & *(ptr + 58) & *(ptr + 59) &
			    *(ptr + 60))
				rem[3] = 1;
			if (*(ptr + 35) & *(ptr + 36) & *(ptr + 37) &
			    *(ptr + 38) & *(ptr + 39) & *(ptr + 40) &
			    *(ptr + 41) & *(ptr + 42) & *(ptr + 43) &
			    *(ptr + 44))
				rem[2] = 1;
			if (*(ptr + 19) & *(ptr + 20) & *(ptr + 21) &
			    *(ptr + 22) & *(ptr + 23) & *(ptr + 24) &
			    *(ptr + 25) & *(ptr + 26) & *(ptr + 27) &
			    *(ptr + 28))
				rem[1] = 1;
			if (*(ptr + 3) & *(ptr + 4) & *(ptr + 5) &
			    *(ptr + 6) & *(ptr + 7) & *(ptr + 8) &
			    *(ptr + 9) & *(ptr + 10) & *(ptr + 11) &
			    *(ptr + 12))
				rem[0] = 1;
			break;
	}

	y += 3;

	for (i = 3; i >= 0; i--) {
		if (rem[i]) {
			acc++;
		}
	}
	if (acc > 0)
		SOUND_sfx(1);

	for (i = 3; i >= 0; i--) {
		if (rem[i]) {
			two_removeline(p, y);
		} else {
			y--;
		}
	}
	return (acc);
}


/*
 * addpiece -
 *
 * in  -> pointer to the pit
 *        co-ordinates of the piece
 *        piece number
 *        rotation of piece
 * out <- void
 *
 * Adds the specified piece to the pit pointed to.
 *
 */
void	addpiece(pit *p, char xp, char yp, char pn, char rot)
{
	char	*ptr1;
	char	*ptr2;
	char	x;
	char	y;

	ptr1 = piece_map[pn].map[rot];
	ptr2 = p + yp * 16 + xp + 2;

	for (y = 0; y < 4; y++) {
		for (x = 0; x < 4; x++) {
			*(ptr2++) |= *(ptr1++);
			}
		ptr2 += 12;
	}
}


/*
 * splitpiece -
 *
 * in  ->
 * out <- void
 *
 * When a piece hits the ground it "cracks" into individual
 * block to make pit movements far simpler.
 *
 */
void	splitpiece(char xp, char yp, char pn, char rot)
{
	short	sx;
	short	sy;
	short	x;
	short	y;
	char	f;
	char	h;
	char	l;
	char	*ptr;

	f = piece_map[pn].face_colour;
	h = piece_map[pn].high_colour;
	l = piece_map[pn].low_colour;
	ptr = piece_map[pn].map[rot];

	sx = xp * 10 + player;
	sy = yp * 10;

	for (y = 0; y < 4; y++) {
		for (x = 0; x < 4; x++) {
			if (*ptr != 0) {
				filledboxX(sx, sy, sx + 9, sy + 9, f);
				lineX(sx, sy, sx + 8, sy, h);
				lineX(sx, sy + 1, sx + 7, sy + 1, h);
				lineX(sx, sy, sx, sy + 8, h);
				lineX(sx + 1, sy, sx + 1, sy + 7, h);
				lineX(sx + 9, sy + 1, sx + 9, sy + 9, l);
				lineX(sx + 8, sy + 2, sx + 8, sy + 8, l);
				lineX(sx + 1, sy + 9, sx + 8, sy + 9, l);
				lineX(sx + 2, sy + 8, sx + 7, sy + 8, l);
			}
			ptr++;
			sx += 10;
		}
		sx -= 40;
		sy += 10;
	}
}


/*
 * removepiece -
 *
 * in  -> co-ordinates of the piece
 *        piece number
 *        rotation of piece
 * out <- void
 *
 * Remove a piece from the screen
 *
 */
void	removepiece(char xp, char yp, char pn, char rot)
{
	short	sx;
	short	sy;
	short	x;
	short	y;
	char	f;
	char	*ptr;

	f = 0xF6;
	ptr = piece_map[pn].map[rot];

	sx = xp * 10 + player;
	sy = yp * 10;

	for (y = 0; y < 4; y++) {
		for (x = 0; x < 4; x++) {
			if (*ptr != 0) {
				filledboxX(sx, sy, sx + 9, sy + 9, f);
			}
			ptr++;
			sx += 10;
		}
		sx -= 40;
		sy += 10;
	}
}


/*
 * removeline -
 *
 * in  -> pointer to pit
 *        line to remove
 * out <- void
 *
 */
void removeline(pit *p, char y)
{
	char	blank_line[16] = {
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 };
	short	sx, sy;
	char	i;
	char	f;
	char	h;
	char	l;
	char 	*ptr;

	h = 0xFE;
	f = 0xFC;
	l = 0xFA;

	sx = player + 10;
	sy = y * 10;


	WaitVsyncStart();

	for (i = 0; i < 10; i++) {
		filledboxX(sx, sy, sx + 9, sy + 9, f);
		lineX(sx, sy, sx + 8, sy, h);
		lineX(sx, sy + 1, sx + 7, sy + 1, h);
		lineX(sx, sy, sx, sy + 8, h);
		lineX(sx + 1, sy, sx + 1, sy + 7, h);
		lineX(sx + 9, sy + 1, sx + 9, sy + 9, l);
		lineX(sx + 8, sy + 2, sx + 8, sy + 8, l);
		lineX(sx + 1, sy + 9, sx + 8, sy + 9, l);
		lineX(sx + 2, sy + 8, sx + 7, sy + 8, l);
		sx += 10;
	}

	WaitVsyncStart();
	WaitVsyncStart();
	WaitVsyncStart();
	WaitVsyncStart();
	WaitVsyncStart();
	WaitVsyncStart();
	WaitVsyncStart();

	filledboxX(player + 10, sy, player + 109, sy + 9, 0xF6);
	ptr = (char *) (0xA000 << 4) + activeStart;
	ptr += (player + 2) >> 2;
	ptr += y * 800 - 80;
	WaitVsyncStart();
	WaitVsyncStart();
	WaitVsyncStart();
	WaitVsyncStart();
	CopyVideoUp(ptr + 2, ptr + 802, y * 10 - 1);

	memmove(p + 16, p, y * 16);
	memcpy(p, blank_line, 16);
}


/*
 * two_removeline -
 *
 * in  -> pointer to pit
 *        line to remove
 * out <- void
 *
 * the two player version, which just does it as fast as it can
 *
 */
void two_removeline(pit *p, char y)
{
	char	blank_line[16] = {
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 };
	char	*ptr;

	ptr = (char *) (0xA000 << 4) + activeStart;
	ptr += (player + 2) >> 2;
	ptr += y * 800 - 80;

	WaitVsyncStart();
	CopyVideoUp(ptr + 2, ptr + 802, y * 10 - 1);
	memmove(p + 16, p, y * 16);
	memcpy(p, blank_line, 16);
}


/*
 * advance_line -
 *
 * in  -> pointer to pit
 * out <- 0 - okay
 *        1 - game over
 *
 */
int advance_line(pit *p)
{
	char	blank_line[16] = {
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 };
	short	sx, sy;
	char	i;
	char	*ptr;

	ptr = (char *) (0xA000 << 4) + activeStart;
	ptr += (player + 2) >> 2;

	WaitVsyncStart();
	CopyVideoDown(ptr + 802, ptr + 2, 210);
	filledboxX(player + 10, 210, player + 109, 219, 0xF6);

	memmove(p, p + 16, 22 * 16);
	memcpy(p + 336, blank_line, 16);
	ptr = p + 338;

	i = (rand() % 10) + 1;
	*(ptr + i) = 1;
	block(i, 21, 0xE2, 0xE3, 0xE1);

	i = (rand() % 10) + 1;
	while (*(ptr + i) == 1)
		i = (rand() % 10) + 1;
	*(ptr + i) = 1;
	block(i, 21, 0xE5, 0xE6, 0xE4);

	i = (rand() % 10) + 1;
	while (*(ptr + i) == 1)
		i = (rand() % 10) + 1;
	*(ptr + i) = 1;
	block(i, 21, 0xE8, 0xE9, 0xE7);

	i = (rand() % 10) + 1;
	while (*(ptr + i) == 1)
		i = (rand() % 10) + 1;
	*(ptr + i) = 1;
	block(i, 21, 0xEB, 0xEC, 0xEA);

	i = (rand() % 10) + 1;
	while (*(ptr + i) == 1)
		i = (rand() % 10) + 1;
	*(ptr + i) = 1;
	block(i, 21, 0xEE, 0xEF, 0xED);

	i = (rand() % 10) + 1;
	while (*(ptr + i) == 1)
		i = (rand() % 10) + 1;
	*(ptr + i) = 1;
	block(i, 21, 0xF1, 0xF2, 0xF0);

	i = (rand() % 10) + 1;
	while (*(ptr + i) == 1)
		i = (rand() % 10) + 1;
	*(ptr + i) = 1;
	block(i, 21, 0xF4, 0xF5, 0xF3);

	i = (rand() % 10) + 1;
	while (*(ptr + i) == 1)
		i = (rand() % 10) + 1;
	*(ptr + i) = 1;
	block(i, 21, 251, 253, 249);

	if (checkline(p, 1))
		return (1);

	return (0);
}


/*
 * gimmepiece -
 *
 * in  -> void
 * out <- a "random" piece number
 */
int	gimmepiece(void)
{
	short	p;

	p = rand();
	p %= 7;
	return(p);
}


/*
 * adjust_movie -
 *
 * in  -> piece number
 *        distance to adjust colours
 * out <- void
 *
 * Since only a single palette is used for holding all of the
 * piece previews the colours in the previews must be shifted
 * as to fit them all in.  Each preview uses 32 palette
 * entries.
 */
void	adjust_movie(char pn, char d)
{
	long	i;
	char	*ptr;

	ptr = piece_preview[pn].movie;
	for (i = 0; i < 144000; i++)
		*(ptr++) += d;
}


/*
 * preview_pal -
 *
 * in  -> void
 * out <- void
 *
 * Sets the palette up for the piece previews
 */
void	preview_pal(void)
{
	char	*buf;
	char	*ptr;
	short	i;
	char	pn;

	WaitVsyncStart();

	ptr = (char *) plrscr_pal;

	outp(0x03c8, 0); // Start with color 0

	for (pn = 0; pn < 7; pn++) {
		buf = (char *) piece_preview[pn].palette;
		i = 32;
		while (i--) {
			*ptr++ = *buf;
			outp(0x03c9, *buf++);
			*ptr++ = *buf;
			outp(0x03c9, *buf++);
			*ptr++ = *buf;
			outp(0x03c9, *buf++);
		}
	}
}


/*
 * plr1_preview -
 *
 * in  -> piece number to preview
 * out <- void
 *
 * updates the piece preview for player 1
 *
 */
void	plr1_preview(char pn)
{
	struct blitbuf		buf;
	static char		frame = 0;
	char			traces;

	buf.xsize = 80;
	buf.ysize = 50;
	buf.image = (unsigned char *) piece_preview[pn].movie + (frame * 4000);
	planar_bitblitX(120, 54, &buf);
	if (++frame == 36)
		frame = 0;
}


/*
 * plr2_preview -
 *
 * in  -> piece number to preview
 * out <- void
 *
 * updates the piece preview for player 2
 *
 */
void	plr2_preview(char pn)
{
	struct blitbuf		buf;
	static char		frame;
	char			traces;

	buf.xsize = 80;
	buf.ysize = 50;
	buf.image = (unsigned char *) piece_preview[pn].movie + (frame * 4000);
	planar_bitblitX(120, 163, &buf);
	if (frame-- == 0)
		frame = 35;
}


/*
 * centrestring -
 *
 * in  -> co-ordinate of the mid-point of the string
 *        pointer to the null terminated string
 * out <- void
 *
 */
void centrestring(short x, short y, char *p)
{
	char	*ptr;
	short	width;

	width = 0;
	ptr = p;
	while (*ptr != '\0')
		width += fontspacing[*(ptr++)];
	printstring(x - (width >> 1), y, p);
}


/*
 * printstring -
 *
 * in  -> co-ordinate of the string
 *        pointer to null terminated string
 * out <- void
 *
 */
void printstring(short x, short y, char *p)
{
//	struct blitbuf	b;
	char		*ptr;
	unsigned char	*cptr;
	short		cx;
	short		cy;

	ptr = p;
//	b.xsize = 12;
//	b.ysize = 12;

	while(*ptr != '\0') {
		cptr = (unsigned char *) font + (fontconvert[*ptr] * 144);
		for (cy = y; cy < (y + 12); cy++) {
			for (cx = x; cx < (x + 12); cx++) {
				if (*(cptr++))
					putpixelX(cx, cy, 255);
			}
		}

//		transparent_bitblitX(x, y, &b);
		x += fontspacing[*ptr];
		*ptr++;
	}
}


/*
 * printstring_slow -
 *
 * in  -> co-ordinate of the string
 *        pointer to null terminated string
 * out <- void
 *
 */
void printstring_slow(short x, short y, char *p)
{
	char		*ptr;
	unsigned char	*cptr;
	short		cx;
	short		cy;

	ptr = p;

	while(*ptr != '\0') {
		WaitVsyncStart();
		cptr = (unsigned char *) font + (fontconvert[*ptr] * 144);
		for (cy = y; cy < (y + 12); cy++) {
			for (cx = x; cx < (x + 12); cx++) {
				if (*(cptr++))
					putpixelX(cx, cy, 255);
			}
		}
		x += fontspacing[*ptr];
		*ptr++;
	}
}



/*
 * printnum -
 *
 * in  -> co-ordinates of number string
 *        pointer to null terminated string
 * out <- void
 */
void printnum(short x, short y, char *p)
{
	char	*ptr;

	ptr = p;
	while(*ptr != '\0') {
		aligned_bitblitX(x, y, &numbers[(*(ptr++)) - 48]);
		x += 8;
	}
}


/*
 * plr1_score -
 *
 * in  -> void
 * out <- void
 *
 * Update the scores for player 1
 *
 */
void	plr1_score(void)
{
	char	tmp[11];

	sprintf(tmp, "%ld", p1score);
	printnum(120, 16, tmp);
	sprintf(tmp, "%d", p1level + 1);
	printnum(120, 34, tmp);
	sprintf(tmp, "%d", p1lines);
	printnum(176, 34, tmp);
}


/*
 * plr2_score -
 *
 * in  -> void
 * out <- void
 *
 * Update the scores for player 2
 *
 */
void	plr2_score(void)
{
	char	tmp[11];

	sprintf(tmp, "%ld", p2score);
	printnum(120, 125, tmp);
	sprintf(tmp, "%d", p2level + 1);
	printnum(120, 143, tmp);
	sprintf(tmp, "%d", p2lines);
	printnum(176, 143, tmp);
}


/*
 * update_stats -
 *
 *
 * in  -> piece number to update, 0xFF clears the stats
 * out <- void
 *
 * Update the stats on the right hand side of the screen
 *
 */
void	update_stats(char p)
{
	static short	piece_count[7];
	short	x;
	short	y;
	short	i;
	char	f;
	char	h;
	char	l;

	if (p == 0xFF) {
		filledboxX(212, 0, 311, 219, 0xF6);
		for (i = 0; i < 7; i++)
			piece_count[i] = 217;
		return;
	}
	if (--piece_count[p] == 0) {
		piece_count[p] = 1;
		return;
	}

	f = piece_map[p].face_colour;
	h = piece_map[p].high_colour;
	l = piece_map[p].low_colour;

	x = p * 12 + 220;
	y = piece_count[p];

	filledboxX(x, y, x + 9, y + 2, f);
	lineX(x, y, x + 8, y, h);
	lineX(x, y + 1, x + 7, y + 1, h);
	lineX(x, y + 2, x + 1, y + 2, h);
	lineX(x + 9, y + 1, x + 9, y + 2, l);
	putpixelX(x + 8, y + 2, l);
}


/*
 * empty_pit -
 *
 * in  -> pointer to pit to empty
 * out <- void
 *
 * Clear out the pit
 */
void	empty_pit(pit *p)
{
	char	ep[416] = {
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

	memcpy(p, ep, 416);
}


/*
 * block -
 *
 * in  -> co-ordinates
 *        face colour
 *        high colour
 *        low colour
 * out <- void
 *
 * Draws a block
 *
 */
void block(short x, short y, char f, char h, char l)
{
	short	sx;
	short	sy;

	sx = x * 10 + player;
	sy = y * 10;

	filledboxX(sx, sy, sx + 9, sy + 9, f);
	lineX(sx, sy, sx + 8, sy, h);
	lineX(sx, sy + 1, sx + 7, sy + 1, h);
	lineX(sx, sy, sx, sy + 8, h);
	lineX(sx + 1, sy, sx + 1, sy + 7, h);
	lineX(sx + 9, sy + 1, sx + 9, sy + 9, l);
	lineX(sx + 8, sy + 2, sx + 8, sy + 8, l);
	lineX(sx + 1, sy + 9, sx + 8, sy + 9, l);
	lineX(sx + 2, sy + 8, sx + 7, sy + 8, l);
}


/*
 * row -
 *
 * in  -> line to be done
 *        face colour
 *        high colour
 *        low colour
 * out <- void
 *
 * Draws a row
 *
 */
void row(short y, char f, char h, char l)
{
	short	sx;
	short	sy;

	sx = 10 + player;
	sy = y * 10;

	filledboxX(sx, sy, sx + 99, sy + 9, f);
	lineX(sx, sy, sx + 98, sy, h);
	lineX(sx, sy + 1, sx + 97, sy + 1, h);
	lineX(sx, sy, sx, sy + 8, h);
	lineX(sx + 1, sy, sx + 1, sy + 7, h);
	lineX(sx + 99, sy + 1, sx + 99, sy + 9, l);
	lineX(sx + 98, sy + 2, sx + 98, sy + 8, l);
	lineX(sx + 1, sy + 9, sx + 98, sy + 9, l);
	lineX(sx + 2, sy + 8, sx + 97, sy + 8, l);
}


/*
 * get_p1inp -
 *
 * in  -> void
 * out <- status byte
 *
 * Gets input from player one, be it keyboard, joystick or mouse
 *
 * The Status Byte has the following format
 *
 * 	Bit	Meaning
 *	0	Left
 *	1	Right
 *	2	Drop
 *	3	Clockwise
 *	4	Anti clockwise
 *	5	End the Game
 *	6	Exit to OS
 *
 */
char get_p1inp(void)
{
	char	status;
	char	ts[40];

	unsigned short	ja_x;
	unsigned short	ja_y;
	unsigned short	jb_x;
	unsigned short	jb_y;

	char		jbuttons;

	static	char	xrcount;
	static	char	xlcount;
	static	char	yucount;
	static	char	ydcount;

	static	char	b1tag;
	static	char	b2tag;

	status = 0;

	if (keys[pause]) {
		keys[pause] = 0;
		pause_game();
	}
	if (keys[endgame]) {
		status |= END_GAME;
	}
	if (keys[quit] && keys[56]) {
		quit_flag = 1;
		status |= QUIT_TO_OS;
	}
	switch(plr1_device) {
		case KEYBOARD:
			if (keys[plr1_left]) {
				keys[plr1_left] = 0;
				status |= LEFT;
			}
			if (keys[plr1_right]) {
				keys[plr1_right] = 0;
				status |= RIGHT;
			}
			if (keys[plr1_clock]) {
				keys[plr1_clock] = 0;
				status |= CLOCKWISE;
			}
			if (keys[plr1_aclock]) {
				keys[plr1_aclock] = 0;
				status |= ANTICLOCKWISE;
			}
			if (keys[plr1_drop]) {
				status |= DROP;
			}
			break;
		case JOYSTICK1:
			jbuttons = inp(0x201);
			pretest();
			ja_x = time_ja_x();
			pretest();
			ja_y = time_ja_y();

			if (ja_x > jstick1_xmax) {
				xlcount = 0;
				if (xrcount++ == 2) {
					xrcount = 0;
					status |= RIGHT;
				}
			} else if (ja_x < jstick1_xmin) {
				xrcount = 0;
				if (xlcount++ == 2) {
					xlcount = 0;
					status |= LEFT;
				}
			} else {
				xrcount = 0;
				xlcount = 0;
			}
			if (ja_y > jstick1_ymax) {
				status |= DROP;
			}
			if (jbuttons & 0x10) {
				b1tag = 0;
			} else if (b1tag == 0) {
				b1tag = 1;
				status |= ANTICLOCKWISE;
			}
			if (jbuttons & 0x20) {
				b2tag = 0;
			} else if (b2tag == 0) {
				b2tag = 1;
				status |= CLOCKWISE;
			}
			break;
		case JOYSTICK2:
			jbuttons = inp(0x201);
			pretest();
			jb_x = time_jb_x();
			pretest();
			jb_y = time_jb_y();

			if (jb_x > jstick2_xmax) {
				xlcount = 0;
				if (xrcount++ == 2) {
					xrcount = 0;
					status |= RIGHT;
				}
			} else if (jb_x < jstick2_xmin) {
				xrcount = 0;
				if (xlcount++ == 2) {
					xlcount = 0;
					status |= LEFT;
				}
			} else {
				xrcount = 0;
				xlcount = 0;
			}
			if (jb_y > jstick2_ymax) {
				status |= DROP;
			}
			if (jbuttons & 0x40) {
				b1tag = 0;
			} else if (b1tag == 0) {
				b1tag = 1;
				status |= ANTICLOCKWISE;
			}
			if (jbuttons & 0x80) {
				b2tag = 0;
			} else if (b2tag == 0) {
				b2tag = 1;
				status |= CLOCKWISE;
			}
			break;
		case MOUSE:
			break;
	}
	return (status);
}


/*
 * get_p2inp -
 *
 * in  -> void
 * out <- status byte
 *
 * Gets input from player two, be it keyboard, joystick or mouse
 *
 * The Status Byte has the following format
 *
 * 	Bit	Meaning
 *	0	Left
 *	1	Right
 *	2	Drop
 *	3	Clockwise
 *	4	Anti clockwise
 *	5	End the Game
 *	6	Exit to OS
 *
 */
char get_p2inp(void)
{
	char	status;
	char	ts[40];

	unsigned short	ja_x;
	unsigned short	ja_y;
	unsigned short	jb_x;
	unsigned short	jb_y;

	char		jbuttons;

	static	char	xrcount;
	static	char	xlcount;
	static	char	yucount;
	static	char	ydcount;

	static	char	b1tag;
	static	char	b2tag;

	status = 0;

	if (keys[pause]) {
		keys[pause] = 0;
		pause_game();
	}
	if (keys[endgame]) {
		status |= END_GAME;
	}
	if (keys[quit] && keys[56]) {
		quit_flag = 1;
		status |= QUIT_TO_OS;
	}
	switch(plr2_device) {
		case KEYBOARD:
			if (keys[plr2_left]) {
				keys[plr2_left] = 0;
				status |= LEFT;
			}
			if (keys[plr2_right]) {
				keys[plr2_right] = 0;
				status |= RIGHT;
			}
			if (keys[plr2_clock]) {
				keys[plr2_clock] = 0;
				status |= CLOCKWISE;
			}
			if (keys[plr2_aclock]) {
				keys[plr2_aclock] = 0;
				status |= ANTICLOCKWISE;
			}
			if (keys[plr2_drop]) {
				status |= DROP;
			}
			break;
		case JOYSTICK1:
			jbuttons = inp(0x201);
			pretest();
			ja_x = time_ja_x();
			pretest();
			ja_y = time_ja_y();

			if (ja_x > jstick1_xmax) {
				xlcount = 0;
				if (xrcount++ == 2) {
					xrcount = 0;
					status |= RIGHT;
				}
			} else if (ja_x < jstick1_xmin) {
				xrcount = 0;
				if (xlcount++ == 2) {
					xlcount = 0;
					status |= LEFT;
				}
			} else {
				xrcount = 0;
				xlcount = 0;
			}
			if (ja_y > jstick1_ymax) {
				status |= DROP;
			}
			if (jbuttons & 0x10) {
				b1tag = 0;
			} else if (b1tag == 0) {
				b1tag = 1;
				status |= ANTICLOCKWISE;
			}
			if (jbuttons & 0x20) {
				b2tag = 0;
			} else if (b2tag == 0) {
				b2tag = 1;
				status |= CLOCKWISE;
			}
			break;
		case JOYSTICK2:
			jbuttons = inp(0x201);
			pretest();
			jb_x = time_jb_x();
			pretest();
			jb_y = time_jb_y();

			if (jb_x > jstick2_xmax) {
				xlcount = 0;
				if (xrcount++ == 2) {
					xrcount = 0;
					status |= RIGHT;
				}
			} else if (jb_x < jstick2_xmin) {
				xrcount = 0;
				if (xlcount++ == 2) {
					xlcount = 0;
					status |= LEFT;
				}
			} else {
				xrcount = 0;
				xlcount = 0;
			}
			if (jb_y > jstick2_ymax) {
				status |= DROP;
			}
			if (jbuttons & 0x40) {
				b1tag = 0;
			} else if (b1tag == 0) {
				b1tag = 1;
				status |= ANTICLOCKWISE;
			}
			if (jbuttons & 0x80) {
				b2tag = 0;
			} else if (b2tag == 0) {
				b2tag = 1;
				status |= CLOCKWISE;
			}
			break;
		case MOUSE:
			break;
	}
	return (status);
}


/*
 * init_pit -
 *
 * in  -> pointer to the pit
 *        number of pit you want
 * out <- void
 */
void init_pit(char *p, char n)
{
	char	lp[128] = {
		1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
		1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
		1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
		1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
		1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
		1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
		1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
		1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 };
	char	rp[128] = {
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1,
		1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1,
		1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1 };
	char	pp[128] = {
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1,
		1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1,
		1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1 };
	char	bp[128] = {
		1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1 };
	char	mp[128] = {
		1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1,
		1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1,
		1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1,
		1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1,
		1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1,
		1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1,
		1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1,
		1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1 };

	filledboxX(player + 10, 0, player + 109, 219, 0xF6);

	switch(n) {
		case CLEAR:
			break;
		case LINES:
			block(1, 14, 235, 236, 234);
			block(10, 14, 235, 236, 234);
			block(1, 15, 241, 242, 240);
			block(10, 15, 241, 242, 240);
			block(1, 16, 229, 230, 228);
			block(10, 16, 229, 230, 228);
			block(1, 17, 238, 239, 237);
			block(10, 17, 238, 239, 237);
			block(1, 18, 251, 253, 249);
			block(10, 18, 251, 253, 249);
			block(1, 19, 232, 233, 231);
			block(10, 19, 232, 233, 231);
			block(1, 20, 226, 227, 225);
			block(10, 20, 226, 227, 225);
			block(1, 21, 244, 245, 243);
			block(10, 21, 244, 245, 243);
			memcpy(p + 224, lp, 128);
			break;
		case RANDOM:
			block(1, 17, 244, 245, 243);
			block(2, 21, 226, 227, 225);
			block(3, 18, 232, 233, 231);
			block(4, 20, 251, 253, 249);
			block(5, 17, 238, 239, 237);
			block(6, 19, 229, 230, 228);
			block(7, 18, 235, 236, 234);
			block(7, 21, 241, 242, 240);
			block(8, 18, 244, 245, 243);
			block(9, 20, 226, 227, 225);
			block(10, 17, 232, 233, 231);
			memcpy(p + 224, rp, 128);
			break;
		case PYRAMID:
			block(1, 21, 226, 227, 225);
			block(2, 20, 232, 233, 231);
			block(2, 21, 232, 233, 231);
			block(3, 19, 251, 253, 249);
			block(3, 20, 251, 253, 249);
			block(4, 18, 238, 239, 237);
			block(4, 19, 238, 239, 237);
			block(4, 21, 238, 239, 237);
			block(5, 17, 229, 230, 228);
			block(5, 18, 229, 230, 228);
			block(5, 20, 229, 230, 228);
			block(5, 21, 229, 230, 228);
			block(6, 17, 241, 242, 240);
			block(6, 18, 241, 242, 240);
			block(6, 20, 241, 242, 240);
			block(6, 21, 241, 242, 240);
			block(7, 18, 235, 236, 234);
			block(7, 19, 235, 236, 234);
			block(7, 21, 235, 236, 234);
			block(8, 19, 244, 245, 243);
			block(8, 20, 244, 245, 243);
			block(9, 20, 226, 227, 225);
			block(9, 21, 226, 227, 225);
			block(10, 21, 232, 233, 231);
			memcpy(p + 224, pp, 128);
			break;
		case BASTARD:
			block( 1, 14, 0xF1, 0xF2, 0xF0);
			block( 2, 14, 0xEB, 0xEC, 0xEA);
			block( 3, 14, 0xF4, 0xF5, 0xF3);
			block( 4, 14, 0xE2, 0xE3, 0xE1);
			block( 6, 14, 0xE8, 0xE9, 0xE7);
			block( 7, 14, 0xFB, 0xFD, 0xF9);
			block( 8, 14, 0xEE, 0xEF, 0xED);
			block( 9, 14, 0xE5, 0xE6, 0xE4);
			block(10, 15, 0xF1, 0xF2, 0xF0);
			block( 1, 15, 0xEB, 0xEC, 0xEA);
			block( 2, 15, 0xF4, 0xF5, 0xF3);
			block( 3, 15, 0xE2, 0xE3, 0xE1);
			block( 5, 15, 0xE8, 0xE9, 0xE7);
			block( 6, 15, 0xFB, 0xFD, 0xF9);
			block( 7, 15, 0xEE, 0xEF, 0xED);
			block( 8, 15, 0xE5, 0xE6, 0xE4);
			block( 9, 16, 0xF1, 0xF2, 0xF0);
			block(10, 16, 0xEB, 0xEC, 0xEA);
			block( 1, 16, 0xF4, 0xF5, 0xF3);
			block( 2, 16, 0xE2, 0xE3, 0xE1);
			block( 4, 16, 0xE8, 0xE9, 0xE7);
			block( 5, 16, 0xFB, 0xFD, 0xF9);
			block( 6, 16, 0xEE, 0xEF, 0xED);
			block( 7, 16, 0xE5, 0xE6, 0xE4);
			block( 8, 17, 0xF1, 0xF2, 0xF0);
			block( 9, 17, 0xEB, 0xEC, 0xEA);
			block(10, 17, 0xF4, 0xF5, 0xF3);
			block( 1, 17, 0xE2, 0xE3, 0xE1);
			block( 3, 17, 0xE8, 0xE9, 0xE7);
			block( 4, 17, 0xFB, 0xFD, 0xF9);
			block( 5, 17, 0xEE, 0xEF, 0xED);
			block( 6, 17, 0xE5, 0xE6, 0xE4);
			block( 7, 18, 0xF1, 0xF2, 0xF0);
			block( 8, 18, 0xEB, 0xEC, 0xEA);
			block( 9, 18, 0xF4, 0xF5, 0xF3);
			block(10, 18, 0xE2, 0xE3, 0xE1);
			block( 2, 18, 0xE8, 0xE9, 0xE7);
			block( 3, 18, 0xFB, 0xFD, 0xF9);
			block( 4, 18, 0xEE, 0xEF, 0xED);
			block( 5, 18, 0xE5, 0xE6, 0xE4);
			block( 6, 19, 0xF1, 0xF2, 0xF0);
			block( 7, 19, 0xEB, 0xEC, 0xEA);
			block( 8, 19, 0xF4, 0xF5, 0xF3);
			block( 9, 19, 0xE2, 0xE3, 0xE1);
			block( 1, 19, 0xE8, 0xE9, 0xE7);
			block( 2, 19, 0xFB, 0xFD, 0xF9);
			block( 3, 19, 0xEE, 0xEF, 0xED);
			block( 4, 19, 0xE5, 0xE6, 0xE4);
			block( 5, 20, 0xF1, 0xF2, 0xF0);
			block( 6, 20, 0xEB, 0xEC, 0xEA);
			block( 7, 20, 0xF4, 0xF5, 0xF3);
			block( 8, 20, 0xE2, 0xE3, 0xE1);
			block(10, 20, 0xE8, 0xE9, 0xE7);
			block( 1, 20, 0xFB, 0xFD, 0xF9);
			block( 2, 20, 0xEE, 0xEF, 0xED);
			block( 3, 20, 0xE5, 0xE6, 0xE4);
			block( 4, 21, 0xF1, 0xF2, 0xF0);
			block( 5, 21, 0xEB, 0xEC, 0xEA);
			block( 6, 21, 0xF4, 0xF5, 0xF3);
			block( 7, 21, 0xE2, 0xE3, 0xE1);
			block( 9, 21, 0xE8, 0xE9, 0xE7);
			block(10, 21, 0xFB, 0xFD, 0xF9);
			block( 1, 21, 0xEE, 0xEF, 0xED);
			block( 2, 21, 0xE5, 0xE6, 0xE4);
			memcpy(p + 224, bp, 128);
			break;
		case MESH:
			block(1, 14, 235, 236, 234);
			block(3, 14, 235, 236, 234);
			block(5, 14, 235, 236, 234);
			block(7, 14, 235, 236, 234);
			block(9, 14, 235, 236, 234);

			block(2, 15, 241, 242, 240);
			block(4, 15, 241, 242, 240);
			block(6, 15, 241, 242, 240);
			block(8, 15, 241, 242, 240);
			block(10, 15, 241, 242, 240);

			block(1, 16, 229, 230, 228);
			block(3, 16, 229, 230, 228);
			block(5, 16, 229, 230, 228);
			block(7, 16, 229, 230, 228);
			block(9, 16, 229, 230, 228);

			block(2, 17, 238, 239, 237);
			block(4, 17, 238, 239, 237);
			block(6, 17, 238, 239, 237);
			block(8, 17, 238, 239, 237);
			block(10, 17, 238, 239, 237);

			block(1, 18, 251, 253, 249);
			block(3, 18, 251, 253, 249);
			block(5, 18, 251, 253, 249);
			block(7, 18, 251, 253, 249);
			block(9, 18, 251, 253, 249);

			block(2, 19, 232, 233, 231);
			block(4, 19, 232, 233, 231);
			block(6, 19, 232, 233, 231);
			block(8, 19, 232, 233, 231);
			block(10, 19, 232, 233, 231);

			block(1, 20, 226, 227, 225);
			block(3, 20, 226, 227, 225);
			block(5, 20, 226, 227, 225);
			block(7, 20, 226, 227, 225);
			block(9, 20, 226, 227, 225);

			block(2, 21, 244, 245, 243);
			block(4, 21, 244, 245, 243);
			block(6, 21, 244, 245, 243);
			block(8, 21, 244, 245, 243);
			block(10, 21, 244, 245, 243);

			memcpy(p + 224, mp, 128);
			break;
	}
}


/*
 * collect_bonus
 *
 * in  -> pointer to pit
 *        level
 * out <- void
 *
 * does the bonus stuff for a pit
 *
 */
void collect_bonus(char *p, char level)
{
	char	s[20];
	char	*ptr;
	char	l;
	long	bonus;
	short	sx;
	short	sy;
	short	i;
	short	freq = 11025;

	ptr = p + 32;
	l = 1;
	bonus = 0;

	filledboxX(player + 10, 0, player + 109, 19, 0xF6);
	centrestring(player + 60, 1, "YOU DID IT !");

	sleep(1);
	WaitVsyncStart();

	filledboxX(128, 164, 158, 175, 0xFC);
	sprintf(s, "0");
	printnum(128, 164, s);
	filledboxX(120, 54, 199, 103, 0xF6);
	plr1_score();

	sprintf(s, "%ld", bonus);
	printnum(128, 187, s);

	filledboxX(player + 10, 0, player + 109, 19, 0xF6);
	centrestring(player + 60, 0, "BONUS FOR");
	centrestring(player + 60, 10, "LOW PUZZLE");

	while(l++ < 21) {
		if (*(ptr + 3) | *(ptr + 4) | *(ptr + 5) |
		    *(ptr + 6) | *(ptr + 7) | *(ptr + 8) |
		    *(ptr + 9) | *(ptr + 10) | *(ptr + 11) |
		    *(ptr + 12)) {
			break;
		} else {
			SOUND_enhsfx(9, freq, 0);
			freq += 1000;
			WaitVsyncStart();
			WaitVsyncStart();
			WaitVsyncStart();
			WaitVsyncStart();
			WaitVsyncStart();
			WaitVsyncStart();
			bonus += level * (l * l + l);
			sprintf(s, "%ld", bonus);
			printnum(128, 187, s);
			ptr += 16;
			for (sx = 1; sx < 11; sx++)
				block(sx, l, 0xFC, 0xFE, 0xFA);
		}
	}

	sleep(1);
	if (player = PLAYER_1) {
		p1score += bonus;
		plr1_score();
	} else if (player = PLAYER_2) {
		p2score += bonus;
		plr2_score();
	}

	filledboxX(player + 10, 0, player + 109, 19, 0xF6);
	filledboxX(128, 187, 190, 197, 0xFC);

	sleep(1);
	for (i = 0; i < 200; i++) {
		WaitVsyncStart();
		sx = random[i] % 10 + 1;
		sy = random[i] / 10 + 2;
		filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
		i++;
		sx = random[i] % 10 + 1;
		sy = random[i] / 10 + 2;
		filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
		i++;
		sx = random[i] % 10 + 1;
		sy = random[i] / 10 + 2;
		filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
		i++;
		sx = random[i] % 10 + 1;
		sy = random[i] / 10 + 2;
		filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
		i++;
		sx = random[i] % 10 + 1;
		sy = random[i] / 10 + 2;
		filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
		i++;
		sx = random[i] % 10 + 1;
		sy = random[i] / 10 + 2;
		filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
		i++;
		sx = random[i] % 10 + 1;
		sy = random[i] / 10 + 2;
		filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
		i++;
		sx = random[i] % 10 + 1;
		sy = random[i] / 10 + 2;
		filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
	}
	sleep(1);
}


/*
 * game_over -
 *
 * in  -> void
 * out <- void
 *
 * Called when a games is finished to tell 'em its game over
 * and to clean the pit up
 */
void game_over(void)
{
	signed short	sx;
	signed short	sy;
	char		f;
	char		h;
	char		l;
	short		i;
	short		y;

	h = 0xFE;
	f = 0xFC;
	l = 0xFA;

	sleep(1);
	SOUND_sfx(7);
	WaitVsyncStart();

	filledboxX(player + 10, 0, player + 109, 19, 0xF6);
	centrestring(player + 60, 5, "GAME OVER");

	for (i = 0; i < 200; i++) {
		WaitVsyncStart();
		sx = random[i] % 10 + 1;
		sy = random[i] / 10 + 2;
		block(sx, sy, f, h, l);
	}
}


/*
 * display_highscores
 *
 * in  -> pointer to highscore table
 * out <- void
 */
void display_highscores(struct highscore_table *ht)
{
	short 	i;
	char	tmp[20];

	printstring(100, 10, ht->name);
	printstring( 27, 40, "SCORE");
	printstring( 77, 40, "LINES");
	printstring(122, 40, "LEVEL");
	printstring(182, 40, "NAME");
	for (i = 0; i < 10; i++) {
		sprintf(tmp, "%7ld", ht->table[i].score);
		printstring(12, i * 16 + 60, tmp);
		sprintf(tmp, "%3d", ht->table[i].lines);
		printstring(82, i * 16 + 60, tmp);
		sprintf(tmp, "%3d", ht->table[i].level);
		printstring(122, i * 16 + 60, tmp);
		printstring(172, i * 16 + 60, ht->table[i].name);
	}
}


/*
 * top_score
 *
 * in  -> void
 * out <- void
 *
 */
void top_score(void)
{
	char		wpal[768];
	short		i;

	wide_bitblitX(0, &high_score_scrn);
	for (i = 0; i < 768; i++) {
		wpal[i] = 0x3F;
	}

	SOUND_sfx(10);
	WaitVsyncStart();
	set_paletteX((unsigned char *) wpal, 0);
	WaitVsyncStart();
	WaitVsyncStart();
	WaitVsyncStart();
	set_paletteX((unsigned char *) high_score_pal, 0);
	WaitVsyncStart();
	WaitVsyncStart();
	WaitVsyncStart();
	set_paletteX((unsigned char *) wpal, 0);
	WaitVsyncStart();
	WaitVsyncStart();
	WaitVsyncStart();
	set_paletteX((unsigned char *) high_score_pal, 0);
	WaitVsyncStart();
	WaitVsyncStart();
	WaitVsyncStart();
	set_paletteX((unsigned char *) wpal, 0);
	WaitVsyncStart();
	WaitVsyncStart();
	WaitVsyncStart();
	set_paletteX((unsigned char *) high_score_pal, 0);
	sleep(2);

	fade_to_black(high_score_pal);
}


/*
 * high_score -
 *
 * in  -> score of player
 *        level of player
 *        lines of player
 * 	  number of player
 *	  pointer to highscore table
 * out <- void
 *
 * Given a highscore it checks the highscore table to see
 * if the score is high enough to make it in.  If it is
 * the name of the player is entered and the player is
 * added to the high score table.
 */
void high_score(long score, char pn, short level, short lines, struct highscore_table *ht)
{
	signed short		i;
	struct highscore_entry	t;
	char			*ptr;
	char			*ptr2;
	char			n[30];
	char			c;
	char			s;
	char			sflag;
	char			place;
	short			nx;
	short			ny;

	if (score <= ht->table[9].score) {
		SOUND_sfxmod(volume);
		fade_to_black(plrscr_pal);
		wide_bitblitX(0, &tetris);
		display_highscores(ht);
		fade_from_black(tetris_faded_pal);
		sleep(2);
		fade_to_black(tetris_faded_pal);
		return;
	}

	/*
	 * okay, we've gotten rid of the losers
	 * at this point now we need to find
	 * just how high Jim Bob made it up the
	 * ladder.
	 *
	 * We know that 10th place (ht[9]) is gone
	 * so we'll fill in the details of ht[9]
	 * with our winner.
	 */

	ht->table[9].score = score;
	ht->table[9].level = level;
	ht->table[9].lines = lines;
	ht->table[9].name[0] = '\0';

	/*
	 * now we can use a swap search (from the bottom)
	 * and see just how far we made it.
	 */

	i = 8;
	while (ht->table[i + 1].score > ht->table[i].score) {
		memcpy(&t, &ht->table[i], sizeof(ht->table[0]));
		memcpy(&ht->table[i], &ht->table[i + 1], sizeof(ht->table[0]));
		memcpy(&ht->table[i + 1], &t, sizeof(ht->table[0]));
		if (i < 0)
			break;
		i--;
	}
	place = i + 1;

	/*
	 * If they made a high score, then lets let then know about it
 	 */
	fade_to_black(plrscr_pal);
	top_score();
	SOUND_sfxmod(volume);

	/*
	 * So place holds the number of the position
	 * we have obtained.  Lets get da name.
	 *
	 * Clear out the keyboard buffer (just incase)
	 * Load up the tetris screen, fade it down a little
	 * and paster the high scores over it
	 * Read in the name
	 * Save it into the highscore table
	 * and dats about it
	 */

	wide_bitblitX(0, &tetris);
	display_highscores(ht);
	fade_from_black(tetris_faded_pal);

	n[0] = '\0';
	i = 0;
	nx = 172;
	ny = 60 + place * 16;
	aligned_getblitX(nx, ny, &name_buf);

	for (;;) {
		s = gimme_key();
		if (s == 0x1C) {
			strcpy(ht->table[place].name, n);
			fade_to_black(tetris_faded_pal);
			return;
		}
		if (s == 0x0E) {
			if (i > 0) {
				i--;
				n[i] = '\0';
				WaitVsyncStart();
				aligned_bitblitX(nx, ny, &name_buf);
				printstring(nx, ny, n);
			}
		} else {
			if (s & 0x80)
				c = shiftscantoascii[s & 0x7F];
			else
				c = scantoascii[s];

			s &= 0x7F;
			if (s > 57)
				continue;

			if ((fontspacing[c] != 0) && (i < 12)) {
				n[i] = c;
				i++;
				n[i] = '\0';
				WaitVsyncStart();
				aligned_bitblitX(nx, ny, &name_buf);
				printstring(nx, ny, n);
			}
		}
	}
}


/*
 * stygian_tetris -
 *
 * in  -> void
 * out <- void
 *
 * My personal rendition of tetris
 * Guaranteed to annoy the hell out of you
 *
 */
void	stygian_tetris(void)
{
	short		sx;
	short		sy;
	short		advancing_count;
	short		random_count;
	short		random_count2;
	char		lines;
	signed char	level_lines;
	char		drop_count[2];
	long		ts;
	char		tmps[10];
	char		status;
	char		milestone;
	char		x;		/* position of current_piece */
	char		y;
	char		rot;
	char		speed[2];	/* speed is the number of retraces between drops */
	char		speed_mode;
	char		speed_count;	/* counter used for speed timing putposes */
	char		preview_count;	/* counter for the preview */
	char		current_piece;
	char		next_piece;
	pit		p[416];		/* the pit */
	char		col;
	char		row;
	char		*ptr;
	char		tmpst[20];

	/*
	 * Load up the 1 Player Screen
	 */
	wide_bitblitX(0, &plrscr);
	fade_from_black(plrscr_pal);

	quit = 0x10;
	pause = 0x45;
	endgame = 0x01;


	/*
	 * Initialize the player variable
	 */
	player = PLAYER_1;
	p1score = 0;
	p1lines = 0;
	p1level = 0;
	advancing_count = 0;
	random_count = 0;
	random_count2 = 0;
	bonus[0] = 50;
	bonus[1] = 150;
	bonus[2] = 450;
	bonus[3] = 900;
	speed[0] = slevels[0].speed;
	speed[1] = 0;
	drop_count[0] = 0;
	drop_count[1] = 0;
	level_lines = slevels[0].lines;
	speed_mode = 0;
	update_stats(0xFF);
	speed_count = speed[speed_mode];
	preview_count = PREVIEW_FRAMES;
	preview_pal();
	x = 4;
	y = 0;
	rot = 0;
	empty_pit(p);
	srand(GrabTick());
	current_piece = gimmepiece();
	next_piece = gimmepiece();


	empty_pit(p);
	init_pit(p, slevels[p1level].base);
	update_stats(current_piece);
	plr1_score();

	filledboxX(128, 164, 158, 175, 0xFC);
	sprintf(tmps, "%ld", level_lines);
	printnum(128, 164, tmps);

	centrestring(player + 60, 30, "LEVEL 1");
	centrestring(player + 60, 45, "WELCOME");
	centrestring(player + 60, 60, "TO");
	centrestring(player + 60, 75, "STYGIAN");
	centrestring(player + 60, 90, "TETRIS");

	sleep(2);
	WaitVsyncStart();
	filledboxX(8, 20, 107, 100, 0xF6);
	drawpiece(x, y, current_piece, rot);
	plr1_preview(next_piece);

	for (;;) {

		WaitVsyncStart();
		if (advancing_count-- == 1) {
			advancing_count = slevels[p1level].advancing;
			removepiece(x, y, current_piece, rot);
			if (advance_line(p)) {
				if (testpiece(p, x, y, current_piece, rot))
					y--;
				drawpiece(x, y, current_piece, rot);
				SOUND_sfx(6);
				game_over();
				high_score(p1score, 1, p1level, p1lines, &stygian);
				return;
			}
			if (testpiece(p, x, y, current_piece, rot)) {
				y--;
				speed_count = 0;
			}
			drawpiece(x, y, current_piece, rot);
			SOUND_sfx(6);
			WaitVsyncStart();
		}

		if (random_count) {
			lines = 0;
			if (random_count-- == 1) {
			  RETRY:
				random_count = slevels[p1level].random;
				col = (rand() % 10) + 1;
				row = 21;
				ptr = p + col + 338;
				while (*ptr == 1) {
					ptr -= 16;
					row--;
					if (row < 2)
						goto RETRY;
				}
				*ptr = 1;
				if (lines++ == 10) {
					random_count += slevels[p1level].random >> 1;
					goto TOOHARD;
				}
				if (testline(p, row)) {
					*ptr = 0;
					goto RETRY;
				}
				if (testpiece(p, x, y, current_piece, rot)) {
					*ptr = 0;
					goto RETRY;
				}
				block(col, row, 0xFC, 0xFE, 0xFA);
				SOUND_sfx(5);
			TOOHARD:
				lines = 0;
			}
		}

		if (random_count2) {
			lines = 0;
			if (random_count2-- == 1) {
			  RETRY2:
				random_count2 = slevels[p1level].random2;
				col = (rand() % 10) + 1;
				row = 21;
				ptr = p + col + 338;
				if (lines++ == 10) {
					random_count += slevels[p1level].random2 >> 1;
					goto TOOHARD2;
				}
				while (*ptr == 0) {
					ptr -= 16;
					row--;
					if (row < 2)
						goto RETRY2;
				}
				*ptr = 0;
				sx = col * 10 + player;
				sy = row * 10;
				filledboxX(sx, sy, sx + 9, sy + 9, 0xF6);
				SOUND_sfx(8);
			TOOHARD2:
				lines = 0;
			}
		}

		if (speed_count-- == 0) {
			speed_count = speed[speed_mode];
			drop_count[speed_mode]++;
			removepiece(x, y, current_piece, rot);
			y++;
			if (testpiece(p, x, y, current_piece, rot)) {
				SOUND_sfx(0);
				y--;
				splitpiece(x, y, current_piece, rot);
				addpiece(p, x, y, current_piece, rot);
				ts = (slevels[p1level].score_m * (22 - y) + slevels[p1level].score_b);
				p1score += ((ts * drop_count[1]) / (drop_count[0] + drop_count[1])) + ts;
				drop_count[0] = 0;
				drop_count[1] = 0;
				plr1_score();

				lines = testlines(p, y);
				if (lines > 0) {
					p1score += bonus[lines - 1];
					p1lines += lines;
					level_lines -= lines;
					if (level_lines <= 0) {
						collect_bonus(p, p1level + 1);
						p1level++;
						empty_pit(p);
						init_pit(p, slevels[p1level].base);
						sprintf(tmpst, "LEVEL %d", p1level + 1);
						centrestring(player + 60, 30, tmpst);
						if (slevels[p1level].special == 1) {
							centrestring(player + 60, 45, slevels[p1level].msg1);
							centrestring(player + 60, 60, slevels[p1level].msg2);
							centrestring(player + 60, 75, slevels[p1level].msg3);
							centrestring(player + 60, 90, slevels[p1level].msg4);
							sleep(2);
							WaitVsyncStart();
							filledboxX(8, 20, 107, 100, 0xF6);
						} else {
							centrestring(player + 60, 45, "COMPLETE\0");
							sprintf(tmpst, "%d LINES\0", slevels[p1level].lines);
							centrestring(player + 60, 60, tmpst);
							centrestring(player + 60, 75, "TO GO TO\0");
							centrestring(player + 60, 90, "NEXT ROUND\0");
							sleep(2);
							WaitVsyncStart();
							filledboxX(8, 20, 107, 100, 0xF6);
						}

						level_lines = slevels[p1level].lines;
						random_count = slevels[p1level].random;
						random_count2 = slevels[p1level].random2;
						advancing_count = slevels[p1level].advancing;
						speed[0] = slevels[p1level].speed;
						next_piece = gimmepiece();
					}
					filledboxX(128, 164, 158, 175, 0xFC);
					sprintf(tmps, "%ld", level_lines);
					printnum(128, 164, tmps);

					plr1_score();
				}
				if (y < 2) {
					filledboxX(120, 54, 199, 103, 0xF6);
					game_over();
					high_score(p1score, 1, p1level, p1lines, &stygian);
					return;
				}

				current_piece = next_piece;
				update_stats(current_piece);
				next_piece = gimmepiece();
				x = 4;
				y = 0;
				rot = 0;
				drawpiece(x, y, current_piece, rot);
				plr1_preview(next_piece);
			} else {
				drawpiece(x, y, current_piece, rot);
			}
		}
		if (preview_count-- == 0) {
			plr1_preview(next_piece);
			preview_count = PREVIEW_FRAMES;
		}
		status = get_p1inp();
		if (status & (LEFT | RIGHT | CLOCKWISE | ANTICLOCKWISE)) {
			removepiece(x, y, current_piece, rot);
			if ((status & LEFT) && (!testpiece(p, x - 1, y, current_piece, rot) && x > 0))
					x--;
			if ((status & RIGHT) && (!testpiece(p, x + 1, y, current_piece, rot) && x < 11))
					x++;
			if (status & CLOCKWISE) {
				if (!testpiece(p, x, y, current_piece, rot + 1)) {
					rot++;
					rot &= 0x03;
				} else if (!testpiece(p, x - 1, y, current_piece, rot + 1) && (x > 0)) {
					rot++;
					x--;
					rot &= 0x03;
				} else if (!testpiece(p, x - 2, y, current_piece, rot + 1) && (x > 0)) {
					rot++;
					x--; x--;
					rot &= 0x03;
				} else if (!testpiece(p, x + 1, y, current_piece, rot + 1)) {
					rot++;
					x++;
					rot &= 0x03;
				}
			}
			if (status & ANTICLOCKWISE) {
				if (!testpiece(p, x, y, current_piece, rot - 1)) {
					rot--;
					rot &= 0x03;
				} else if (!testpiece(p, x - 1, y, current_piece, rot - 1) && (x > 0)) {
					rot--;
					x--;
					rot &= 0x03;
				} else if (!testpiece(p, x - 2, y, current_piece, rot - 1) && (x > 0)) {
					rot--;
					x--; x--;
					rot &= 0x03;
				} else if (!testpiece(p, x + 1, y, current_piece, rot - 1)) {
					rot--;
					x++;
					rot &= 0x03;
				}
			}
			drawpiece(x, y, current_piece, rot);
		}
		if (status & DROP) {
			speed_mode = 1;
			if (speed_count > speed[1])
				speed_count = speed[1];
		} else  {
			speed_mode = 0;
		}
		if (status & END_GAME) {
			fade_to_black(plrscr_pal);
			SOUND_sfxmod(volume);
			return;
		}
		if (status & QUIT_TO_OS) {
			fade_to_black(plrscr_pal);
			return;
		}
	}
}


/*
 * arcade_tetris -
 *
 * in  -> void
 * out <- void
 *
 * Lets play arcade tetris
 *
 */
void	arcade_tetris(void)
{
	short		advancing_count;
	short		random_count;
	char		lines;
	signed char	level_lines;
	char		drop_count[2];
	long		ts;
	char		tmps[10];
	char		status;
	char		milestone;
	char		x;		/* position of current_piece */
	char		y;
	char		rot;
	char		speed[2];	/* speed is the number of retraces between drops */
	char		speed_mode;
	char		speed_count;	/* counter used for speed timing putposes */
	char		preview_count;	/* counter for the preview */
	char		current_piece;
	char		next_piece;
	pit		p[416];		/* the pit */
	char		col;
	char		row;
	char		*ptr;
	char		tmpst[20];

	/*
	 * Load up the 1 Player Screen
	 */
	wide_bitblitX(0, &plrscr);
	fade_from_black(plrscr_pal);

	quit = 0x10;
	pause = 0x45;
	endgame = 0x01;


	/*
	 * Initialize the player variable
	 */
	player = PLAYER_1;
	p1score = 0;
	p1lines = 0;
	p1level = 0;
	advancing_count = 0;
	random_count = 0;
	bonus[0] = 50;
	bonus[1] = 150;
	bonus[2] = 450;
	bonus[3] = 900;
	speed[0] = 12;
	speed[1] = 0;
	drop_count[0] = 0;
	drop_count[1] = 0;
	level_lines = alevels[0].lines;
	speed_mode = 0;
	update_stats(0xFF);
	speed_count = speed[speed_mode];
	preview_count = PREVIEW_FRAMES;
	preview_pal();
	x = 4;
	y = 0;
	rot = 0;
	empty_pit(p);
	srand(GrabTick());
	current_piece = gimmepiece();
	next_piece = gimmepiece();


	empty_pit(p);
	init_pit(p, alevels[p1level].base);
	update_stats(current_piece);
	plr1_score();

	filledboxX(128, 164, 158, 175, 0xFC);
	sprintf(tmps, "%ld", level_lines);
	printnum(128, 164, tmps);

	centrestring(player + 60, 30, "LEVEL 1\0");
	centrestring(player + 60, 45, "COMPLETE\0");
	centrestring(player + 60, 60, "5 LINES\0");
	centrestring(player + 60, 75, "TO GO TO\0");
	centrestring(player + 60, 90, "NEXT ROUND\0");

	sleep(2);
	WaitVsyncStart();
	filledboxX(8, 20, 107, 100, 0xF6);
	drawpiece(x, y, current_piece, rot);
	plr1_preview(next_piece);

	for (;;) {

		WaitVsyncStart();
		if (advancing_count-- == 1) {
			advancing_count = alevels[p1level].advancing;
			removepiece(x, y, current_piece, rot);
			if (advance_line(p)) {
				if (testpiece(p, x, y, current_piece, rot))
					y--;
				drawpiece(x, y, current_piece, rot);
				SOUND_sfx(6);
				game_over();
				high_score(p1score, 1, p1level, p1lines, &arcade);
				return;
			}
			if (testpiece(p, x, y, current_piece, rot)) {
				y--;
				speed_count = 0;
			}
			drawpiece(x, y, current_piece, rot);
			SOUND_sfx(6);
			WaitVsyncStart();
		}

		if (random_count) {
			lines = 0;
			if (random_count-- == 1) {
			  RETRY:
				random_count = alevels[p1level].random;
				col = (rand() % 10) + 1;
				row = 21;
				ptr = p + col + 338;
				while (*ptr == 1) {
					ptr -= 16;
					row--;
					if (row < 2)
						goto RETRY;
				}
				*ptr = 1;
				if (lines++ == 10) {
					random_count += alevels[p1level].random >> 1;
					goto TOOHARD;
				}
				if (testline(p, row)) {
					*ptr = 0;
					goto RETRY;
				}
				if (testpiece(p, x, y, current_piece, rot)) {
					*ptr = 0;
					goto RETRY;
				}
				block(col, row, 0xFC, 0xFE, 0xFA);
				SOUND_sfx(5);
			TOOHARD:
				lines = 0;
			}
		}

		if (speed_count-- == 0) {
			speed_count = speed[speed_mode];
			drop_count[speed_mode]++;
			removepiece(x, y, current_piece, rot);
			y++;
			if (testpiece(p, x, y, current_piece, rot)) {
				SOUND_sfx(0);
				y--;
				splitpiece(x, y, current_piece, rot);
				addpiece(p, x, y, current_piece, rot);
				ts = (alevels[p1level].score_m * (22 - y) + alevels[p1level].score_b);
				p1score += ((ts * drop_count[1]) / (drop_count[0] + drop_count[1])) + ts;
				drop_count[0] = 0;
				drop_count[1] = 0;
				plr1_score();

				lines = testlines(p, y);
				if (lines > 0) {
					p1score += bonus[lines - 1];
					p1lines += lines;
					level_lines -= lines;
					if (level_lines <= 0) {
						collect_bonus(p, p1level + 1);
						p1level++;
						empty_pit(p);
						init_pit(p, alevels[p1level].base);
						sprintf(tmpst, "LEVEL %d", p1level + 1);
						centrestring(player + 60, 30, tmpst);
						if (alevels[p1level].special == 1 && a>0) {
							centrestring(player + 60, 45, alevels[p1level].msg1);
							centrestring(player + 60, 60, alevels[p1level].msg2);
							centrestring(player + 60, 75, alevels[p1level].msg3);
							centrestring(player + 60, 90, alevels[p1level].msg4);
							sleep(2);
							WaitVsyncStart();
							filledboxX(8, 20, 107, 100, 0xF6);
						} else {
							centrestring(player + 60, 45, "COMPLETE\0");
							sprintf(tmpst, "%d LINES\0", alevels[p1level].lines);
							centrestring(player + 60, 60, tmpst);
							centrestring(player + 60, 75, "TO GO TO\0");
							centrestring(player + 60, 90, "NEXT ROUND\0");
							sleep(2);
							WaitVsyncStart();
							filledboxX(8, 20, 107, 100, 0xF6);
						}

						level_lines = alevels[p1level].lines;
						random_count = alevels[p1level].random;
						advancing_count = alevels[p1level].advancing;
						if (speed[0] & 0x01) {
							speed[0]--;
							if (speed[0] == 3)
								speed[0] = 4;
						}
						next_piece = gimmepiece();
					}
					filledboxX(128, 164, 158, 175, 0xFC);
					sprintf(tmps, "%ld", level_lines);
					printnum(128, 164, tmps);

					plr1_score();
				}
				if (y < 2) {
					filledboxX(120, 54, 199, 103, 0xF6);
					game_over();
					high_score(p1score, 1, p1level, p1lines, &arcade);
					return;
				}

				current_piece = next_piece;
				update_stats(current_piece);
				next_piece = gimmepiece();
				x = 4;
				y = 0;
				rot = 0;
				drawpiece(x, y, current_piece, rot);
				plr1_preview(next_piece);
			} else {
				drawpiece(x, y, current_piece, rot);
			}
		}
		if (preview_count-- == 0) {
			plr1_preview(next_piece);
			preview_count = PREVIEW_FRAMES;
		}

		status = get_p1inp();
		if (status & (LEFT | RIGHT | CLOCKWISE | ANTICLOCKWISE)) {
			removepiece(x, y, current_piece, rot);
			if ((status & LEFT) && (!testpiece(p, x - 1, y, current_piece, rot) && x > 0))
					x--;
			if ((status & RIGHT) && (!testpiece(p, x + 1, y, current_piece, rot) && x < 11))
					x++;
			if (status & CLOCKWISE) {
				if (!testpiece(p, x, y, current_piece, rot + 1)) {
					rot++;
					rot &= 0x03;
				} else if (!testpiece(p, x - 1, y, current_piece, rot + 1) && (x > 0)) {
					rot++;
					x--;
					rot &= 0x03;
				} else if (!testpiece(p, x - 2, y, current_piece, rot + 1) && (x > 0)) {
					rot++;
					x--; x--;
					rot &= 0x03;
				} else if (!testpiece(p, x + 1, y, current_piece, rot + 1)) {
					rot++;
					x++;
					rot &= 0x03;
				}
			}
			if (status & ANTICLOCKWISE) {
				if (!testpiece(p, x, y, current_piece, rot - 1)) {
					rot--;
					rot &= 0x03;
				} else if (!testpiece(p, x - 1, y, current_piece, rot - 1) && (x > 0)) {
					rot--;
					x--;
					rot &= 0x03;
				} else if (!testpiece(p, x - 2, y, current_piece, rot - 1) && (x > 0)) {
					rot--;
					x--; x--;
					rot &= 0x03;
				} else if (!testpiece(p, x + 1, y, current_piece, rot - 1)) {
					rot--;
					x++;
					rot &= 0x03;
				}
			}
			drawpiece(x, y, current_piece, rot);
		}
		if (status & DROP) {
			speed_mode = 1;
			if (speed_count > speed[1])
				speed_count = speed[1];
		} else  {
			speed_mode = 0;
		}
		if (status & END_GAME) {
			fade_to_black(plrscr_pal);
			SOUND_sfxmod(volume);
			return;
		}
		if (status & QUIT_TO_OS) {
			fade_to_black(plrscr_pal);
			return;
		}
	}
}


/*
 * traditional_tetris -
 *
 * in  -> void
 * out <- void
 *
 * Lets play traditional tetris
 *
 */

void	traditional_tetris(void)
{
	char	status;
	char	lines;
	char	milestone;
	char	x;		/* position of current_piece */
	char	y;
	char	rot;
	char	speed[2];	/* speed is the number of retraces between drops */
	char	speed_mode;
	char	speed_count;	/* counter used for speed timing putposes */
	char	preview_count;	/* counter for the preview */
	char	current_piece;
	char	next_piece;
	pit	p[416];		/* the pit */

	/*
	 * Load up the 1 Player Screen
	 */
	wide_bitblitX(0, &plrscr);
	filledboxX(116, 145, 203, 218, 0xFC);

	fade_from_black(plrscr_pal);


	quit = 0x10;
	pause = 0x45;
	endgame = 0x01;


	/*
	 * Initialize the player variable
	 */
	player = PLAYER_1;
	p1score = 0;
	p1lines = 0;
	p1level = 0;
	milestone = 19;
	bonus[0] = 1;
	bonus[1] = 5;
	bonus[2] = 10;
	bonus[3] = 20;
	speed[0] = 20;
	speed[1] = 0;
	speed_mode = 0;
	update_stats(0xFF);
	speed_count = speed[speed_mode];
	preview_count = PREVIEW_FRAMES;
	preview_pal();
	x = 4;
	y = 0;
	rot = 0;
	empty_pit(p);
	srand(GrabTick());
	current_piece = gimmepiece();
	next_piece = gimmepiece();

	WaitVsyncStart();
	update_stats(current_piece);
	plr1_score();
	drawpiece(x, y, current_piece, rot);
	plr1_preview(next_piece);

	for (;;) {
		WaitVsyncStart();
		if (speed_count-- == 0) {
			speed_count = speed[speed_mode];
			removepiece(x, y, current_piece, rot);
			y++;
			if (testpiece(p, x, y, current_piece, rot)) {
				SOUND_sfx(0);
				y--;
				splitpiece(x, y, current_piece, rot);
				addpiece(p, x, y, current_piece, rot);

				lines = testlines(p, y);
				if (lines > 0) {
					p1score += bonus[lines - 1];
					p1lines += lines;
					if (p1lines > milestone) {
						milestone += 20;
						p1level++;
						bonus[0] += 1;
						bonus[1] += 5;
						bonus[2] += 10;
						bonus[3] += 20;
						if (speed[0]-- == 1)
							speed[0] = 1;
					}
					plr1_score();
				}
				if (y < 2) {
					filledboxX(120, 54, 199, 103, 0xF6);
					game_over();
					high_score(p1score, 1, p1level, p1lines, &traditional);
					return;
				}
				current_piece = next_piece;
				update_stats(current_piece);
				next_piece = gimmepiece();
				x = 4;
				y = 0;
				rot = 0;
				drawpiece(x, y, current_piece, rot);
				plr1_preview(next_piece);
			} else {
				drawpiece(x, y, current_piece, rot);
			}
		}
		if (preview_count-- == 0) {
			plr1_preview(next_piece);
			preview_count = PREVIEW_FRAMES;
		}

		status = get_p1inp();
		if (status & (LEFT | RIGHT | CLOCKWISE | ANTICLOCKWISE)) {
			removepiece(x, y, current_piece, rot);
			if ((status & LEFT) && (!testpiece(p, x - 1, y, current_piece, rot) && x > 0))
					x--;
			if ((status & RIGHT) && (!testpiece(p, x + 1, y, current_piece, rot) && x < 11))
					x++;
			if (status & CLOCKWISE) {
				if (!testpiece(p, x, y, current_piece, rot + 1)) {
					rot++;
					rot &= 0x03;
				} else if (!testpiece(p, x - 1, y, current_piece, rot + 1) && (x > 0)) {
					rot++;
					x--;
					rot &= 0x03;
				} else if (!testpiece(p, x - 2, y, current_piece, rot + 1) && (x > 0)) {
					rot++;
					x--; x--;
					rot &= 0x03;
				} else if (!testpiece(p, x + 1, y, current_piece, rot + 1)) {
					rot++;
					x++;
					rot &= 0x03;
				}
			}
			if (status & ANTICLOCKWISE) {
				if (!testpiece(p, x, y, current_piece, rot - 1)) {
					rot--;
					rot &= 0x03;
				} else if (!testpiece(p, x - 1, y, current_piece, rot - 1) && (x > 0)) {
					rot--;
					x--;
					rot &= 0x03;
				} else if (!testpiece(p, x - 2, y, current_piece, rot - 1) && (x > 0)) {
					rot--;
					x--; x--;
					rot &= 0x03;
				} else if (!testpiece(p, x + 1, y, current_piece, rot - 1)) {
					rot--;
					x++;
					rot &= 0x03;
				}
			}
			drawpiece(x, y, current_piece, rot);
		}

		if (status & DROP) {
			speed_mode = 1;
			if (speed_count > speed[1])
				speed_count = speed[1];
		} else  {
			speed_mode = 0;
		}
		if (status & END_GAME) {
			fade_to_black(plrscr_pal);
			SOUND_sfxmod(volume);
			return;
		}
		if (status & QUIT_TO_OS) {
			fade_to_black(plrscr_pal);
			return;
		}
	}
}


void	competition_tetris(void)
{
	char	status;
	char	preview_count;		/* counter for the preview */
	char	i;
	char	t_s[12];
	short	sx;
	short	sy;

	char	lines;
	char	milestone;
	char	level;

	char	p1x;			/* position of current_piece */
	char	p1y;
	char	p1rot;
	char	p1speed[2];		/* speed is the number of retraces between drops */
	char	p1speed_mode;
	char	p1speed_count;		/* counter used for speed timing putposes */
	char	p1current_piece;
	char	p1next_piece;
	pit	p1[416];		/* the pit */
	short	p1wins = 0;

	char	p2x;			/* position of current_piece */
	char	p2y;
	char	p2rot;
	char	p2speed[2];		/* speed is the number of retraces between drops */
	char	p2speed_mode;
	char	p2speed_count;		/* counter used for speed timing putposes */
	char	p2current_piece;
	char	p2next_piece;
	pit	p2[416];
	short	p2wins = 0;

	char	*scrn;

	/*
	 * Load up the 1 Player Screen
	 * and then, lets make some changes
	 */

	wide_bitblitX(0, &plrscr);
	filledboxX(116, 145, 203, 218, 0xFC);
	scrn = (char *) (0xA000 << 4) + activeStart;
	scrn += 348;
	CopyVideoDown(scrn, scrn + 8720, 108);
	lineX(211, 113, 211, 133, 0xFC);
	putpixelX(159, 119, 0xFC);
	putpixelX(159, 120, 0xFC);
	putpixelX(159, 121, 0xFC);
	putpixelX(159, 122, 0xFC);
	putpixelX(160, 122, 0xFC);
	putpixelX(161, 119, 0x00);
	putpixelX(161, 120, 0x00);
	putpixelX(161, 121, 0x00);
	putpixelX(162, 122, 0xFC);
	putpixelX(163, 119, 0xFC);
	putpixelX(163, 120, 0xFC);
	putpixelX(163, 121, 0xFC);
	putpixelX(163, 122, 0xFC);
	putpixelX(166, 119, 0xFC);
	putpixelX(166, 121, 0x00);
	putpixelX(172, 120, 0xFC);
	putpixelX(173, 120, 0xFC);
	putpixelX(174, 120, 0xFC);
	putpixelX(175, 119, 0x00);
	putpixelX(175, 120, 0x00);
	putpixelX(175, 121, 0x00);
	fade_from_black(plrscr_pal);

	quit = 0x10;
	pause = 0x45;
	endgame = 0x01;


	/*
	 * Initialize the player variables
	 */
	centrestring(PLAYER_1 + 60, 226, "0 WINS");
	centrestring(PLAYER_2 + 60, 226, "0 WINS");

restart_point:
	srand(GrabTick());

	p1score = 0;
	p1lines = 0;
	p1level = 0;
	p1speed[0] = 20;
	p1speed[1] = 0;
	p1speed_mode = 0;
	p1speed_count = p1speed[p1speed_mode];
	p1x = 4;
	p1y = 0;
	p1rot = 0;
	empty_pit(p1);
	p1current_piece = gimmepiece();
	p1next_piece = gimmepiece();

	p2score = 0;
	p2lines = 0;
	p2level = 0;
	p2speed[0] = 20;
	p2speed[1] = 0;
	p2speed_mode = 0;
	p2speed_count = p2speed[p2speed_mode];
	p2x = 4;
	p2y = 0;
	p2rot = 0;
	empty_pit(p2);
	p2current_piece = gimmepiece();
	p2next_piece = gimmepiece();

	level = 0;
	milestone = 19;
	bonus[0] = 1;
	bonus[1] = 5;
	bonus[2] = 10;
	bonus[3] = 20;

	preview_count = PREVIEW_FRAMES;
	preview_pal();

	WaitVsyncStart();
	filledboxX(120, 16, 200, 26, 0xFC);
	filledboxX(120, 34, 200, 44, 0xFC);
	filledboxX(120, 125, 200, 135, 0xFC);
	filledboxX(120, 143, 200, 153, 0xFC);
	plr1_score();
	plr2_score();

	player = PLAYER_1;
	drawpiece(p1x, p1y, p1current_piece, p1rot);
	plr1_preview(p1next_piece);

	player = PLAYER_2;
	drawpiece(p2x, p2y, p2current_piece, p2rot);
	plr2_preview(p2next_piece);

	for (;;) {

		WaitVsyncStart();

		player = PLAYER_1;
		if (p1speed_count-- == 0) {
			p1speed_count = p1speed[p1speed_mode];
			removepiece(p1x, p1y, p1current_piece, p1rot);
			p1y++;
			if (testpiece(p1, p1x, p1y, p1current_piece, p1rot)) {
				SOUND_sfx(0);
				p1y--;
				splitpiece(p1x, p1y, p1current_piece, p1rot);
				addpiece(p1, p1x, p1y, p1current_piece, p1rot);

				lines = two_testlines(p1, p1y);
				if (lines > 0) {
					p1score += bonus[lines - 1];
					p1lines += lines;
					if (p1lines > milestone) {
						milestone += 20;
						p1level++;
						bonus[0] += 1;
						bonus[1] += 5;
						bonus[2] += 10;
						bonus[3] += 20;
						if (p1speed[0]-- == 1)
							p1speed[0] = 1;
					}
					plr1_score();
				}
				for (i = lines; i > 1; i--) {
					player = PLAYER_2;
					SOUND_sfx(6);
					removepiece(p2x, p2y, p2current_piece, p2rot);
					if (advance_line(p2)) {
						if (testpiece(p2, p2x, p2y, p2current_piece, p2rot))
							p2y--;
						drawpiece(p2x, p2y, p2current_piece, p2rot);
						goto p2_gameover;
					}
					if (testpiece(p2, p2x, p2y, p2current_piece, p2rot)) {
						p2y--;
						if (p2y < 2)
							goto p2_gameover;
						p2speed_count = 0;
					}
					drawpiece(p2x, p2y, p2current_piece, p2rot);
				}
				player = PLAYER_1;
				if (p1y < 2) {
				p1_gameover:
					filledboxX(120, 54, 199, 103, 0xF6);
					filledboxX(120, 163, 199, 212, 0xF6);
					game_over();
					sleep(1);
					for (i = 0; i < 200; i++) {
						WaitVsyncStart();
						sx = random[i] % 10 + 1;
						sy = random[i] / 10 + 2;
						filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
						i++;
						sx = random[i] % 10 + 1;
						sy = random[i] / 10 + 2;
						filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
						i++;
						sx = random[i] % 10 + 1;
						sy = random[i] / 10 + 2;
						filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
						i++;
						sx = random[i] % 10 + 1;
						sy = random[i] / 10 + 2;
						filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
						i++;
						sx = random[i] % 10 + 1;
						sy = random[i] / 10 + 2;
						filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
						i++;
						sx = random[i] % 10 + 1;
						sy = random[i] / 10 + 2;
						filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
						i++;
						sx = random[i] % 10 + 1;
						sy = random[i] / 10 + 2;
						filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
						i++;
						sx = random[i] % 10 + 1;
						sy = random[i] / 10 + 2;
						filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
					}
					sleep(1);

					filledboxX(player + 10, 20, player + 109, 219, 0xF6);
					centrestring(player + 60, 45, "PLAYER 2");
					i = rand();
					i %= 11;
					centrestring(player + 60, 60, insults[i].msg1);
					centrestring(player + 60, 75, insults[i].msg2);
					centrestring(player + 60, 90, insults[i].msg3);
					centrestring(player + 60, 120, "REMATCH ?");
					centrestring(player + 60, 135, "Y OR N");
					p2wins++;
					sprintf(t_s, "%d WINS", p2wins);
					filledboxX(PLAYER_2 + 10, 226, PLAYER_2 + 109, 236, 0xFC);
					centrestring(PLAYER_2 + 60, 226, t_s);
					while(1) {
						i = gimme_key();
						if (i == 0x31) {
							SOUND_sfxmod(volume);
							fade_to_black(plrscr_pal);
							return;
						}
						if (i == 0x15) {
							filledboxX(player + 10, 0, player + 109, 219, 0xF6);
							player = PLAYER_2;
							filledboxX(player + 10, 0, player + 109, 219, 0xF6);
							goto restart_point;
						}
					}
				}
				p1current_piece = p1next_piece;
				p1next_piece = gimmepiece();
				p1x = 4;
				p1y = 0;
				p1rot = 0;
				drawpiece(p1x, p1y, p1current_piece, p1rot);
			} else {
				drawpiece(p1x, p1y, p1current_piece, p1rot);
			}
		}

		player = PLAYER_2;
		if (p2speed_count-- == 0) {
			p2speed_count = p2speed[p2speed_mode];
			removepiece(p2x, p2y, p2current_piece, p2rot);
			p2y++;
			if (testpiece(p2, p2x, p2y, p2current_piece, p2rot)) {
				SOUND_sfx(0);
				p2y--;
				splitpiece(p2x, p2y, p2current_piece, p2rot);
				addpiece(p2, p2x, p2y, p2current_piece, p2rot);

				lines = two_testlines(p2, p2y);
				if (lines > 0) {
					p2score += bonus[lines - 1];
					p2lines += lines;
					if (p2lines > milestone) {
						milestone += 20;
						p2level++;
						bonus[0] += 1;
						bonus[1] += 5;
						bonus[2] += 10;
						bonus[3] += 20;
						if (p2speed[0]-- == 1)
							p2speed[0] = 1;
					}
					plr2_score();
				}
				for (i = lines; i > 1; i--) {
					player = PLAYER_1;
					SOUND_sfx(6);
					removepiece(p1x, p1y, p1current_piece, p1rot);
					if (advance_line(p1)) {
						if (testpiece(p1, p1x, p1y, p1current_piece, p1rot))
							p1y--;
						drawpiece(p1x, p1y, p1current_piece, p1rot);
						goto p1_gameover;
					}
					if (testpiece(p1, p1x, p1y, p1current_piece, p1rot)) {
						p1y--;
						if (p1y < 2)
							goto p1_gameover;
						p1speed_count = 0;
					}
					drawpiece(p1x, p1y, p1current_piece, p1rot);
				}
				player = PLAYER_2;
				if (p2y < 2) {
				p2_gameover:
					filledboxX(120, 54, 199, 103, 0xF6);
					filledboxX(120, 163, 199, 212, 0xF6);
					game_over();

					sleep(1);
					for (i = 0; i < 200; i++) {
						WaitVsyncStart();
						sx = random[i] % 10 + 1;
						sy = random[i] / 10 + 2;
						filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
						i++;
						sx = random[i] % 10 + 1;
						sy = random[i] / 10 + 2;
						filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
						i++;
						sx = random[i] % 10 + 1;
						sy = random[i] / 10 + 2;
						filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
						i++;
						sx = random[i] % 10 + 1;
						sy = random[i] / 10 + 2;
						filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
						i++;
						sx = random[i] % 10 + 1;
						sy = random[i] / 10 + 2;
						filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
						i++;
						sx = random[i] % 10 + 1;
						sy = random[i] / 10 + 2;
						filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
						i++;
						sx = random[i] % 10 + 1;
						sy = random[i] / 10 + 2;
						filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
						i++;
						sx = random[i] % 10 + 1;
						sy = random[i] / 10 + 2;
						filledboxX(sx * 10 + player, sy * 10, sx * 10 + player + 9, sy * 10 + 9, 0xF6);
					}
					sleep(1);

					filledboxX(player + 10, 20, player + 109, 219, 0xF6);
					centrestring(player + 60, 45, "PLAYER 1");
					i = rand();
					i %= 11;
					centrestring(player + 60, 60, insults[i].msg1);
					centrestring(player + 60, 75, insults[i].msg2);
					centrestring(player + 60, 90, insults[i].msg3);
					centrestring(player + 60, 120, "REMATCH ?");
					centrestring(player + 60, 135, "Y OR N");
					p1wins++;
					sprintf(t_s, "%d WINS", p1wins);
					filledboxX(PLAYER_1 + 10, 226, PLAYER_1 + 109, 236, 0xFC);
					centrestring(PLAYER_1 + 60, 226, t_s);
					while(1) {
						i = gimme_key();
						if (i == 0x31) {
							SOUND_sfxmod(volume);
							fade_to_black(plrscr_pal);
							return;
						}
						if (i == 0x15) {
							filledboxX(player + 10, 0, player + 109, 219, 0xF6);
							player = PLAYER_1;
							filledboxX(player + 10, 0, player + 109, 219, 0xF6);
							goto restart_point;
						}
					}

				}
				p2current_piece = p2next_piece;
				p2next_piece = gimmepiece();
				p2x = 4;
				p2y = 0;
				p2rot = 0;
				drawpiece(p2x, p2y, p2current_piece, p2rot);
			} else {
				drawpiece(p2x, p2y, p2current_piece, p2rot);
			}
		}

		if (preview_count == 1) {
			plr2_preview(p2next_piece);
		}
		if (preview_count-- == 0) {
			plr1_preview(p1next_piece);
			preview_count = PREVIEW_FRAMES;
		}

		status = get_p1inp();
		player = PLAYER_1;
		if (status & (LEFT | RIGHT | CLOCKWISE | ANTICLOCKWISE)) {
			removepiece(p1x, p1y, p1current_piece, p1rot);
			if ((status & LEFT) && (!testpiece(p1, p1x - 1, p1y, p1current_piece, p1rot) && p1x > 0))
					p1x--;
			if ((status & RIGHT) && (!testpiece(p1, p1x + 1, p1y, p1current_piece, p1rot) && p1x < 11))
					p1x++;
			if (status & CLOCKWISE) {
				if (!testpiece(p1, p1x, p1y, p1current_piece, p1rot + 1)) {
					p1rot++;
					p1rot &= 0x03;
				} else if (!testpiece(p1, p1x - 1, p1y, p1current_piece, p1rot + 1) && (p1x > 0)) {
					p1rot++;
					p1x--;
					p1rot &= 0x03;
				} else if (!testpiece(p1, p1x - 2, p1y, p1current_piece, p1rot + 1) && (p1x > 0)) {
					p1rot++;
					p1x--; p1x--;
					p1rot &= 0x03;
				} else if (!testpiece(p1, p1x + 1, p1y, p1current_piece, p1rot + 1)) {
					p1rot++;
					p1x++;
					p1rot &= 0x03;
				}
			}
			if (status & ANTICLOCKWISE) {
				if (!testpiece(p1, p1x, p1y, p1current_piece, p1rot - 1)) {
					p1rot--;
					p1rot &= 0x03;
				} else if (!testpiece(p1, p1x - 1, p1y, p1current_piece, p1rot - 1) && (p1x > 0)) {
					p1rot--;
					p1x--;
					p1rot &= 0x03;
				} else if (!testpiece(p1, p1x - 2, p1y, p1current_piece, p1rot - 1) && (p1x > 0)) {
					p1rot--;
					p1x--; p1x--;
					p1rot &= 0x03;
				} else if (!testpiece(p1, p1x + 1, p1y, p1current_piece, p1rot - 1)) {
					p1rot--;
					p1x++;
					p1rot &= 0x03;
				}
			}
			drawpiece(p1x, p1y, p1current_piece, p1rot);
		}
		if (status & DROP) {
			p1speed_mode = 1;
			if (p1speed_count > p1speed[1])
				p1speed_count = p1speed[1];
		} else  {
			p1speed_mode = 0;
		}

		status = get_p2inp();
		player = PLAYER_2;
		if (status & (LEFT | RIGHT | CLOCKWISE | ANTICLOCKWISE)) {
			removepiece(p2x, p2y, p2current_piece, p2rot);
			if ((status & LEFT) && (!testpiece(p2, p2x - 1, p2y, p2current_piece, p2rot) && p2x > 0))
					p2x--;
			if ((status & RIGHT) && (!testpiece(p2, p2x + 1, p2y, p2current_piece, p2rot) && p2x < 11))
					p2x++;
			if (status & CLOCKWISE) {
				if (!testpiece(p2, p2x, p2y, p2current_piece, p2rot + 1)) {
					p2rot++;
					p2rot &= 0x03;
				} else if (!testpiece(p2, p2x - 1, p2y, p2current_piece, p2rot + 1) && (p2x > 0)) {
					p2rot++;
					p2x--;
					p2rot &= 0x03;
				} else if (!testpiece(p2, p2x - 2, p2y, p2current_piece, p2rot + 1) && (p2x > 0)) {
					p2rot++;
					p2x--; p2x--;
					p2rot &= 0x03;
				} else if (!testpiece(p2, p2x + 1, p2y, p2current_piece, p2rot + 1)) {
					p2rot++;
					p2x++;
					p2rot &= 0x03;
				}
			}
			if (status & ANTICLOCKWISE) {
				if (!testpiece(p2, p2x, p2y, p2current_piece, p2rot - 1)) {
					p2rot--;
					p2rot &= 0x03;
				} else if (!testpiece(p2, p2x - 1, p2y, p2current_piece, p2rot - 1) && (p2x > 0)) {
					p2rot--;
					p2x--;
					p2rot &= 0x03;
				} else if (!testpiece(p2, p2x - 2, p2y, p2current_piece, p2rot - 1) && (p2x > 0)) {
					p2rot--;
					p2x--; p2x--;
					p2rot &= 0x03;
				} else if (!testpiece(p2, p2x + 1, p2y, p2current_piece, p2rot - 1)) {
					p2rot--;
					p2x++;
					p2rot &= 0x03;
				}
			}
			drawpiece(p2x, p2y, p2current_piece, p2rot);
		}
		if (status & DROP) {
			p2speed_mode = 1;
			if (p2speed_count > p2speed[1])
				p2speed_count = p2speed[1];
		} else  {
			p2speed_mode = 0;
		}

		if (status & END_GAME) {
			fade_to_black(plrscr_pal);
			SOUND_sfxmod(volume);
			return;
		}
		if (status & QUIT_TO_OS) {
			fade_to_black(plrscr_pal);
			return;
		}
	}
}


void __interrupt __far kbhandler(void)
{
	char 		latch;
	char		toggle;
	char		key;
	static char	extended;

	key = inp(0x60);
	latch = ~key & 0x80;

	if (extended == 2) {
		extended--;
	} else if (extended == 1) {
		extended--;
		if (key != 0x2A) {
			key &= 0x7F;
			anykey = key;
			keys[key] = latch;
		}
	} else {
		if (key == 0xE0)
			extended = 1;
		else if (key == 0xE1)
			extended = 2;
		else {
			extended = 0;
			key &= 0x7F;
			anykey = key;
			keys[key] = latch;
		}
	}

	toggle = inp(0x61);
	outp(0x61, toggle | 0x80);
	outp(0x61, toggle & 0x7F);
	outp(0x20, 0x20);
}


void kbinst(void)
{
	oldhandler = _dos_getvect(0x09);
	_dos_setvect(0x09, kbhandler);
}


void kbdeinst(void)
{
	_dos_setvect(0x09, oldhandler);
}


