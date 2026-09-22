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
	int ret;

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

	ret = cfb_framebuffer_clear(dev, true);
	if (ret < 0) {
		printf("cfb_framebuffer_clear(%s, true) failed: %d\n", dev->name, ret);
		return 0;
	}

	ret = display_blanking_off(dev);
	if (ret < 0 && ret != -ENOSYS) {
		printf("display_blanking_off(%s) failed: %d\n", dev->name, ret);
		return 0;
	}

	x_res = cfb_get_display_parameter(dev, CFB_DISPLAY_WIDTH);
	y_res = cfb_get_display_parameter(dev, CFB_DISPLAY_HEIGHT);
	rows = cfb_get_display_parameter(dev, CFB_DISPLAY_ROWS);
	ppt = cfb_get_display_parameter(dev, CFB_DISPLAY_PPT);

	for (int idx = 0; idx < 42; idx++) {
		if (cfb_get_font_size(dev, idx, &font_width, &font_height)) {
			break;
		}
		ret = cfb_framebuffer_set_font(dev, idx);
		if (ret < 0) {
			printf("cfb_framebuffer_set_font(%s, %d) failed: %d\n", dev->name, idx,
			       ret);
			return 0;
		}
		printf("font width %d, font height %d\n",
		       font_width, font_height);
	}

	printf("x_res %d, y_res %d, ppt %d, rows %d, cols %d\n",
	       x_res,
	       y_res,
	       ppt,
	       rows,
	       cfb_get_display_parameter(dev, CFB_DISPLAY_COLS));

	ret = cfb_framebuffer_invert(dev);
	if (ret < 0) {
		printf("cfb_framebuffer_invert(%s) failed: %d\n", dev->name, ret);
		return 0;
	}

	ret = cfb_set_kerning(dev, 3);
	if (ret < 0) {
		printf("cfb_set_kerning(%s, 3) failed: %d\n", dev->name, ret);
		return 0;
	}

	while (1) {
		for (int i = 0; i < MIN(x_res, y_res); i++) {
#if defined(CONFIG_ARCH_POSIX)
			k_sleep(K_MSEC(20));
#endif

			ret = cfb_framebuffer_clear(dev, false);
			if (ret < 0) {
				printf("Failed to clear the framebuffer: %d\n", ret);
				continue;
			}

			ret = cfb_print(dev, "0123456789mMgj!\"\xc2\xA7$%&/()=", i, i);
			if (ret < 0) {
				printf("Failed to print a string: %d\n", ret);
				continue;
			}

			ret = cfb_framebuffer_finalize(dev);
			if (ret < 0) {
				printf("Finalize failed: %d\n", ret);
				continue;
			}
		}
	}
	return 0;
}
