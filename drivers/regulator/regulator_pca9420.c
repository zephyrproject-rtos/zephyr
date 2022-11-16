/*
 * Copyright (c) 2021 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pca9420

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/dt-bindings/regulator/pmic_i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(pca9420, CONFIG_REGULATOR_LOG_LEVEL);

/** Register memory map. See datasheet for more details. */
/** General purpose registers */
/** @brief Top level system ctrl 0 */
#define PCA9420_TOP_CNTL0     0x09U

/** Regulator status indication registers */
/** @brief Mode configuration for mode 0_0 */
#define PCA9420_MODECFG_0_0          0x22U
/** @brief Mode configuration for mode 0_1 */
#define PCA9420_MODECFG_0_1          0x23U
/** @brief Mode configuration for mode 0_2 */
#define PCA9420_MODECFG_0_2          0x24U
/** @brief Mode configuration for mode 0_3 */
#define PCA9420_MODECFG_0_3          0x25U

/** @brief VIN input current limit selection */
#define PCA9420_TOP_CNTL0_VIN_ILIM_SEL_MASK 0xE0U

/*
 * @brief Mode control selection mask. When this bit is set, the external
 * PMIC pins MODESEL0 and MODESEL1 can be used to select the active mode
 */
#define PCA9420_MODECFG_0_MODE_CTRL_SEL_MASK 0x40U

/*
 * @brief Mode configuration upon falling edge applied to ON pin. If set,
 * the device will switch to mode 0 when a valid falling edge is applied.
 * to the ON pin
 */
/** @brief Mode output voltage mask */
#define PCA9420_MODECFG_0_SW1_OUT_MASK       0x3FU
/** @brief SW2_OUT offset and voltage level mask */
#define PCA9420_MODECFG_1_SW2_OUT_MASK       0x3FU
/** @brief LDO1_OUT voltage level mask */
#define PCA9420_MODECFG_2_LDO1_OUT_MASK      0xF0U
/** @brief SW1 Enable */
#define PCA9420_MODECFG_2_SW1_EN_MASK	     0x08U
#define PCA9420_MODECFG_2_SW1_EN_VAL	     0x08U
/** @brief SW2 Enable */
#define PCA9420_MODECFG_2_SW2_EN_MASK	     0x04U
#define PCA9420_MODECFG_2_SW2_EN_VAL	     0x04U
/** @brief LDO1 Enable */
#define PCA9420_MODECFG_2_LDO1_EN_MASK	     0x02U
#define PCA9420_MODECFG_2_LDO1_EN_VAL	     0x02U
/** @brief LDO2 Enable */
#define PCA9420_MODECFG_2_LDO2_EN_MASK	     0x01U
#define PCA9420_MODECFG_2_LDO2_EN_VAL	     0x01U
/** @brief LDO2_OUT offset and voltage level mask */
#define PCA9420_MODECFG_3_LDO2_OUT_MASK      0x3FU

struct voltage_range {
	int32_t uV; /* Voltage in uV */
	uint8_t reg_val; /* Register value for voltage */
};

struct current_range {
	int32_t uA; /* Current limit in uA */
	uint8_t reg_val; /* Register value for current limit */
};

struct regulator_pca9420_desc {
	uint8_t enable_reg;
	uint8_t enable_mask;
	uint8_t enable_val;
	uint8_t ilim_reg;
	uint8_t ilim_mask;
	uint8_t vsel_reg;
	uint8_t vsel_mask;
};

struct regulator_pca9420_data {
	struct onoff_sync_service srv;
	const struct voltage_range *voltages;
	const struct current_range *current_levels;
};

struct regulator_pca9420_config {
	int num_voltages;
	int num_current_levels;
	int num_modes;
	int32_t max_uv;
	int32_t min_uv;
	bool enable_inverted;
	bool boot_on;
	struct i2c_dt_spec i2c;
	uint16_t initial_mode;
	const uint32_t *voltage_array;
	const uint32_t *current_array;
	const uint16_t *allowed_modes;
	uint8_t modesel_reg;
	uint8_t modesel_mask;
	const struct regulator_pca9420_desc *desc;
};

static const struct regulator_pca9420_desc buck1_desc = {
	.enable_reg = PCA9420_MODECFG_0_2,
	.enable_mask = PCA9420_MODECFG_2_SW1_EN_MASK,
	.enable_val = PCA9420_MODECFG_2_SW1_EN_VAL,
	.ilim_reg = PCA9420_TOP_CNTL0,
	.ilim_mask = PCA9420_TOP_CNTL0_VIN_ILIM_SEL_MASK,
	.vsel_mask = PCA9420_MODECFG_0_SW1_OUT_MASK,
	.vsel_reg = PCA9420_MODECFG_0_0,
};

static const struct regulator_pca9420_desc buck2_desc = {
	.enable_reg = PCA9420_MODECFG_0_2,
	.enable_mask = PCA9420_MODECFG_2_SW2_EN_MASK,
	.enable_val = PCA9420_MODECFG_2_SW2_EN_VAL,
	.vsel_mask = PCA9420_MODECFG_1_SW2_OUT_MASK,
	.vsel_reg = PCA9420_MODECFG_0_1,
	.ilim_reg = PCA9420_TOP_CNTL0,
	.ilim_mask = PCA9420_TOP_CNTL0_VIN_ILIM_SEL_MASK,
};

static const struct regulator_pca9420_desc ldo1_desc = {
	.enable_reg = PCA9420_MODECFG_0_2,
	.enable_mask = PCA9420_MODECFG_2_LDO1_EN_MASK,
	.enable_val = PCA9420_MODECFG_2_LDO1_EN_VAL,
	.vsel_mask = PCA9420_MODECFG_2_LDO1_OUT_MASK,
	.vsel_reg = PCA9420_MODECFG_0_2,
	.ilim_reg = PCA9420_TOP_CNTL0,
	.ilim_mask = PCA9420_TOP_CNTL0_VIN_ILIM_SEL_MASK,
};

static const struct regulator_pca9420_desc ldo2_desc = {
	.enable_reg = PCA9420_MODECFG_0_2,
	.enable_mask = PCA9420_MODECFG_2_LDO2_EN_MASK,
	.enable_val = PCA9420_MODECFG_2_LDO2_EN_VAL,
	.vsel_reg = PCA9420_MODECFG_0_3,
	.vsel_mask = PCA9420_MODECFG_3_LDO2_OUT_MASK,
	.ilim_reg = PCA9420_TOP_CNTL0,
	.ilim_mask = PCA9420_TOP_CNTL0_VIN_ILIM_SEL_MASK,
};

static int regulator_pca9420_is_supported_voltage(const struct device *dev,
						  int32_t min_uv,
						  int32_t max_uv);

/**
 * Reads a register from the PMIC
 * Returns 0 on success, or errno on error
 */
static int regulator_pca9420_read_register(const struct device *dev,
					   uint8_t reg, uint8_t *out)
{
	const struct regulator_pca9420_config *conf = dev->config;
	int ret;

	ret = i2c_reg_read_byte_dt(&conf->i2c, reg, out);
	LOG_DBG("READ 0x%x: 0x%x", reg, *out);
	return ret;
}

/**
 * Modifies a register within the PMIC
 * Returns 0 on success, or errno on error
 */
static int regulator_pca9420_modify_register(const struct device *dev,
					     uint8_t reg, uint8_t reg_mask,
					     uint8_t reg_val)
{
	const struct regulator_pca9420_config *conf = dev->config;
	uint8_t reg_current;
	int rc;

	rc = regulator_pca9420_read_register(dev, reg, &reg_current);
	if (rc) {
		return rc;
	}

	reg_current &= ~reg_mask;
	reg_current |= (reg_val & reg_mask);
	LOG_DBG("WRITE 0x%02X to 0x%02X at I2C addr 0x%02X", reg_current,
		reg, conf->i2c.addr);
	return i2c_reg_write_byte_dt(&conf->i2c, reg, reg_current);
}

/*
 * Internal helper function- gets the voltage from a regulator, with an
 * offset applied to the vsel_reg. Useful to support reading voltages
 * in another target mode
 */
static int32_t regulator_pca9420_get_voltage_offset(const struct device *dev,
						    uint32_t off)
{
	const struct regulator_pca9420_config *config = dev->config;
	struct regulator_pca9420_data *data = dev->data;
	int rc, i = 0;
	uint8_t raw_reg;

	rc = regulator_pca9420_read_register(dev, config->desc->vsel_reg + off,
					     &raw_reg);
	if (rc) {
		return rc;
	}
	raw_reg &= config->desc->vsel_mask;
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
 * Internal helper function- sets the voltage for a regulator, with an
 * offset applied to the vsel_reg. Useful to support setting voltages in
 * another target mode.
 */
static int regulator_set_voltage_offset(const struct device *dev,
					int32_t min_uv, int32_t max_uv,
					uint32_t off)
{
	const struct regulator_pca9420_config *config = dev->config;
	struct regulator_pca9420_data *data = dev->data;
	int i = 0;

	if (!regulator_pca9420_is_supported_voltage(dev, min_uv, max_uv) ||
		min_uv > max_uv) {
		return -EINVAL;
	}
	/* Find closest supported voltage */
	while (i < config->num_voltages && min_uv > data->voltages[i].uV) {
		i++;
	}
	if (data->voltages[i].uV > max_uv) {
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
	return regulator_pca9420_modify_register(dev,
						 config->desc->vsel_reg + off,
						 config->desc->vsel_mask,
						 data->voltages[i].reg_val);
}


/**
 * Part of the extended regulator consumer API
 * Returns the number of supported voltages
 */
static int regulator_pca9420_count_voltages(const struct device *dev)
{
	const struct regulator_pca9420_config *config = dev->config;

	return config->num_voltages;
}

/**
 * Part of the extended regulator consumer API
 * Counts the number of modes supported by a regulator
 */
static int regulator_pca9420_count_modes(const struct device *dev)
{
	const struct regulator_pca9420_config *config = dev->config;

	return config->num_modes;
}

/**
 * Part of the extended regulator consumer API
 * Returns the supported voltage in uV for a given selector value
 */
static int32_t regulator_pca9420_list_voltages(const struct device *dev,
					       unsigned int selector)
{
	const struct regulator_pca9420_config *config = dev->config;
	struct regulator_pca9420_data *data = dev->data;

	if (config->num_voltages <= selector) {
		return -ENODEV;
	}
	return data->voltages[selector].uV;
}

/**
 * Part of the extended regulator consumer API
 * Returns true if the regulator supports a voltage in the given range.
 */
static int regulator_pca9420_is_supported_voltage(const struct device *dev,
						  int32_t min_uv,
						  int32_t max_uv)
{
	const struct regulator_pca9420_config *config = dev->config;

	return !((config->max_uv < min_uv) || (config->min_uv > max_uv));
}

/**
 * Part of the extended regulator consumer API
 * Sets the output voltage to the closest supported voltage value
 */
static int regulator_pca9420_set_voltage(const struct device *dev,
					 int32_t min_uv, int32_t max_uv)
{
	return regulator_set_voltage_offset(dev, min_uv, max_uv, 0);
}


/**
 * Part of the extended regulator consumer API
 * Gets the current output voltage in uV
 */
static int32_t regulator_pca9420_get_voltage(const struct device *dev)
{
	return regulator_pca9420_get_voltage_offset(dev, 0);
}

/**
 * Part of the extended regulator consumer API
 * Set the current limit for this device
 */
static int regulator_pca9420_set_current_limit(const struct device *dev,
					       int32_t min_ua, int32_t max_ua)
{
	const struct regulator_pca9420_config *config = dev->config;
	struct regulator_pca9420_data *data = dev->data;
	int i = 0;

	if (config->num_current_levels == 0) {
		/* Regulator cannot limit current */
		return -ENOTSUP;
	}
	/* Locate the desired current limit */
	while (i < config->num_current_levels &&
		min_ua > data->current_levels[i].uA) {
		i++;
	}
	if (i == config->num_current_levels ||
		data->current_levels[i].uA > max_ua) {
		return -EINVAL;
	}
	/* Set the current limit */
	return regulator_pca9420_modify_register(
		dev, config->desc->ilim_reg, config->desc->ilim_mask,
		data->current_levels[i].reg_val);
}

/**
 * Part of the extended regulator consumer API
 * Gets the set current limit for the regulator
 */
static int regulator_pca9420_get_current_limit(const struct device *dev)
{
	const struct regulator_pca9420_config *config = dev->config;
	struct regulator_pca9420_data *data = dev->data;
	int rc, i = 0;
	uint8_t raw_reg;

	if (config->num_current_levels == 0) {
		return -ENOTSUP;
	}
	rc = regulator_pca9420_read_register(dev, config->desc->ilim_reg,
					     &raw_reg);
	if (rc) {
		return rc;
	}
	raw_reg &= config->desc->ilim_mask;
	while (i < config->num_current_levels &&
		data->current_levels[i].reg_val != raw_reg) {
		i++;
	}
	if (i == config->num_current_levels) {
		return -EIO;
	}
	return data->current_levels[i].uA;
}

/*
 * Part of the extended regulator consumer API.
 * sets the target voltage for a given regulator mode. This mode does
 * not need to be the active mode. This API can be used to configure
 * voltages for a mode, then the regulator can be switched to that mode
 * with the regulator_pca9420_set_mode api
 */
static int regulator_pca9420_set_mode_voltage(const struct device *dev,
					      uint32_t mode, int32_t min_uv,
					      int32_t max_uv)
{
	const struct regulator_pca9420_config *config = dev->config;
	uint8_t i, sel_off;

	if (config->num_modes == 0) {
		return -ENOTSUP;
	}

	/* Search for mode ID in allowed modes. */
	for (i = 0 ; i < config->num_modes; i++) {
		if (config->allowed_modes[i] == mode) {
			break;
		}
	}
	if (i == config->num_modes) {
		/* Mode was not found */
		return -EINVAL;
	}
	sel_off = ((mode & PMIC_MODE_OFFSET_MASK) >> PMIC_MODE_OFFSET_SHIFT);
	return regulator_set_voltage_offset(dev, min_uv, max_uv, sel_off);
}

/*
 * Part of the extended regulator consumer API.
 * Disables the regulator in a given mode. Does not implement the
 * onoff service, as this is incompatible with multiple mode operation
 */
static int regulator_pca9420_mode_disable(const struct device *dev,
					  uint32_t mode)
{
	const struct regulator_pca9420_config *config = dev->config;
	uint8_t i, sel_off, dis_val;

	if (config->num_modes == 0) {
		return -ENOTSUP;
	}

	/* Search for mode ID in allowed modes. */
	for (i = 0 ; i < config->num_modes; i++) {
		if (config->allowed_modes[i] == mode) {
			break;
		}
	}
	if (i == config->num_modes) {
		/* Mode was not found */
		return -EINVAL;
	}
	sel_off = ((mode & PMIC_MODE_OFFSET_MASK) >> PMIC_MODE_OFFSET_SHIFT);
	dis_val = config->enable_inverted ? config->desc->enable_val : 0;
	return regulator_pca9420_modify_register(
		dev, config->desc->enable_reg + sel_off,
		config->desc->enable_mask, dis_val);
}

/*
 * Part of the extended regulator consumer API.
 * Enables the regulator in a given mode. Does not implement the
 * onoff service, as this is incompatible with multiple mode operation
 */
static int regulator_pca9420_mode_enable(const struct device *dev,
					 uint32_t mode)
{
	const struct regulator_pca9420_config *config = dev->config;
	uint8_t i, sel_off, en_val;

	if (config->num_modes == 0) {
		return -ENOTSUP;
	}

	/* Search for mode ID in allowed modes. */
	for (i = 0 ; i < config->num_modes; i++) {
		if (config->allowed_modes[i] == mode) {
			break;
		}
	}
	if (i == config->num_modes) {
		/* Mode was not found */
		return -EINVAL;
	}
	sel_off = ((mode & PMIC_MODE_OFFSET_MASK) >> PMIC_MODE_OFFSET_SHIFT);
	en_val = config->enable_inverted ? 0 : config->desc->enable_val;
	return regulator_pca9420_modify_register(
		dev, config->desc->enable_reg + sel_off,
		config->desc->enable_mask, en_val);
}

/*
 * Part of the extended regulator consumer API.
 * gets the target voltage for a given regulator mode. This mode does
 * not need to be the active mode. This API can be used to read voltages
 * from a regulator mode other than the default.
 */
static int32_t regulator_pca9420_get_mode_voltage(const struct device *dev,
						  uint32_t mode)
{
	const struct regulator_pca9420_config *config = dev->config;
	uint8_t i, sel_off;

	if (config->num_modes == 0) {
		return -ENOTSUP;
	}

	/* Search for mode ID in allowed modes. */
	for (i = 0 ; i < config->num_modes; i++) {
		if (config->allowed_modes[i] == mode) {
			break;
		}
	}
	if (i == config->num_modes) {
		/* Mode was not found */
		return -EINVAL;
	}
	sel_off = ((mode & PMIC_MODE_OFFSET_MASK) >> PMIC_MODE_OFFSET_SHIFT);
	return regulator_pca9420_get_voltage_offset(dev, sel_off);
}

/*
 * Part of the extended regulator consumer API
 * switches the regulator to a given mode. This API will apply a mode for
 * the regulator.
 */
static int regulator_pca9420_set_mode(const struct device *dev, uint32_t mode)
{
	const struct regulator_pca9420_config *config = dev->config;
	int rc;
	uint8_t i, sel_off;

	if (config->num_modes == 0) {
		return -ENOTSUP;
	}

	/* Search for mode ID in allowed modes. */
	for (i = 0 ; i < config->num_modes; i++) {
		if (config->allowed_modes[i] == mode) {
			break;
		}
	}
	if (i == config->num_modes) {
		/* Mode was not found */
		return -EINVAL;
	}
	sel_off = ((mode & PMIC_MODE_OFFSET_MASK) >> PMIC_MODE_OFFSET_SHIFT);
	/* Configure mode */
	if (mode & PMIC_MODE_FLAG_MODESEL_MULTI_REG) {
		/* Select mode with offset calculation */
		rc = regulator_pca9420_modify_register(
			dev, config->modesel_reg + sel_off,
			mode & PMIC_MODE_SELECTOR_MASK, config->modesel_mask);
	} else {
		/* Select mode without offset to modesel_reg */
		rc = regulator_pca9420_modify_register(
			dev, config->modesel_reg,
			mode & PMIC_MODE_SELECTOR_MASK, config->modesel_mask);
	}
	return rc;
}

static int regulator_pca9420_enable(const struct device *dev,
				    struct onoff_client *cli)
{
	k_spinlock_key_t key;
	int rc;
	uint8_t en_val;
	struct regulator_pca9420_data *data = dev->data;
	const struct regulator_pca9420_config *config = dev->config;

	LOG_DBG("Enabling regulator");
	rc = onoff_sync_lock(&data->srv, &key);
	if (rc) {
		/* Request has already enabled PMIC */
		return onoff_sync_finalize(&data->srv, key, cli, rc, true);
	}
	en_val = config->enable_inverted ? 0 : config->desc->enable_val;
	rc = regulator_pca9420_modify_register(dev, config->desc->enable_reg,
					       config->desc->enable_mask,
					       en_val);
	if (rc != 0) {
		return onoff_sync_finalize(&data->srv, key, NULL, rc, false);
	}
	return onoff_sync_finalize(&data->srv, key, cli, rc, true);
}

static int regulator_pca9420_disable(const struct device *dev)
{
	struct regulator_pca9420_data *data = dev->data;
	const struct regulator_pca9420_config *config = dev->config;
	k_spinlock_key_t key;
	uint8_t dis_val;
	int rc;

	LOG_DBG("Disabling regulator");
	rc = onoff_sync_lock(&data->srv, &key);
	if (rc == 0) {
		rc = -EINVAL;
		return onoff_sync_finalize(&data->srv, key, NULL, rc, false);
	} else if (rc == 1) {
		/* Disable regulator */
		dis_val = config->enable_inverted ? config->desc->enable_val : 0;
		rc = regulator_pca9420_modify_register(
			dev, config->desc->enable_reg,
			config->desc->enable_mask, dis_val);
	}
	return onoff_sync_finalize(&data->srv, key, NULL, rc, false);
}

static int regulator_pca9420_init(const struct device *dev)
{
	const struct regulator_pca9420_config *config = dev->config;
	struct regulator_pca9420_data *data = dev->data;
	int rc  = 0;

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
	if (config->boot_on) {
		rc = regulator_pca9420_enable(dev, NULL);
	}
	if (config->initial_mode) {
		rc = regulator_pca9420_set_mode(dev, config->initial_mode);
	}
	return rc;
}


static const struct regulator_driver_api api = {
	.enable = regulator_pca9420_enable,
	.disable = regulator_pca9420_disable,
	.count_voltages = regulator_pca9420_count_voltages,
	.count_modes = regulator_pca9420_count_modes,
	.list_voltages = regulator_pca9420_list_voltages,
	.is_supported_voltage = regulator_pca9420_is_supported_voltage,
	.set_voltage = regulator_pca9420_set_voltage,
	.get_voltage = regulator_pca9420_get_voltage,
	.set_current_limit = regulator_pca9420_set_current_limit,
	.get_current_limit = regulator_pca9420_get_current_limit,
	.set_mode = regulator_pca9420_set_mode,
	.set_mode_voltage = regulator_pca9420_set_mode_voltage,
	.get_mode_voltage = regulator_pca9420_get_mode_voltage,
	.mode_disable = regulator_pca9420_mode_disable,
	.mode_enable = regulator_pca9420_mode_enable,
};

#define REGULATOR_PCA9420_DEFINE(node_id, id, name)                            \
	static const uint32_t curr_limits_##id[] =                             \
		DT_PROP_OR(node_id, current_levels, {});                       \
	static const uint32_t vol_range_##id[] =                               \
		DT_PROP(node_id, voltage_range);                               \
	static const uint16_t allowed_modes_##id[] =                           \
		DT_PROP_OR(DT_PARENT(node_id), regulator_allowed_modes, {});   \
                                                                               \
	static struct regulator_pca9420_data data_##id;                        \
                                                                               \
	static const struct regulator_pca9420_config config_##id = {           \
		.num_voltages = DT_PROP(node_id, num_voltages),                \
		.num_current_levels = DT_PROP(node_id, num_current_levels),    \
		.min_uv = DT_PROP(node_id, regulator_min_microvolt),           \
		.max_uv = DT_PROP(node_id, regulator_max_microvolt),           \
		.enable_inverted = DT_PROP(node_id, enable_inverted),          \
		.boot_on = DT_PROP(node_id, regulator_boot_on),                \
		.num_modes = ARRAY_SIZE(allowed_modes_##id),                   \
		.initial_mode = DT_PROP_OR(DT_PARENT(node_id),                 \
					   regulator_initial_mode, 0),         \
		.i2c = I2C_DT_SPEC_GET(DT_PARENT(node_id)),                    \
		.voltage_array = vol_range_##id,                               \
		.current_array = curr_limits_##id,                             \
		.allowed_modes = allowed_modes_##id,                           \
		.modesel_reg = DT_PROP_OR(DT_PARENT(node_id), modesel_reg, 0), \
		.modesel_mask =                                                \
			DT_PROP_OR(DT_PARENT(node_id), modesel_mask, 0),       \
		.desc = &name ## _desc,                                        \
	};                                                                     \
                                                                               \
	DEVICE_DT_DEFINE(node_id, regulator_pca9420_init, NULL, &data_##id,    \
			 &config_##id, POST_KERNEL,                            \
			 CONFIG_REGULATOR_PCA9420_INIT_PRIORITY, &api);

#define REGULATOR_PCA9420_DEFINE_COND(inst, child)                             \
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(inst, child)),                \
		    (REGULATOR_PCA9420_DEFINE(DT_INST_CHILD(inst, child),      \
					      child ## inst, child)), ())

#define REGULATOR_PCA9420_DEFINE_ALL(inst)                                     \
	REGULATOR_PCA9420_DEFINE_COND(inst, buck1)                             \
	REGULATOR_PCA9420_DEFINE_COND(inst, buck2)                             \
	REGULATOR_PCA9420_DEFINE_COND(inst, ldo1)                              \
	REGULATOR_PCA9420_DEFINE_COND(inst, ldo2)

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_PCA9420_DEFINE_ALL)
