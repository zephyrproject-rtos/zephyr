/*
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/i2c.h>
#include <drivers/sensor.h>
#include <sys/byteorder.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(max17055, CONFIG_SENSOR_LOG_LEVEL);

#include "max17055.h"

#define DT_DRV_COMPAT maxim_max17055

/**
 * @brief Read a register value
 *
 * Registers have an address and a 16-bit value
 *
 * @param priv Private data for the driver
 * @param reg_addr Register address to read
 * @param val Place to put the value on success
 * @return 0 if successful, or negative error code from I2C API
 */
static int max17055_reg_read(struct max17055_data *priv, int reg_addr,
			     int16_t *valp)
{
	uint8_t i2c_data[2];
	int rc;

	rc = i2c_burst_read(priv->i2c, DT_INST_REG_ADDR(0), reg_addr, i2c_data, 2);
	if (rc < 0) {
		LOG_ERR("Unable to read register");
		return rc;
	}
	*valp = (i2c_data[1] << 8) | i2c_data[0];

	return 0;
}

static int max17055_reg_write(struct max17055_data *priv, int reg_addr,
			      int16_t val)
{
	uint8_t i2c_data[2];

	sys_put_le16(val, i2c_data);

	return i2c_burst_write(priv->i2c, DT_INST_REG_ADDR(0), reg_addr, i2c_data, 2);
}

/**
 * @brief Convert current in MAX17055 units to milliamps
 *
 * @param rsense_mohms Value of Rsense in milliohms
 * @param val Value to convert (taken from a MAX17055 register)
 * @return corresponding value in milliamps
 */
static int current_to_ma(unsigned int rsense_mohms, int16_t val)
{
	return (val * 1.5625) / rsense_mohms;
}

/**
 * @brief Convert current in milliamps to MAX17055 units
 *
 * @param rsense_mohms Value of Rsense in milliohms
 * @param val Value in mA to convert
 * @return corresponding value in MAX17055 units, ready to write to a register
 */
static int current_ma_to_max17055(unsigned int rsense_mohms, uint16_t val)
{
	return val * rsense_mohms / 1.5625;
}

/**
 * @brief Convert capacity in MAX17055 units to milliamps
 *
 * @param rsense_mohms Value of Rsense in milliohms
 * @param val Value to convert (taken from a MAX17055 register)
 * @return corresponding value in milliamps
 */
static int capacity_to_ma(unsigned int rsense_mohms, int16_t val)
{
	int lsb_units, rem;

	/* Get units for the LSB in uA */
	lsb_units = 5 * 1000 / rsense_mohms;
	/* Get remaining capacity in uA */
	rem = val * lsb_units;

	return rem;
}

/**
 * @brief Convert capacity in milliamphours to MAX17055 units
 *
 * @param rsense_mohms Value of Rsense in milliohms
 * @param val_mha Value in milliamphours to convert
 * @return corresponding value in MAX17055 units, ready to write to a register
 */
static int capacity_to_max17055(unsigned int rsense_mohms, uint16_t val_mha)
{
	return val_mha * rsense_mohms / 5;
}

/**
 * @brief Convert voltage in millivolts to MAX17055 units
 *
 * @param val_mv Value in millivolts to convert
 * @return corresponding value in MAX17055 units, ready to write to a register
 */
static int voltage_mV_to_max17055(uint16_t val_mv)
{
	return val_mv * 16 / 1.25;
}

static void set_millis(struct sensor_value *val, int val_millis)
{
	val->val1 = val_millis / 1000;
	val->val2 = (val_millis % 1000) * 1000;
}

/**
 * @brief sensor value get
 *
 * @param dev MAX17055 device to access
 * @param chan Channel number to read
 * @param valp Returns the sensor value read on success
 * @return 0 if successful
 * @return -ENOTSUP for unsupported channels
 */
static int max17055_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *valp)
{
	const struct max17055_config *const config = dev->config;
	struct max17055_data *const priv = dev->data;
	unsigned int tmp;

	switch (chan) {
	case SENSOR_CHAN_GAUGE_VOLTAGE:
		/* Get voltage in uV */
		tmp = priv->voltage * 1250 / 16;
		valp->val1 = tmp / 1000000;
		valp->val2 = tmp % 1000000;
		break;
	case SENSOR_CHAN_GAUGE_AVG_CURRENT: {
		int current_ma;

		current_ma = current_to_ma(config->rsense_mohms, priv->avg_current);
		set_millis(valp, current_ma);
		break;
	}
	case SENSOR_CHAN_GAUGE_STATE_OF_CHARGE:
		valp->val1 = priv->state_of_charge / 256;
		valp->val2 = priv->state_of_charge % 256 * 1000000 / 256;
		break;
	case SENSOR_CHAN_GAUGE_TEMP:
		valp->val1 = priv->internal_temp / 256;
		valp->val2 = priv->internal_temp % 256 * 1000000 / 256;
		break;
	case SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY:
		tmp = capacity_to_ma(config->rsense_mohms, priv->full_cap);
		set_millis(valp, tmp);
		break;
	case SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY:
		tmp = capacity_to_ma(config->rsense_mohms, priv->remaining_cap);
		set_millis(valp, tmp);
		break;
	case SENSOR_CHAN_GAUGE_TIME_TO_EMPTY:
		/* Get time in ms */
		if (priv->time_to_empty == 0xffff) {
			valp->val1 = 0;
			valp->val2 = 0;
		} else {
			tmp = priv->time_to_empty * 5625;
			set_millis(valp, tmp);
		}
		break;
	case SENSOR_CHAN_GAUGE_TIME_TO_FULL:
		if (priv->time_to_full == 0xffff) {
			valp->val1 = 0;
			valp->val2 = 0;
		} else {
			/* Get time in ms */
			tmp = priv->time_to_full * 5625;
			set_millis(valp, tmp);
		}
		break;
	case SENSOR_CHAN_GAUGE_CYCLE_COUNT:
		valp->val1 = priv->cycle_count / 100;
		valp->val2 = priv->cycle_count % 100 * 10000;
		break;
	case SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY:
		tmp = capacity_to_ma(config->rsense_mohms, priv->design_cap);
		set_millis(valp, tmp);
		break;
	case SENSOR_CHAN_GAUGE_DESIGN_VOLTAGE:
		set_millis(valp, config->design_voltage);
		break;
	case SENSOR_CHAN_GAUGE_DESIRED_VOLTAGE:
		set_millis(valp, config->desired_voltage);
		break;
	case SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT:
		valp->val1 = config->desired_charging_current;
		valp->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int max17055_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	struct max17055_data *priv = dev->data;

	struct {
		int reg_addr;
		int16_t *dest;
	} regs[] = {
		{ VCELL, &priv->voltage },
		{ AVG_CURRENT, &priv->avg_current },
		{ REP_SOC, &priv->state_of_charge },
		{ INT_TEMP, &priv->internal_temp },
		{ REP_CAP, &priv->remaining_cap },
		{ FULL_CAP_REP, &priv->full_cap },
		{ TTE, &priv->time_to_empty },
		{ TTF, &priv->time_to_full },
		{ CYCLES, &priv->cycle_count },
		{ DESIGN_CAP, &priv->design_cap },
	};

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);
	for (size_t i = 0; i < ARRAY_SIZE(regs); i++) {
		int rc;

		rc = max17055_reg_read(priv, regs[i].reg_addr, regs[i].dest);
		if (rc != 0) {
			LOG_ERR("Failed to read channel %d", chan);
			return rc;
		}
	}

	return 0;
}

static int max17055_exit_hibernate(struct max17055_data *priv)
{
	LOG_DBG("Exit hibernate");

	if (max17055_reg_write(priv, SOFT_WAKEUP, SOFT_WAKEUP_WAKEUP)) {
		return -EIO;
	}
	if (max17055_reg_write(priv, HIB_CFG, HIB_CFG_CLEAR)) {
		return -EIO;
	}
	if (max17055_reg_write(priv, SOFT_WAKEUP, SOFT_WAKEUP_CLEAR)) {
		return -EIO;
	}

	return 0;
}

static int max17055_write_config(struct max17055_data *priv,
				 const struct max17055_config *const config)
{
	uint16_t design_capacity = capacity_to_max17055(config->rsense_mohms,
							config->design_capacity);
	uint16_t d_qacc = design_capacity / 32;
	uint16_t d_pacc = d_qacc * 44138 / design_capacity;
	uint16_t i_chg_term = current_ma_to_max17055(config->rsense_mohms, config->i_chg_term);
	uint16_t v_empty = voltage_mV_to_max17055(config->v_empty);

	LOG_DBG("Writing configuration parameters");
	LOG_DBG("DesignCap: %u, dQAcc: %u, IChgTerm: %u, VEmpty: %u, dPAcc: %u",
		design_capacity, d_qacc, i_chg_term, v_empty, d_pacc);

	if (max17055_reg_write(priv, DESIGN_CAP, design_capacity)) {
		return -EIO;
	}
	if (max17055_reg_write(priv, D_QACC, d_qacc)) {
		return -EIO;
	}
	if (max17055_reg_write(priv, ICHG_TERM, i_chg_term)) {
		return -EIO;
	}
	if (max17055_reg_write(priv, V_EMPTY, v_empty)) {
		return -EIO;
	}
	if (max17055_reg_write(priv, D_PACC, d_pacc)) {
		return -EIO;
	}
	if (max17055_reg_write(priv, MODEL_CFG, MODELCFG_REFRESH)) {
		return -EIO;
	}

	uint16_t model_cfg = MODELCFG_REFRESH;

	while (model_cfg & MODELCFG_REFRESH) {
		max17055_reg_read(priv, MODEL_CFG, &model_cfg);
		k_sleep(K_MSEC(10));
	}

	return 0;
}

static int max17055_init_config(struct max17055_data *priv,
				const struct max17055_config *const config)
{
	int16_t hib_cfg;

	if (max17055_reg_read(priv, HIB_CFG, &hib_cfg)) {
		return -EIO;
	}

	if (max17055_exit_hibernate(priv)) {
		return -EIO;
	}

	if (max17055_write_config(priv, config)) {
		return -EIO;
	}

	if (max17055_reg_write(priv, HIB_CFG, hib_cfg)) {
		return -EIO;
	}

	return 0;
}

/**
 * @brief initialise the fuel gauge
 *
 * @return 0 for success
 * @return -EIO on I2C communication error
 * @return -EINVAL if the I2C controller could not be found
 */
static int max17055_gauge_init(const struct device *dev)
{
	int16_t tmp;
	struct max17055_data *priv = dev->data;
	const struct max17055_config *const config = dev->config;

	priv->i2c = device_get_binding(config->bus_name);
	if (!priv->i2c) {
		LOG_ERR("Could not get pointer to %s device", config->bus_name);
		return -EINVAL;
	}

	if (max17055_reg_read(priv, STATUS, &tmp)) {
		return -EIO;
	}

	if (!(tmp & STATUS_POR)) {
		LOG_DBG("No POR event detected - skip device configuration");
		return 0;
	}

	/* Wait for FSTAT_DNR to be cleared */
	tmp = FSTAT_DNR;
	while (tmp & FSTAT_DNR) {
		max17055_reg_read(priv, FSTAT, &tmp);
	}

	if (max17055_init_config(priv, config)) {
		return -EIO;
	}

	/* Clear PowerOnReset bit */
	if (max17055_reg_read(priv, STATUS, &tmp)) {
		return -EIO;
	}

	tmp &= ~STATUS_POR;
	return max17055_reg_write(priv, STATUS, tmp);
}

static const struct sensor_driver_api max17055_battery_driver_api = {
	.sample_fetch = max17055_sample_fetch,
	.channel_get = max17055_channel_get,
};

#define MAX17055_INIT(index)								   \
	static struct max17055_data max17055_driver_##index;				   \
											   \
	static const struct max17055_config max17055_config_##index = {			   \
		.bus_name = DT_INST_BUS_LABEL(index),					   \
		.design_capacity = DT_INST_PROP(index, design_capacity),		   \
		.design_voltage = DT_INST_PROP(index, design_voltage),			   \
		.desired_charging_current = DT_INST_PROP(index, desired_charging_current), \
		.desired_voltage = DT_INST_PROP(index, desired_voltage),		   \
		.i_chg_term = DT_INST_PROP(index, i_chg_term),				   \
		.rsense_mohms = DT_INST_PROP(index, rsense_mohms),			   \
		.v_empty = DT_INST_PROP(index, v_empty),				   \
	};										   \
											   \
	DEVICE_DT_INST_DEFINE(index, &max17055_gauge_init,				   \
			      device_pm_control_nop,					   \
			      &max17055_driver_##index,					   \
			      &max17055_config_##index, POST_KERNEL,			   \
			      CONFIG_SENSOR_INIT_PRIORITY,				   \
			      &max17055_battery_driver_api)

DT_INST_FOREACH_STATUS_OKAY(MAX17055_INIT);
