/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_VT100_H__
#define SHELL_VT100_H__

#define SHELL_VT100_ASCII_ESC		(0x1b)
#define SHELL_VT100_ASCII_DEL		(0x7F)
#define SHELL_VT100_ASCII_BSPACE	(0x08)
#define SHELL_VT100_ASCII_CTRL_A	(0x01)
#define SHELL_VT100_ASCII_CTRL_B	(0x02)
#define SHELL_VT100_ASCII_CTRL_C	(0x03)
#define SHELL_VT100_ASCII_CTRL_D	(0x04)
#define SHELL_VT100_ASCII_CTRL_E	(0x05)
#define SHELL_VT100_ASCII_CTRL_F	(0x06)
#define SHELL_VT100_ASCII_CTRL_K	(0x0B)
#define SHELL_VT100_ASCII_CTRL_L	(0x0C)
#define SHELL_VT100_ASCII_CTRL_N	(0x0E)
#define SHELL_VT100_ASCII_CTRL_P	(0x10)
#define SHELL_VT100_ASCII_CTRL_U	(0x15)
#define SHELL_VT100_ASCII_CTRL_W	(0x17)
#define SHELL_VT100_ASCII_ALT_B		(0x62)
#define SHELL_VT100_ASCII_ALT_F		(0x66)
#define SHELL_VT100_ASCII_ALT_R		(0x72)

/* Set new line mode */
#define SHELL_VT100_SETNL		"\e[20h\0"
/* Set cursor key to application */
#define SHELL_VT100_SETAPPL		"\e[?1h\0"
/* Set number of columns to 132 */
#define SHELL_VT100_SETCOL_132		"\e[?3h\0"
/* Set smooth scrolling */
#define SHELL_VT100_SETSMOOTH		"\e[?4h\0"
/* Set reverse video on screen */
#define SHELL_VT100_SETREVSCRN		"\e[?5h\0"
/* Set origin to relative */
#define SHELL_VT100_SETORGREL		"\e[?6h\0"
/* Set auto-wrap mode */
#define SHELL_VT100_SETWRAP_ON		"\e[?7h\0"
/* Set auto-wrap mode */
#define SHELL_VT100_SETWRAP_OFF		"\e[?7l\0"
/* Set auto-repeat mode */
#define SHELL_VT100_SETREP		"\e[?8h\0"
/* Set interlacing mode */
#define SHELL_VT100_SETINTER		"\e[?9h\0"
/* Set line feed mode */
#define SHELL_VT100_SETLF		"\e[20l\0"
/* Set cursor key to cursor */
#define SHELL_VT100_SETCURSOR		"\e[?1l\0"
/* Set VT52 (versus ANSI) */
#define SHELL_VT100_SETVT52		"\e[?2l\0"
/* Set number of columns to 80 */
#define SHELL_VT100_SETCOL_80		"\e[?3l\0"
/* Set jump scrolling */
#define SHELL_VT100_SETJUMP		"\e[?4l\0"
/* Set normal video on screen */
#define SHELL_VT100_SETNORMSCRN		"\e[?5l\0"
/* Set origin to absolute */
#define SHELL_VT100_SETORGABS		"\e[?6l\0"
/* Reset auto-wrap mode */
#define SHELL_VT100_RESETWRAP		"\e[?7l\0"
/* Reset auto-repeat mode */
#define SHELL_VT100_RESETREP		"\e[?8l\0"
/* Reset interlacing mode */
#define SHELL_VT100_RESETINTER		"\e[?9l\0"
/* Set alternate keypad mode */
#define SHELL_VT100_ALTKEYPAD		"\e=\0"
/* Set numeric keypad mode */
#define SHELL_VT100_NUMKEYPAD		"\e>\0"
/* Set United Kingdom G0 character set */
#define SHELL_VT100_SETUKG0		"\e(A\0"
/* Set United Kingdom G1 character set */
#define SHELL_VT100_SETUKG1		"\e)A\0"
/* Set United States G0 character set */
#define SHELL_VT100_SETUSG0		"\e(B\0"
/* Set United States G1 character set */
#define SHELL_VT100_SETUSG1		"\e)B\0"
/* Set G0 special chars. & line set */
#define SHELL_VT100_SETSPECG0		"\e(0\0"
/* Set G1 special chars. & line set */
#define SHELL_VT100_SETSPECG1		"\e)0\0"
/* Set G0 alternate character ROM */
#define SHELL_VT100_SETALTG0		"\e(1\0"
/* Set G1 alternate character ROM */
#define SHELL_VT100_SETALTG1		"\e)1\0"
/* Set G0 alt char ROM and spec. graphics */
#define SHELL_VT100_SETALTSPECG0	"\e(2\0"
/* Set G1 alt char ROM and spec. graphics */
#define SHELL_VT100_SETALTSPECG1	"\e)2\0"
/* Set single shift 2 */
#define SHELL_VT100_SETSS2		"\eN\0"
/* Set single shift 3 */
#define SHELL_VT100_SETSS3		"\eO\0"
/* Turn off character attributes */
#define SHELL_VT100_MODESOFF		"\e[m\0"
/* Turn off character attributes */
#define SHELL_VT100_MODESOFF_		"\e[0m\0"
/* Turn bold mode on */
#define SHELL_VT100_BOLD		"\e[1m\0"
/* Turn low intensity mode on */
#define SHELL_VT100_LOWINT		"\e[2m\0"
/* Turn underline mode on */
#define SHELL_VT100_UNDERLINE		"\e[4m\0"
/* Turn blinking mode on */
#define SHELL_VT100_BLINK		"\e[5m\0"
/* Turn reverse video on */
#define SHELL_VT100_REVERSE		"\e[7m\0"
/* Turn invisible text mode on */
#define SHELL_VT100_INVISIBLE		"\e[8m\0"
/* Move cursor to upper left corner */
#define SHELL_VT100_CURSORHOME		"\e[H\0"
/* Move cursor to upper left corner */
#define SHELL_VT100_CURSORHOME_		"\e[;H\0"
/* Move cursor to upper left corner */
#define SHELL_VT100_HVHOME		"\e[f\0"
/* Move cursor to upper left corner */
#define SHELL_VT100_HVHOME_		"\e[;f\0"
/* Move/scroll window up one line */
#define SHELL_VT100_INDEX		"\eD\0"
/* Move/scroll window down one line */
#define SHELL_VT100_REVINDEX		"\eM\0"
/* Move to next line */
#define SHELL_VT100_NEXTLINE		"\eE\0"
/* Save cursor position and attributes */
#define SHELL_VT100_SAVECURSOR		"\e7\0"
/* Restore cursor position and attribute */
#define SHELL_VT100_RESTORECURSOR	"\e8\0"
/* Set a tab at the current column */
#define SHELL_VT100_TABSET		"\eH\0"
/* Clear a tab at the current column */
#define SHELL_VT100_TABCLR		"\e[g\0"
/* Clear a tab at the current column */
#define SHELL_VT100_TABCLR_		"\e[0g\0"
/* Clear all tabs */
#define SHELL_VT100_TABCLRALL		"\e[3g\0"
/* Double-height letters, top half */
#define SHELL_VT100_DHTOP		"\e#3\0"
/* Double-height letters, bottom hal */
#define SHELL_VT100_DHBOT		"\e#4\0"
/* Single width, single height letters */
#define SHELL_VT100_SWSH		"\e#5\0"
/* Double width, single height letters */
#define SHELL_VT100_DWSH		"\e#6\0"
/* Clear line from cursor right */
#define SHELL_VT100_CLEAREOL		"\e[K\0"
/* Clear line from cursor right */
#define SHELL_VT100_CLEAREOL_		"\e[0K\0"
/* Clear line from cursor left */
#define SHELL_VT100_CLEARBOL		"\e[1K\0"
/* Clear entire line */
#define SHELL_VT100_CLEARLINE		"\e[2K\0"
/* Clear screen from cursor down */
#define SHELL_VT100_CLEAREOS		"\e[J\0"
/* Clear screen from cursor down */
#define SHELL_VT100_CLEAREOS_		"\e[0J\0"
/* Clear screen from cursor up */
#define SHELL_VT100_CLEARBOS		"\e[1J\0"
/* Clear entire screen */
#define SHELL_VT100_CLEARSCREEN		"\e[2J\0"
/* Device status report */
#define SHELL_VT100_DEVSTAT		"\e5n\0"
/* Response: terminal is OK */
#define SHELL_VT100_TERMOK		"\e0n\0"
/* Response: terminal is OK */
#define SHELL_VT100_TERMNOK		"\e3n\0"
/* Get cursor position */
#define SHELL_VT100_GETCURSOR		"\e[6n\0"
/* Response: cursor is at v,h */
#define SHELL_VT100_CURSORPOSAT		"\e, (v);', (h)R\0"
/* Identify what terminal type */
#define SHELL_VT100_IDENT		"\e[c\0"
/* Identify what terminal type */
#define SHELL_VT100_IDENT_		"\e[0c\0"
/* Response: terminal type code n */
#define SHELL_VT100_GETTYPE		"\e[?1;', (n)0c\0"
/*Reset terminal to initial state */
#define SHELL_VT100_RESET		"\ec\0"
/* Screen alignment display */
#define SHELL_VT100_ALIGN		"\e#8\0"
/* Confidence power up test */
#define SHELL_VT100_TESTPU		"\e[2;1y\0"
/* Confidence loopback test */
#define SHELL_VT100_TESTLB		"\e[2;2y\0"
/* Repeat power up test */
#define SHELL_VT100_TESTPUREP		"\e[2;9y\0"
/* Repeat loopback test */
#define SHELL_VT100_TESTLBREP		"\e[2;10y\0"
/* Turn off all four leds */
#define SHELL_VT100_LEDSOFF		"\e[0q\0"
/* Turn on LED #1 */
#define SHELL_VT100_LED1		"\e[1q\0"
/* Turn on LED #2 */
#define SHELL_VT100_LED2		"\e[2q\0"
/* Turn on LED #3 */
#define SHELL_VT100_LED3		"\e[3q\0"
/* Turn on LED #4 */
#define SHELL_VT100_LED4		"\e[4q\0"

/* Function Keys */
#define SHELL_VT100_PF1			"\eOP\0"
#define SHELL_VT100_PF2			"\eOQ\0"
#define SHELL_VT100_PF3			"\eOR\0"
#define SHELL_VT100_PF4			"\eOS\0"

/* Arrow keys */
#define SHELL_VT100_UP_RESET		"\eA\0"
#define SHELL_VT100_UP_SET		"\eOA\0"
#define SHELL_VT100_DOWN_RESET		"\eB\0"
#define SHELL_VT100_DOWN_SET		"\eOB\0"
#define SHELL_VT100_RIGHT_RESET		"\eC\0"
#define SHELL_VT100_RIGHT_SET		"\eOC\0"
#define SHELL_VT100_LEFT_RESET		"\eD\0"
#define SHELL_VT100_LEFT_SET		"\eOD\0"

/* Numeric Keypad Keys */
#define SHELL_VT100_NUMERIC_0		"0\0"
#define SHELL_VT100_ALT_0		"\eOp\0"
#define SHELL_VT100_NUMERIC_1		"1\0"
#define SHELL_VT100_ALT_1		"\eOq\0"
#define SHELL_VT100_NUMERIC_2		"2\0"
#define SHELL_VT100_ALT_2		"\eOr\0"
#define SHELL_VT100_NUMERIC_3		"3\0"
#define SHELL_VT100_ALT_3		"\eOs\0"
#define SHELL_VT100_NUMERIC_4		"4\0"
#define SHELL_VT100_ALT_4		"\eOt\0"
#define SHELL_VT100_NUMERIC_5		"5\0"
#define SHELL_VT100_ALT_5		"\eOu\0"
#define SHELL_VT100_NUMERIC_6		"6\0"
#define SHELL_VT100_ALT_6		"\eOv\0"
#define SHELL_VT100_NUMERIC_7		"7\0"
#define SHELL_VT100_ALT_7		"\eOw\0"
#define SHELL_VT100_NUMERIC_8		"8\0"
#define SHELL_VT100_ALT_8		"\eOx\0"
#define SHELL_VT100_NUMERIC_9		"9\0"
#define SHELL_VT100_ALT_9		"\eOy\0"
#define SHELL_VT100_NUMERIC_MINUS	"-\0"
#define SHELL_VT100_ALT_MINUS		"\eOm\0"
#define SHELL_VT100_NUMERIC_COMMA	",\0"
#define SHELL_VT100_ALT_COMMA		"\eOl\0"
#define SHELL_VT100_NUMERIC_ENTER	ASCII_CR
#define SHELL_VT100_NUMERIC_PERIOD	".\0"
#define SHELL_VT100_ALT_PERIOD		"\eOn\0"
#define SHELL_VT100_ALT_ENTER		"\eOM\0"

#endif /* SHELL_VT100_H__ */
