/*
 * Copyright (c) 2025 Sensirion
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_STCC4_STCC4_H_
#define ZEPHYR_DRIVERS_SENSOR_STCC4_STCC4_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#define STCC4_I2C_ADDR_64 0x64

typedef enum {
	STCC4_START_CONTINUOUS_MEASUREMENT_CMD_ID = 0x218b,
	STCC4_READ_MEASUREMENT_RAW_CMD_ID = 0xec05,
	STCC4_STOP_CONTINUOUS_MEASUREMENT_CMD_ID = 0x3f86,
	STCC4_MEASURE_SINGLE_SHOT_CMD_ID = 0x219d,
	STCC4_PERFORM_FORCED_RECALIBRATION_CMD_ID = 0x362f,
	STCC4_GET_PRODUCT_ID_CMD_ID = 0x365b,
	STCC4_SET_RHT_COMPENSATION_CMD_ID = 0xe000,
	STCC4_SET_PRESSURE_COMPENSATION_RAW_CMD_ID = 0xe016,
	STCC4_PERFORM_SELF_TEST_CMD_ID = 0x278c,
	STCC4_PERFORM_CONDITIONING_CMD_ID = 0x29bc,
	STCC4_ENTER_SLEEP_MODE_CMD_ID = 0x3650,
	STCC4_EXIT_SLEEP_MODE_CMD_ID = 0x0,
	STCC4_ENABLE_TESTING_MODE_CMD_ID = 0x3fbc,
	STCC4_DISABLE_TESTING_MODE_CMD_ID = 0x3f3d,
	STCC4_PERFORM_FACTORY_RESET_CMD_ID = 0x3632,
} STCC4_CMD_ID;

struct stcc4_config {
	struct i2c_dt_spec bus;
	uint32_t pressure;
	uint16_t humidity_compensation;
	uint16_t temperature_compensation;
	bool do_perform_conditioning;
};

struct stcc4_data {
	int16_t co2_concentration_raw;
	uint16_t temperature_raw;
	uint16_t relative_humidity_raw;
	uint16_t sensor_status_raw;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_STCC4_STCC4_H_*/
