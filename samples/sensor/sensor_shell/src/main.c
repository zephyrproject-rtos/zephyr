/*
 * Copyright (c) 2018 Diego Sueiro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/iterable_sections.h>

#include "trigger.h"

LOG_MODULE_REGISTER(app);

int main(void)
{
	if (IS_ENABLED(CONFIG_INIT_TRIG_DATA_READY)) {
		STRUCT_SECTION_FOREACH(sensor_info, sensor)
		{
			struct sensor_trigger trigger = {
				.chan = SENSOR_CHAN_ALL,
				.type = SENSOR_TRIG_DATA_READY,
			};
			sensor_trigger_set(sensor->dev, &trigger,
					   sensor_shell_data_ready_trigger_handler);
		}
	}
	return 0;
}
