/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <lvgl.h>
#include <string.h>
#include "lvgl_display.h"

static uint8_t *vtile_buffer;
static uint32_t vtile_buffer_size;

static ALWAYS_INLINE uint8_t reverse_bits(uint8_t byte)
{
	byte = (byte & 0xF0) >> 4 | (byte & 0x0F) << 4;
	byte = (byte & 0xCC) >> 2 | (byte & 0x33) << 2;
	byte = (byte & 0xAA) >> 1 | (byte & 0x55) << 1;
	return byte;
}

static void lvgl_transform_buffer(uint8_t **px_map, uint32_t width, uint32_t height,
				  const struct display_capabilities *caps)
{
	uint32_t src_buf_size = (width * height) / 8U;
	bool is_lsb = !(caps->screen_info & SCREEN_INFO_MONO_MSB_FIRST);

	/* Needed because LVGL reserves 2x4 bytes in the buffer for the color palette. */
	*px_map += 8;

	/* LVGL renders in a htiled MSB buffer layout. */
	if (caps->screen_info & SCREEN_INFO_MONO_VTILED) {

#ifdef CONFIG_LV_Z_MONOCHROME_VTILE_CONVERSION_BUFFER
		lv_draw_sw_i1_convert_to_vtiled(*px_map, src_buf_size, width, height, vtile_buffer,
						vtile_buffer_size, is_lsb);
		memcpy(*px_map, vtile_buffer, src_buf_size);
#endif /* CONFIG_LV_Z_MONOCHROME_VTILE_CONVERSION_BUFFER */

	} else {
		/* Handle bit order */
		for (uint32_t i = 0; i < src_buf_size; i++) {
			*px_map[i] = reverse_bits(*px_map[i]);
		}
	}

	/* LVGL sets the bit if the luminescence of the corresponding RGB color >127. */
	if (caps->current_pixel_format == PIXEL_FORMAT_MONO10) {
		lv_draw_sw_i1_invert(*px_map, src_buf_size);
	}
}

void lvgl_flush_cb_mono(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
	uint16_t w = area->x2 - area->x1 + 1;
	uint16_t h = area->y2 - area->y1 + 1;
	struct lvgl_disp_data *data = (struct lvgl_disp_data *)lv_display_get_user_data(display);
	const struct device *display_dev = data->display_dev;
	const bool is_epd = data->cap.screen_info & SCREEN_INFO_EPD;
	const bool is_last = lv_display_flush_is_last(display);

	lvgl_transform_buffer(&px_map, w, h, &data->cap);

	if (is_epd && !data->blanking_on && !is_last) {
		/*
		 * Turn on display blanking when using an EPD
		 * display. This prevents updates and the associated
		 * flicker if the screen is rendered in multiple
		 * steps.
		 */
		display_blanking_on(display_dev);
		data->blanking_on = true;
	}

	struct display_buffer_descriptor desc = {
		.buf_size = (w * h) / 8U,
		.width = w,
		.pitch = w,
		.height = h,
		.frame_incomplete = !is_last,
	};

	display_write(display_dev, area->x1, area->y1, &desc, (void *)px_map);
	if (data->cap.screen_info & SCREEN_INFO_DOUBLE_BUFFER) {
		display_write(display_dev, area->x1, area->y1, &desc, (void *)px_map);
	}

	if (is_epd && is_last && data->blanking_on) {
		/*
		 * The entire screen has now been rendered. Update the
		 * display by disabling blanking.
		 */
		display_blanking_off(display_dev);
		data->blanking_on = false;
	}

	lv_display_flush_ready(display);
}

void lvgl_rounder_cb_mono(lv_event_t *e)
{
	lv_area_t *area = lv_event_get_param(e);
	lv_display_t *display = lv_event_get_user_data(e);
	struct lvgl_disp_data *data = (struct lvgl_disp_data *)lv_display_get_user_data(display);

	if (data->cap.screen_info & SCREEN_INFO_X_ALIGNMENT_WIDTH) {
		area->x1 = 0;
		area->x2 = data->cap.x_resolution - 1;
	} else {
		area->x1 &= ~0x7;
		area->x2 |= 0x7;
		if (data->cap.screen_info & SCREEN_INFO_MONO_VTILED) {
			area->y1 &= ~0x7;
			area->y2 |= 0x7;
		}
	}
}

void lvgl_set_mono_vtile_buffer(uint8_t *buffer, uint32_t buffer_size)
{
	vtile_buffer = buffer;
	vtile_buffer_size = buffer_size;
}
