/*
 * Copyright (c) 2024 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_WSEN_HIDS_2525020210002_WSEN_HIDS_2525020210002_H_
#define ZEPHYR_DRIVERS_SENSOR_WSEN_HIDS_2525020210002_WSEN_HIDS_2525020210002_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

#include <weplatform.h>

#include "WSEN_HIDS_2525020210002_hal.h"

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor/wsen_hids_2525020210002.h>

struct hids_2525020210002_data {
	/* WE sensor interface configuration */
	WE_sensorInterface_t sensor_interface;

	/* Last humidity sample */
	int32_t humidity;

	/* Last temperature sample */
	int32_t temperature;

	hids_2525020210002_precision_t sensor_precision;

	hids_2525020210002_heater_t sensor_heater;
};

struct hids_2525020210002_config {
	union {
		const struct i2c_dt_spec i2c;
	} bus_cfg;

	const hids_2525020210002_precision_t precision;

	const hids_2525020210002_heater_t heater;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_WSEN_HIDS_2525020210002_WSEN_HIDS_2525020210002_H_ */
