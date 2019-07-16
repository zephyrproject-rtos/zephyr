/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/log.h>

#include "peripherals/led.h"
#include "peripherals/display.h"
#include "peripherals/sensors.h"

LOG_MODULE_REGISTER(app_peripherals, LOG_LEVEL_DBG);

int peripherals_init(void)
{
	int err = 0;

	err = led_init();
	if (err) {
		LOG_ERR("LED initialization failed: err %d", err);
		return err;
	}

	err = display_init();
	if (err) {
		LOG_ERR("Display initialization failed: err %d", err);
		return err;
	}

	/* starting sensors thread */
	err = sensors_init();
	if (err) {
		LOG_ERR("Sensors initialization failed: err %d", err);
		return err;
	}

	LOG_INF("Peripherals initialized");

	return 0;
}
