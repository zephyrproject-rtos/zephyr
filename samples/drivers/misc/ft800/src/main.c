/*
 * Copyright (c) 2021 Hubert Miś <hubert.mis@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <zephyr/drivers/misc/ft8xx/ft8xx.h>
#include <zephyr/drivers/misc/ft8xx/ft8xx_copro.h>
#include <zephyr/drivers/misc/ft8xx/ft8xx_dl.h>

/**
 * @file Display a counter using FT800.
 */

/* sleep time in msec */
#define SLEEPTIME  100

/* touch tags */
#define TAG_PLUS 1
#define TAG_MINUS 2

static volatile bool process_touch;

static void touch_irq(const struct device *dev, void *user_data)
{
	(void)dev;
	(void)user_data;

	process_touch = true;
}

int main(void)
{
	const struct device *ft8xx = DEVICE_DT_GET_ONE(ftdi_ft800);
	int cnt;
	int val;
	struct ft8xx_touch_transform tt;

	if (!device_is_ready(ft8xx)) {
		printk("FT8xx device is not ready. Aborting\n");
		return -1;
	}

	/* To get touch events calibrate the display */
	ft8xx_calibrate(ft8xx, &tt);

	/* Get interrupts on touch event */
	ft8xx_register_int(ft8xx, touch_irq, NULL);

	/* Starting counting */
	val = 0;
	cnt = 0;

	while (1) {
		/* Start Display List */
		ft8xx_copro_cmd_dlstart(ft8xx);
		ft8xx_copro_cmd(ft8xx, FT8XX_CLEAR_COLOR_RGB(0x00, 0x00, 0x00));
		ft8xx_copro_cmd(ft8xx, FT8XX_CLEAR(1, 1, 1));

		/* Set color */
		ft8xx_copro_cmd(ft8xx, FT8XX_COLOR_RGB(0xf0, 0xf0, 0xf0));

		/* Display the counter */
		ft8xx_copro_cmd_number(ft8xx, 20, 20, 29, FT8XX_OPT_SIGNED, cnt);
		cnt++;

		/* Display the hello message */
		ft8xx_copro_cmd_text(ft8xx, 20, 70, 30, 0, "Hello,");
		/* Set Zephyr color */
		ft8xx_copro_cmd(ft8xx, FT8XX_COLOR_RGB(0x78, 0x29, 0xd2));
		ft8xx_copro_cmd_text(ft8xx, 20, 105, 30, 0, "Zephyr!");

		/* Display value set with buttons */
		ft8xx_copro_cmd(ft8xx, FT8XX_COLOR_RGB(0xff, 0xff, 0xff));
		ft8xx_copro_cmd_number(ft8xx, 80, 170, 29,
					FT8XX_OPT_SIGNED | FT8XX_OPT_RIGHTX,
					val);
		ft8xx_copro_cmd(ft8xx, FT8XX_TAG(TAG_PLUS));
		ft8xx_copro_cmd_text(ft8xx, 90, 160, 31, 0, "+");
		ft8xx_copro_cmd(ft8xx, FT8XX_TAG(TAG_MINUS));
		ft8xx_copro_cmd_text(ft8xx, 20, 160, 31, 0, "-");

		/* Finish Display List */
		ft8xx_copro_cmd(ft8xx, FT8XX_DISPLAY());
		/* Display created frame */
		ft8xx_copro_cmd_swap(ft8xx);

		if (process_touch) {
			int tag = ft8xx_get_touch_tag(ft8xx);

			if (tag == TAG_PLUS) {
				val++;
			} else if (tag == TAG_MINUS) {
				val--;
			}

			process_touch = false;
		}

		/* Wait a while */
		k_msleep(SLEEPTIME);
	}
	return 0;
}
