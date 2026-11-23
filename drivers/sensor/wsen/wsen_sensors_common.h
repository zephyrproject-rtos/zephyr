/*
 * Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_WSEN_COMMON_H_
#define ZEPHYR_DRIVERS_SENSOR_WSEN_COMMON_H_

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/drivers/sensor.h>

#define MAX_POLL_STEP_COUNT 10

static inline bool
wsen_sensor_step_sleep_duration_milli_from_odr_hz(const struct sensor_value *odr_hz,
						  uint32_t *step_sleep_duation_milli)
{
	if ((odr_hz == NULL) || (step_sleep_duation_milli == NULL)) {
		return false;
	}

	uint32_t odr_milli = (uint32_t)sensor_value_to_milli(odr_hz);

	if (odr_milli == 0) {
		return false;
	}

	uint32_t poll_cycle_duration = 1000000000 / odr_milli;

	*step_sleep_duation_milli = poll_cycle_duration / MAX_POLL_STEP_COUNT;

	return true;
}

#endif /* ZEPHYR_DRIVERS_SENSOR_WSEN_COMMON_H_ */
