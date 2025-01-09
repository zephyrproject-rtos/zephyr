/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <lvgl.h>
#include <string.h>
#include "lvgl_display.h"

static uint8_t *mono_conv_buf;
static uint32_t mono_conv_buf_size;

static ALWAYS_INLINE void set_px_at_pos(uint8_t *dst_buf, uint32_t x, uint32_t y, uint32_t width,
					const struct display_capabilities *caps)
{
	uint8_t bit;
	uint8_t *buf;

	if (caps->screen_info & SCREEN_INFO_MONO_VTILED) {
		buf = dst_buf + x + y / 8 * width;

		if (caps->screen_info & SCREEN_INFO_MONO_MSB_FIRST) {
			bit = 7 - y % 8;
		} else {
			bit = y % 8;
		}
	} else {
		buf = dst_buf + x / 8 + y * width / 8;

		if (caps->screen_info & SCREEN_INFO_MONO_MSB_FIRST) {
			bit = 7 - x % 8;
		} else {
			bit = x % 8;
		}
	}

	if (caps->current_pixel_format == PIXEL_FORMAT_MONO10) {
		*buf |= BIT(bit);
	} else {
		*buf &= ~BIT(bit);
	}
}

static void lvgl_transform_buffer(uint8_t **px_map, uint32_t width, uint32_t height,
				  const struct display_capabilities *caps)
{
	uint8_t clear_color = caps->current_pixel_format == PIXEL_FORMAT_MONO10 ? 0x00 : 0xFF;

	memset(mono_conv_buf, clear_color, mono_conv_buf_size);

	/* Needed because LVGL reserves 2x4 bytes in the buffer for the color palette. */
	*px_map += 8;
	uint8_t *src_buf = *px_map;
	uint32_t stride = (width + CONFIG_LV_DRAW_BUF_STRIDE_ALIGN - 1) &
			  ~(CONFIG_LV_DRAW_BUF_STRIDE_ALIGN - 1);

	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < width; x++) {
			uint32_t bit_idx = x + y * stride;
			uint8_t src_bit = (src_buf[bit_idx / 8] >> (7 - (bit_idx % 8))) & 1;

			if (src_bit) {
				set_px_at_pos(mono_conv_buf, x, y, width, caps);
			}
		}
	}

	memcpy(src_buf, mono_conv_buf, mono_conv_buf_size);
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

void lvgl_set_mono_conversion_buffer(uint8_t *buffer, uint32_t buffer_size)
{
	mono_conv_buf = buffer;
	mono_conv_buf_size = buffer_size;
}
