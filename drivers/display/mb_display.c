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
#include <string.h>
#include <misc/printk.h>

#include <display/mb_display.h>

#include "mb_font.h"

#define DISPLAY_ROWS 3
#define DISPLAY_COLS 9

#define SCROLL_OFF   0
#define SCROLL_START 1

/* Time between scroll shifts */
#define SCROLL_DURATION K_MSEC(CONFIG_MICROBIT_DISPLAY_SCROLL_STEP)

struct mb_display {
	struct device  *dev;      /* GPIO device */

	struct k_timer  timer;    /* Rendering timer */

	/* The following variables track the currently shown image */
	int             cur;         /* Currently rendered row */
	uint32_t        row[3];      /* Content (columns) for each row */
	int64_t         expiry;      /* When to stop showing current image */
	int32_t         duration;    /* Duration for each shown image */

	struct mb_image img[2];      /* Current and next image */
	uint16_t        scroll;      /* Scroll shift */

	const char     *str;         /* String to be shown */

	/* Buffer for printed strings */
	char            str_buf[CONFIG_MICROBIT_DISPLAY_STR_MAX];
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

static inline void img_copy(struct mb_image *dst, const struct mb_image *src)
{
	memcpy(dst, src, sizeof(*dst));
}

static inline const struct mb_image *get_font(char ch)
{
	if (ch < MB_FONT_START || ch > MB_FONT_END) {
		return &mb_font[' ' - MB_FONT_START];
	}

	return &mb_font[ch - MB_FONT_START];
}

#define GET_PIXEL(img, x, y) ((img)->row[y] & BIT(x))

/* Precalculate all three rows of an image and start the rendering. */
static void start_image(struct mb_display *disp, const struct mb_image *img)
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

	if (disp->duration == K_FOREVER) {
		disp->expiry = K_FOREVER;
	} else {
		disp->expiry = k_uptime_get() + disp->duration;
	}

	k_timer_start(&disp->timer, K_NO_WAIT, K_MSEC(5));
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

static void update_scroll(struct mb_display *disp)
{
	if (disp->scroll < 5) {
		struct mb_image img;
		int i;

		for (i = 0; i < 5; i++) {
			img.row[i] = (disp->img[0].row[i] >> disp->scroll) |
				(disp->img[1].row[i] << (5 - disp->scroll));
		}

		disp->scroll++;
		start_image(disp, &img);
	} else {
		if (!disp->str) {
			disp->scroll = SCROLL_OFF;
			return;
		}

		img_copy(&disp->img[0], &disp->img[1]);

		if (disp->str[0]) {
			img_copy(&disp->img[1], get_font(disp->str[0]));
			disp->str++;
		} else {
			img_copy(&disp->img[1], get_font(' '));
			disp->str = NULL;
		}

		disp->scroll = SCROLL_START;
		start_image(disp, &disp->img[0]);
	}
}

static void show_row(struct k_timer *timer)
{
	struct mb_display *disp = CONTAINER_OF(timer, struct mb_display, timer);

	update_pins(disp, disp->row[disp->cur]);
	disp->cur = (disp->cur + 1) % DISPLAY_ROWS;

	if (disp->cur == 0 && disp->expiry != K_FOREVER &&
	    k_uptime_get() > disp->expiry) {
		if (disp->scroll) {
			update_scroll(disp);
		} else if (disp->str && disp->str[0]) {
			start_image(disp, get_font(disp->str[0]));
			disp->str++;
		} else {
			k_timer_stop(&disp->timer);
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
	disp->str = NULL;
	disp->scroll = SCROLL_OFF;
	disp->duration = duration;

	start_image(disp, img);
}

void mb_display_stop(struct mb_display *disp)
{
	k_timer_stop(&disp->timer);
	disp->str = NULL;
	disp->scroll = SCROLL_OFF;
}

void mb_display_char(struct mb_display *disp, char chr, int32_t duration)
{
	mb_display_image(disp, get_font(chr), duration);
}

void mb_display_string(struct mb_display *disp, int32_t duration,
		       const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsnprintk(disp->str_buf, sizeof(disp->str_buf), fmt, ap);
	va_end(ap);

	if (disp->str_buf[0] == '\0') {
		return;
	}

	disp->str = &disp->str_buf[1];
	disp->duration = duration;
	disp->scroll = SCROLL_OFF;

	start_image(disp, get_font(disp->str_buf[0]));
}

void mb_display_print(struct mb_display *disp, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsnprintk(disp->str_buf, sizeof(disp->str_buf), fmt, ap);
	va_end(ap);

	if (disp->str_buf[0] == '\0') {
		return;
	}

	if (disp->str_buf[1] == '\0') {
		disp->str = NULL;
	} else {
		disp->str = &disp->str_buf[2];
	}

	img_copy(&disp->img[0], get_font(disp->str_buf[0]));
	img_copy(&disp->img[1], get_font(disp->str_buf[1]));
	disp->scroll = SCROLL_START;
	disp->duration = SCROLL_DURATION;

	start_image(disp, &disp->img[0]);
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
