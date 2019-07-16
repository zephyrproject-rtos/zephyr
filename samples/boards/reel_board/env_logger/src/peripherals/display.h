/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DISPLAY_H__
#define DISPLAY_H__

enum screen_ids {
	SCREEN_BOOT = 0,
	SCREEN_SENSORS = 1,
	SCREEN_LAST,
};

enum font_size {
	FONT_BIG = 0,
	FONT_MEDIUM = 1,
	FONT_SMALL = 2,
};

/* Function initializing display */
void board_show_text(enum font_size font, const char *text, bool center);
int display_init(void);
int display_screen(enum screen_ids id);
enum screen_ids display_screen_get(void);
void display_screen_increment(void);

#define display_small(text, center)	\
	board_show_text(FONT_SMALL, text, center)

#define display_medium(text, center)	\
	board_show_text(FONT_MEDIUM, text, center)

#define display_big(text, center)	\
	board_show_text(FONT_BIG, text, center)

#endif /* DISPLAY_H__ */
