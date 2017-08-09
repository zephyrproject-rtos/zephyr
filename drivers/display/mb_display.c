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

#define MODE_MASK    BIT_MASK(16)

#define DISPLAY_ROWS 3
#define DISPLAY_COLS 9

#define SCROLL_OFF   0
#define SCROLL_START 1

#define SCROLL_DEFAULT_DURATION K_MSEC(80)

struct mb_display {
	struct device  *dev;         /* GPIO device */

	struct k_timer  timer;       /* Rendering timer */

	u8_t            img_count;   /* Image count */

	u8_t            cur_img;     /* Current image or character to show */

	u8_t            scroll:3,    /* Scroll shift */
			first:1,     /* First frame of a scroll sequence */
			loop:1,      /* Loop to beginning */
			text:1,      /* We're showing a string (not image) */
			img_sep:1;   /* One column image separation */

	/* The following variables track the currently shown image */
	u8_t            cur;         /* Currently rendered row */
	u32_t           row[3];      /* Content (columns) for each row */
	s64_t           expiry;      /* When to stop showing current image */
	s32_t           duration;    /* Duration for each shown image */

	union {
		const struct mb_image *img; /* Array of images to show */
		const char            *str; /* String to be shown */
	};

	/* Buffer for printed strings */
	char            str_buf[CONFIG_MICROBIT_DISPLAY_STR_MAX];
};

struct x_y {
	u8_t x:4,
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
static const u32_t col_mask = (((~0UL) << LED_COL1_GPIO_PIN) &
			       ((~0UL) >> (31 - LED_COL9_GPIO_PIN)));

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

	k_timer_start(&disp->timer, K_NO_WAIT, K_MSEC(4));
}

#define ROW_PIN(n) (LED_ROW1_GPIO_PIN + (n))

static inline void update_pins(struct mb_display *disp, u32_t val)
{
	if (IS_ENABLED(CONFIG_MICROBIT_DISPLAY_PIN_GRANULARITY)) {
		u32_t pin, prev = (disp->cur + 2) % 3;

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

static void reset_display(struct mb_display *disp)
{
	k_timer_stop(&disp->timer);

	disp->str = NULL;
	disp->cur_img = 0;
	disp->img = NULL;
	disp->img_count = 0;
	disp->scroll = SCROLL_OFF;
}

static const struct mb_image *current_img(struct mb_display *disp)
{
	if (disp->scroll && disp->first) {
		return get_font(' ');
	}

	if (disp->text) {
		return get_font(disp->str[disp->cur_img]);
	} else {
		return &disp->img[disp->cur_img];
	}
}

static const struct mb_image *next_img(struct mb_display *disp)
{
	if (disp->text) {
		if (disp->first) {
			return get_font(disp->str[0]);
		} else if (disp->str[disp->cur_img]) {
			return get_font(disp->str[disp->cur_img + 1]);
		} else {
			return get_font(' ');
		}
	} else {
		if (disp->first) {
			return &disp->img[0];
		} else if (disp->cur_img < (disp->img_count - 1)) {
			return &disp->img[disp->cur_img + 1];
		} else {
			return get_font(' ');
		}
	}
}

static inline bool last_frame(struct mb_display *disp)
{
	if (disp->text) {
		return (disp->str[disp->cur_img] == '\0');
	} else {
		return (disp->cur_img >= disp->img_count);
	}
}

static inline u8_t scroll_steps(struct mb_display *disp)
{
	return 5 + disp->img_sep;
}

static void update_scroll(struct mb_display *disp)
{
	if (disp->scroll < scroll_steps(disp)) {
		struct mb_image img;
		int i;

		for (i = 0; i < 5; i++) {
			const struct mb_image *i1 = current_img(disp);
			const struct mb_image *i2 = next_img(disp);

			img.row[i] = ((i1->row[i] >> disp->scroll) |
				      (i2->row[i] << (scroll_steps(disp) -
						      disp->scroll)));
		}

		disp->scroll++;
		start_image(disp, &img);
	} else {
		if (disp->first) {
			disp->first = 0;
		} else {
			disp->cur_img++;
		}

		if (last_frame(disp)) {
			if (!disp->loop) {
				reset_display(disp);
				return;
			}

			disp->cur_img = 0;
			disp->first = 1;
		}

		disp->scroll = SCROLL_START;
		start_image(disp, current_img(disp));
	}
}

static void update_image(struct mb_display *disp)
{
	disp->cur_img++;

	if (last_frame(disp)) {
		if (!disp->loop) {
			reset_display(disp);
			return;
		}

		disp->cur_img = 0;
	}

	start_image(disp, current_img(disp));
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
		} else {
			update_image(disp);
		}
	}
}

static void clear_display(struct k_timer *timer)
{
	struct mb_display *disp = CONTAINER_OF(timer, struct mb_display, timer);

	update_pins(disp, col_mask);
}

static struct mb_display display = {
	.timer = _K_TIMER_INITIALIZER(display.timer, show_row, clear_display),
};

static void start_scroll(struct mb_display *disp, s32_t duration)
{
	/* Divide total duration by number of scrolling steps */
	if (duration) {
		disp->duration = duration / scroll_steps(disp);
	} else {
		disp->duration = SCROLL_DEFAULT_DURATION;
	}

	disp->scroll = SCROLL_START;
	disp->first = 1;
	disp->cur_img = 0;
	start_image(disp, get_font(' '));
}

static void start_single(struct mb_display *disp, s32_t duration)
{
	disp->duration = duration;

	if (disp->text) {
		start_image(disp, get_font(disp->str[0]));
	} else {
		start_image(disp, disp->img);
	}
}

void mb_display_image(struct mb_display *disp, u32_t mode, s32_t duration,
		      const struct mb_image *img, u8_t img_count)
{
	reset_display(disp);

	__ASSERT(img && img_count > 0, "Invalid parameters");

	disp->text = 0;
	disp->img_count = img_count;
	disp->img = img;
	disp->img_sep = 0;
	disp->cur_img = 0;
	disp->loop = !!(mode & MB_DISPLAY_FLAG_LOOP);

	switch (mode & MODE_MASK) {
	case MB_DISPLAY_MODE_DEFAULT:
	case MB_DISPLAY_MODE_SINGLE:
		start_single(disp, duration);
		break;
	case MB_DISPLAY_MODE_SCROLL:
		start_scroll(disp, duration);
		break;
	default:
		__ASSERT(0, "Invalid display mode");
	}
}

void mb_display_stop(struct mb_display *disp)
{
	reset_display(disp);
}

void mb_display_print(struct mb_display *disp, u32_t mode,
		      s32_t duration, const char *fmt, ...)
{
	va_list ap;

	reset_display(disp);

	va_start(ap, fmt);
	vsnprintk(disp->str_buf, sizeof(disp->str_buf), fmt, ap);
	va_end(ap);

	if (disp->str_buf[0] == '\0') {
		return;
	}

	disp->str = disp->str_buf;
	disp->text = 1;
	disp->img_sep = 1;
	disp->cur_img = 0;
	disp->loop = !!(mode & MB_DISPLAY_FLAG_LOOP);

	switch (mode & MODE_MASK) {
	case MB_DISPLAY_MODE_DEFAULT:
	case MB_DISPLAY_MODE_SCROLL:
		start_scroll(disp, duration);
		break;
	case MB_DISPLAY_MODE_SINGLE:
		start_single(disp, duration);
		break;
	default:
		__ASSERT(0, "Invalid display mode");
	}
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
