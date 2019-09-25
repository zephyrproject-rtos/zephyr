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

#define SHELL_VT100_SETNL				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '2', '0', 'h', '\0'	\
} /* Set new line mode */
#define SHELL_VT100_SETAPPL				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '?', '1', 'h', '\0'	\
} /* Set cursor key to application */
#define SHELL_VT100_SETCOL_132				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '?', '3', 'h', '\0'	\
} /* Set number of columns to 132 */
#define SHELL_VT100_SETSMOOTH				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '?', '4', 'h', '\0'	\
} /* Set smooth scrolling */
#define SHELL_VT100_SETREVSCRN				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '?', '5', 'h', '\0'	\
} /* Set reverse video on screen */
#define SHELL_VT100_SETORGREL				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '?', '6', 'h', '\0'	\
} /* Set origin to relative */
#define SHELL_VT100_SETWRAP_ON				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '?', '7', 'h', '\0'	\
} /* Set auto-wrap mode */
#define SHELL_VT100_SETWRAP_OFF				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '?', '7', 'l', '\0'	\
} /* Set auto-wrap mode */

#define SHELL_VT100_SETREP				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '?', '8', 'h', '\0'	\
} /* Set auto-repeat mode */
#define SHELL_VT100_SETINTER				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '?', '9', 'h', '\0'	\
} /* Set interlacing mode */

#define SHELL_VT100_SETLF				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '2', '0', 'l', '\0'	\
} /* Set line feed mode */
#define SHELL_VT100_SETCURSOR				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '?', '1', 'l', '\0'	\
} /* Set cursor key to cursor */
#define SHELL_VT100_SETVT52				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '?', '2', 'l', '\0'	\
} /* Set VT52 (versus ANSI) */
#define SHELL_VT100_SETCOL_80				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '?', '3', 'l', '\0'	\
} /* Set number of columns to 80 */
#define SHELL_VT100_SETJUMP				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '?', '4', 'l', '\0'	\
} /* Set jump scrolling */
#define SHELL_VT100_SETNORMSCRN				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '?', '5', 'l', '\0'	\
} /* Set normal video on screen */
#define SHELL_VT100_SETORGABS				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '?', '6', 'l', '\0'	\
} /* Set origin to absolute */
#define SHELL_VT100_RESETWRAP				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '?', '7', 'l', '\0'	\
} /* Reset auto-wrap mode */
#define SHELL_VT100_RESETREP				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '?', '8', 'l', '\0'	\
} /* Reset auto-repeat mode */
#define SHELL_VT100_RESETINTER				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '?', '9', 'l', '\0'	\
} /* Reset interlacing mode */

#define SHELL_VT100_ALTKEYPAD			\
{						\
	SHELL_VT100_ASCII_ESC, '=', '\0'	\
} /* Set alternate keypad mode */
#define SHELL_VT100_NUMKEYPAD			\
{						\
	SHELL_VT100_ASCII_ESC, '>', '\0'	\
} /* Set numeric keypad mode */

#define SHELL_VT100_SETUKG0			\
{						\
	SHELL_VT100_ASCII_ESC, '(', 'A', '\0'	\
} /* Set United Kingdom G0 character set */
#define SHELL_VT100_SETUKG1			\
{						\
	SHELL_VT100_ASCII_ESC, ')', 'A', '\0'	\
} /* Set United Kingdom G1 character set */
#define SHELL_VT100_SETUSG0			\
{						\
	SHELL_VT100_ASCII_ESC, '(', 'B', '\0'	\
} /* Set United States G0 character set */
#define SHELL_VT100_SETUSG1			\
{						\
	SHELL_VT100_ASCII_ESC, ')', 'B', '\0'	\
} /* Set United States G1 character set */
#define SHELL_VT100_SETSPECG0			\
{						\
	SHELL_VT100_ASCII_ESC, '(', '0', '\0'	\
} /* Set G0 special chars. & line set */
#define SHELL_VT100_SETSPECG1			\
{						\
	SHELL_VT100_ASCII_ESC, ')', '0', '\0'	\
} /* Set G1 special chars. & line set */
#define SHELL_VT100_SETALTG0			\
{						\
	SHELL_VT100_ASCII_ESC, '(', '1', '\0'	\
} /* Set G0 alternate character ROM */
#define SHELL_VT100_SETALTG1			\
{						\
	SHELL_VT100_ASCII_ESC, ')', '1', '\0'	\
} /* Set G1 alternate character ROM */
#define SHELL_VT100_SETALTSPECG0		\
{						\
	SHELL_VT100_ASCII_ESC, '(', '2', '\0'	\
} /* Set G0 alt char ROM and spec. graphics */
#define SHELL_VT100_SETALTSPECG1		\
{						\
	SHELL_VT100_ASCII_ESC, ')', '2', '\0'	\
} /* Set G1 alt char ROM and spec. graphics */

#define SHELL_VT100_SETSS2			\
{						\
	SHELL_VT100_ASCII_ESC, 'N', '\0'	\
} /* Set single shift 2 */
#define SHELL_VT100_SETSS3			\
{						\
	SHELL_VT100_ASCII_ESC, 'O', '\0'	\
} /* Set single shift 3 */

#define SHELL_VT100_MODESOFF			\
{						\
	SHELL_VT100_ASCII_ESC, '[', 'm', '\0'	\
} /* Turn off character attributes */
#define SHELL_VT100_MODESOFF_				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '0', 'm', '\0'	\
} /* Turn off character attributes */
#define SHELL_VT100_BOLD				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '1', 'm', '\0'	\
} /* Turn bold mode on */
#define SHELL_VT100_LOWINT				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '2', 'm', '\0'	\
} /* Turn low intensity mode on */
#define SHELL_VT100_UNDERLINE				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '4', 'm', '\0'	\
} /* Turn underline mode on */
#define SHELL_VT100_BLINK				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '5', 'm', '\0'	\
} /* Turn blinking mode on */
#define SHELL_VT100_REVERSE				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '7', 'm', '\0'	\
} /* Turn reverse video on */
#define SHELL_VT100_INVISIBLE				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '8', 'm', '\0'	\
} /* Turn invisible text mode on */

#define SHELL_VT100_SETWIN(t, b)				\
{								\
	SHELL_VT100_ASCII_ESC, '[', (t), ';', (b), 'r', '\0'	\
} /* Set top and bottom line#s of a window */

#define SHELL_VT100_CURSORUP(n)				\
{							\
	SHELL_VT100_ASCII_ESC, '[', (n), 'A', '\0'	\
} /* Move cursor up n lines */
#define SHELL_VT100_CURSORDN(n)				\
{							\
	SHELL_VT100_ASCII_ESC, '[', (n), 'B', '\0'	\
} /* Move cursor down n lines */
#define SHELL_VT100_CURSORRT(n)				\
{							\
	SHELL_VT100_ASCII_ESC, '[', (n), 'C', '\0'	\
} /* Move cursor right n lines */
#define SHELL_VT100_CURSORLF(n)				\
{							\
	SHELL_VT100_ASCII_ESC, '[', (n), 'D', '\0'	\
} /* Move cursor left n lines */
#define SHELL_VT100_CURSORHOME			\
{						\
	SHELL_VT100_ASCII_ESC, '[', 'H', '\0'	\
} /* Move cursor to upper left corner */
#define SHELL_VT100_CURSORHOME_				\
{							\
	SHELL_VT100_ASCII_ESC, '[', ';', 'H', '\0'	\
} /* Move cursor to upper left corner */
#define SHELL_VT100_CURSORPOS(v, h)				\
{								\
	SHELL_VT100_ASCII_ESC, '[', (v), ';', (h), 'H', '\0'	\
} /* Move cursor to screen location v,h */

#define SHELL_VT100_HVHOME			\
{						\
	SHELL_VT100_ASCII_ESC, '[', 'f', '\0'	\
} /* Move cursor to upper left corner */
#define SHELL_VT100_HVHOME_				\
{							\
	SHELL_VT100_ASCII_ESC, '[', ';', 'f', '\0'	\
} /* Move cursor to upper left corner */
#define SHELL_VT100_HVPOS(v, h)					\
{								\
	SHELL_VT100_ASCII_ESC, '[', (v), ';', (h), 'f', '\0'	\
} /* Move cursor to screen location v,h */
#define SHELL_VT100_INDEX			\
{						\
	SHELL_VT100_ASCII_ESC, 'D', '\0'	\
} /* Move/scroll window up one line */
#define SHELL_VT100_REVINDEX			\
{						\
	SHELL_VT100_ASCII_ESC, 'M', '\0'	\
} /* Move/scroll window down one line */
#define SHELL_VT100_NEXTLINE			\
{						\
	SHELL_VT100_ASCII_ESC, 'E', '\0'	\
} /* Move to next line */
#define SHELL_VT100_SAVECURSOR		\
{						\
	SHELL_VT100_ASCII_ESC, '7', '\0'	\
} /* Save cursor position and attributes */
#define SHELL_VT100_RESTORECURSOR		\
{						\
	SHELL_VT100_ASCII_ESC, '8', '\0'	\
} /* Restore cursor position and attribute */

#define SHELL_VT100_TABSET			\
{						\
	SHELL_VT100_ASCII_ESC, 'H', '\0'	\
} /* Set a tab at the current column */
#define SHELL_VT100_TABCLR			\
{						\
	SHELL_VT100_ASCII_ESC, '[', 'g', '\0'	\
} /* Clear a tab at the current column */
#define SHELL_VT100_TABCLR_				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '0', 'g', '\0'	\
} /* Clear a tab at the current column */
#define SHELL_VT100_TABCLRALL				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '3', 'g', '\0'	\
} /* Clear all tabs */

#define SHELL_VT100_DHTOP			\
{						\
	SHELL_VT100_ASCII_ESC, '#', '3', '\0'	\
} /* Double-height letters, top half */
#define SHELL_VT100_DHBOT			\
{						\
	SHELL_VT100_ASCII_ESC, '#', '4', '\0'	\
} /* Double-height letters, bottom hal */
#define SHELL_VT100_SWSH			\
{						\
	SHELL_VT100_ASCII_ESC, '#', '5', '\0'	\
} /* Single width, single height letters */
#define SHELL_VT100_DWSH			\
{						\
	SHELL_VT100_ASCII_ESC, '#', '6', '\0'	\
} /* Double width, single height letters */

#define SHELL_VT100_CLEAREOL			\
{						\
	SHELL_VT100_ASCII_ESC, '[', 'K', '\0'	\
} /* Clear line from cursor right */
#define SHELL_VT100_CLEAREOL_				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '0', 'K', '\0'	\
} /* Clear line from cursor right */
#define SHELL_VT100_CLEARBOL				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '1', 'K', '\0'	\
} /* Clear line from cursor left */
#define SHELL_VT100_CLEARLINE				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '2', 'K', '\0'	\
} /* Clear entire line */

#define SHELL_VT100_CLEAREOS			\
{						\
	SHELL_VT100_ASCII_ESC, '[', 'J', '\0'	\
} /* Clear screen from cursor down */
#define SHELL_VT100_CLEAREOS_				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '0', 'J', '\0'	\
} /* Clear screen from cursor down */
#define SHELL_VT100_CLEARBOS				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '1', 'J', '\0'	\
} /* Clear screen from cursor up */
#define SHELL_VT100_CLEARSCREEN				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '2', 'J', '\0'	\
} /* Clear entire screen */

#define SHELL_VT100_DEVSTAT			\
{						\
	SHELL_VT100_ASCII_ESC, '5', 'n', '\0'	\
} /* Device status report */
#define SHELL_VT100_TERMOK			\
{						\
	SHELL_VT100_ASCII_ESC, '0', 'n', '\0'	\
} /* Response: terminal is OK */
#define SHELL_VT100_TERMNOK				\
{							\
	SHELL_VT100_ASCII_ESC, '3', 'n', '\0'		\
} /* Response: terminal is not OK */

#define SHELL_VT100_GETCURSOR				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '6', 'n', '\0'	\
} /* Get cursor position */
#define SHELL_VT100_CURSORPOSAT				\
{							\
	SHELL_VT100_ASCII_ESC, (v), ';', (h), 'R', '\0'	\
} /* Response: cursor is at v,h */

#define SHELL_VT100_IDENT				\
{							\
	SHELL_VT100_ASCII_ESC, '[', 'c', '\0'		\
} /* Identify what terminal type */
#define SHELL_VT100_IDENT_				\
{							\
	SHELL_VT100_ASCII_ESC, '[', '0', 'c', '\0'	\
} /* Identify what terminal type */
#define SHELL_VT100_GETTYPE						\
{									\
	SHELL_VT100_ASCII_ESC, '[', '?', '1', ';', (n), '0', 'c', '\0'	\
} /* Response: terminal type code n */

#define SHELL_VT100_RESET			\
{						\
	SHELL_VT100_ASCII_ESC, 'c', '\0'	\
} /*Reset terminal to initial state */

#define SHELL_VT100_ALIGN			\
{						\
	SHELL_VT100_ASCII_ESC, '#', '8', '\0'	\
} /* Screen alignment display */
#define SHELL_VT100_TESTPU					\
{								\
	SHELL_VT100_ASCII_ESC, '[', '2', ';', '1', 'y', '\0'	\
} /* Confidence power up test */
#define SHELL_VT100_TESTLB					\
{								\
	SHELL_VT100_ASCII_ESC, '[', '2', ';', '2', 'y', '\0'	\
} /* Confidence loopback test */
#define SHELL_VT100_TESTPUREP					\
{								\
	SHELL_VT100_ASCII_ESC, '[', '2', ';', '9', 'y', '\0'	\
} /* Repeat power up test */
#define SHELL_VT100_TESTLBREP						\
{									\
	SHELL_VT100_ASCII_ESC, '[', '2', ';', '1', '0', 'y', '\0'	\
} /* Repeat loopback test */

#define SHELL_VT100_LEDSOFF			\
{						\
SHELL_VT100_ASCII_ESC, '[', '0', 'q', '\0'	\
} /* Turn off all four leds */
#define SHELL_VT100_LED1			\
{						\
SHELL_VT100_ASCII_ESC, '[', '1', 'q', '\0'	\
} /* Turn on LED #1 */
#define SHELL_VT100_LED2			\
{						\
SHELL_VT100_ASCII_ESC, '[', '2', 'q', '\0'	\
} /* Turn on LED #2 */
#define SHELL_VT100_LED3			\
{						\
SHELL_VT100_ASCII_ESC, '[', '3', 'q', '\0'	\
} /* Turn on LED #3 */
#define SHELL_VT100_LED4			\
{						\
SHELL_VT100_ASCII_ESC, '[', '4', 'q', '\0'	\
} /* Turn on LED #4 */

/* Function Keys */

#define SHELL_VT100_PF1			\
{					\
SHELL_VT100_ASCII_ESC, 'O', 'P', '\0'	\
}
#define SHELL_VT100_PF2			\
{					\
SHELL_VT100_ASCII_ESC, 'O', 'Q', '\0'	\
}

#define SHELL_VT100_PF3				\
{						\
	SHELL_VT100_ASCII_ESC, 'O', 'R', '\0'	\
}
#define SHELL_VT100_PF4				\
{						\
	SHELL_VT100_ASCII_ESC, 'O', 'S', '\0'	\
}

/* Arrow keys */

#define SHELL_VT100_UP_RESET			\
{						\
	SHELL_VT100_ASCII_ESC, 'A', '\0'	\
}
#define SHELL_VT100_UP_SET			\
{						\
	SHELL_VT100_ASCII_ESC, 'O', 'A', '\0'	\
}
#define SHELL_VT100_DOWN_RESET			\
{						\
	SHELL_VT100_ASCII_ESC, 'B', '\0'	\
}
#define SHELL_VT100_DOWN_SET			\
{						\
	SHELL_VT100_ASCII_ESC, 'O', 'B', '\0'	\
}
#define SHELL_VT100_RIGHT_RESET			\
{						\
	SHELL_VT100_ASCII_ESC, 'C', '\0'	\
}
#define SHELL_VT100_RIGHT_SET			\
{						\
	SHELL_VT100_ASCII_ESC, 'O', 'C', '\0'	\
}
#define SHELL_VT100_LEFT_RESET			\
{						\
	SHELL_VT100_ASCII_ESC, 'D', '\0'	\
}
#define SHELL_VT100_LEFT_SET			\
{						\
	SHELL_VT100_ASCII_ESC, 'O', 'D', '\0'	\
}

/* Numeric Keypad Keys */
#define SHELL_VT100_NUMERIC_0	\
{				\
	'0', '\0'		\
}
#define SHELL_VT100_ALT_0			\
{						\
	SHELL_VT100_ASCII_ESC, 'O', 'p', '\0'	\
}
#define SHELL_VT100_NUMERIC_1	\
{				\
	'1', '\0'		\
}
#define SHELL_VT100_ALT_1			\
{						\
	SHELL_VT100_ASCII_ESC, 'O', 'q', '\0'	\
}
#define SHELL_VT100_NUMERIC_2	\
{				\
	'2', '\0'		\
}
#define SHELL_VT100_ALT_2			\
{						\
	SHELL_VT100_ASCII_ESC, 'O', 'r', '\0'	\
}
#define SHELL_VT100_NUMERIC_3	\
{				\
	'3', '\0'		\
}
#define SHELL_VT100_ALT_3			\
{						\
	SHELL_VT100_ASCII_ESC, 'O', 's', '\0'	\
}
#define SHELL_VT100_NUMERIC_4	\
{				\
	'4', '\0'		\
}
#define SHELL_VT100_ALT_4			\
{						\
	SHELL_VT100_ASCII_ESC, 'O', 't', '\0'	\
}
#define SHELL_VT100_NUMERIC_5	\
{				\
	'5', '\0'		\
}
#define SHELL_VT100_ALT_5			\
{						\
	SHELL_VT100_ASCII_ESC, 'O', 'u', '\0'	\
}
#define SHELL_VT100_NUMERIC_6		\
{					\
	'6', '\0'			\
}
#define SHELL_VT100_ALT_6			\
{						\
	SHELL_VT100_ASCII_ESC, 'O', 'v', '\0'	\
}
#define SHELL_VT100_NUMERIC_7	\
{				\
	'7', '\0'		\
}
#define SHELL_VT100_ALT_7			\
{						\
	SHELL_VT100_ASCII_ESC, 'O', 'w', '\0'	\
}
#define SHELL_VT100_NUMERIC_8	\
{				\
	'8', '\0'		\
}
#define SHELL_VT100_ALT_8			\
{						\
	SHELL_VT100_ASCII_ESC, 'O', 'x', '\0'	\
}
#define SHELL_VT100_NUMERIC_9	\
{				\
	'9', '\0'		\
}
#define SHELL_VT100_ALT_9		\
{					\
	SHELL_VT100_ASCII_ESC, 'O', 'y'	\
}
#define SHELL_VT100_NUMERIC_MINUS	\
{					\
	'-', '\0'			\
}
#define SHELL_VT100_ALT_MINUS			\
{						\
	SHELL_VT100_ASCII_ESC, 'O', 'm', '\0'	\
}
#define SHELL_VT100_NUMERIC_COMMA	\
{					\
	',', '\0'			\
}
#define SHELL_VT100_ALT_COMMA			\
{						\
	SHELL_VT100_ASCII_ESC, 'O', 'l', '\0'	\
}
#define SHELL_VT100_NUMERIC_ENTER	\
{					\
	ASCII_CR			\
}
#define SHELL_VT100_NUMERIC_PERIOD	\
{					\
	'.', '\0'			\
}
#define SHELL_VT100_ALT_PERIOD			\
{						\
	SHELL_VT100_ASCII_ESC, 'O', 'n', '\0'	\
}
#define SHELL_VT100_ALT_ENTER			\
{						\
	SHELL_VT100_ASCII_ESC, 'O', 'M', '\0'	\
}
#define SHELL_VT100_BGCOLOR(__col)				  \
{								  \
	SHELL_VT100_ASCII_ESC, '[', '4', '0' + (__col), 'm', '\0' \
}
#define SHELL_VT100_COLOR(__col)					    \
{									    \
	SHELL_VT100_ASCII_ESC, '[', '1', ';', '3', '0' + (__col), 'm', '\0' \
}

#endif /* SHELL_VT100_H__ */
