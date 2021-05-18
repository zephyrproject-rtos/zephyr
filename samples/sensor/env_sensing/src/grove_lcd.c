/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>

#include <display/grove_lcd.h>
#include <stdio.h>
#include <string.h>
#include <drivers/sensor.h>

#include "disp.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(main);

extern const struct device *display;

void init_display(void)
{

	display = device_get_binding(GROVE_LCD_NAME);
	if (display == NULL) {
		LOG_ERR("Failed to get Grove LCD");
		return NULL;
	}

	/* configure LCD */
	glcd_function_set(display, GLCD_FS_ROWS_2 | GLCD_FS_DOT_SIZE_LITTLE |
			  GLCD_FS_8BIT_MODE);
	glcd_display_state_set(display, GLCD_DS_DISPLAY_ON);
}

void reset_display(void)
{
	char row[16];

	/* clear LCD */
	(void)memset(row, ' ', sizeof(row));
	glcd_cursor_pos_set(display, 0, 0);
	glcd_print(display, row, sizeof(row));
	glcd_cursor_pos_set(display, 0, 1);
	glcd_print(display, row, sizeof(row));
}


void update_display(struct sensor_value temp,
			struct sensor_value hum,
			struct sensor_value press,
			struct sensor_value gas)
{

	char row[16];

	glcd_cursor_pos_set(display, 0, 0);
#if CONFIG_HAS_TEMPERATURE
	snprintk(row, sizeof(row), "T: %.1f%cC", sensor_value_to_double(&temp),
		223 /* degree symbol */);
	glcd_print(display, row, strlen(row));
#endif
#if CONFIG_HAS_HUMIDITY
	glcd_cursor_pos_set(display, 0, 1);
	snprintk(row, sizeof(row), "H: %.1f%%",
		sensor_value_to_double(&hum));
	glcd_print(display, row, strlen(row));
#endif
}
