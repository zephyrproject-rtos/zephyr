/*
 * Copyright 2021 Matija Tudan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/i2c.h>
#include <drivers/sensor.h>
#include <drivers/sensor/max1726x.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(max1726x, CONFIG_SENSOR_LOG_LEVEL);

#include "max1726x.h"

#define DT_DRV_COMPAT maxim_max1726x
#define FIX_POINT_COEFF 1000

/**
 * @brief Read a register value
 *
 * Registers have an address and a 16-bit value
 *
 * @param dev MAX1726X device to access
 * @param reg_addr Register address to read
 * @param valp Place to put the value on success
 * @return 0 if successful, or negative error code from I2C API
 */
static int max1726x_reg_read(const struct device *dev, uint8_t reg_addr,
			     int16_t *valp)
{
	const struct max1726x_config *cfg = dev->config;
	uint8_t i2c_data[2];
	int rc;

	rc = i2c_burst_read(cfg->i2c, cfg->i2c_addr, reg_addr,
			    i2c_data, 2);
	if (rc < 0) {
		LOG_ERR("Unable to read register");
		return rc;
	}
	*valp = ((int16_t)i2c_data[1] << 8) | i2c_data[0];

	return 0;
}

/**
 * @brief Write a register value
 *
 * Registers have an address and a 16-bit value
 *
 * @param dev MAX1726X device to access
 * @param reg_addr Register address to write to
 * @param val Register value to write
 * @return 0 if successful, or negative error code from I2C API
 */
static int max1726x_reg_write(const struct device *dev, uint8_t reg_addr,
			     int16_t val)
{
	const struct max1726x_config *cfg = dev->config;
	uint8_t i2c_data[3] = {reg_addr, val & 0xFF, (uint16_t)val >> 8};

	return i2c_write(cfg->i2c, i2c_data, sizeof(i2c_data),
		     cfg->i2c_addr);
}

/**
 * @brief Convert current in MAX1726X units to milliamps
 *
 * @param rsense_mohms Value of Rsense in milliohms
 * @param val Value to convert (taken from a MAX1726X register)
 * @return corresponding value in milliamps
 */
static int current_to_ma(unsigned int rsense_mohms, int16_t val)
{
	return (val * CURRENT_MEASUREMENT_RES) / rsense_mohms;
}

/**
 * @brief Convert capacity in MAX1726X units to milliamps/hours
 *
 * @param rsense_mohms Value of Rsense in milliohms
 * @param val Value to convert (taken from a MAX26X register)
 * @return corresponding value in milliamps/hours
 */
static int capacity_to_mah(unsigned int rsense_mohms, int16_t val)
{
	int lsb_units, rem;

	/* Get units for the LSB in mA */
	lsb_units = CAPACITY_LSB_MULTIPLIER * FIX_POINT_COEFF / rsense_mohms;
	/* Get remaining capacity in mA */
	rem = val * lsb_units;

	return rem;
}

/**
 * @brief Convert sensor value from millis
 *
 * @param val Where to store converted value in sensor_value format
 * @param val_millis Value in millis
 */
static void convert_fp(struct sensor_value *val, int32_t val_millis)
{
	val->val1 = val_millis / FIX_POINT_COEFF;
	val->val2 = (val_millis % FIX_POINT_COEFF) * FIX_POINT_COEFF;
}

/**
 * @brief Convert milliamps/hours in MAX1726X units
 *
 * @param rsense_mohms Value of Rsense in milliohms
 * @param val Value to convert (taken from the MAX1726X DESIGN_CAP register)
 * @return corresponding value in MAX1726X unit
 */
static int16_t mah_to_capacity(uint16_t rsense_mohms, uint16_t val)
{
	int cap;

	cap = val * rsense_mohms / CAPACITY_LSB_MULTIPLIER;

	return cap;
}

/**
 * @brief Set hibernate mode
 *
 * @param dev MAX1726X device to access
 * @return 0 if successful, or negative error code
 */
static int max1726x_set_hibernate(const struct device *dev)
{
	const struct max1726x_config *const config = dev->config;

	/* Enable hibernate */
	return max1726x_reg_write(
		dev, HIBCFG,
		MAX1726X_EN_HIB |
			MAX1726X_HIB_ENTER_TIME(config->hibernate_enter_time) |
			MAX1726X_HIB_THRESHOLD(config->hibernate_threshold) |
			MAX1726X_HIB_EXIT_TIME(config->hibernate_exit_time) |
			MAX1726X_HIB_SCALAR(config->hibernate_scalar));
}

/**
 * @brief Set shutdown mode
 *
 * @param dev MAX1726X device to access
 * @return 0 if successful, or -EIO error code
 */
static int max1726x_shutdown(const struct device *dev)
{
	uint16_t tmp, rc;

	max1726x_reg_read(dev, HIBCFG, &tmp);
	rc = max1726x_reg_write(dev, HIBCFG, tmp & (~MAX1726X_EN_HIB));
	if (rc){
		return -EIO;
	}

	rc = max1726x_reg_write(dev, SHDN_TIMER, 0x001E);
	if (rc){
		return -EIO;
	}

	rc = max1726x_reg_write(dev, CONFIG, MAX1726X_EN_SHDN);
	if (rc){
		return -EIO;
	}

	return 0;
}

/**
 * @brief Convert raw register values for specific channel
 *
 * @param dev MAX1726X device to access
 * @param chan Channel number to read
 * @param valp Returns the sensor value read on success
 * @return 0 if successful
 * @return -ENOTSUP for unsupported channels
 */
static int max1726x_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *valp)
{
	const struct max1726x_config *const config = dev->config;
	struct max1726x_data *const data = dev->data;
	int32_t tmp;

	switch (chan) {
	case SENSOR_CHAN_GAUGE_VOLTAGE:
		/* Get voltage in uV */
		tmp = data->voltage * VOLTAGE_MULTIPLIER_UV;
		/* Convert to V */
		valp->val1 = tmp / 1000000;
		valp->val2 = tmp % 1000000;
		break;
	case SENSOR_CHAN_GAUGE_AVG_CURRENT: {
		int current;
		/* Get avg current in mA */
		current =
			current_to_ma(config->rsense_mohms, data->avg_current);
		convert_fp(valp, current);
		break;
	}
	case SENSOR_CHAN_GAUGE_STATE_OF_CHARGE:
		valp->val1 = data->state_of_charge / 256;
		valp->val2 = data->state_of_charge % 256 * 1000000 / 256;
		break;
	case SENSOR_CHAN_GAUGE_TEMP:
		valp->val1 = data->internal_temp / 256;
		valp->val2 = data->internal_temp % 256 * 1000000 / 256;
		break;
	case SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY:
		tmp = capacity_to_mah(config->rsense_mohms, data->full_cap);
		convert_fp(valp, tmp);
		break;
	case SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY:
		tmp = capacity_to_mah(config->rsense_mohms, data->remaining_cap);
		convert_fp(valp, tmp);
		break;
	case SENSOR_CHAN_GAUGE_TIME_TO_EMPTY:
		/* Get time in ms */
		if (data->time_to_empty == 0xffff) {
			valp->val1 = 0;
			valp->val2 = 0;
		} else {
			tmp = data->time_to_empty * TIME_MULTIPLIER_MS;
			convert_fp(valp, tmp);
		}
		break;
	case SENSOR_CHAN_GAUGE_TIME_TO_FULL:
		/* Get time in ms */
		if (data->time_to_full == 0xffff) {
			valp->val1 = 0;
			valp->val2 = 0;
		} else {
			tmp = data->time_to_full * TIME_MULTIPLIER_MS;
			convert_fp(valp, tmp);
		}
		break;
	case SENSOR_CHAN_GAUGE_CYCLE_COUNT:
		valp->val1 = data->cycle_count / 100;
		valp->val2 = data->cycle_count % 100 * 10000;
		break;
	case SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY:
		tmp = capacity_to_mah(config->rsense_mohms, data->design_cap);
		convert_fp(valp, tmp);
		break;
	case SENSOR_CHAN_GAUGE_DESIGN_VOLTAGE:
		convert_fp(valp, config->design_voltage);
		break;
	case SENSOR_CHAN_GAUGE_DESIRED_VOLTAGE:
		convert_fp(valp, config->desired_voltage);
		break;
	case SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT:
		valp->val1 = data->ichg_term;
		valp->val2 = 0;
		break;
	case MAX1726X_COULOMB_COUNTER:
		/* Get spent capacity in mAh */
		data->coulomb_counter = 0xffff - data->coulomb_counter;
		valp->val1 = data->coulomb_counter / 2;
		valp->val2 = data->coulomb_counter % 2 * 10 / 2;
		break;
	default:
		LOG_ERR("Unsupported channel!");
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief Set a max1726x config
 *
 * @param dev MAX1726X device to access
 * @param chan Channel number to read
 * @param attr A sensor device private attribute
 * @param val The value of the channel
 * @return 0 if successful
 * @return -ENOTSUP for unsupported channels
 */
static int max1726x_config(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	/* Check MAX1726x private attribute types */
	switch ((enum max1726x_sensor_attribute)attr) {
	case SENSOR_ATTR_MAX1726X_HIBERNATE:
		return max1726x_set_hibernate(dev);
	case SENSOR_ATTR_MAX1726X_SHUTDOWN:
		return max1726x_shutdown(dev);
	default:
		LOG_DBG("max1726x attribute not supported");
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief Set a max1726x attributes
 *
 * @param dev MAX1726X device to access
 * @param chan Channel number to read
 * @param attr A sensor device private attribute
 * @param val The value of the channel
 * @return 0 if max1726x config is successful
 * @return -ENOTSUP if max1726x config failed
 */
static int max1726x_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr,
			     const struct sensor_value *val)
{
	return max1726x_config(dev, chan, attr, val);
}

/**
 * @brief Read register values for supported channels
 *
 * @param dev MAX1726X device to access
 * @return 0 if successful, or negative error code from I2C API
 */
static int max1726x_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	struct max1726x_data *data = dev->data;
	struct {
		int reg_addr;
		int16_t *dest;
	} regs[] = {
		{ VCELL, &data->voltage },
		{ AVG_CURRENT, &data->avg_current },
		{ ICHG_TERM, &data->ichg_term },
		{ REP_SOC, &data->state_of_charge },
		{ INT_TEMP, &data->internal_temp },
		{ REP_CAP, &data->remaining_cap },
		{ FULL_CAP_REP, &data->full_cap },
		{ TTE, &data->time_to_empty },
		{ TTF, &data->time_to_full },
		{ CYCLES, &data->cycle_count },
		{ DESIGN_CAP, &data->design_cap },
		{ COULOMB_COUNTER, &data->coulomb_counter },
	};

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);
	for (size_t i = 0; i < ARRAY_SIZE(regs); i++) {
		int rc;

		rc = max1726x_reg_read(dev, regs[i].reg_addr, regs[i].dest);
		if (rc != 0) {
			LOG_ERR("Failed to read channel %d", chan);
			return rc;
		}
	}

	return 0;
}

/**
 * @brief Initialise the fuel gauge
 *
 * @param dev MAX1726X device to access
 * @return 0 for success
 * @return -EINVAL if the I2C controller could not be found
 */
static int max1726x_gauge_init(const struct device *dev)
{
	const struct max1726x_config *const config = dev->config;
	int16_t tmp, hibcfg;

	if (!device_is_ready(config->i2c)) {
		LOG_ERR("Could not get pointer to %s device", config->i2c->name);
		return -EINVAL;
	}

	/* Read Status register */
	max1726x_reg_read(dev, STATUS, &tmp);

	if (!(tmp & STATUS_POR)) {
		/*
		 * Status.POR bit is set to 1 when MAX1726X detects that
		 * a software or hardware POR event has occurred and
		 * therefore a custom configuration needs to be set...
		 * If POR event did not happen (Status.POR == 0), skip
		 * init and continue with measurements.
		 */
		LOG_DBG("No POR event detected - skip device configuration");
		return 0;
	}
	LOG_DBG("POR detected, setting custom device configuration...");

	/** STEP 1 */
	max1726x_reg_read(dev, FSTAT, &tmp);

	/* Do not continue until FSTAT.DNR bit is cleared */
	while (tmp & FSTAT_DNR) {
		k_sleep(K_MSEC(10));
		max1726x_reg_read(dev, FSTAT, &tmp);
	}

	/** STEP 2 */
	/* Store original HibCFG value */
	max1726x_reg_read(dev, HIBCFG, &hibcfg);

	/* Exit Hibernate Mode step 1 */
	max1726x_reg_write(dev, SOFT_WAKEUP, 0x0090);
	/* Exit Hibernate Mode step 2 */
	max1726x_reg_write(dev, HIBCFG, 0x0000);
	/* Exit Hibernate Mode step 3 */
	max1726x_reg_write(dev, SOFT_WAKEUP, 0x0000);

	/** STEP 2.1 --> OPTION 1 EZ Config (No INI file is needed) */
	/* Write DesignCap */
	max1726x_reg_write(dev, DESIGN_CAP,
			   mah_to_capacity(config->rsense_mohms,
					  config->design_cap));

	/* Write IChgTerm */
	max1726x_reg_write(dev, ICHG_TERM, config->desired_charging_current);

	/* Write VEmpty */
	max1726x_reg_write(dev, VEMPTY, ((config->empty_voltage / 10) << 7) |
					  ((config->recovery_voltage / 40) & 0x7F));

	/* Write ModelCFG */
	if (config->charge_voltage > 4275) {
		max1726x_reg_write(dev, MODELCFG, 0x8400);
	} else {
		max1726x_reg_write(dev, MODELCFG, 0x8000);
	}

	/*
	 * Read ModelCFG.Refresh (highest bit),
	 * proceed to Step 3 when ModelCFG.Refresh == 0
	 */
	max1726x_reg_read(dev, MODELCFG, &tmp);

	/* Do not continue until ModelCFG.Refresh == 0 */
	while (tmp & MODELCFG_REFRESH) {
		k_sleep(K_MSEC(10));
		max1726x_reg_read(dev, MODELCFG, &tmp);
	}

	/* Restore Original HibCFG value */
	max1726x_reg_write(dev, HIBCFG, hibcfg);

	/** STEP 3 */
	/* Read Status register */
	max1726x_reg_read(dev, STATUS, &tmp);

	/* Clear PowerOnReset bit */
	tmp &= ~STATUS_POR;
	max1726x_reg_write(dev, STATUS, tmp);

	return 0;
}

static const struct sensor_driver_api max1726x_battery_driver_api = {
	.attr_set = max1726x_attr_set,
	.sample_fetch = max1726x_sample_fetch,
	.channel_get = max1726x_channel_get,
};

#define MAX1726X_INIT(n)                                                       \
	static struct max1726x_data max1726x_data_##n;                         \
                                                                             \
	static const struct max1726x_config max1726x_config_##n = {            \
		.i2c = DEVICE_DT_GET(DT_BUS(DT_DRV_INST(n))),                  \
		.i2c_addr = DT_INST_REG_ADDR(n),                               \
		.design_voltage = DT_INST_PROP(n, design_voltage),             \
		.desired_voltage = DT_INST_PROP(n, desired_voltage),           \
		.desired_charging_current =                                    \
			DT_INST_PROP(n, desired_charging_current),             \
		.design_cap = DT_INST_PROP(n, design_cap),                     \
		.rsense_mohms = DT_INST_PROP(n, rsense_mohms),                 \
		.empty_voltage = DT_INST_PROP(n, empty_voltage),               \
		.recovery_voltage = DT_INST_PROP(n, recovery_voltage),         \
		.charge_voltage = DT_INST_PROP(n, charge_voltage),             \
		.hibernate_threshold = DT_INST_PROP(n, hibernate_threshold),   \
		.hibernate_scalar = DT_INST_PROP(n, hibernate_scalar),         \
		.hibernate_exit_time = DT_INST_PROP(n, hibernate_exit_time),   \
		.hibernate_enter_time = DT_INST_PROP(n, hibernate_enter_time), \
	};                                                                     \
                                                                             \
	DEVICE_DT_INST_DEFINE(n, &max1726x_gauge_init, NULL,                   \
			      &max1726x_data_##n, &max1726x_config_##n,        \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,        \
			      &max1726x_battery_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX1726X_INIT)
