/*
 * Copyright (c) 2024 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_WSEN_PDUS_25131308XXXXX_WSEN_PDUS_25131308XXXXX_H_
#define ZEPHYR_DRIVERS_SENSOR_WSEN_PDUS_25131308XXXXX_WSEN_PDUS_25131308XXXXX_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

#include <weplatform.h>

#include "WSEN_PDUS_25131308XXX01_hal.h"
#include <zephyr/drivers/i2c.h>

struct pdus_25131308XXXXX_data {
	/* WE sensor interface configuration */
	WE_sensorInterface_t sensor_interface;

	/* Last pressure sample */
	uint16_t pressure;

	/* Last temperature sample */
	uint16_t temperature;
};

struct pdus_25131308XXXXX_config {
	union {
		const struct i2c_dt_spec i2c;
	} bus_cfg;

	PDUS_SensorType_t sensor_type;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_WSEN_PDUS_25131308XXXXX_WSEN_PDUS_25131308XXXXX_H_ */
