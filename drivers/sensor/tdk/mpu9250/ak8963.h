/*
 * Copyright (c) 2021, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MPU9250_AK8963_H_
#define ZEPHYR_DRIVERS_SENSOR_MPU9250_AK8963_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>


int ak8963_convert_magn(struct sensor_value *val, int16_t raw_val,
			int16_t scale, uint8_t st2);

int ak8963_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_MPU9250_AK8963_H_ */
