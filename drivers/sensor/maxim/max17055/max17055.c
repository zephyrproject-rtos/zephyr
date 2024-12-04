/*
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(max17055, CONFIG_SENSOR_LOG_LEVEL);

#include "max17055.h"
#include <zephyr/drivers/sensor/max17055.h>

#define DT_DRV_COMPAT maxim_max17055

/**
 * @brief Read a register value
 *
 * Registers have an address and a 16-bit value
 *
 * @param priv Private data for the driver
 * @param reg_addr Register address to read
 * @param valp Place to put the value on success
 * @return 0 if successful, or negative error code from I2C API
 */
static int max17055_reg_read(const struct device *dev, uint8_t reg_addr,
			     int16_t *valp)
{
	const struct max17055_config *config = dev->config;
	uint8_t i2c_data[2];
	int rc;

	rc = i2c_burst_read_dt(&config->i2c, reg_addr, i2c_data, 2);
	if (rc < 0) {
		LOG_ERR("Unable to read register");
		return rc;
	}
	*valp = sys_get_le16(i2c_data);

	return 0;
}

static int max17055_reg_write(const struct device *dev, uint8_t reg_addr,
			      uint16_t val)
{
	const struct max17055_config *config = dev->config;
	uint8_t buf[3];

	buf[0] = reg_addr;
	sys_put_le16(val, &buf[1]);

	return i2c_write_dt(&config->i2c, buf, sizeof(buf));
}

/**
 * @brief Convert current in MAX17055 units to milliamps
 *
 * @param rsense_mohms Value of Rsense in milliohms
 * @param val Value to convert (taken from a MAX17055 register)
 * @return corresponding value in milliamps
 */
static int current_to_ma(uint16_t rsense_mohms, int16_t val)
{
	return (int32_t)val * 25 / rsense_mohms / 16; /* * 1.5625 */
}

/**
 * @brief Convert current in milliamps to MAX17055 units
 *
 * @param rsense_mohms Value of Rsense in milliohms
 * @param val Value in mA to convert
 * @return corresponding value in MAX17055 units, ready to write to a register
 */
static int current_ma_to_max17055(uint16_t rsense_mohms, uint16_t val)
{
	return (int32_t)val * rsense_mohms * 16 / 25; /* / 1.5625 */
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
 * @brief Update empty voltage target in v_empty
 *
 * @param v_empty The register value to update
 * @param val_mv Value in millivolts to convert
 * @return 0 on success, -EINVAL on invalid val_mv
 */
static int max17055_update_vempty(uint16_t *v_empty, uint16_t val_mv)
{
	uint32_t val = (val_mv / 10) << 7;

	if (val & ~VEMPTY_VE) {
		return -EINVAL;
	}

	*v_empty = (*v_empty & ~VEMPTY_VE) | (uint16_t)val;

	return 0;
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
	case SENSOR_CHAN_MAX17055_VFOCV:
		tmp = priv->ocv * 1250 / 16;
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
		if (priv->time_to_empty == 0xffff) {
			valp->val1 = 0;
			valp->val2 = 0;
		} else {
			/* Get time in milli-minutes */
			tmp = priv->time_to_empty * 5625 / 60;
			set_millis(valp, tmp);
		}
		break;
	case SENSOR_CHAN_GAUGE_TIME_TO_FULL:
		if (priv->time_to_full == 0xffff) {
			valp->val1 = 0;
			valp->val2 = 0;
		} else {
			/* Get time in milli-minutes */
			tmp = priv->time_to_full * 5625 / 60;
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
	int ret = -ENOTSUP;

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GAUGE_VOLTAGE) {
		ret = max17055_reg_read(dev, VCELL, &priv->voltage);
		if (ret < 0) {
			return ret;
		}
	}

	if (chan == SENSOR_CHAN_ALL ||
	    (enum sensor_channel_max17055)chan == SENSOR_CHAN_MAX17055_VFOCV) {
		ret = max17055_reg_read(dev, VFOCV, &priv->ocv);
		if (ret < 0) {
			return ret;
		}
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GAUGE_AVG_CURRENT) {
		ret = max17055_reg_read(dev, AVG_CURRENT, &priv->avg_current);
		if (ret < 0) {
			return ret;
		}
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GAUGE_STATE_OF_CHARGE) {
		ret = max17055_reg_read(dev, REP_SOC, &priv->state_of_charge);
		if (ret < 0) {
			return ret;
		}
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GAUGE_TEMP) {
		ret = max17055_reg_read(dev, INT_TEMP, &priv->internal_temp);
		if (ret < 0) {
			return ret;
		}
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY) {
		ret = max17055_reg_read(dev, REP_CAP, &priv->remaining_cap);
		if (ret < 0) {
			return ret;
		}
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY) {
		ret = max17055_reg_read(dev, FULL_CAP_REP, &priv->full_cap);
		if (ret < 0) {
			return ret;
		}
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GAUGE_TIME_TO_EMPTY) {
		ret = max17055_reg_read(dev, TTE, &priv->time_to_empty);
		if (ret < 0) {
			return ret;
		}
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GAUGE_TIME_TO_FULL) {
		ret = max17055_reg_read(dev, TTF, &priv->time_to_full);
		if (ret < 0) {
			return ret;
		}
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GAUGE_CYCLE_COUNT) {
		ret = max17055_reg_read(dev, CYCLES, &priv->cycle_count);
		if (ret < 0) {
			return ret;
		}
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY) {
		ret = max17055_reg_read(dev, DESIGN_CAP, &priv->design_cap);
		if (ret < 0) {
			return ret;
		}
	}

	return ret;
}

static int max17055_exit_hibernate(const struct device *dev)
{
	LOG_DBG("Exit hibernate");

	if (max17055_reg_write(dev, SOFT_WAKEUP, SOFT_WAKEUP_WAKEUP)) {
		return -EIO;
	}
	if (max17055_reg_write(dev, HIB_CFG, HIB_CFG_CLEAR)) {
		return -EIO;
	}
	if (max17055_reg_write(dev, SOFT_WAKEUP, SOFT_WAKEUP_CLEAR)) {
		return -EIO;
	}

	return 0;
}

static int max17055_write_config(const struct device *dev)
{
	const struct max17055_config *config = dev->config;

	uint16_t design_capacity = capacity_to_max17055(config->rsense_mohms,
							config->design_capacity);
	uint16_t d_qacc = design_capacity / 32;
	uint16_t d_pacc = d_qacc * 44138 / design_capacity;
	uint16_t i_chg_term = current_ma_to_max17055(config->rsense_mohms, config->i_chg_term);
	uint16_t v_empty;
	int ret;

	LOG_DBG("Writing configuration parameters");
	LOG_DBG("DesignCap: %u, dQAcc: %u, IChgTerm: %u, dPAcc: %u",
		design_capacity, d_qacc, i_chg_term, d_pacc);

	if (max17055_reg_write(dev, DESIGN_CAP, design_capacity)) {
		return -EIO;
	}

	if (max17055_reg_write(dev, D_QACC, d_qacc)) {
		return -EIO;
	}

	if (max17055_reg_write(dev, ICHG_TERM, i_chg_term)) {
		return -EIO;
	}

	if (max17055_reg_read(dev, V_EMPTY, &v_empty)) {
		return -EIO;
	}
	if (max17055_update_vempty(&v_empty, config->v_empty)) {
		return -EINVAL;
	}
	if (max17055_reg_write(dev, V_EMPTY, v_empty)) {
		return -EIO;
	}

	if (max17055_reg_write(dev, D_PACC, d_pacc)) {
		return -EIO;
	}

	if (max17055_reg_write(dev, MODEL_CFG, MODELCFG_REFRESH)) {
		return -EIO;
	}

	uint16_t model_cfg = MODELCFG_REFRESH;

	while (model_cfg & MODELCFG_REFRESH) {
		ret = max17055_reg_read(dev, MODEL_CFG, &model_cfg);
		if (ret) {
			return ret;
		}
		k_sleep(K_MSEC(10));
	}

	return 0;
}

static int max17055_init_config(const struct device *dev)
{
	int16_t hib_cfg;

	if (max17055_reg_read(dev, HIB_CFG, &hib_cfg)) {
		return -EIO;
	}

	if (max17055_exit_hibernate(dev)) {
		return -EIO;
	}

	if (max17055_write_config(dev)) {
		return -EIO;
	}

	if (max17055_reg_write(dev, HIB_CFG, hib_cfg)) {
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
	const struct max17055_config *const config = dev->config;
	int ret;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	if (max17055_reg_read(dev, STATUS, &tmp)) {
		return -EIO;
	}

	if (!(tmp & STATUS_POR)) {
		LOG_DBG("No POR event detected - skip device configuration");
		return 0;
	}

	/* Wait for FSTAT_DNR to be cleared */
	tmp = FSTAT_DNR;
	while (tmp & FSTAT_DNR) {
		ret = max17055_reg_read(dev, FSTAT, &tmp);
		if (ret) {
			return ret;
		}
	}

	if (max17055_init_config(dev)) {
		return -EIO;
	}

	/* Clear PowerOnReset bit */
	if (max17055_reg_read(dev, STATUS, &tmp)) {
		return -EIO;
	}

	tmp &= ~STATUS_POR;
	return max17055_reg_write(dev, STATUS, tmp);
}

static DEVICE_API(sensor, max17055_battery_driver_api) = {
	.sample_fetch = max17055_sample_fetch,
	.channel_get = max17055_channel_get,
};

#define MAX17055_INIT(index)								   \
	static struct max17055_data max17055_driver_##index;				   \
											   \
	static const struct max17055_config max17055_config_##index = {			   \
		.i2c = I2C_DT_SPEC_INST_GET(index),					   \
		.design_capacity = DT_INST_PROP(index, design_capacity),		   \
		.design_voltage = DT_INST_PROP(index, design_voltage),			   \
		.desired_charging_current = DT_INST_PROP(index, desired_charging_current), \
		.desired_voltage = DT_INST_PROP(index, desired_voltage),		   \
		.i_chg_term = DT_INST_PROP(index, i_chg_term),				   \
		.rsense_mohms = DT_INST_PROP(index, rsense_mohms),			   \
		.v_empty = DT_INST_PROP(index, v_empty),				   \
	};										   \
											   \
	SENSOR_DEVICE_DT_INST_DEFINE(index, &max17055_gauge_init,			   \
			      NULL,							   \
			      &max17055_driver_##index,					   \
			      &max17055_config_##index, POST_KERNEL,			   \
			      CONFIG_SENSOR_INIT_PRIORITY,				   \
			      &max17055_battery_driver_api)

DT_INST_FOREACH_STATUS_OKAY(MAX17055_INIT);
