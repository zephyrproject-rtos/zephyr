/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_AMS_IAQCORE_IAQCORE_H_
#define ZEPHYR_DRIVERS_SENSOR_AMS_IAQCORE_IAQCORE_H_

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

struct iaq_registers {
	uint16_t co2_pred;
	uint8_t  status;
	int32_t resistance;
	uint16_t voc;
} __packed;

struct iaq_core_config {
	struct i2c_dt_spec i2c;
};

struct iaq_core_data {
	uint16_t co2;
	uint16_t voc;
	uint8_t status;
	int32_t resistance;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_AMS_IAQCORE_IAQCORE_H_ */
