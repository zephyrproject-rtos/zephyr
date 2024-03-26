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
#include "Devices\Drivers\ICP101xx\Icp101xx.h"
#include "Devices\Drivers\ICP101xx\Icp101xxSerif.h"

#define DT_DRV_COMPAT invensense_icp101xx

#define ICP101XX_BUS_I2C DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

typedef struct {
	int raw_pressure;
	int raw_temperature;
	float pressure;
	float temperature;
	inv_icp101xx_t icp_device;
} icp101xx_data;

struct icp101xx_config {
	struct i2c_dt_spec i2c;
	int mode;
};

#endif
