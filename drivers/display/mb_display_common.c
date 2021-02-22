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
#include <drivers/gpio.h>
#include <device.h>
#include <string.h>
#include <sys/printk.h>

#include <display/mb_display.h>

#include "mb_display.h"
#include "mb_font.h"

#define MODE_MASK    BIT_MASK(16)

#define SCROLL_OFF   0
#define SCROLL_START 1

#define SCROLL_DEFAULT_DURATION_MS 80

struct mb_display {
	struct k_timer timer;           /* Rendering timer */

	uint8_t img_count;              /* Image count */

	uint8_t cur_img;                /* Current image or character to show */

	uint8_t scroll : 3,             /* Scroll shift */
		first : 1,              /* First frame of a scroll sequence */
		loop : 1,               /* Loop to beginning */
		text : 1,               /* We're showing a string (not image) */
		img_sep : 1;            /* One column image separation */

	/* The following variables track the currently shown image */
	uint8_t cur;                            /* Currently rendered row */
	uint32_t row[DISPLAY_ROWS][GPIO_PORTS]; /* Content (columns) for each row */
	int64_t expiry;                         /* When to stop showing current image */
	int32_t duration;                       /* Duration for each shown image */

	union {
		const struct mb_image *img;     /* Array of images to show */
		const char            *str;     /* String to be shown */
	};

	/* Buffer for printed strings */
	char str_buf[CONFIG_MICROBIT_DISPLAY_STR_MAX];
};

static inline const struct mb_image *get_font(char ch)
{
	if (ch < MB_FONT_START || ch > MB_FONT_END) {
		return &mb_font[' ' - MB_FONT_START];
	}

	return &mb_font[ch - MB_FONT_START];
}

/* Precalculate all three rows of an image and start the rendering. */
static void start_image(struct mb_display *disp, const struct mb_image *img)
{
	mb_start_image(img, disp->row);

	disp->cur = 0U;

	if (disp->duration == SYS_FOREVER_MS) {
		disp->expiry = SYS_FOREVER_MS;
	} else {
		disp->expiry = k_uptime_get() + disp->duration;
	}

	k_timer_start(&disp->timer, K_NO_WAIT, K_MSEC(4));
}

static void reset_display(struct mb_display *disp)
{
	k_timer_stop(&disp->timer);

	disp->str = NULL;
	disp->cur_img = 0U;
	disp->img = NULL;
	disp->img_count = 0U;
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

static inline uint8_t scroll_steps(struct mb_display *disp)
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
			disp->first = 0U;
		} else {
			disp->cur_img++;
		}

		if (last_frame(disp)) {
			if (!disp->loop) {
				reset_display(disp);
				return;
			}

			disp->cur_img = 0U;
			disp->first = 1U;
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

		disp->cur_img = 0U;
	}

	start_image(disp, current_img(disp));
}

static void show_row(struct k_timer *timer)
{
	struct mb_display *disp = CONTAINER_OF(timer, struct mb_display, timer);

	mb_update_pins(disp->cur, disp->row[disp->cur]);
	disp->cur = (disp->cur + 1) % DISPLAY_ROWS;

	if (disp->cur == 0U && disp->expiry != SYS_FOREVER_MS &&
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

	mb_update_pins(disp->cur, col_mask);
}

static struct mb_display display = {
	.timer = Z_TIMER_INITIALIZER(display.timer, show_row, clear_display),
};

static void start_scroll(struct mb_display *disp, int32_t duration)
{
	/* Divide total duration by number of scrolling steps */
	if (duration) {
		disp->duration = duration / scroll_steps(disp);
	} else {
		disp->duration = SCROLL_DEFAULT_DURATION_MS;
	}

	disp->scroll = SCROLL_START;
	disp->first = 1U;
	disp->cur_img = 0U;
	start_image(disp, get_font(' '));
}

static void start_single(struct mb_display *disp, int32_t duration)
{
	disp->duration = duration;

	if (disp->text) {
		start_image(disp, get_font(disp->str[0]));
	} else {
		start_image(disp, disp->img);
	}
}

void mb_display_image(struct mb_display *disp, uint32_t mode, int32_t duration,
		      const struct mb_image *img, uint8_t img_count)
{
	reset_display(disp);

	__ASSERT(img && img_count > 0, "Invalid parameters");

	disp->text = 0U;
	disp->img_count = img_count;
	disp->img = img;
	disp->img_sep = 0U;
	disp->cur_img = 0U;
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

void mb_display_print(struct mb_display *disp, uint32_t mode,
		      int32_t duration, const char *fmt, ...)
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
	disp->text = 1U;
	disp->img_sep = 1U;
	disp->cur_img = 0U;
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
