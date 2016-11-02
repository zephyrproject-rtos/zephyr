/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <device.h>
#include <sensor.h>
#include <misc/printk.h>
#include <misc/util.h>

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
	{ SENSOR_CHAN_TEMP, "TH02" },
	{ SENSOR_CHAN_HUMIDITY, "TH02" },
};

void main(void)
{
	struct device *dev[ARRAY_SIZE(info)];
	struct sensor_value val[ARRAY_SIZE(info)];
	unsigned int i;
	int rc;

	for (i = 0; i < ARRAY_SIZE(info); i++) {
		dev[i] = device_get_binding(info[i].dev_name);
		if (dev[i] == NULL) {
			printk("Failed to get \"%s\" device\n",
			       info[i].dev_name);
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

		for (i = 0; i < ARRAY_SIZE(info); i++) {
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
		memset(row, ' ', sizeof(row));
		glcd_cursor_pos_set(glcd, 0, 0);
		glcd_print(glcd, row, sizeof(row));
		glcd_cursor_pos_set(glcd, 0, 1);
		glcd_print(glcd, row, sizeof(row));

		/* display temperature on LCD */
		glcd_cursor_pos_set(glcd, 0, 0);
		sprintf(row, "T:%.1f%cC", val[0].dval, 223 /* degree symbol */);
		glcd_print(glcd, row, strlen(row));

		/* display himidity on LCD */
		glcd_cursor_pos_set(glcd, 17 - strlen(row), 0);
		sprintf(row, "RH:%.0f%c", val[1].dval, 37 /* percent symbol */);
		glcd_print(glcd, row, strlen(row));

#endif

		k_sleep(2000);
	}
}
