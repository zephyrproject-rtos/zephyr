/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <lvgl.h>
#include "lvgl_display.h"

void lvgl_flush_cb_32bit(struct _disp_drv_t *disp_drv,
		const lv_area_t *area, lv_color_t *color_p)
{
	struct device *display_dev = (struct device *)disp_drv->user_data;
	u16_t w = area->x2 - area->x1 + 1;
	u16_t h = area->y2 - area->y1 + 1;
	struct display_buffer_descriptor desc;

	desc.buf_size = w * 4U * h;
	desc.width = w;
	desc.pitch = w;
	desc.height = h;
	display_write(display_dev, area->x1, area->y1, &desc, (void *) color_p);

	lv_disp_flush_ready(disp_drv);
}

#ifndef CONFIG_LVGL_COLOR_DEPTH_32
void lvgl_set_px_cb_32bit(struct _disp_drv_t *disp_drv,
		u8_t *buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y,
		lv_color_t color, lv_opa_t opa)
{
	u32_t *buf_xy = (u32_t *)(buf + x * 4U + y * 4U * buf_w);
	*buf_xy = lv_color_to32(color);
}
#endif
