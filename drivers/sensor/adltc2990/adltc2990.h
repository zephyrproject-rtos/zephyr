/*
 * SPDX-FileCopyrightText: Copyright (c) 2023 Carl Zeiss Meditec AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADLTC2990_H
#define ZEPHYR_DRIVERS_SENSOR_ADLTC2990_H

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

enum adltc2990_monitor_pins {
	V1,
	V2,
	V3,
	V4,
	INTERNAL_TEMPERATURE,
	SUPPLY_VOLTAGE
} adltc2990_monitor_pins;

enum adltc2990_monitoring_type {
	NOTHING,
	VOLTAGE_DIFFERENTIAL,
	VOLTAGE_SINGLEENDED,
	TEMPERATURE
} adltc2990_monitoring_type;

union voltage_divider_resistors {
	struct {
		uint32_t v1_r1_r2[2];
		uint32_t v2_r1_r2[2];
	};
	struct {
		uint32_t v3_r1_r2[2];
		uint32_t v4_r1_r2[2];
	};
};

struct pins_configuration {
	uint32_t pins_current_resistor;
	union voltage_divider_resistors voltage_divider_resistors;
};

struct adltc2990_data {
	int32_t internal_temperature;
	int32_t supply_voltage;
	int32_t pins_v1_v2_values[2];
	int32_t pins_v3_v4_values[2];
};

struct adltc2990_config {
	struct i2c_dt_spec bus;
	uint8_t temp_format;
	uint8_t acq_format;
	uint8_t measurement_mode[2];
	struct pins_configuration pins_v1_v2;
	struct pins_configuration pins_v3_v4;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_ADLTC2990_H */
