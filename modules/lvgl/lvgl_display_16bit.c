/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <lvgl.h>
#include "lvgl_display.h"

void lvgl_flush_cb_16bit(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
	uint16_t w = area->x2 - area->x1 + 1;
	uint16_t h = area->y2 - area->y1 + 1;
	struct lvgl_display_flush flush;

	flush.display = display;
	flush.x = area->x1;
	flush.y = area->y1;
	flush.desc.buf_size = w * 2U * h;
	flush.desc.width = w;
	flush.desc.pitch = w;
	flush.desc.height = h;
	flush.buf = (void *)px_map;

	lvgl_flush_display(&flush);
}
