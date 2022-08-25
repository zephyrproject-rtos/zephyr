/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/misc/grove_lcd/grove_lcd.h>
#include <stdio.h>
#include <string.h>

struct channel_info {
	int chan;
};

static struct channel_info info[] = {
	{ SENSOR_CHAN_AMBIENT_TEMP, },
	{ SENSOR_CHAN_HUMIDITY, },
};

void main(void)
{
	const struct device *const glcd = DEVICE_DT_GET(DT_NODELABEL(glcd));
	const struct device *const th02 = DEVICE_DT_GET_ONE(hoperf_th02);
	struct sensor_value val[ARRAY_SIZE(info)];
	unsigned int i;
	int rc;

	if (!device_is_ready(th02)) {
		printk("TH02 is not ready\n");
		return;
	}

	if (!device_is_ready(glcd)) {
		printk("Grove LCD not ready\n");
		return;
	}

	/* configure LCD */
	glcd_function_set(glcd, GLCD_FS_ROWS_2 | GLCD_FS_DOT_SIZE_LITTLE |
			  GLCD_FS_8BIT_MODE);
	glcd_display_state_set(glcd, GLCD_DS_DISPLAY_ON);

	while (1) {
		/* fetch sensor samples */
		rc = sensor_sample_fetch(th02);
		if (rc) {
			printk("Failed to fetch sample for device TH02 (%d)\n", rc);
		}

		for (i = 0U; i < ARRAY_SIZE(info); i++) {
			rc = sensor_channel_get(th02, info[i].chan, &val[i]);
			if (rc) {
				printk("Failed to get data for device TH02 (%d)\n", rc);
				continue;
			}
		}

		char row[16];

		/* clear LCD */
		(void)memset(row, ' ', sizeof(row));
		glcd_cursor_pos_set(glcd, 0, 0);
		glcd_print(glcd, row, sizeof(row));
		glcd_cursor_pos_set(glcd, 0, 1);
		glcd_print(glcd, row, sizeof(row));

		/* display temperature on LCD */
		glcd_cursor_pos_set(glcd, 0, 0);
		sprintf(row, "T:%.1f%cC", sensor_value_to_double(val),
			223 /* degree symbol */);
		glcd_print(glcd, row, strlen(row));

		/* display humidity on LCD */
		glcd_cursor_pos_set(glcd, 17 - strlen(row), 0);
		sprintf(row, "RH:%.0f%c", sensor_value_to_double(val + 1),
			37 /* percent symbol */);
		glcd_print(glcd, row, strlen(row));

		k_sleep(K_MSEC(2000));
	}
}
