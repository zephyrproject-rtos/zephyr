/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <lvgl.h>
#include "lvgl_color.h"

#if CONFIG_LVGL_BITS_PER_PIXEL == 1

static void zephyr_disp_flush(s32_t x1, s32_t y1, s32_t x2, s32_t y2,
		const lv_color_t *color_p)
{
	u16_t w = x2 - x1 + 1;
	u16_t h = y2 - y1 + 1;
	struct display_capabilities cap;
	struct display_buffer_descriptor desc;

	display_get_capabilities(lvgl_display_dev, &cap);

	desc.buf_size = (w * h)/8;
	desc.width = w;
	desc.pitch = w;
	desc.height = h;
	display_write(lvgl_display_dev, x1, y1, &desc, (void *) color_p);
	if (cap.screen_info & SCREEN_INFO_DOUBLE_BUFFER) {
		display_write(lvgl_display_dev, x1, y1, &desc,
				(void *) color_p);
	}

	lv_flush_ready();
}

void zephyr_vdb_write(u8_t *buf, lv_coord_t buf_w, lv_coord_t x,
		lv_coord_t y, lv_color_t color, lv_opa_t opa)
{
	u8_t *buf_xy = buf + x + y/8 * buf_w;
	u8_t bit;
	struct display_capabilities cap;

	display_get_capabilities(lvgl_display_dev, &cap);

	if (cap.screen_info & SCREEN_INFO_MONO_MSB_FIRST) {
		bit = 7 - y%8;
	} else {
		bit = y%8;
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

void zephyr_round_area(lv_area_t *a)
{
	struct display_capabilities cap;

	display_get_capabilities(lvgl_display_dev, &cap);

	if (cap.screen_info & SCREEN_INFO_MONO_VTILED) {
		a->y1 &= ~0x7;
		a->y2 |= 0x7;
	} else {
		a->x1 &= ~0x7;
		a->x2 |= 0x7;
	}
}

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
	return zephyr_round_area;
}
