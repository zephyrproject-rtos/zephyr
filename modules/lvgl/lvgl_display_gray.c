/*
 * Copyright (c) 2022-2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <lvgl.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>

#include "lvgl_display.h"

void lvgl_flush_cb_gray(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
	struct lvgl_disp_data *data = (struct lvgl_disp_data *)disp_drv->user_data;

	uint16_t w = lv_area_get_width(area);
	uint16_t h = lv_area_get_height(area);

	uint16_t pixels_per_byte;

	switch (data->cap.current_pixel_format) {
	case PIXEL_FORMAT_GRAY4:
		pixels_per_byte = 4;
		break;
	case PIXEL_FORMAT_GRAY16:
		pixels_per_byte = 2;
		break;
	default:
		__ASSERT(false, "Unsupported pixel format");
		return;
	}

	struct display_buffer_descriptor desc;

	desc.buf_size = w * h / pixels_per_byte;
	desc.width = w;
	desc.pitch = w;
	desc.height = h;
	display_write(data->display_dev, area->x1, area->y1, &desc, (void *)color_p);

	lv_disp_flush_ready(disp_drv);
}

void lvgl_set_px_cb_gray(lv_disp_drv_t *disp_drv, uint8_t *buf, lv_coord_t buf_w, lv_coord_t x,
			 lv_coord_t y, lv_color_t color, lv_opa_t opa)
{
	struct lvgl_disp_data *data = (struct lvgl_disp_data *)disp_drv->user_data;

	size_t pixel_number = x + y * buf_w;

	switch (data->cap.current_pixel_format) {
	case PIXEL_FORMAT_GRAY4: {
#ifdef CONFIG_LV_Z_BUFFER_ALLOC_STATIC
		__ASSERT(CONFIG_LV_Z_BITS_PER_PIXEL >= 2, "draw buffer too small");
#endif

		uint8_t pixel_offset = (3 - (pixel_number % 4)) * 2;
		uint8_t pixel_mask = 0b11 << pixel_offset;
		uint8_t value = lv_color_to8(color) << pixel_offset;

		uint8_t *buf_xy = &buf[pixel_number / 4];

		*buf_xy &= ~pixel_mask;
		*buf_xy |= (value & pixel_mask);
		break;
	}
	case PIXEL_FORMAT_GRAY16: {
#ifdef CONFIG_LV_Z_BUFFER_ALLOC_STATIC
		__ASSERT(CONFIG_LV_Z_BITS_PER_PIXEL >= 4, "draw buffer too small");
#endif

		uint8_t pixel_offset = (1 - (pixel_number % 2)) * 4;
		uint8_t pixel_mask = 0b1111 << pixel_offset;
		uint8_t value = lv_color_to8(color) << pixel_offset;

		uint8_t *buf_xy = &buf[pixel_number / 2];

		*buf_xy &= ~pixel_mask;
		*buf_xy |= (value & pixel_mask);
		break;
	}
	default:
		__ASSERT(false, "Unsupported pixel format");
	};
}

void lvgl_rounder_cb_gray(lv_disp_drv_t *disp_drv, lv_area_t *area)
{
	struct lvgl_disp_data *data = (struct lvgl_disp_data *)disp_drv->user_data;

	switch (data->cap.current_pixel_format) {
	case PIXEL_FORMAT_GRAY4: {
		area->x1 &= ~0b11;
		area->x2 |= 0b11;
		break;
	}
	case PIXEL_FORMAT_GRAY16: {
		area->x1 &= ~0b1;
		area->x2 |= 0b1;
		break;
	}
	default:
		__ASSERT(false, "Unsupported pixel format");
	}
}
