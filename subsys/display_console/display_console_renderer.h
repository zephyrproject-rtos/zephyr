/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

/* Interface between the console and the renderer drawing its text grid. */

#ifndef ZEPHYR_SUBSYS_DISPLAY_CONSOLE_DISPLAY_CONSOLE_RENDERER_H_
#define ZEPHYR_SUBSYS_DISPLAY_CONSOLE_DISPLAY_CONSOLE_RENDERER_H_

#include <stdbool.h>
#include <stdint.h>

/* Text attribute of a cell, selected by the SGR colour codes. */
enum display_console_attr {
	DISPLAY_CONSOLE_ATTR_DEFAULT,
	DISPLAY_CONSOLE_ATTR_RED,
	DISPLAY_CONSOLE_ATTR_GREEN,
	DISPLAY_CONSOLE_ATTR_YELLOW,
	DISPLAY_CONSOLE_ATTR_CYAN,
	DISPLAY_CONSOLE_ATTR_COUNT,
};

struct display_console_rgb {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

/* Foreground colour of each attribute and the background colour. */
extern const struct display_console_rgb display_console_fg_rgb[DISPLAY_CONSOLE_ATTR_COUNT];
extern const struct display_console_rgb display_console_bg_rgb;

/*
 * Copy of the text grid handed to the renderer. Rows form a ring: row r on
 * the display is entry (top + r) % rows, and dirty is indexed by entry.
 */
struct display_console_snapshot {
	char text[CONFIG_DISPLAY_CONSOLE_MAX_ROWS][CONFIG_DISPLAY_CONSOLE_MAX_COLS];
	uint8_t attr[CONFIG_DISPLAY_CONSOLE_MAX_ROWS][CONFIG_DISPLAY_CONSOLE_MAX_COLS];
	bool dirty[CONFIG_DISPLAY_CONSOLE_MAX_ROWS];
	uint16_t top;
	uint16_t rows;
	uint16_t cols;
};

struct display_console_renderer_api {
	/* Prepare the display and report how many rows and columns fit on it. */
	int (*init)(uint16_t *rows, uint16_t *cols);
	/* Draw the dirty rows. */
	int (*render)(const struct display_console_snapshot *snap);
};

/* The renderer selected with Kconfig. */
extern const struct display_console_renderer_api display_console_renderer;

/* Have every row drawn again, for a renderer that becomes able to show them. */
void display_console_redraw(void);

/* After a fatal error: draw now, then every completed line as it is written. */
void display_console_panic(void);

#endif /* ZEPHYR_SUBSYS_DISPLAY_CONSOLE_DISPLAY_CONSOLE_RENDERER_H_ */
