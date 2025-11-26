/*
 * Copyright (c) 2024 LCKFB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_GXHTC3_GXHTC3_H_
#define ZEPHYR_DRIVERS_SENSOR_GXHTC3_GXHTC3_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

/* GXHTC3 I2C address */
#define GXHTC3_I2C_ADDR		0x70

/* GXHTC3 commands */
#define GXHTC3_CMD_READ_ID_MSB	0xEF
#define GXHTC3_CMD_READ_ID_LSB	0xC8
#define GXHTC3_CMD_WAKE_UP_MSB	0x35
#define GXHTC3_CMD_WAKE_UP_LSB	0x17
#define GXHTC3_CMD_MEASURE_MSB	0x7C
#define GXHTC3_CMD_MEASURE_LSB	0xA2
#define GXHTC3_CMD_SLEEP_MSB	0xB0
#define GXHTC3_CMD_SLEEP_LSB	0x98

/* CRC polynomial */
#define GXHTC3_CRC_POLYNOMIAL	0x31

/* Measurement delay in milliseconds */
#define GXHTC3_MEASURE_DELAY_MS	20

struct gxhtc3_data {
	uint16_t raw_temp;
	uint16_t raw_humi;
	float temperature;
	float humidity;
};

struct gxhtc3_config {
	struct i2c_dt_spec i2c;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_GXHTC3_GXHTC3_H_ */

