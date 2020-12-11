/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <sys/printk.h>
#include <sys/util.h>

#ifdef CONFIG_GROVE_LCD_RGB
#include <display/grove_lcd.h>
#include <stdio.h>
#include <string.h>
#endif

struct channel_info {
	int chan;
	char *dev_name;
};

/* change device names if you want to use different sensors */
static struct channel_info info[] = {
	{ SENSOR_CHAN_AMBIENT_TEMP, "TH02" },
	{ SENSOR_CHAN_HUMIDITY, "TH02" },
};

void main(void)
{
	const struct device *dev[ARRAY_SIZE(info)];
	struct sensor_value val[ARRAY_SIZE(info)];
	unsigned int i;
	int rc;

	for (i = 0U; i < ARRAY_SIZE(info); i++) {
		dev[i] = device_get_binding(info[i].dev_name);
		if (dev[i] == NULL) {
			printk("Failed to get \"%s\" device\n",
			       info[i].dev_name);
			return;
		}
	}

#ifdef CONFIG_GROVE_LCD_RGB
	const struct device *glcd;

	glcd = device_get_binding(GROVE_LCD_NAME);
	if (glcd == NULL) {
		printk("Failed to get Grove LCD\n");
		return;
	}

	/* configure LCD */
	glcd_function_set(glcd, GLCD_FS_ROWS_2 | GLCD_FS_DOT_SIZE_LITTLE |
			  GLCD_FS_8BIT_MODE);
	glcd_display_state_set(glcd, GLCD_DS_DISPLAY_ON);
#endif

	while (1) {
		/* fetch sensor samples */
		for (i = 0U; i < ARRAY_SIZE(info); i++) {
			rc = sensor_sample_fetch(dev[i]);
			if (rc) {
				printk("Failed to fetch sample for device %s (%d)\n",
				       info[i].dev_name, rc);
			}
		}

		for (i = 0U; i < ARRAY_SIZE(info); i++) {
			rc = sensor_channel_get(dev[i], info[i].chan, &val[i]);
			if (rc) {
				printk("Failed to get data for device %s (%d)\n",
				       info[i].dev_name, rc);
				continue;
			}
		}

#ifdef CONFIG_GROVE_LCD_RGB
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

		/* display himidity on LCD */
		glcd_cursor_pos_set(glcd, 17 - strlen(row), 0);
		sprintf(row, "RH:%.0f%c", sensor_value_to_double(val + 1),
			37 /* percent symbol */);
		glcd_print(glcd, row, strlen(row));

#endif

		k_sleep(K_MSEC(2000));
	}
}
