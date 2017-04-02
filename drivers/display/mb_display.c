/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * References:
 *
 * https://www.microbit.co.uk/device/screen
 * https://lancaster-university.github.io/microbit-docs/ubit/display/
 */

#include <zephyr.h>
#include <init.h>
#include <board.h>
#include <gpio.h>
#include <device.h>

#include <display/mb_display.h>

#include "mb_font.h"

#define DISPLAY_ROWS 3
#define DISPLAY_COLS 9

struct mb_display {
	struct device  *dev;      /* GPIO device */

	struct k_timer  timer;    /* Rendering timer */

	/* The following variables track the currently shown image */
	int             cur;      /* Currently rendered row */
	uint32_t        row[3];   /* Content (columns) for each row */
	int64_t         expiry;   /* When to stop showing the current image */

	int32_t         duration; /* Duration for each shown image */

	const char     *str;      /* String to be shown */
};

struct x_y {
	uint8_t x:4,
		y:4;
};

/* Where the X,Y coordinates of each row/col are found.
 * The top left corner has the coordinates 0,0.
 */
static const struct x_y map[DISPLAY_ROWS][DISPLAY_COLS] = {
	{{0, 0}, {2, 0}, {4, 0}, {4, 3}, {3, 3}, {2, 3}, {1, 3}, {0, 3}, {1, 2} },
	{{4, 2}, {0, 2}, {2, 2}, {1, 0}, {3, 0}, {3, 4}, {1, 4}, {0, 0}, {0, 0} },
	{{2, 4}, {4, 4}, {0, 4}, {0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1}, {3, 2} },
};

/* Mask of all the column bits */
static const uint32_t col_mask = (((~0UL) << LED_COL1_GPIO_PIN) &
				  ((~0UL) >> (31 - LED_COL9_GPIO_PIN)));

#define GET_PIXEL(img, x, y) ((img)->row[y] & BIT(x))

/* Precalculate all three rows of an image */
static void set_img(struct mb_display *disp, const struct mb_image *img)
{
	int row, col;

	for (row = 0; row < DISPLAY_ROWS; row++) {
		disp->row[row] = 0;

		for (col = 0; col < DISPLAY_COLS; col++) {
			if (GET_PIXEL(img, map[row][col].x, map[row][col].y)) {
				disp->row[row] |= BIT(LED_COL1_GPIO_PIN + col);
			}
		}

		disp->row[row] = ~disp->row[row] & col_mask;
		disp->row[row] |= BIT(LED_ROW1_GPIO_PIN + row);
	}

	disp->cur = 0;
}

#define ROW_PIN(n) (LED_ROW1_GPIO_PIN + (n))

static inline void update_pins(struct mb_display *disp, uint32_t val)
{
	if (IS_ENABLED(CONFIG_MICROBIT_DISPLAY_PIN_GRANULARITY)) {
		uint32_t pin, prev = (disp->cur + 2) % 3;

		/* Disable the previous row */
		gpio_pin_write(disp->dev, ROW_PIN(prev), 0);

		/* Set the column pins to their correct values */
		for (pin = LED_COL1_GPIO_PIN; pin <= LED_COL9_GPIO_PIN; pin++) {
			gpio_pin_write(disp->dev, pin, !!(val & BIT(pin)));
		}

		/* Enable the new row */
		gpio_pin_write(disp->dev, ROW_PIN(disp->cur), 1);
	} else {
		gpio_port_write(disp->dev, val);
	}
}

static void show_row(struct k_timer *timer)
{
	struct mb_display *disp = CONTAINER_OF(timer, struct mb_display, timer);

	update_pins(disp, disp->row[disp->cur]);
	disp->cur = (disp->cur + 1) % DISPLAY_ROWS;

	if (disp->cur == 0 && disp->expiry != K_FOREVER &&
	    k_uptime_get() > disp->expiry) {
		k_timer_stop(&disp->timer);

		if (disp->str && disp->str[0]) {
			mb_display_char(disp, disp->str[0], disp->duration);
			disp->str++;
		}
	}
}

static void clear_display(struct k_timer *timer)
{
	struct mb_display *disp = CONTAINER_OF(timer, struct mb_display, timer);

	update_pins(disp, col_mask);
}

static struct mb_display display = {
	.timer = K_TIMER_INITIALIZER(display.timer, show_row, clear_display),
};

void mb_display_image(struct mb_display *disp, const struct mb_image *img,
		      int32_t duration)
{
	set_img(disp, img);

	if (duration == K_FOREVER) {
		disp->expiry = K_FOREVER;
	} else {
		disp->expiry = k_uptime_get() + duration;
	}

	k_timer_start(&disp->timer, K_NO_WAIT, K_MSEC(5));
}

void mb_display_stop(struct mb_display *disp)
{
	k_timer_stop(&disp->timer);
	disp->str = NULL;
}

void mb_display_char(struct mb_display *disp, char chr, int32_t duration)
{
	if (chr < MB_FONT_START || chr > MB_FONT_END) {
		chr = ' ';
	}

	mb_display_image(disp, &mb_font[chr - MB_FONT_START], duration);
}

void mb_display_str(struct mb_display *disp, const char *str, int32_t duration)
{
	__ASSERT(str, "NULL string");

	if (*str == '\0') {
		return;
	}

	disp->str = &str[1];
	disp->duration = duration;

	mb_display_char(disp, *str, duration);
}

struct mb_display *mb_display_get(void)
{
	return &display;
}

static int mb_display_init(struct device *dev)
{
	ARG_UNUSED(dev);

	display.dev = device_get_binding(CONFIG_GPIO_NRF5_P0_DEV_NAME);

	__ASSERT(dev, "No GPIO device found");

	gpio_pin_configure(display.dev, LED_ROW1_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_configure(display.dev, LED_ROW2_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_configure(display.dev, LED_ROW3_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_configure(display.dev, LED_COL1_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_configure(display.dev, LED_COL2_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_configure(display.dev, LED_COL3_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_configure(display.dev, LED_COL4_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_configure(display.dev, LED_COL5_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_configure(display.dev, LED_COL6_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_configure(display.dev, LED_COL7_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_configure(display.dev, LED_COL8_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_configure(display.dev, LED_COL9_GPIO_PIN, GPIO_DIR_OUT);

	return 0;
}

SYS_INIT(mb_display_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
