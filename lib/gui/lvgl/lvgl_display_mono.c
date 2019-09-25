/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <lvgl.h>
#include "lvgl_display.h"

void lvgl_flush_cb_mono(struct _disp_drv_t *disp_drv,
		const lv_area_t *area, lv_color_t *color_p)
{
	u16_t w = area->x2 - area->x1 + 1;
	u16_t h = area->y2 - area->y1 + 1;
	struct device *display_dev = (struct device *)disp_drv->user_data;
	struct display_capabilities cap;
	struct display_buffer_descriptor desc;

	display_get_capabilities(display_dev, &cap);

	desc.buf_size = (w * h)/8U;
	desc.width = w;
	desc.pitch = w;
	desc.height = h;
	display_write(display_dev, area->x1, area->y1, &desc, (void *) color_p);
	if (cap.screen_info & SCREEN_INFO_DOUBLE_BUFFER) {
		display_write(display_dev, area->x1, area->y1, &desc,
				(void *) color_p);
	}

	lv_disp_flush_ready(disp_drv);
}


void lvgl_set_px_cb_mono(struct _disp_drv_t *disp_drv,
		u8_t *buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y,
		lv_color_t color, lv_opa_t opa)
{
	struct device *display_dev = (struct device *)disp_drv->user_data;
	u8_t *buf_xy;
	u8_t bit;
	struct display_capabilities cap;

	display_get_capabilities(display_dev, &cap);

	if (cap.screen_info & SCREEN_INFO_MONO_VTILED) {
		buf_xy = buf + x + y/8 * buf_w;

		if (cap.screen_info & SCREEN_INFO_MONO_MSB_FIRST) {
			bit = 7 - y%8;
		} else {
			bit = y%8;
		}
	} else {
		buf_xy = buf + x/8 + y * buf_w/8;

		if (cap.screen_info & SCREEN_INFO_MONO_MSB_FIRST) {
			bit = 7 - x%8;
		} else {
			bit = x%8;
		}
	}

	if (cap.current_pixel_format == PIXEL_FORMAT_MONO10) {
		if (color.full == 0) {
			*buf_xy &= ~BIT(bit);
		} else {
			*buf_xy |= BIT(bit);
		}
	} else {
		if (color.full == 0) {
			*buf_xy |= BIT(bit);
		} else {
			*buf_xy &= ~BIT(bit);
		}
	}
}

void lvgl_rounder_cb_mono(struct _disp_drv_t *disp_drv,
		lv_area_t *area)
{
	struct device *display_dev = (struct device *)disp_drv->user_data;
	struct display_capabilities cap;

	display_get_capabilities(display_dev, &cap);

	if (cap.screen_info & SCREEN_INFO_MONO_VTILED) {
		area->y1 &= ~0x7;
		area->y2 |= 0x7;
	} else {
		area->x1 &= ~0x7;
		area->x2 |= 0x7;
	}
}
