/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_STC31_STC31_H_
#define ZEPHYR_DRIVERS_SENSOR_STC31_STC31_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/dt-bindings/sensor/stc31.h>

#define STC31_CMD_SET_BINARY_GAS            0x3615
#define STC31_CMD_SET_RELATIVE_HUMIDITY     0x3624
#define STC31_CMD_SET_TEMPERATURE           0x361E
#define STC31_CMD_SET_PRESSURE              0x362F
#define STC31_CMD_MEASURE_GAS_CONCENTRATION 0x3639
#define STC31_CMD_FORCED_RECALIBRATION      0x3661
#define STC31_CMD_READ_PRODUCT_ID_1         0x367C
#define STC31_CMD_READ_PRODUCT_ID_2         0xE102

/* I2C general call soft reset (address 0x00, 8-bit command) */
#define STC31_GENERAL_CALL_ADDR 0x00
#define STC31_SOFT_RESET_CMD    0x06

#define STC31_CRC_POLY 0x31
#define STC31_CRC_INIT 0xFF

#define STC31_WORD_SIZE 3

/* Datasheet timings */
#define STC31_POWER_UP_TIME_MS              14
#define STC31_SOFT_RESET_TIME_MS            12
#define STC31_MEASURE_DURATION_LN_MS        66
#define STC31_MEASURE_DURATION_LOW_XSENS_MS 110

/* Product number STC31 (revision nibble ignored in check) */
#define STC31_PRODUCT_NUMBER 0x08010300
#define STC31_PRODUCT_MASK   0xFFFFFF00

struct stc31_config {
	struct i2c_dt_spec bus;
	uint16_t binary_gas;
};

struct stc31_data {
	float co2_ppm;
	float temp_c;
	uint16_t binary_gas;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_STC31_STC31_H_ */
