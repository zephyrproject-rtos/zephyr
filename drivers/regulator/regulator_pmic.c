/*
 * Copyright (c) 2021 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief PMIC Regulator Driver
 * This driver implements the regulator API within Zephyr, and additionally
 * implements support for a broader API. Most consumers will want to use
 * the API provided in drivers/regulator/consumer.h to manipulate the voltage
 * levels of the regulator device.
 * manipulate.
 */

#define DT_DRV_COMPAT regulator_pmic

#include <zephyr/kernel.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/regulator/consumer.h>
#include <zephyr/drivers/i2c.h>
#include <errno.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pmic_regulator, CONFIG_REGULATOR_LOG_LEVEL);

struct __packed voltage_range {
	int uV; /* Voltage in uV */
	int reg_val; /* Register value for voltage */
};

struct __packed current_range {
	int uA; /* Current limit in uA */
	int reg_val; /* Register value for current limit */
};

struct regulator_data {
	struct onoff_sync_service srv;
	const struct voltage_range *voltages;
	const struct current_range *current_levels;
};

struct regulator_config {
	int num_voltages;
	int num_current_levels;
	uint8_t vsel_reg;
	uint8_t vsel_mask;
	uint32_t max_uV;
	uint32_t min_uV;
	uint8_t enable_reg;
	uint8_t enable_mask;
	uint8_t enable_val;
	bool enable_inverted;
	uint8_t ilim_reg;
	uint8_t ilim_mask;
	struct i2c_dt_spec i2c;
	uint32_t *voltage_array;
	uint32_t *current_array;
};

/**
 * Modifies a register within the PMIC
 * Returns 0 on success, or errno on error
 */
static int regulator_modify_register(const struct regulator_config *conf,
		uint8_t reg, uint8_t reg_mask, uint8_t reg_val)
{
	uint8_t reg_current;
	int rc;

	rc = i2c_reg_read_byte_dt(&conf->i2c, reg, &reg_current);
	if (rc) {
		return rc;
	}
	reg_current &= ~reg_mask;
	reg_current |= reg_val;
	LOG_DBG("Writing 0x%02X to reg 0x%02X at I2C addr 0x%02X", reg_current, reg,
		conf->i2c.addr);
	return i2c_reg_write_byte_dt(&conf->i2c, reg, reg_current);
}


/**
 * Part of the extended regulator consumer API
 * Returns the number of supported voltages
 */
int regulator_count_voltages(const struct device *dev)
{
	const struct regulator_config *config = dev->config;

	return config->num_voltages;
}

/**
 * Part of the extended regulator consumer API
 * Returns the supported voltage in uV for a given selector value
 */
int regulator_list_voltages(const struct device *dev, unsigned int selector)
{
	const struct regulator_config *config = dev->config;
	struct regulator_data *data = dev->data;

	if (config->num_voltages <= selector) {
		return -ENODEV;
	}
	return data->voltages[selector].uV;
}

/**
 * Part of the extended regulator consumer API
 * Returns true if the regulator supports a voltage in the given range.
 */
int regulator_is_supported_voltage(const struct device *dev,
		int min_uV, int max_uV)
{
	const struct regulator_config *config = dev->config;

	return !((config->max_uV < min_uV) || (config->min_uV > max_uV));
}

/**
 * Part of the extended regulator consumer API
 * Sets the output voltage to the closest supported voltage value
 */
int regulator_set_voltage(const struct device *dev, int min_uV, int max_uV)
{
	const struct regulator_config *config = dev->config;
	struct regulator_data *data = dev->data;
	int i = 0;

	if (!regulator_is_supported_voltage(dev, min_uV, max_uV) ||
		min_uV > max_uV) {
		return -EINVAL;
	}
	/* Find closest supported voltage */
	while (i < config->num_voltages && min_uV > data->voltages[i].uV) {
		i++;
	}
	if (data->voltages[i].uV > max_uV) {
		LOG_DBG("Regulator could not satisfy voltage range, too narrow");
		return -EINVAL;
	}
	if (i == config->num_voltages) {
		LOG_WRN("Regulator could not locate supported voltage,"
				"but voltage is in range.");
		return -EINVAL;
	}
	LOG_DBG("Setting regulator %s to %duV", dev->name,
			data->voltages[i].uV);
	return regulator_modify_register(config, config->vsel_reg,
			config->vsel_mask, data->voltages[i].reg_val);
}

/**
 * Part of the extended regulator consumer API
 * Gets the current output voltage in uV
 */
int regulator_get_voltage(const struct device *dev)
{
	const struct regulator_config *config = dev->config;
	struct regulator_data *data = dev->data;
	int rc, i = 0;
	uint8_t raw_reg;

	rc = i2c_reg_read_byte_dt(&config->i2c, config->vsel_reg, &raw_reg);
	if (rc) {
		return rc;
	}
	raw_reg &= config->vsel_mask;
	/* Locate the voltage value in the voltage table */
	while (i < config->num_voltages &&
		raw_reg != data->voltages[i].reg_val){
		i++;
	}
	if (i == config->num_voltages) {
		LOG_WRN("Regulator vsel reg has unknown value");
		return -EIO;
	}
	return data->voltages[i].uV;
}

/**
 * Part of the extended regulator consumer API
 * Set the current limit for this device
 */
int regulator_set_current_limit(const struct device *dev, int min_uA, int max_uA)
{
	const struct regulator_config *config = dev->config;
	struct regulator_data *data = dev->data;
	int i = 0;

	if (config->num_current_levels == 0) {
		/* Regulator cannot limit current */
		return -ENOTSUP;
	}
	/* Locate the desired current limit */
	while (i < config->num_current_levels &&
		min_uA > data->current_levels[i].uA) {
		i++;
	}
	if (i == config->num_current_levels ||
		data->current_levels[i].uA > max_uA) {
		return -EINVAL;
	}
	/* Set the current limit */
	return regulator_modify_register(config, config->ilim_reg,
		config->ilim_mask, data->current_levels[i].reg_val);
}

/**
 * Part of the extended regulator consumer API
 * Gets the set current limit for the regulator
 */
int regulator_get_current_limit(const struct device *dev)
{
	const struct regulator_config *config = dev->config;
	struct regulator_data *data = dev->data;
	int rc, i = 0;
	uint8_t raw_reg;

	if (config->num_current_levels == 0) {
		return -ENOTSUP;
	}
	rc = i2c_reg_read_byte_dt(&config->i2c, config->ilim_reg, &raw_reg);
	if (rc) {
		return rc;
	}
	raw_reg &= config->ilim_mask;
	while (i < config->num_current_levels &&
		data->current_levels[i].reg_val != raw_reg) {
		i++;
	}
	if (i == config->num_current_levels) {
		return -EIO;
	}
	return data->current_levels[i].uA;
}


static int enable_regulator(const struct device *dev, struct onoff_client *cli)
{
	k_spinlock_key_t key;
	int rc;
	uint8_t en_val;
	struct regulator_data *data = dev->data;
	const struct regulator_config *config = dev->config;

	LOG_DBG("Enabling regulator");
	rc = onoff_sync_lock(&data->srv, &key);
	if (rc) {
		/* Request has already enabled PMIC */
		return onoff_sync_finalize(&data->srv, key, cli, rc, true);
	}
	en_val = config->enable_inverted ? 0 : config->enable_val;
	rc = regulator_modify_register(config, config->enable_reg,
			config->enable_mask, en_val);
	if (rc) {
		return onoff_sync_finalize(&data->srv, key, NULL, rc, false);
	}
	return onoff_sync_finalize(&data->srv, key, cli, rc, true);
}

static int disable_regulator(const struct device *dev)
{
	struct regulator_data *data = dev->data;
	const struct regulator_config *config = dev->config;
	k_spinlock_key_t key;
	uint8_t dis_val;
	int rc;

	LOG_DBG("Disabling regulator");
	rc = onoff_sync_lock(&data->srv, &key);
	if (rc == 0) {
		rc = -EINVAL;
		return onoff_sync_finalize(&data->srv, key, NULL, rc, false);
	}
	dis_val = config->enable_inverted ? config->enable_val : 0;
	rc = regulator_modify_register(config, config->enable_reg,
			config->enable_mask, dis_val);
	if (rc) {
		/* Error writing configs */
		return onoff_sync_finalize(&data->srv, key, NULL, rc, true);
	}
	return onoff_sync_finalize(&data->srv, key, NULL, rc, true);
}

static int pmic_reg_init(const struct device *dev)
{
	const struct regulator_config *config = dev->config;
	struct regulator_data *data = dev->data;

	/* Cast the voltage array set at compile time to the voltage range
	 * struct
	 */
	data->voltages = (struct voltage_range *)config->voltage_array;
	/* Do the same cast for current limit ranges */
	data->current_levels = (struct current_range *)config->current_array;
	/* Check to verify we have a valid I2C device */
	if (!device_is_ready(config->i2c.bus)) {
		return -ENODEV;
	}
	return 0;
}


static const struct regulator_driver_api api = {
	.enable = enable_regulator,
	.disable = disable_regulator
};

#define CONFIGURE_REGULATOR(id)									\
	static uint32_t pmic_reg_##id##_cur_limits[] =						\
		DT_INST_PROP_OR(id, current_levels, {});					\
	static uint32_t pmic_reg_##id##_vol_range[] =						\
		DT_INST_PROP(id, voltage_range);						\
	static struct regulator_data pmic_reg_##id##_data;					\
	static struct regulator_config pmic_reg_##id##_cfg = {					\
		.vsel_mask = DT_INST_PROP(id, vsel_mask),					\
		.vsel_reg = DT_INST_PROP(id, vsel_reg),						\
		.num_voltages = DT_INST_PROP(id, num_voltages),					\
		.num_current_levels = DT_INST_PROP(id, num_current_levels),			\
		.enable_reg = DT_INST_PROP(id, enable_reg),					\
		.enable_mask = DT_INST_PROP(id, enable_mask),					\
		.enable_val = DT_INST_PROP(id, enable_val),					\
		.min_uV = DT_INST_PROP(id, min_uv),						\
		.max_uV = DT_INST_PROP(id, max_uv),						\
		.ilim_reg = DT_INST_PROP_OR(id, ilim_reg, 0),			\
		.ilim_mask = DT_INST_PROP_OR(id, ilim_mask, 0),			\
		.enable_inverted = DT_INST_PROP(id, enable_inverted),				\
		.i2c = I2C_DT_SPEC_INST_GET(id),						\
		.voltage_array = pmic_reg_##id##_vol_range,					\
		.current_array = pmic_reg_##id##_cur_limits,					\
	};											\
	DEVICE_DT_INST_DEFINE(id, pmic_reg_init, NULL,						\
			      &pmic_reg_##id##_data, &pmic_reg_##id##_cfg,			\
			      POST_KERNEL, CONFIG_PMIC_REGULATOR_INIT_PRIORITY,			\
			      &api);								\

DT_INST_FOREACH_STATUS_OKAY(CONFIGURE_REGULATOR)
