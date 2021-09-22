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

#include <kernel.h>
#include <drivers/regulator.h>
#include <drivers/regulator/consumer.h>
#include <drivers/i2c.h>
#include <errno.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(pmic_regulator, CONFIG_REGULATOR_LOG_LEVEL);

struct regulator_data {
	struct onoff_sync_service srv;
};

struct __packed voltage_range {
	int uV; /* Voltage in uV */
	int reg_val; /* Register value for voltage */
};

struct regulator_config {
	struct voltage_range *voltages;
	int num_voltages;
	uint8_t vsel_reg;
	uint8_t vsel_mask;
	uint32_t max_uV;
	uint32_t min_uV;
	uint8_t enable_reg;
	uint8_t enable_mask;
	uint8_t enable_val;
	bool enable_inverted;
	uint8_t i2c_address;
	const struct device *i2c_dev;
	uint32_t voltage_array[];
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

	rc = i2c_reg_read_byte(conf->i2c_dev, conf->i2c_address,
			reg, &reg_current);
	if (rc) {
		return rc;
	}
	reg_current &= ~reg_mask;
	reg_current |= reg_val;
	return i2c_reg_write_byte(conf->i2c_dev, conf->i2c_address, reg,
			reg_current);
}


/**
 * Part of the extended regulator consumer API
 * Returns the number of supported voltages
 */
int regulator_count_voltages(const struct device *dev)
{
	return ((struct regulator_config *)dev->config)->num_voltages;
}

/**
 * Part of the extended regulator consumer API
 * Returns the supported voltage in uV for a given selector value
 */
int regulator_list_voltages(const struct device *dev, unsigned int selector)
{
	const struct regulator_config *config = dev->config;

	if (config->num_voltages <= selector) {
		return -ENODEV;
	}
	return config->voltages[selector].uV;
}

int regulator_is_supported_voltage(const struct device *dev,
		int min_uV, int max_uV)
{
	const struct regulator_config *config = dev->config;

	return !((config->max_uV < min_uV) || (config->min_uV > max_uV));
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
			config->enable_reg, en_val);
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
			config->enable_reg, dis_val);
	if (rc) {
		/* Error writing configs */
		return onoff_sync_finalize(&data->srv, key, NULL, rc, true);
	}
	return onoff_sync_finalize(&data->srv, key, NULL, rc, true);
}

static int pmic_reg_init(const struct device *dev)
{
	struct regulator_config *config = (struct regulator_config *)dev->config;

	LOG_INF("PMIC regulator initializing");
	/* Cast the voltage array set at compile time to the voltage range
	 * struct
	 */
	config->voltages = (struct voltage_range *)config->voltage_array;
	/* Check to verify we have a valid I2C device */
	if (config->i2c_dev == NULL || !device_is_ready(config->i2c_dev)) {
		return -ENODEV;
	}
	return 0;
}


static const struct regulator_driver_api api = {
	.enable = enable_regulator,
	.disable = disable_regulator
};

#define CONFIGURE_REGULATOR(id)									\
	static struct regulator_data pmic_reg_##id##_data;					\
	static struct regulator_config pmic_reg_##id##_cfg = {					\
		.vsel_mask = DT_INST_PROP(id, vsel_mask),					\
		.vsel_reg = DT_INST_PROP(id, vsel_reg),						\
		.num_voltages = DT_INST_PROP(id, num_voltages),					\
		.enable_reg = DT_INST_PROP(id, enable_reg),					\
		.enable_mask = DT_INST_PROP(id, enable_mask),					\
		.enable_val = DT_INST_PROP(id, enable_val),					\
		.min_uV = DT_INST_PROP(id, min_uv),						\
		.max_uV = DT_INST_PROP(id, max_uv),						\
		.enable_inverted = DT_INST_PROP(id, enable_inverted),				\
		.i2c_address = DT_REG_ADDR(DT_PARENT(DT_DRV_INST(id))),				\
		.i2c_dev = DEVICE_DT_GET(DT_BUS(DT_PARENT(DT_DRV_INST(id)))),			\
		.voltage_array = DT_INST_PROP(id, voltage_range),				\
	};											\
	DEVICE_DT_INST_DEFINE(id, pmic_reg_init, NULL,						\
			      &pmic_reg_##id##_data, &pmic_reg_##id##_cfg,			\
			      POST_KERNEL, CONFIG_PMIC_REGULATOR_INIT_PRIORITY,			\
			      &api);								\

DT_INST_FOREACH_STATUS_OKAY(CONFIGURE_REGULATOR)
