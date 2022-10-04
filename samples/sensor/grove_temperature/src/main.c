/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/init.h>
#include <stdio.h>
#include <zephyr/drivers/sensor.h>

#ifdef CONFIG_GROVE_LCD_RGB
#include <zephyr/drivers/misc/grove_lcd/grove_lcd.h>
#include <stdio.h>
#include <string.h>
#endif

#define SLEEP_TIME	K_MSEC(1000)

void main(void)
{
	const struct device *dev = DEVICE_DT_GET_ONE(seeed_grove_temperature);
	struct sensor_value temp;
	int read;

	if (!device_is_ready(dev)) {
		printk("sensor: device not ready.\n");
		return;
	}

#ifdef CONFIG_GROVE_LCD_RGB
	const struct device *glcd;

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

		read = sensor_sample_fetch(dev);
		if (read) {
			printk("sample fetch error\n");
			continue;
		}
		sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
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
#ifdef CONFIG_NEWLIB_LIBC_FLOAT_PRINTF
		sprintf(row, "T:%.2f%cC",
			sensor_value_to_double(&temp),
			223 /* degree symbol */);
#else
		sprintf(row, "T:%d%cC", temp.val1,
			223 /* degree symbol */);
#endif
		glcd_print(glcd, row, strlen(row));

#endif

#ifdef CONFIG_NEWLIB_LIBC_FLOAT_PRINTF
		printf("Temperature: %.2f C\n", sensor_value_to_double(&temp));
#else
		printk("Temperature: %d\n", temp.val1);
#endif
		k_sleep(SLEEP_TIME);
	}
}
