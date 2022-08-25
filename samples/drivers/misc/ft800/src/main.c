/*
 * Copyright (c) 2021 Hubert Miś <hubert.mis@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

static void touch_irq(void)
{
	process_touch = true;
}

void main(void)
{
	int cnt;
	int val;
	struct ft8xx_touch_transform tt;

	/* To get touch events calibrate the display */
	ft8xx_calibrate(&tt);

	/* Get interrupts on touch event */
	ft8xx_register_int(touch_irq);

	/* Starting counting */
	val = 0;
	cnt = 0;

	while (1) {
		/* Start Display List */
		ft8xx_copro_cmd_dlstart();
		ft8xx_copro_cmd(FT8XX_CLEAR_COLOR_RGB(0x00, 0x00, 0x00));
		ft8xx_copro_cmd(FT8XX_CLEAR(1, 1, 1));

		/* Set color */
		ft8xx_copro_cmd(FT8XX_COLOR_RGB(0xf0, 0xf0, 0xf0));

		/* Display the counter */
		ft8xx_copro_cmd_number(20, 20, 29, FT8XX_OPT_SIGNED, cnt);
		cnt++;

		/* Display the hello message */
		ft8xx_copro_cmd_text(20, 70, 30, 0, "Hello,");
		/* Set Zephyr color */
		ft8xx_copro_cmd(FT8XX_COLOR_RGB(0x78, 0x29, 0xd2));
		ft8xx_copro_cmd_text(20, 105, 30, 0, "Zephyr!");

		/* Display value set with buttons */
		ft8xx_copro_cmd(FT8XX_COLOR_RGB(0xff, 0xff, 0xff));
		ft8xx_copro_cmd_number(80, 170, 29,
					FT8XX_OPT_SIGNED | FT8XX_OPT_RIGHTX,
					val);
		ft8xx_copro_cmd(FT8XX_TAG(TAG_PLUS));
		ft8xx_copro_cmd_text(90, 160, 31, 0, "+");
		ft8xx_copro_cmd(FT8XX_TAG(TAG_MINUS));
		ft8xx_copro_cmd_text(20, 160, 31, 0, "-");

		/* Finish Display List */
		ft8xx_copro_cmd(FT8XX_DISPLAY());
		/* Display created frame */
		ft8xx_copro_cmd_swap();

		if (process_touch) {
			int tag = ft8xx_get_touch_tag();

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
}
