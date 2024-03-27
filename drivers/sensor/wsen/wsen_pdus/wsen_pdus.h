/*
 * Copyright (c) 2023 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_WSEN_PDUS_WSEN_PDUS_H_
#define ZEPHYR_DRIVERS_SENSOR_WSEN_PDUS_WSEN_PDUS_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

#include <weplatform.h>

#include "WSEN_PDUS_25131308XXX01.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

struct pdus_data {
	/* WE sensor interface configuration */
	WE_sensorInterface_t sensor_interface;

	/* Last pressure sample */
	float pressure_k_pa;

	/* Last temperature sample */
	float temperature_deg_c;
};

struct pdus_config {
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct i2c_dt_spec i2c;
#endif
	} bus_cfg;

	PDUS_SensorType_t sensor_type;
};

int pdus_i2c_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_WSEN_PDUS_WSEN_PDUS_H_ */
