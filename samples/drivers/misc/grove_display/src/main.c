/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/drivers/misc/grove_lcd/grove_lcd.h>

/**
 * @file Display a counter through I2C and Grove LCD.
 */

/* sleep time in msec */
#define SLEEPTIME  100

uint8_t clamp_rgb(int val)
{
	if (val > 255) {
		return 255;
	} else if (val < 0) {
		return 0;
	} else {
		return val;
	}
}

int main(void)
{
	const struct device *const glcd = DEVICE_DT_GET(DT_NODELABEL(glcd));
	char str[20];
	int rgb[] = { 0x0, 0x0, 0x0 };
	uint8_t rgb_chg[3];
	const uint8_t rgb_step = 16U;
	uint8_t set_config;
	int i, j, m;
	int cnt;

	if (!device_is_ready(glcd)) {
		printk("Grove LCD: Device not ready.\n");
		return 0;
	}

	/* Now configure the LCD the way we want it */

	set_config = GLCD_FS_ROWS_2
		     | GLCD_FS_DOT_SIZE_LITTLE
		     | GLCD_FS_8BIT_MODE;

	glcd_function_set(glcd, set_config);

	set_config = GLCD_DS_DISPLAY_ON | GLCD_DS_CURSOR_ON | GLCD_DS_BLINK_ON;

	glcd_display_state_set(glcd, set_config);

	/* Setup variables /*/
	for (i = 0; i < sizeof(str); i++) {
		str[i] = '\0';
	}

	/* Starting counting */
	cnt = 0;

	while (1) {
		glcd_cursor_pos_set(glcd, 0, 0);

		/* RGB values are from 0 - 511.
		 * First half means incrementally brighter.
		 * Second half is opposite (i.e. goes darker).
		 */

		/* Update the RGB values for backlight */
		m = (rgb[2] > 255) ? (512 - rgb[2]) : (rgb[2]);
		rgb_chg[2] = clamp_rgb(m);

		m = (rgb[1] > 255) ? (512 - rgb[1]) : (rgb[1]);
		rgb_chg[1] = clamp_rgb(m);

		m = (rgb[0] > 255) ? (512 - rgb[0]) : (rgb[0]);
		rgb_chg[0] = clamp_rgb(m);

		glcd_color_set(glcd, rgb_chg[0], rgb_chg[1], rgb_chg[2]);

		/* Display the counter
		 *
		 * well... sprintf() might be easier,
		 * but this is more fun.
		 */
		m = cnt;
		i = 1000000000;
		j = 0;
		while (i > 0) {
			str[j] = '0' + (m / i);

			m = m % i;
			i = i / 10;
			j++;
		}
		cnt++;

		glcd_print(glcd, str, j);

		/* Rotate RGB values */
		rgb[2] += rgb_step;
		if (rgb[2] > 511) {
			rgb[2] = 0;
			rgb[1] += rgb_step;
		}
		if (rgb[1] > 511) {
			rgb[1] = 0;
			rgb[0] += rgb_step;
		}
		if (rgb[0] > 511) {
			rgb[0] = 0;
		}

		/* wait a while */
		k_msleep(SLEEPTIME);
	}
	return 0;
}
