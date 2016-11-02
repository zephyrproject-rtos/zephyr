/*
 * Copyright (c) 2016 Intel Corporation
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
#include <init.h>
#include <stdio.h>
#include <sensor.h>

#ifdef CONFIG_GROVE_LCD_RGB
#include <display/grove_lcd.h>
#include <stdio.h>
#include <string.h>
#endif

#define SLEEP_TIME	1000

void main(void)
{
	struct device *dev = device_get_binding("GROVE_TEMPERATURE_SENSOR");

	if (dev == NULL) {
		printf("device not found.  aborting test.\n");
		return;
	}
#ifdef CONFIG_GROVE_LCD_RGB
	struct device *glcd;

	glcd = device_get_binding(GROVE_LCD_NAME);
	if (glcd == NULL) {
		printf("Failed to get Grove LCD\n");
		return;
	}

	/* configure LCD */
	glcd_function_set(glcd, GLCD_FS_ROWS_2 | GLCD_FS_DOT_SIZE_LITTLE |
			  GLCD_FS_8BIT_MODE);
	glcd_display_state_set(glcd, GLCD_DS_DISPLAY_ON);
#endif

	while (1) {
		struct sensor_value temp;

		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_TEMP, &temp);
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
		sprintf(row, "T:%.2f%cC", temp.dval, 223 /* degree symbol */);
		glcd_print(glcd, row, strlen(row));

#endif
		printf("Temperature: %.2f C\n", temp.dval);

		k_sleep(SLEEP_TIME);
	}
}

