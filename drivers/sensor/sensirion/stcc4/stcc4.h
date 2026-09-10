/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * The STCC4 is a miniature CO2 sensor based on thermal conductivity.
 * It communicates via I2C (default address 0x64) and provides CO2 (ppm),
 * temperature, and humidity readings.
 *
 * Data is exchanged using Sensirion's standard word+CRC format:
 * each word is 2 data bytes (big-endian) followed by 1 CRC-8 byte.
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_STCC4_STCC4_H_
#define ZEPHYR_DRIVERS_SENSOR_STCC4_STCC4_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/dt-bindings/sensor/stcc4.h>

/* Sensirion standard CRC-8 parameters */
#define STCC4_CRC_POLY 0x31U
#define STCC4_CRC_INIT 0xFFU

/*
 * Sensirion I2C word format: 2 data bytes (big-endian) + 1 CRC-8 byte.
 * A measurement response contains 4 words (CO2, temperature, humidity, status).
 */
#define STCC4_WORD_SIZE         3U
#define STCC4_MEASUREMENT_WORDS 4U
#define STCC4_RX_BUF_LEN        (STCC4_MEASUREMENT_WORDS * STCC4_WORD_SIZE)

/* EXIT_SLEEP wakeup byte (datasheet §5.3.4.9) */
#define STCC4_WAKEUP_BYTE 0x00U

/* Command table indices */
#define STCC4_CMD_START_CONTINUOUS     0U
#define STCC4_CMD_READ_MEASUREMENT_RAW 1U
#define STCC4_CMD_STOP_CONTINUOUS      2U
#define STCC4_CMD_MEASURE_SINGLE_SHOT  3U
#define STCC4_CMD_ENTER_SLEEP          4U
#define STCC4_CMD_EXIT_SLEEP           5U
#define STCC4_CMD_SELF_TEST            6U
#define STCC4_CMD_GET_PRODUCT_ID       7U

/* Temperature conversion: T[C] = MIN_TEMP + MAX_TEMP * (raw / 65535) */
#define STCC4_MAX_TEMP 175
#define STCC4_MIN_TEMP (-45)

/* Humidity conversion: RH[%] = MIN_HUMI + MAX_HUMI * (raw / 65535) */
#define STCC4_MAX_HUMI 125
#define STCC4_MIN_HUMI (-6)

enum stcc4_mode {
	STCC4_MODE_CONTINUOUS = STCC4_DT_MODE_CONTINUOUS,
	STCC4_MODE_SINGLE_SHOT = STCC4_DT_MODE_SINGLE_SHOT,
};

struct stcc4_config {
	struct i2c_dt_spec bus;
	enum stcc4_mode mode;
};

struct stcc4_data {
	int16_t co2_sample;   /* CO2 concentration in ppm (signed per datasheet) */
	uint16_t temp_sample; /* Raw temperature value */
	uint16_t humi_sample; /* Raw humidity value */
};

struct stcc4_cmd {
	uint16_t code;        /* 16-bit I2C command code (big-endian on wire) */
	uint16_t duration_ms; /* Post-command delay before sensor is ready */
};

/**
 * Command lookup table indexed by STCC4_CMD_* defines.
 * EXIT_SLEEP is special: only the LSB (0x00) is sent as an 8-bit command.
 */
static const struct stcc4_cmd stcc4_cmds[] = {
	[STCC4_CMD_START_CONTINUOUS] = {0x218BU, 0U},
	[STCC4_CMD_READ_MEASUREMENT_RAW] = {0xEC05U, 1U},
	[STCC4_CMD_STOP_CONTINUOUS] = {0x3F86U, 1200U},
	[STCC4_CMD_MEASURE_SINGLE_SHOT] = {0x219DU, 500U},
	[STCC4_CMD_ENTER_SLEEP] = {0x3650U, 1U},
	[STCC4_CMD_EXIT_SLEEP] = {0x0000U, 5U},
	[STCC4_CMD_SELF_TEST] = {0x278CU, 360U},
	[STCC4_CMD_GET_PRODUCT_ID] = {0x365BU, 1U},
};

#endif /* ZEPHYR_DRIVERS_SENSOR_STCC4_STCC4_H_ */
