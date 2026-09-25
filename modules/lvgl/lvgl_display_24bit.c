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
	uint16_t w;
	uint16_t h;
	struct lvgl_disp_data *data = (struct lvgl_disp_data *)lv_display_get_user_data(display);
	struct lvgl_display_flush flush;

#if defined(CONFIG_LV_Z_DIRECT_RENDERING)
	if (lv_display_flush_is_last(display)) {
		/* For direct rendering, flush whole buffer */
		w = lv_display_get_original_horizontal_resolution(display);
		h = lv_display_get_original_vertical_resolution(display);
		flush.x = 0;
		flush.y = 0;
	} else {
		lv_display_flush_ready(display);
		return;
	}
#else
	w = lv_area_get_width(area);
	h = lv_area_get_height(area);
	flush.x = area->x1;
	flush.y = area->y1;
#endif
	flush.display = display;
	flush.desc.buf_size = w * 3U * h;
	flush.desc.width = w;
	flush.desc.pitch = ROUND_UP(w * 3U, LV_DRAW_BUF_STRIDE_ALIGN) / 3U;
	flush.desc.height = h;
	flush.buf = (void *)px_map;

	/*
	 * LVGL's RGB888 stores bytes in memory as B, G, R, which matches Zephyr's
	 * PIXEL_FORMAT_RGB_888. For a PIXEL_FORMAT_BGR_888 panel (R, G, B in memory)
	 * the red and blue channels have to be swapped before flushing.
	 */
	if (data->cap.current_pixel_format == PIXEL_FORMAT_BGR_888) {
		for (size_t i = 0; i < flush.desc.buf_size; i += 3) {
			uint8_t tmp = px_map[i];

			px_map[i] = px_map[i + 2];
			px_map[i + 2] = tmp;
		}
	}

	lvgl_flush_display(&flush);
}
