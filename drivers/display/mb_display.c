/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2021, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This tool uses display controller driver API and requires
 * a suitable LED matrix controller driver.
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <string.h>
#include <zephyr/sys/printk.h>

#include <zephyr/display/mb_display.h>
#include <zephyr/drivers/display.h>

#include "mb_font.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mb_disp, CONFIG_DISPLAY_LOG_LEVEL);

#define MODE_MASK    BIT_MASK(16)

#define SCROLL_OFF   0
#define SCROLL_START 1
#define SCROLL_DEFAULT_DURATION_MS 80

#define MB_DISP_XRES 5
#define MB_DISP_YRES 5

struct mb_display {
	const struct device *lm_dev;   /* LED matrix display device */

	struct k_work_delayable dwork; /* Delayable work item */

	uint8_t         img_count;     /* Image count */

	uint8_t         cur_img;       /* Current image or character to show */

	uint8_t         scroll:3,      /* Scroll shift */
			first:1,       /* First frame of a scroll sequence */
			loop:1,        /* Loop to beginning */
			text:1,        /* We're showing a string (not image) */
			img_sep:1,     /* One column image separation */
			msb:1;         /* MSB represents the first pixel */

	int32_t         duration;      /* Duration for each shown image */

	union {
		const struct mb_image *img; /* Array of images to show */
		const char            *str; /* String to be shown */
	};

	/* Buffer for printed strings */
	char            str_buf[CONFIG_MICROBIT_DISPLAY_STR_MAX];
};

static inline const struct mb_image *get_font(char ch)
{
	if (ch < MB_FONT_START || ch > MB_FONT_END) {
		return &mb_font[' ' - MB_FONT_START];
	}

	return &mb_font[ch - MB_FONT_START];
}

static ALWAYS_INLINE uint8_t flip_pixels(uint8_t b)
{
	b = (b & 0xf0) >> 4 | (b & 0x0f) << 4;
	b = (b & 0xcc) >> 2 | (b & 0x33) << 2;
	b = (b & 0xaa) >> 1 | (b & 0x55) << 1;

	return b;
}

static int update_content(struct mb_display *disp, const struct mb_image *img)
{
	const struct display_buffer_descriptor buf_desc = {
		.buf_size = sizeof(struct mb_image),
		.width    = MB_DISP_XRES,
		.height   = MB_DISP_YRES,
		.pitch    = 8,
	};
	struct mb_image tmp_img;
	int ret;

	if (disp->msb) {
		for (int i = 0; i < sizeof(struct mb_image); i++) {
			tmp_img.row[i] = flip_pixels(img->row[i]);
		}

		ret = display_write(disp->lm_dev, 0, 0, &buf_desc, &tmp_img);
	} else {
		ret = display_write(disp->lm_dev, 0, 0, &buf_desc, img);
	}

	if (ret < 0) {
		LOG_ERR("Write to display controller failed");
		return ret;
	}

	LOG_DBG("Image duration %d", disp->duration);
	if (disp->duration != SYS_FOREVER_MS) {
		k_work_reschedule(&disp->dwork, K_MSEC(disp->duration));
	}

	return ret;
}

static int start_image(struct mb_display *disp, const struct mb_image *img)
{
	int ret;

	ret = display_blanking_off(disp->lm_dev);
	if (ret < 0) {
		LOG_ERR("Set blanking off failed");
		return ret;
	}

	return update_content(disp, img);
}

static int reset_display(struct mb_display *disp)
{
	int ret;

	disp->str = NULL;
	disp->cur_img = 0U;
	disp->img = NULL;
	disp->img_count = 0U;
	disp->scroll = SCROLL_OFF;

	ret = display_blanking_on(disp->lm_dev);
	if (ret < 0) {
		LOG_ERR("Set blanking on failed");
	}

	return ret;
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
	return MB_DISP_XRES + disp->img_sep;
}

static int update_scroll(struct mb_display *disp)
{
	if (disp->scroll < scroll_steps(disp)) {
		struct mb_image img;

		for (int i = 0; i < MB_DISP_XRES; i++) {
			const struct mb_image *i1 = current_img(disp);
			const struct mb_image *i2 = next_img(disp);

			img.row[i] = ((i1->row[i] >> disp->scroll) |
				      (i2->row[i] << (scroll_steps(disp) -
						      disp->scroll)));
		}

		disp->scroll++;
		return update_content(disp, &img);
	} else {
		if (disp->first) {
			disp->first = 0U;
		} else {
			disp->cur_img++;
		}

		if (last_frame(disp)) {
			if (!disp->loop) {
				return reset_display(disp);
			}

			disp->cur_img = 0U;
			disp->first = 1U;
		}

		disp->scroll = SCROLL_START;
		return update_content(disp, current_img(disp));
	}
}

static int update_image(struct mb_display *disp)
{
	disp->cur_img++;

	if (last_frame(disp)) {
		if (!disp->loop) {
			return reset_display(disp);
		}

		disp->cur_img = 0U;
	}

	return update_content(disp, current_img(disp));
}

static void update_display_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct mb_display *disp = CONTAINER_OF(dwork, struct mb_display, dwork);
	int ret;

	if (disp->scroll) {
		ret = update_scroll(disp);
	} else {
		ret = update_image(disp);
	}

	__ASSERT(ret == 0, "Failed to update display");
}

static int start_scroll(struct mb_display *disp, int32_t duration)
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
	return start_image(disp, get_font(' '));
}

static int start_single(struct mb_display *disp, int32_t duration)
{
	disp->duration = duration;

	if (disp->text) {
		return start_image(disp, get_font(disp->str[0]));
	} else {
		return start_image(disp, disp->img);
	}
}

void mb_display_stop(struct mb_display *disp)
{
	struct k_work_sync sync;
	int ret;

	k_work_cancel_delayable_sync(&disp->dwork, &sync);
	LOG_DBG("delayable work stopped %p", disp);
	ret = reset_display(disp);
	__ASSERT(ret == 0, "Failed to reset display");
}

void mb_display_image(struct mb_display *disp, uint32_t mode, int32_t duration,
		      const struct mb_image *img, uint8_t img_count)
{
	int ret;

	mb_display_stop(disp);

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
		ret = start_single(disp, duration);
		__ASSERT(ret == 0, "Failed to start single mode");
		break;
	case MB_DISPLAY_MODE_SCROLL:
		ret = start_scroll(disp, duration);
		__ASSERT(ret == 0, "Failed to start scroll mode");
		break;
	default:
		__ASSERT(0, "Invalid display mode");
	}
}

void mb_display_print(struct mb_display *disp, uint32_t mode,
		      int32_t duration, const char *fmt, ...)
{
	va_list ap;
	int ret;

	mb_display_stop(disp);

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
		ret = start_scroll(disp, duration);
		__ASSERT(ret == 0, "Failed to start scroll mode");
		break;
	case MB_DISPLAY_MODE_SINGLE:
		ret = start_single(disp, duration);
		__ASSERT(ret == 0, "Failed to start single mode");
		break;
	default:
		__ASSERT(0, "Invalid display mode");
	}
}

static int mb_display_init(struct mb_display *disp)
{
	struct display_capabilities caps;
	int ret;

	display_get_capabilities(disp->lm_dev, &caps);
	if (caps.x_resolution != MB_DISP_XRES ||
	    caps.y_resolution != MB_DISP_YRES) {
		LOG_ERR("Not supported display resolution");
		return -ENOTSUP;
	}

	if (caps.screen_info & SCREEN_INFO_MONO_MSB_FIRST) {
		disp->msb = 1U;
	}

	ret = display_set_brightness(disp->lm_dev, 0xFF);
	if (ret < 0) {
		LOG_ERR("Failed to set brightness");
		return ret;
	}

	k_work_init_delayable(&disp->dwork, update_display_work);

	return 0;
}

static struct mb_display display;

struct mb_display *mb_display_get(void)
{
	return &display;
}

static int mb_display_init_on_boot(const struct device *dev)
{
	ARG_UNUSED(dev);

	display.lm_dev = DEVICE_DT_GET_ONE(nordic_nrf_led_matrix);
	if (!device_is_ready(display.lm_dev)) {
		LOG_ERR("Display controller device not ready");
		return -ENODEV;
	}

	return mb_display_init(&display);
}

SYS_INIT(mb_display_init_on_boot, APPLICATION, CONFIG_DISPLAY_INIT_PRIORITY);
