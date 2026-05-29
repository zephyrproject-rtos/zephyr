/*
 * Copyright (c) 2026 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/dt-bindings/display/panel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

static const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

#define DISPLAY_WIDTH  DT_PROP(DT_CHOSEN(zephyr_display), width)
#define DISPLAY_HEIGHT DT_PROP(DT_CHOSEN(zephyr_display), height)

#define DISPLAY_BUFFER_SIZE                                                                        \
	((DISPLAY_WIDTH * DISPLAY_HEIGHT * DISPLAY_BITS_PER_PIXEL(PIXEL_FORMAT_I_4)) / 8)

static uint8_t display_buffer[DISPLAY_BUFFER_SIZE];

static void fill_band_i4(uint8_t *buf, uint16_t x_start, uint16_t band_width, uint16_t height,
			 uint16_t pitch, uint8_t color_idx)
{
	uint16_t x_end = x_start + band_width;

	for (uint16_t y = 0; y < height; y++) {
		for (uint16_t x = x_start; x < x_end; x++) {
			uint32_t byte_idx = (y * pitch + x) / 2;
			bool is_high_nibble = ((x % 2) == 0);

			if (is_high_nibble) {
				buf[byte_idx] = (buf[byte_idx] & 0x0F) | (color_idx << 4);
			} else {
				buf[byte_idx] = (buf[byte_idx] & 0xF0) | (color_idx & 0x0F);
			}
		}
	}
}

static int draw_color_bands(const struct display_capabilities *caps, uint8_t *buf, size_t buf_size,
			    uint16_t num_colors)
{
	struct display_buffer_descriptor desc;
	uint16_t band_width;
	uint16_t color_pos = 0;
	int ret;

	band_width = caps->x_resolution / num_colors;

	if (band_width == 0) {
		LOG_WRN("Display too narrow for %u colors, using 1 pixel bands", num_colors);
		band_width = 1;
	}

	memset(buf, 0, buf_size);

	if (caps->current_pixel_format == PIXEL_FORMAT_I_4) {

		for (uint16_t palette_idx = 0; palette_idx < CONFIG_DISPLAY_COLOR_PALETTE_MAX_SIZE;
		     palette_idx++) {

			uint32_t palette_entry;

			memcpy(&palette_entry, &caps->color_palette[palette_idx],
			       sizeof(palette_entry));

			/* Skip unset palette entries */
			if (palette_entry == PANEL_COLOR_PALETTE_NULL) {
				continue;
			}

			uint16_t x_start = color_pos * band_width;
			uint16_t actual_width = band_width;

			/* Last visible band extends to display edge */
			if (color_pos == num_colors - 1) {
				actual_width = caps->x_resolution - x_start;
			}

			fill_band_i4(buf, x_start, actual_width, caps->y_resolution,
				     caps->x_resolution, palette_idx);

			color_pos++;
		}

	} else {
		LOG_ERR("Unsupported pixel format: %u", caps->current_pixel_format);
		return -ENOTSUP;
	}

	desc.buf_size = buf_size;
	desc.width = caps->x_resolution;
	desc.height = caps->y_resolution;
	desc.pitch = caps->x_resolution;
	desc.frame_incomplete = false;

	ret = display_write(display, 0, 0, &desc, buf);
	if (ret < 0) {
		LOG_ERR("Failed to write to display: %d", ret);
		return ret;
	}

	return 0;
}

int main(void)
{
	struct display_capabilities caps;
	int ret;

	if (!device_is_ready(display)) {
		LOG_ERR("Display device not ready");
		return 0;
	}

	display_get_capabilities(display, &caps);

	LOG_INF("Display: %ux%u, pixel format: %u", caps.x_resolution, caps.y_resolution,
		caps.current_pixel_format);

	uint16_t num_colors = 0;

	for (uint16_t i = 0; i < CONFIG_DISPLAY_COLOR_PALETTE_MAX_SIZE; i++) {

		uint32_t palette_entry;

		memcpy(&palette_entry, &caps.color_palette[i], sizeof(palette_entry));

		if (palette_entry != PANEL_COLOR_PALETTE_NULL) {
			num_colors++;
		}
	}

	LOG_INF("Color palette has %u active entries", num_colors);

	if (num_colors > 0) {

		ret = display_blanking_off(display);
		if (ret < 0) {
			LOG_ERR("Failed to disable blanking: %d", ret);
		}

		ret = draw_color_bands(&caps, display_buffer, sizeof(display_buffer), num_colors);

		if (ret < 0) {
			LOG_ERR("Failed to draw color bands: %d", ret);
		}

	} else {
		LOG_WRN("No colors defined in palette");
	}

	return 0;
}
