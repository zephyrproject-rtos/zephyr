/*
 * Copyright (c) 2023 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICP101XXX_ICP101XXX_H_
#define ZEPHYR_DRIVERS_SENSOR_ICP101XXX_ICP101XXX_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include "Icp101xx.h"
#include "Icp101xxSerif.h"

#define ICP101XX_BUS_I2C DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

struct icp101xx_data {
	int raw_pressure;
	int raw_temperature;
#ifdef ICP101XX_DRV_USE_FLOATS
	float pressure;
	float temperature;
#else
	int32_t pressure;
	int32_t temperature;
#endif
	inv_icp101xx_t icp_device;
};

struct icp101xx_config {
	struct i2c_dt_spec i2c;
	int mode;
};

#endif
