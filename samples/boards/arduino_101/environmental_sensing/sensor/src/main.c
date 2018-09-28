/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <ipm.h>
#include <ipm/ipm_quark_se.h>
#include <misc/printk.h>
#include <misc/util.h>
#include <sensor.h>
#include <zephyr.h>

#ifdef CONFIG_GROVE_LCD_RGB
#include <display/grove_lcd.h>
#include <stdio.h>
#include <string.h>
#endif

QUARK_SE_IPM_DEFINE(ess_ipm, 0, QUARK_SE_IPM_OUTBOUND);

struct channel_info {
	int chan;
	char *dev_name;
};

/* change device names if you want to use different sensors */
static struct channel_info info[] = {
	{SENSOR_CHAN_AMBIENT_TEMP, "HDC1008"},
	{SENSOR_CHAN_HUMIDITY, "HDC1008"},
	{SENSOR_CHAN_PRESS, "BMP280"},
};

void main(void)
{
	struct device *ipm, *dev[ARRAY_SIZE(info)];
	struct sensor_value val[ARRAY_SIZE(info)];
	unsigned int i;
	int rc;

	ipm = device_get_binding("ess_ipm");
	if (ipm == NULL) {
		printk("Failed to get ESS IPM device\n");
		return;
	}


	for (i = 0; i < ARRAY_SIZE(info); i++) {
		dev[i] = device_get_binding(info[i].dev_name);
		if (dev[i] == NULL) {
			printk("Failed to get \"%s\" device\n", info[i].dev_name);
			return;
		}
	}

#ifdef CONFIG_GROVE_LCD_RGB
	struct device *glcd;

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
		for (i = 0; i < ARRAY_SIZE(info); i++) {
			rc = sensor_sample_fetch(dev[i]);
			if (rc) {
				printk("Failed to fetch sample for device %s (%d)\n",
				       info[i].dev_name, rc);
			}
		}

		/* send sensor data to x86 core via IPM */
		for (i = 0; i < ARRAY_SIZE(info); i++) {
			rc = sensor_channel_get(dev[i], info[i].chan, &val[i]);
			if (rc) {
				printk("Failed to get data for device %s (%d)\n",
				       info[i].dev_name, rc);
				continue;
			}

			rc = ipm_send(ipm, 1, info[i].chan, &val[i], sizeof(val[i]));
			if (rc) {
				printk("Failed to send data for device %s (%d)\n",
				       info[i].dev_name, rc);
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
		sprintf(row, "T:%d.%d%cC", val[0].val1,
			val[0].val2/100000, 223 /* degree symbol */);
		glcd_print(glcd, row, strlen(row));

		/* display himidity on LCD */
		glcd_cursor_pos_set(glcd, 17 - strlen(row), 0);
		sprintf(row, "RH:%d%%", val[1].val1);
		glcd_print(glcd, row, strlen(row));

		/* display pressure on LCD */
		glcd_cursor_pos_set(glcd, 0, 1);
		sprintf(row, "P:%d.%02dkPa", val[2].val1,
		       val[2].val2 / 10000);
		glcd_print(glcd, row, strlen(row));
#endif

		k_sleep(K_MSEC(100));
	}
}
