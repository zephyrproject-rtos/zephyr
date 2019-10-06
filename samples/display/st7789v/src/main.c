/*
 * Copyright (c) 2019 Marc Reilly
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#include <zephyr.h>
#include <device.h>
#include <display.h>

static void backlight_init(void)
{
    /* If you have a backlight, set it up and turn it on here */
}

void main(void)
{
	LOG_INF("ST7789V display sample");
	struct device *display_dev;

	struct display_capabilities capabilities;
	struct display_buffer_descriptor buf_desc;

	backlight_init();

#ifdef CONFIG_ST7789V_RGB565
	const size_t rgb_size = 2;
#else
	const size_t rgb_size = 3;
#endif

	display_dev = device_get_binding(DT_INST_0_SITRONIX_ST7789V_LABEL);

	if (display_dev == NULL) {
		LOG_ERR("Device not found. Aborting test.");
		return;
	}

	display_get_capabilities(display_dev, &capabilities);

	/* size of the rectangle */
	const size_t w = 40;
	const size_t h = 20;
	const size_t buf_size = rgb_size * w * h;

	/* points are clockwise, from top left */
	size_t x0, y0, x1, y1, x2, y2, x3, y3;

	/* top left */
	x0 = 0;
	y0 = 0;
	/* top right */
	x1 = capabilities.x_resolution - w;
	y1 = 0;
	/* bottom right */
	x2 = capabilities.x_resolution - w;
	y2 = capabilities.y_resolution - h;
	/* bottom left */
	x3 = 0;
	y3 = capabilities.y_resolution - h;

	/* Allocate rectangular buffer for corner data */
	u8_t *buf = k_malloc(buf_size);

	if (buf == NULL) {
		LOG_ERR("Could not allocate memory. Aborting test.");
		return;
	}

	/* Clear frame buffer before enabling LCD, reuse corner buffer
	 */
	int h_step;
	(void)memset(buf, 0, buf_size);
	h_step = (w * h) / capabilities.x_resolution;

	buf_desc.buf_size = buf_size;
	buf_desc.pitch = capabilities.x_resolution;
	buf_desc.width = capabilities.x_resolution;
	buf_desc.height = h_step;

	for (int idx = 0; idx < capabilities.y_resolution; idx += h_step) {
		display_write(display_dev, 0, idx, &buf_desc,  buf);
	}

	display_blanking_off(display_dev);

	buf_desc.pitch = w;
	buf_desc.width = w;
	buf_desc.height = h;

	int grey_count = 0;
	size_t cnt = 0;

	while (1) {
		/* Update the color of the rectangle buffer and write the buffer
		 * to  one of the corners
		 */
#ifdef CONFIG_ST7789V_RGB565
		int color = cnt % 4;
		/* RGB565 format */
		u16_t color_r;
		u16_t color_g;
		u16_t color_b;
		u16_t color_rgb;

		color_r = (color == 0) ? 0xF800U : 0U;
		color_g = (color == 1) ? 0x07E0U : 0U;
		color_b = (color == 2) ? 0x001FU : 0U;
		color_rgb = color_r + color_g + color_b;
		if (color == 3) {
			u16_t t = grey_count & 0x1f;
			/* shift the green an extra bit, it has 6 bits */
			color_rgb = t << 11 | t << (5+1) | t;
			grey_count++;
		}

		for (size_t idx = 0; idx < buf_size; idx += rgb_size) {
			*(buf + idx + 0) = (color_rgb >> 8) & 0xFFU;
			*(buf + idx + 1) = (color_rgb >> 0) & 0xFFU;
		}
#else
		u32_t color_rgb;
		u32_t c = grey_count & 0xff;

		switch (cnt % 4) {
		case 0:
			color_rgb = 0x00FF0000u;
			break;
		case 1:
			color_rgb = 0x0000FF00u;
			break;
		case 2:
			color_rgb = 0x000000FFu;
			break;
		case 3:
			color_rgb = c << 16 | c << 8 | c;
			grey_count++;
			break;
		}

		/* RGB888 format */
		for (size_t idx = color; idx < buf_size; idx += rgb_size) {
			*(buf + idx + 0) = color_rgb >> 16;
			*(buf + idx + 1) = color_rgb >> 8;
			*(buf + idx + 2) = color_rgb >> 0;
		}
#endif

		switch (cnt % 4) {
		case 0:
			/* top left, red */
			display_write(display_dev, x0, y0, &buf_desc, buf);
			break;
		case 1:
			/* top right, green */
			display_write(display_dev, x1, y1, &buf_desc, buf);
			break;
		case 2:
			/* bottom right, blue */
			display_write(display_dev, x2, y2, &buf_desc, buf);
			break;
		case 3:
			/* bottom left, alternating grey */
			display_write(display_dev, x3, y3, &buf_desc, buf);
			break;
		}
		++cnt;
		k_sleep(K_MSEC(100));
	}
}
