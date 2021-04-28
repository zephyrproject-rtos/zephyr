/*
 * Copyright (c) 2021 Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SGP40_SGP40_H_
#define ZEPHYR_DRIVERS_SENSOR_SGP40_SGP40_H_

#include <device.h>
#include <kernel.h>
#include <drivers/gpio.h>

#define SGP40_CMD_MEASURE_RAW	0x260F
#define SGP40_CMD_MEASURE_TEST	0x280E
#define SGP40_CMD_HEATER_OFF	0x3615

#define SGP40_TEST_OK		0xD400
#define SGP40_TEST_FAIL		0x4B00

#define SGP40_RESET_WAIT_MS	10
#define SGP40_MEASURE_WAIT_MS	30
#define SGP40_TEST_WAIT_MS	250

struct sgp40_config {
	const struct device *bus;
	uint8_t i2c_addr;
};

struct sgp40_data {
	uint16_t raw_sample;
	int8_t rh_param[3];
	int8_t t_param[3];

#ifdef CONFIG_PM_DEVICE
	uint32_t pm_state;
#endif
};

#endif /* ZEPHYR_DRIVERS_SENSOR_SGP40_SGP40_H_ */
