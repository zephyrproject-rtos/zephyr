/*
 * Copyright (c) 2021 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LTC4162_LTC4162_H_
#define ZEPHYR_DRIVERS_SENSOR_LTC4162_LTC4162_H_

#include <device.h>
#include <sys/util.h>
#include <zephyr/types.h>
#include <drivers/sensor.h>

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

/* Enumeration as in datasheet. Individual bits are mutually exclusive */
enum ltc4162_state {
	bat_detect_failed_fault = 4096,
	battery_detection = 2048,
	equalize_charge = 1024,
	absorb_charge = 512,
	charger_suspended = 256,
	cc_cv_charge = 64, /* normal charge */
	bat_missing_fault = 2,
	bat_short_fault = 1
};

/* Individual Bits are mutually exclusive */
enum ltc4162_charge_status {
	ilim_reg_active = 32,
	thermal_reg_active = 16,
	vin_uvcl_active = 8,
	iin_limit_active = 4,
	constant_current = 2,
	constant_voltage = 1,
	charger_off = 0
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
	int16_t rsnsb;
	int16_t rsnsi;
	uint16_t i2c_addr;
	uint8_t cell_count;
	const struct device *i2c_dev;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_LTC4162_LTC4162_H_ */
