/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <lvgl.h>
#include "lvgl_display.h"

void lvgl_flush_cb_32bit(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
	uint16_t w = area->x2 - area->x1 + 1;
	uint16_t h = area->y2 - area->y1 + 1;
	struct lvgl_display_flush flush;

	flush.disp_drv = disp_drv;
	flush.x = area->x1;
	flush.y = area->y1;
	flush.desc.buf_size = w * 4U * h;
	flush.desc.width = w;
	flush.desc.pitch = w;
	flush.desc.height = h;
	flush.buf = (void *)color_p;
	lvgl_flush_display(&flush);
}

#ifndef CONFIG_LV_COLOR_DEPTH_32
void lvgl_set_px_cb_32bit(lv_disp_drv_t *disp_drv, uint8_t *buf, lv_coord_t buf_w, lv_coord_t x,
			  lv_coord_t y, lv_color_t color, lv_opa_t opa)
{
	uint32_t *buf_xy = (uint32_t *)(buf + x * 4U + y * 4U * buf_w);

	if (opa == LV_OPA_COVER) {
		/* Do not mix if not required */
		*buf_xy = lv_color_to32(color);
	} else {
		lv_color_t bg_color = *((lv_color_t *)buf_xy);
		*buf_xy = lv_color_to32(lv_color_mix(color, bg_color, opa));
	}
}
#endif
