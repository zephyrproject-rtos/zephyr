/*
 * Copyright (c) 2025 RIT Launch Initiative <launchinitiative@rit.edu>
 * Copyright (c) 2025 Aaron Chan
 * Copyright (c) 2025 Richard Sommers
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_INA260_INA260_H_
#define ZEPHYR_DRIVERS_SENSOR_INA260_INA260_H_
#include <zephyr/drivers/i2c.h>

#define INA260_VOLTS_PER_LSB (0.00125)
#define INA260_AMPS_PER_LSB  (0.00125)
#define INA260_WATTS_PER_LSB (0.01)

/* Device register addresses */
#define INA260_REG_CONF    0x00
#define INA260_REG_CURRENT 0x01
#define INA260_REG_V_BUS   0x02
#define INA260_REG_POWER   0x03
#define INA260_REG_MASK    0x06
#define INA260_REG_ALERT   0x07
#define INA260_REG_MAN_ID  0xfe
#define INA260_REG_DIE_ID  0xff

/* Config register shifts and masks */
#define INA260_RST                    BIT(15)
#define INA260_CONF_REQUIRED_TOP_BITS 0b0110

/* Mode selection */
enum ina260_mode {
	TRIG_OFF = 0b000,
	TRIG_CURRENT = 0b001,
	TRIG_VOLTAGE = 0b010,
	TRIG_BOTH = 0b011,
	CONT_OFF = 0b100,
	CONT_CURRENT = 0b101,
	CONT_VOLTAGE = 0b110,
	CONT_BOTH = 0b111,
};

enum average_samples {
	AVG_1 = 0b000,
	AVG_4 = 0b001,
	AVG_16 = 0b010,
	AVG_64 = 0b011,
	AVG_128 = 0b100,
	AVG_256 = 0b101,
	AVG_512 = 0b110,
	AVG_1024 = 0b111,
};

enum conv_time {
	CONV_TIME_140US = 0b000,
	CONV_TIME_204US = 0b001,
	CONV_TIME_332US = 0b010,
	CONV_TIME_588US = 0b011,
	CONV_TIME_1100US = 0b100,
	CONV_TIME_2116US = 0b101,
	CONV_TIME_4156US = 0b110,
	CONV_TIME_8244US = 0b111,
};

struct ina260_config {
	struct i2c_dt_spec bus;
	uint16_t average;           /* Number of samples to average */
	uint16_t voltage_conv_time; /* Voltage conversion time in microseconds */
	uint16_t current_conv_time; /* Current conversion time in microseconds */
	enum ina260_mode mode;
};

struct ina260_data {
	uint16_t v_bus;
	uint16_t power;
	uint16_t current;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_INA260_INA260_H_ */
