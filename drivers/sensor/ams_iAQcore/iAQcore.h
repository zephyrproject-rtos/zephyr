/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_AMS_IAQCORE_IAQCORE_H_
#define ZEPHYR_DRIVERS_SENSOR_AMS_IAQCORE_IAQCORE_H_

#include <device.h>
#include <sys/util.h>

struct iaq_registers {
	u16_t co2_pred;
	u8_t  status;
	s32_t resistance;
	u16_t voc;
} __packed;

struct iaq_core_data {
	struct device *i2c;
	u16_t co2;
	u16_t voc;
	u8_t status;
	s32_t resistance;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_AMS_IAQCORE_IAQCORE_H_ */
