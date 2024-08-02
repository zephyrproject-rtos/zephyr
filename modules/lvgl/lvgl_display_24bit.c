/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <lvgl.h>
#include "lvgl_display.h"

void lvgl_flush_cb_24bit(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
	uint16_t w = area->x2 - area->x1 + 1;
	uint16_t h = area->y2 - area->y1 + 1;
	struct lvgl_display_flush flush;

	flush.disp_drv = disp_drv;
	flush.x = area->x1;
	flush.y = area->y1;
	flush.desc.buf_size = w * 3U * h;
	flush.desc.width = w;
	flush.desc.pitch = w;
	flush.desc.height = h;
	flush.buf = (void *)color_p;
	lvgl_flush_display(&flush);
}

void lvgl_set_px_cb_24bit(lv_disp_drv_t *disp_drv, uint8_t *buf, lv_coord_t buf_w, lv_coord_t x,
			  lv_coord_t y, lv_color_t color, lv_opa_t opa)
{
	uint8_t *buf_xy = buf + x * 3U + y * 3U * buf_w;
	lv_color32_t converted_color;

#ifdef CONFIG_LV_COLOR_DEPTH_32
	if (opa != LV_OPA_COVER) {
		lv_color_t mix_color;

		mix_color.ch.red = *buf_xy;
		mix_color.ch.green = *(buf_xy + 1);
		mix_color.ch.blue = *(buf_xy + 2);
		color = lv_color_mix(color, mix_color, opa);
	}
#endif

	converted_color.full = lv_color_to32(color);
	*buf_xy = converted_color.ch.red;
	*(buf_xy + 1) = converted_color.ch.green;
	*(buf_xy + 2) = converted_color.ch.blue;
}
