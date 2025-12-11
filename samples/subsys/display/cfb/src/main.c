/*
 * Copyright (c) 2018 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/display/cfb.h>
#include <stdio.h>

int main(void)
{
	const struct device *dev;
	uint16_t x_res;
	uint16_t y_res;
	uint16_t rows;
	uint8_t ppt;
	uint8_t font_width;
	uint8_t font_height;

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(dev)) {
		printf("Device %s not ready\n", dev->name);
		return 0;
	}

	if (display_set_pixel_format(dev, PIXEL_FORMAT_MONO10) != 0) {
		if (display_set_pixel_format(dev, PIXEL_FORMAT_MONO01) != 0) {
			printf("Failed to set required pixel format");
			return 0;
		}
	}

	printf("Initialized %s\n", dev->name);

	if (cfb_framebuffer_init(dev)) {
		printf("Framebuffer initialization failed!\n");
		return 0;
	}

	cfb_framebuffer_clear(dev, true);

	display_blanking_off(dev);

	x_res = cfb_get_display_parameter(dev, CFB_DISPLAY_WIDTH);
	y_res = cfb_get_display_parameter(dev, CFB_DISPLAY_HEIGHT);
	rows = cfb_get_display_parameter(dev, CFB_DISPLAY_ROWS);
	ppt = cfb_get_display_parameter(dev, CFB_DISPLAY_PPT);

	for (int idx = 0; idx < 42; idx++) {
		if (cfb_get_font_size(dev, idx, &font_width, &font_height)) {
			break;
		}
		cfb_framebuffer_set_font(dev, idx);
		printf("font width %d, font height %d\n",
		       font_width, font_height);
	}

	printf("x_res %d, y_res %d, ppt %d, rows %d, cols %d\n",
	       x_res,
	       y_res,
	       ppt,
	       rows,
	       cfb_get_display_parameter(dev, CFB_DISPLAY_COLS));

	cfb_framebuffer_invert(dev);

	cfb_set_kerning(dev, 3);

	while (1) {
		for (int i = 0; i < MIN(x_res, y_res); i++) {
			cfb_framebuffer_clear(dev, false);
			if (cfb_print(dev,
				      "0123456789mMgj!\"§$%&/()=",
				      i, i)) {
				printf("Failed to print a string\n");
				continue;
			}

			cfb_framebuffer_finalize(dev);
#if defined(CONFIG_ARCH_POSIX)
			k_sleep(K_MSEC(20));
#endif
		}
	}
	return 0;
}
