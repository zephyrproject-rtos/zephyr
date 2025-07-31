/*
 * Copyright (c) 2024 VCI Development
 * SPDX-License-Identifier: Apache-2.0
 *
 * LCD display test application for LPC54S018
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>

LOG_MODULE_REGISTER(lcd_test, LOG_LEVEL_INF);

/* Colors for 16-bit RGB565 format */
#define COLOR_BLACK   0x0000
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_WHITE   0xFFFF
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F

static void fill_rect(const struct device *display, uint16_t x, uint16_t y,
		      uint16_t width, uint16_t height, uint16_t color)
{
	uint16_t *buf;
	struct display_buffer_descriptor desc;
	size_t buf_size;
	int ret;
	
	buf_size = width * height * sizeof(uint16_t);
	buf = k_malloc(buf_size);
	if (!buf) {
		LOG_ERR("Failed to allocate buffer");
		return;
	}
	
	/* Fill buffer with color */
	for (int i = 0; i < width * height; i++) {
		buf[i] = color;
	}
	
	/* Set up display buffer descriptor */
	desc.buf_size = buf_size;
	desc.width = width;
	desc.height = height;
	desc.pitch = width;
	
	/* Write to display */
	ret = display_write(display, x, y, &desc, buf);
	if (ret < 0) {
		LOG_ERR("Failed to write to display: %d", ret);
	}
	
	k_free(buf);
}

static void draw_pattern(const struct device *display)
{
	struct display_capabilities caps;
	uint16_t colors[] = {
		COLOR_RED, COLOR_GREEN, COLOR_BLUE,
		COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA,
		COLOR_WHITE, COLOR_BLACK
	};
	int num_colors = ARRAY_SIZE(colors);
	uint16_t block_width, block_height;
	int i;
	
	display_get_capabilities(display, &caps);
	
	LOG_INF("Display resolution: %dx%d", caps.x_resolution, caps.y_resolution);
	LOG_INF("Pixel format: %d", caps.current_pixel_format);
	
	/* Draw color blocks */
	block_width = caps.x_resolution / 4;
	block_height = caps.y_resolution / 2;
	
	for (i = 0; i < num_colors; i++) {
		uint16_t x = (i % 4) * block_width;
		uint16_t y = (i / 4) * block_height;
		
		LOG_INF("Drawing %d block at (%d, %d) - %dx%d", 
			i, x, y, block_width, block_height);
		
		fill_rect(display, x, y, block_width, block_height, colors[i]);
	}
}

static void draw_gradient(const struct device *display)
{
	struct display_capabilities caps;
	uint16_t *line_buf;
	struct display_buffer_descriptor desc;
	int y, x;
	
	display_get_capabilities(display, &caps);
	
	/* Allocate buffer for one line */
	line_buf = k_malloc(caps.x_resolution * sizeof(uint16_t));
	if (!line_buf) {
		LOG_ERR("Failed to allocate line buffer");
		return;
	}
	
	/* Set up display buffer descriptor for one line */
	desc.buf_size = caps.x_resolution * sizeof(uint16_t);
	desc.width = caps.x_resolution;
	desc.height = 1;
	desc.pitch = caps.x_resolution;
	
	/* Draw horizontal gradient */
	for (y = 0; y < caps.y_resolution; y++) {
		/* Create gradient from blue to red */
		for (x = 0; x < caps.x_resolution; x++) {
			uint8_t red = (x * 31) / caps.x_resolution;
			uint8_t blue = 31 - red;
			uint8_t green = (y * 63) / caps.y_resolution;
			
			line_buf[x] = (red << 11) | (green << 5) | blue;
		}
		
		/* Write line to display */
		display_write(display, 0, y, &desc, line_buf);
		
		/* Show progress every 100 lines */
		if ((y % 100) == 0) {
			LOG_INF("Drawing line %d/%d", y, caps.y_resolution);
		}
	}
	
	k_free(line_buf);
}

int main(void)
{
	const struct device *display;
	int ret;
	
	LOG_INF("LCD Test Application");
	
	display = device_get_binding("LCDC");
	if (!display) {
		LOG_ERR("Display device not found");
		return -ENODEV;
	}
	
	LOG_INF("Display device found");
	
	/* Turn on display */
	ret = display_blanking_off(display);
	if (ret < 0) {
		LOG_ERR("Failed to turn on display: %d", ret);
		return ret;
	}
	
	LOG_INF("Display turned on");
	
	while (1) {
		/* Draw color pattern */
		LOG_INF("Drawing color pattern...");
		draw_pattern(display);
		k_msleep(3000);
		
		/* Clear screen (black) */
		LOG_INF("Clearing screen...");
		fill_rect(display, 0, 0, 480, 800, COLOR_BLACK);
		k_msleep(1000);
		
		/* Draw gradient */
		LOG_INF("Drawing gradient...");
		draw_gradient(display);
		k_msleep(3000);
		
		/* Draw test rectangles */
		LOG_INF("Drawing test rectangles...");
		fill_rect(display, 0, 0, 480, 800, COLOR_BLACK);
		fill_rect(display, 50, 50, 100, 100, COLOR_RED);
		fill_rect(display, 200, 50, 100, 100, COLOR_GREEN);
		fill_rect(display, 350, 50, 100, 100, COLOR_BLUE);
		fill_rect(display, 50, 200, 380, 100, COLOR_YELLOW);
		fill_rect(display, 50, 350, 380, 100, COLOR_CYAN);
		fill_rect(display, 50, 500, 380, 100, COLOR_MAGENTA);
		fill_rect(display, 50, 650, 380, 100, COLOR_WHITE);
		k_msleep(3000);
	}
	
	return 0;
}