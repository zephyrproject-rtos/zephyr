/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <lvgl.h>
#include "lvgl_display.h"

void lvgl_flush_cb_32bit(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
	uint16_t w = area->x2 - area->x1 + 1;
	uint16_t h = area->y2 - area->y1 + 1;
	struct lvgl_display_flush flush;

	flush.display = display;
	flush.x = area->x1;
	flush.y = area->y1;
	flush.desc.buf_size = w * 4U * h;
	flush.desc.width = w;
	flush.desc.pitch = ROUND_UP(w * 4U, LV_DRAW_BUF_STRIDE_ALIGN) / 4U;
	flush.desc.height = h;
	flush.buf = (void *)px_map;
	lvgl_flush_display(&flush);
}
