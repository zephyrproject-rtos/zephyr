/*
 * Copyright (c) 2025 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM42670_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM42670_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

#include "icm42x70.h"

int icm42670_gyro_config(struct icm42x70_data *drv_data, enum sensor_attribute attr,
			 const struct sensor_value *val);
void icm42670_convert_gyro(struct sensor_value *val, int16_t raw_val, uint16_t fs);
int icm42670_sample_fetch_gyro(const struct device *dev);
uint16_t convert_bitfield_to_gyr_fs(uint8_t bitfield);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM42670_H_ */
