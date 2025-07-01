/*
 * Copyright (c) 2025 Philipp Steiner <philipp.steiner1987@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr driver for LC709203F Battery Monitor
 *
 * This driver implements the sensor API for the LC709203F battery monitor,
 * providing battery voltage, state-of-charge (SOC), and temperature measurements.
 *
 * Note:
 * - The LC709203F is assumed to be connected via I2C.
 * - The register addresses and conversion factors used here are based on
 *   common LC709203F implementations. Consult your datasheet and adjust as needed.
 * - To use this driver, create a matching device tree node (with a "compatible"
 *   string, I2C bus, and register address) so that the DT_INST_* macros can pick it up.
 * - The LC chip works best when queried every few seconds at the fastest. Don't disconnect the LiPo
 *   battery, it is used to power the LC chip!
 */

#define DT_DRV_COMPAT onnn_lc709203f

#include "lc709203f.h"

#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lc709203f);

/* Battery temperature source */
enum lc709203f_temp_mode {
	LC709203F_TEMPERATURE_I2C = 0x0000,
	LC709203F_TEMPERATURE_THERMISTOR = 0x0001,
};

/* Chip power state */
enum lc709203f_power_mode {
	LC709203F_POWER_MODE_OPERATIONAL = 0x0001,
	LC709203F_POWER_MODE_SLEEP = 0x0002,
};

/* Current Direction Auto/Charge/Discharge mode */
enum lc709203f_current_direction {
	LC709203F_DIRECTION_AUTO = 0x0000,
	LC709203F_DIRECTION_CHARGE = 0x0001,
	LC709203F_DIRECTION_DISCHARGE = 0xFFFF,
};

/* Selects a battery profile */
enum lc709203f_battery_profile {
	LC709203F_BATTERY_PROFILE_0 = 0x0000,
	LC709203F_BATTERY_PROFILE_1 = 0x0001,
};

/* Approx battery pack size. Pick the closest of the following values for your battery size. */
enum lc709203f_battery_apa {
	LC709203F_APA_100MAH = 0x08,
	LC709203F_APA_200MAH = 0x0B,
	LC709203F_APA_500MAH = 0x10,
	LC709203F_APA_1000MAH = 0x19,
	LC709203F_APA_2000MAH = 0x2D,
	LC709203F_APA_3000MAH = 0x36,
};

struct lc709203f_config {
	struct i2c_dt_spec i2c;
	bool initial_rsoc;
	char *battery_apa;
	enum lc709203f_battery_profile battery_profile;
	bool thermistor;
	int thermistor_b_value;
	int thermistor_apt;
	enum lc709203f_temp_mode thermistor_mode;
};

#define LC709203F_INIT_RSOC_VAL  0xAA55 /* RSOC initialization value */
#define LC709203F_CRC_POLYNOMIAL 0x07   /* Polynomial to calculate CRC-8-ATM */

static int lc709203f_read_word(const struct device *dev, uint8_t reg, uint16_t *value);
static int lc709203f_write_word(const struct device *dev, uint8_t reg, uint16_t value);

static int lc709203f_get_alarm_low_rsoc(const struct device *dev, uint8_t *rsoc);
static int lc709203f_get_alarm_low_voltage(const struct device *dev, uint16_t *voltage);
static int lc709203f_get_apa(const struct device *dev, enum lc709203f_battery_apa *apa);
static int lc709203f_get_cell_temperature(const struct device *dev, uint16_t *temperature);
static int lc709203f_get_cell_voltage(const struct device *dev, uint16_t *voltage);
static int lc709203f_get_current_direction(const struct device *dev,
					   enum lc709203f_current_direction *direction);
static int lc709203f_get_power_mode(const struct device *dev, enum lc709203f_power_mode *mode);
static int lc709203f_get_rsoc(const struct device *dev, uint8_t *rsoc);

static int lc709203f_set_initial_rsoc(const struct device *dev);
static int lc709203f_set_alarm_low_rsoc(const struct device *dev, uint8_t rsoc);
static int lc709203f_set_alarm_low_voltage(const struct device *dev, uint16_t voltage);
static int lc709203f_set_apa(const struct device *dev, enum lc709203f_battery_apa apa);
static int lc709203f_set_apt(const struct device *dev, uint16_t apt);
static int lc709203f_set_battery_profile(const struct device *dev,
					 enum lc709203f_battery_profile profile);
static int lc709203f_set_current_direction(const struct device *dev,
					   enum lc709203f_current_direction direction);
static int lc709203f_set_power_mode(const struct device *dev, enum lc709203f_power_mode mode);
static int lc709203f_set_temp_mode(const struct device *dev, enum lc709203f_temp_mode mode);
static int lc709203f_set_thermistor_b(const struct device *dev, uint16_t value);

/*
 * Read a 16-bit register value (with CRC check).
 *
 * The LC709203F expects the following transaction:
 *   Write: [reg]
 *   Read:  [LSB, MSB, CRC]
 *
 * The CRC is computed over:
 *   [I2C_addr (write), reg, I2C_addr (read), LSB, MSB]
 */
static int lc709203f_read_word(const struct device *dev, uint8_t reg, uint16_t *value)
{
	const struct lc709203f_config *config = dev->config;
	uint8_t buf[3];
	int ret;

	ret = i2c_write_read_dt(&config->i2c, &reg, sizeof(reg), buf, sizeof(buf));
	if (ret) {
		LOG_ERR("i2c_write_read failed (reg 0x%02x): %d", reg, ret);
		return ret;
	}

	uint8_t crc_buf[5];

	/* Build buffer for CRC calculation */
	crc_buf[0] = config->i2c.addr << 1;
	crc_buf[1] = reg;
	crc_buf[2] = (config->i2c.addr << 1) | 0x01;
	crc_buf[3] = buf[0]; /* LSB */
	crc_buf[4] = buf[1]; /* MSB */

	uint8_t crc = crc8(crc_buf, sizeof(crc_buf), LC709203F_CRC_POLYNOMIAL, 0, false);

	if (crc != buf[2]) {
		LOG_ERR("CRC mismatch on reg 0x%02x", reg);
		return -EIO;
	}

	if (value) {
		*value = sys_get_le16(buf); /* LSB, MSB */
	}

	return 0;
}

/*
 * Write a 16-bit word to a register (with CRC appended).
 *
 * The transaction is:
 *   Write: [reg, LSB, MSB, CRC]
 *
 * The CRC is computed over:
 *   [I2C_addr (write), reg, LSB, MSB]
 */
static int lc709203f_write_word(const struct device *dev, uint8_t reg, uint16_t value)
{
	const struct lc709203f_config *config = dev->config;
	uint8_t crc_buf[4];
	uint8_t write_buf[4];

	crc_buf[0] = config->i2c.addr << 1;
	crc_buf[1] = reg;
	sys_put_le16(value, &crc_buf[2]); /* LSB, MSB */

	write_buf[0] = reg;
	sys_put_le16(value, &write_buf[1]); /* LSB, MSB */
	write_buf[3] = crc8(crc_buf, sizeof(crc_buf), LC709203F_CRC_POLYNOMIAL, 0, false);

	return i2c_write_dt(&config->i2c, write_buf, sizeof(write_buf));
}

static int lc709203f_get_alarm_low_rsoc(const struct device *dev, uint8_t *rsoc)
{
	uint16_t tmp;
	int ret;

	if (!dev || !rsoc) {
		return -EINVAL;
	}

	ret = lc709203f_read_word(dev, LC709203F_REG_ALARM_LOW_RSOC, &tmp);
	if (ret) {
		return ret;
	}

	*rsoc = (uint8_t)tmp;
	return 0;
}

static int lc709203f_get_alarm_low_voltage(const struct device *dev, uint16_t *voltage)
{
	if (!dev || !voltage) {
		return -EINVAL;
	}
	return lc709203f_read_word(dev, LC709203F_REG_ALARM_LOW_VOLTAGE, voltage);
}

static int lc709203f_get_apa(const struct device *dev, enum lc709203f_battery_apa *apa)
{
	uint16_t tmp;
	int ret;

	if (!dev || !apa) {
		return -EINVAL;
	}

	ret = lc709203f_read_word(dev, LC709203F_REG_APA, &tmp);
	if (ret) {
		return ret;
	}

	*apa = (enum lc709203f_battery_apa)tmp;
	return 0;
}

static int lc709203f_get_cell_temperature(const struct device *dev, uint16_t *temperature)
{
	if (!dev || !temperature) {
		return -EINVAL;
	}
	return lc709203f_read_word(dev, LC709203F_REG_CELL_TEMPERATURE, temperature);
}

static int lc709203f_get_cell_voltage(const struct device *dev, uint16_t *voltage)
{
	if (!dev || !voltage) {
		return -EINVAL;
	}
	return lc709203f_read_word(dev, LC709203F_REG_CELL_VOLTAGE, voltage);
}

static int lc709203f_get_current_direction(const struct device *dev,
					   enum lc709203f_current_direction *direction)
{
	uint16_t tmp;
	int ret;

	if (!dev || !direction) {
		return -EINVAL;
	}

	ret = lc709203f_read_word(dev, LC709203F_REG_CURRENT_DIRECTION, &tmp);
	if (ret) {
		return ret;
	}

	*direction = (enum lc709203f_current_direction)tmp;
	return 0;
}

static int lc709203f_get_power_mode(const struct device *dev, enum lc709203f_power_mode *mode)
{
	uint16_t tmp;
	int ret;

	if (!dev || !mode) {
		return -EINVAL;
	}

	ret = lc709203f_read_word(dev, LC709203F_REG_IC_POWER_MODE, &tmp);
	if (ret) {
		return ret;
	}

	*mode = (enum lc709203f_power_mode)tmp;
	return 0;
}

static int lc709203f_get_rsoc(const struct device *dev, uint8_t *rsoc)
{
	uint16_t tmp;
	int ret;

	if (!dev || !rsoc) {
		return -EINVAL;
	}

	ret = lc709203f_read_word(dev, LC709203F_REG_RSOC, &tmp);
	if (ret) {
		return ret;
	}

	*rsoc = (uint8_t)tmp;
	return 0;
}

static int lc709203f_set_initial_rsoc(const struct device *dev)
{
	if (!dev) {
		return -EINVAL;
	}
	return lc709203f_write_word(dev, LC709203F_REG_INITIAL_RSOC, LC709203F_INIT_RSOC_VAL);
}

static int lc709203f_set_alarm_low_rsoc(const struct device *dev, uint8_t rsoc)
{
	if (!dev) {
		return -EINVAL;
	}
	if (rsoc > 100) {
		return -EINVAL;
	}
	return lc709203f_write_word(dev, LC709203F_REG_ALARM_LOW_RSOC, rsoc);
}

static int lc709203f_set_alarm_low_voltage(const struct device *dev, uint16_t voltage)
{
	if (!dev) {
		return -EINVAL;
	}
	return lc709203f_write_word(dev, LC709203F_REG_ALARM_LOW_VOLTAGE, voltage);
}

static int lc709203f_set_apa(const struct device *dev, enum lc709203f_battery_apa apa)
{
	if (!dev) {
		return -EINVAL;
	}
	return lc709203f_write_word(dev, LC709203F_REG_APA, (uint16_t)apa);
}

static int lc709203f_set_apt(const struct device *dev, uint16_t apt)
{
	if (!dev) {
		return -EINVAL;
	}
	return lc709203f_write_word(dev, LC709203F_REG_APT, apt);
}

static int lc709203f_set_battery_profile(const struct device *dev,
					 enum lc709203f_battery_profile profile)
{
	if (!dev) {
		return -EINVAL;
	}
	return lc709203f_write_word(dev, LC709203F_REG_BAT_PROFILE, (uint16_t)profile);
}

static int lc709203f_set_current_direction(const struct device *dev,
					   enum lc709203f_current_direction direction)
{
	if (!dev) {
		return -EINVAL;
	}
	return lc709203f_write_word(dev, LC709203F_REG_CURRENT_DIRECTION, (uint16_t)direction);
}

static int lc709203f_set_power_mode(const struct device *dev, enum lc709203f_power_mode mode)
{
	if (!dev) {
		return -EINVAL;
	}
	return lc709203f_write_word(dev, LC709203F_REG_IC_POWER_MODE, (uint16_t)mode);
}

static int lc709203f_set_temp_mode(const struct device *dev, enum lc709203f_temp_mode mode)
{
	if (!dev) {
		return -EINVAL;
	}
	return lc709203f_write_word(dev, LC709203F_REG_STATUS_BIT, (uint16_t)mode);
}

static int lc709203f_set_thermistor_b(const struct device *dev, uint16_t value)
{
	if (!dev) {
		return -EINVAL;
	}
	return lc709203f_write_word(dev, LC709203F_REG_THERMISTOR_B, value);
}

enum lc709203f_battery_apa lc709203f_string_to_apa(const char *apa_string)
{
	static const char *const apas[] = {"100mAh",  "200mAh",  "500mAh",
					   "1000mAh", "2000mAh", "3000mAh"};

	static const enum lc709203f_battery_apa apa_values[] = {
		LC709203F_APA_100MAH,  LC709203F_APA_200MAH,  LC709203F_APA_500MAH,
		LC709203F_APA_1000MAH, LC709203F_APA_2000MAH, LC709203F_APA_3000MAH};

	/* Check if the string is NULL or empty */
	for (size_t i = 0; i < ARRAY_SIZE(apas); i++) {
		if (strncmp(apa_string, apas[i], strlen(apas[i])) == 0) {
			return apa_values[i];
		}
	}
	LOG_ERR("Invalid apa_string: %s, returning default: %d", apa_string, LC709203F_APA_100MAH);
	return LC709203F_APA_100MAH;
}

enum lc709203f_power_mode lc709203f_num_to_power_mode(uint16_t num)
{
	switch (num) {
	case 1:
		return LC709203F_POWER_MODE_OPERATIONAL;
	case 2:
		return LC709203F_POWER_MODE_SLEEP;
	default:
		LOG_ERR("Invalid power mode: %d", num);
		return LC709203F_POWER_MODE_OPERATIONAL;
	}
}

enum lc709203f_current_direction lc709203f_num_to_current_direction(uint16_t num)
{
	switch (num) {
	case 0:
		return LC709203F_DIRECTION_AUTO;
	case 1:
		return LC709203F_DIRECTION_CHARGE;
	case 0xFFFF:
		return LC709203F_DIRECTION_DISCHARGE;
	default:
		LOG_ERR("Invalid current direction: %d", num);
		return LC709203F_DIRECTION_AUTO;
	}
}

/*
 * Device initialization function.
 */
static int lc709203f_init(const struct device *dev)
{
	const struct lc709203f_config *config = dev->config;
	int ret = 0;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	enum lc709203f_power_mode mode = LC709203F_POWER_MODE_OPERATIONAL;

	LOG_DBG("Get power mode");
	ret = lc709203f_get_power_mode(dev, &mode);
	if (ret) {
		LOG_ERR("Failed to get power mode: %d", ret);
	}

	LOG_DBG("Power mode: %d", mode);
	if (mode == LC709203F_POWER_MODE_SLEEP) {
		LOG_DBG("Set Power mode");
		ret = lc709203f_set_power_mode(dev, LC709203F_POWER_MODE_OPERATIONAL);

		if (ret) {
			LOG_ERR("Failed to set power mode: %d", ret);
		}
	}

	LOG_DBG("Set battery pack: %s", config->battery_apa);
	ret = lc709203f_set_apa(dev, lc709203f_string_to_apa(config->battery_apa));

	if (ret) {
		LOG_ERR("Failed to set battery pack: %d", ret);
	}

	LOG_DBG("Set battery profile: %d", config->battery_profile);
	ret = lc709203f_set_battery_profile(dev, config->battery_profile);

	if (ret) {
		LOG_ERR("Failed to set battery profile: %d", ret);
	}

	if (config->thermistor) {
		LOG_DBG("Set temperature mode: %d", config->thermistor_mode);
		lc709203f_set_temp_mode(dev, config->thermistor_mode);
		if (ret) {
			LOG_ERR("Failed to set temperature mode: %d", ret);
		}

		LOG_DBG("Set thermistor B value: %d", config->thermistor_b_value);
		ret = lc709203f_set_thermistor_b(dev, config->thermistor_b_value);

		if (ret) {
			LOG_ERR("Failed to set thermistor B value: %d", ret);
		}

		LOG_DBG("Set thermistor APT: %d", config->thermistor_apt);
		ret = lc709203f_set_apt(dev, config->thermistor_apt);

		if (ret) {
			LOG_ERR("Failed to set thermistor APT: %d", ret);
		}
	}

	if (config->initial_rsoc) {
		LOG_DBG("lc709203f_set_initial_rsoc");
		ret = lc709203f_set_initial_rsoc(dev);

		if (ret) {
			LOG_ERR("Quickstart failed: %d", ret);
			return ret;
		}
	}

	LOG_DBG("initialized");
	return 0;
}

static int lc709203f_get_prop(const struct device *dev, fuel_gauge_prop_t prop,
			      union fuel_gauge_prop_val *val)
{
	int rc = 0;
	uint16_t tmp_val = 0;
	const struct lc709203f_config *config = dev->config;

	switch (prop) {
	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE:
		rc = lc709203f_get_rsoc(dev, &val->relative_state_of_charge);
		break;
	case FUEL_GAUGE_TEMPERATURE:
		if (!config->thermistor) {
			LOG_ERR("Thermistor not enabled");
			return -ENOTSUP;
		}
		rc = lc709203f_get_cell_temperature(dev, &val->temperature);
		break;
	case FUEL_GAUGE_VOLTAGE:
		rc = lc709203f_get_cell_voltage(dev, &tmp_val);
		val->voltage = tmp_val * 1000;
		break;
	case FUEL_GAUGE_SBS_MODE:
		rc = lc709203f_get_power_mode(dev, (enum lc709203f_power_mode *)&val->sbs_mode);
		break;
	case FUEL_GAUGE_DESIGN_CAPACITY: {
		enum lc709203f_battery_apa apa = LC709203F_APA_100MAH;

		rc = lc709203f_get_apa(dev, &apa);

		switch (apa) {
		case LC709203F_APA_100MAH:
			val->design_cap = 100;
			break;
		case LC709203F_APA_200MAH:
			val->design_cap = 200;
			break;
		case LC709203F_APA_500MAH:
			val->design_cap = 500;
			break;
		case LC709203F_APA_1000MAH:
			val->design_cap = 1000;
			break;
		case LC709203F_APA_2000MAH:
			val->design_cap = 2000;
			break;
		case LC709203F_APA_3000MAH:
			val->design_cap = 3000;
			break;
		default:
			LOG_ERR("Invalid battery capacity: %d", apa);
			return -EINVAL;
		}
		break;
	}
	case FUEL_GAUGE_CURRENT_DIRECTION:
		rc = lc709203f_get_current_direction(
			dev, (enum lc709203f_current_direction *)&val->current_direction);
		break;
	case FUEL_GAUGE_STATE_OF_CHARGE_ALARM:
		rc = lc709203f_get_alarm_low_rsoc(dev, &val->state_of_charge_alarm);
		break;
	case FUEL_GAUGE_LOW_VOLTAGE_ALARM:
		rc = lc709203f_get_alarm_low_voltage(dev, &tmp_val);
		val->low_voltage_alarm = tmp_val * 1000;
		break;
	default:
		rc = -ENOTSUP;
		break;
	}

	return rc;
}

static int lc709203f_set_prop(const struct device *dev, fuel_gauge_prop_t prop,
			      union fuel_gauge_prop_val val)
{
	int rc = 0;

	switch (prop) {
	case FUEL_GAUGE_SBS_MODE:
		rc = lc709203f_set_power_mode(dev, lc709203f_num_to_power_mode(val.sbs_mode));
		break;
	case FUEL_GAUGE_CURRENT_DIRECTION:
		rc = lc709203f_set_current_direction(
			dev, lc709203f_num_to_current_direction(val.current_direction));
		break;
	case FUEL_GAUGE_STATE_OF_CHARGE_ALARM:
		rc = lc709203f_set_alarm_low_rsoc(dev, val.state_of_charge_alarm);
		break;
	case FUEL_GAUGE_LOW_VOLTAGE_ALARM:
		rc = lc709203f_set_alarm_low_voltage(dev, val.low_voltage_alarm / 1000);
		break;
	default:
		rc = -ENOTSUP;
		break;
	}

	return rc;
}

static DEVICE_API(fuel_gauge, lc709203f_driver_api) = {
	.get_property = &lc709203f_get_prop,
	.set_property = &lc709203f_set_prop,
};

#define LC709203F_INIT(inst)                                                                       \
                                                                                                   \
	static const struct lc709203f_config lc709203f_config_##inst = {                           \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.initial_rsoc = DT_INST_PROP(inst, initial_rsoc),                                  \
		.battery_apa = DT_INST_PROP(inst, apa),                                            \
		.battery_profile = DT_INST_PROP(inst, battery_profile),                            \
		.thermistor = DT_INST_PROP(inst, thermistor),                                      \
		.thermistor_b_value = DT_INST_PROP(inst, thermistor_b_value),                      \
		.thermistor_apt = DT_INST_PROP(inst, apt),                                         \
		.thermistor_mode = DT_INST_PROP(inst, thermistor_mode),                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &lc709203f_init, NULL, NULL, &lc709203f_config_##inst,         \
			      POST_KERNEL, CONFIG_FUEL_GAUGE_INIT_PRIORITY,                        \
			      &lc709203f_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LC709203F_INIT)
