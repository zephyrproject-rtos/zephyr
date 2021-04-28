/*
 * Copyright (c) 2021 Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SGP40_SGP40_H_
#define ZEPHYR_DRIVERS_SENSOR_SGP40_SGP40_H_

#include <device.h>

#define SGP40_CMD_MEASURE_RAW	0x260F
#define SGP40_CMD_MEASURE_TEST	0x280E
#define SGP40_CMD_HEATER_OFF	0x3615

#define SGP40_TEST_OK		0xD400
#define SGP40_TEST_FAIL		0x4B00

#define SGP40_RESET_WAIT_MS	10
#define SGP40_MEASURE_WAIT_MS	30
#define SGP40_TEST_WAIT_MS	250

/*
 * CRC parameters were taken from the
 * "Checksum Calculation" section of the datasheet.
 */
#define SGP40_CRC_POLY		0x31
#define SGP40_CRC_INIT		0xFF

/*
 * Value range of compensation data parameters
 */
#define SGP40_COMP_MIN_RH	0
#define SGP40_COMP_MAX_RH	100
#define SGP40_COMP_MIN_T	-45
#define SGP40_COMP_MAX_T	130
#define SGP40_COMP_DEFAULT_T	25
#define SGP40_COMP_DEFAULT_RH	50

struct sgp40_config {
	const struct device *bus;
	uint8_t i2c_addr;
	bool selftest;
};

struct sgp40_data {
	uint16_t raw_sample;
	int8_t rh_param[3];
	int8_t t_param[3];

#ifdef CONFIG_PM_DEVICE
	enum pm_device_state pm_state;
#endif
};

#endif /* ZEPHYR_DRIVERS_SENSOR_SGP40_SGP40_H_ */
