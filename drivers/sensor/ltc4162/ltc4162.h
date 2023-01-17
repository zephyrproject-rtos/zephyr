/*
 * Copyright (c) 2021 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LTC4162_LTC4162_H_
#define ZEPHYR_DRIVERS_SENSOR_LTC4162_LTC4162_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <stdbool.h>

#define LTC4162_IIN_LIMIT_TARGET		0x15
#define LTC4162_CHARGE_CURRENT_SETTING		0x1A
#define LTC4162_VCHARGE_SETTING			0x1B
#define LTC4162_CHARGER_STATE			0x34
#define LTC4162_CHARGE_STATUS			0x35
#define LTC4162_VBAT				0x3A
#define LTC4162_VIN				0x3B
#define LTC4162_IBAT				0x3D
#define LTC4162_IIN				0x3E
#define LTC4162_DIE_TEMPERATURE			0x3F
#define LTC4162_ICHARGE_DAC			0x44
#define LTC4162_VCHARGE_DAC			0x45
#define LTC4162_IIN_LIMIT_DAC			0x46

#define DIE_TEMP_LSB_SIZE			215
#define CENTIDEGREES_SCALE			100
#define DIE_TEMP_OFFSET				26440

#define CHRG_VOLTAGE_OFFSET			0.02857

#define ONE_MILLI_VOLT_CONSTANT			0.001
#define MAX_CHRG_CURRENT_SERVO_LEVEL		31
#define MAX_CHRG_VOLTAGE_SERVO_LEVEL		63

/* Enumeration as in datasheet. Individual bits are mutually exclusive */
enum ltc4162_state {
	ABSORB_CHARGE = BIT(9),
	CHARGER_SUSPENDED = BIT(8),
	CC_CV_CHARGE = BIT(6),
	BAT_MISSING_FAULT = BIT(1),
	BAT_SHORT_FAULT = BIT(0),
};

/* Individual Bits are mutually exclusive */
enum ltc4162_charge_status {
	IIN_LIMIT_ACTIVE = BIT(2),
};

struct ltc4162_data {
	uint8_t cell_count;
	uint16_t const_volt;
	uint32_t rsnsb;		/* series resistor that sets charge current, microOhm */
	uint32_t rsnsi;		/* series resistor to measure input current, microOhm */
	uint16_t in_voltage;
	uint16_t in_current;
	uint16_t charger_temp;
	uint16_t const_current;
	uint32_t const_voltage;
	uint32_t in_current_limit;
	uint32_t const_current_max;
	uint32_t const_voltage_max;
	enum charge_type chrg_type;
	enum charger_status chrg_status;
	enum charger_health chrg_health;
};

struct ltc4162_config {
	struct i2c_dt_spec bus;
	int16_t rsnsb;
	int16_t rsnsi;
	uint8_t cell_count;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_LTC4162_LTC4162_H_ */
