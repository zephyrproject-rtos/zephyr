/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <lvgl.h>
#include "lvgl_color.h"

#if CONFIG_LVGL_BITS_PER_PIXEL == 0

static void zephyr_disp_flush(s32_t x1, s32_t y1, s32_t x2, s32_t y2,
		const lv_color_t *color_p)
{
	u16_t w = x2 - x1 + 1;
	u16_t h = y2 - y1 + 1;
	struct display_buffer_descriptor desc;

	desc.buf_size = w * 2U * h;
	desc.width = w;
	desc.pitch = w;
	desc.height = h;
	display_write(lvgl_display_dev, x1, y1, &desc, (void *) color_p);

	lv_flush_ready();
}

#define zephyr_vdb_write NULL

#else

#error "Unsupported pixel format conversion"

#endif /* CONFIG_LVGL_BITS_PER_PIXEL */

void *get_disp_flush(void)
{
	return zephyr_disp_flush;
}

void *get_vdb_write(void)
{
	return zephyr_vdb_write;
}

void *get_round_func(void)
{
	return NULL;
}
