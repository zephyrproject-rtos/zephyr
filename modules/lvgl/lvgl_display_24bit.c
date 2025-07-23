/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <lvgl.h>
#include "lvgl_display.h"

void lvgl_flush_cb_24bit(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
	uint16_t w = area->x2 - area->x1 + 1;
	uint16_t h = area->y2 - area->y1 + 1;
	struct lvgl_display_flush flush;

	flush.display = display;
	flush.x = area->x1;
	flush.y = area->y1;
	flush.desc.buf_size = w * 3U * h;
	flush.desc.width = w;
	flush.desc.pitch = ROUND_UP(w * 3U, LV_DRAW_BUF_STRIDE_ALIGN) / 3U;
	flush.desc.height = h;
	flush.buf = (void *)px_map;

	if (IS_ENABLED(CONFIG_LV_Z_COLOR_24_BGR_TO_RGB)) {
		/* LVGL assumes BGR byte ordering, convert to RGB */
		for (size_t i = 0; i < flush.desc.buf_size; i += 3) {
			uint8_t tmp = px_map[i];

			px_map[i] = px_map[i + 2];
			px_map[i + 2] = tmp;
		}
	}

	lvgl_flush_display(&flush);
}
