/*
 * Copyright 2021 Matija Tudan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(max17262, CONFIG_SENSOR_LOG_LEVEL);

#include "max17262.h"

#define DT_DRV_COMPAT maxim_max17262

/**
 * @brief Read a register value
 *
 * Registers have an address and a 16-bit value
 *
 * @param dev MAX17262 device to access
 * @param reg_addr Register address to read
 * @param valp Place to put the value on success
 * @return 0 if successful, or negative error code from I2C API
 */
static int max17262_reg_read(const struct device *dev, uint8_t reg_addr, int16_t *valp)
{
	const struct max17262_config *cfg = dev->config;
	uint8_t i2c_data[2];
	int rc;

	rc = i2c_burst_read_dt(&cfg->i2c, reg_addr, i2c_data, 2);
	if (rc < 0) {
		LOG_ERR("Unable to read register 0x%02x", reg_addr);
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
 * @param dev MAX17262 device to access
 * @param reg_addr Register address to write to
 * @param val Register value to write
 * @return 0 if successful, or negative error code from I2C API
 */
static int max17262_reg_write(const struct device *dev, uint8_t reg_addr, int16_t val)
{
	const struct max17262_config *cfg = dev->config;
	uint8_t i2c_data[3] = {reg_addr, val & 0xFF, (uint16_t)val >> 8};

	return i2c_write_dt(&cfg->i2c, i2c_data, sizeof(i2c_data));
}

/**
 * @brief Convert sensor value from millis
 *
 * @param val Where to store converted value in sensor_value format
 * @param val_millis Value in millis
 */
static void convert_millis(struct sensor_value *val, int32_t val_millis)
{
	val->val1 = val_millis / 1000;
	val->val2 = (val_millis % 1000) * 1000;
}

/**
 * @brief Convert raw register values for specific channel
 *
 * @param dev MAX17262 device to access
 * @param chan Channel number to read
 * @param valp Returns the sensor value read on success
 * @return 0 if successful
 * @return -ENOTSUP for unsupported channels
 */
static int max17262_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *valp)
{
	const struct max17262_config *const config = dev->config;
	struct max17262_data *const data = dev->data;
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
		/* Get avg current in nA */
		current = data->avg_current * CURRENT_MULTIPLIER_NA;
		/* Convert to mA */
		valp->val1 = current / 1000000;
		valp->val2 = current % 1000000;
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
		convert_millis(valp, data->full_cap);
		break;
	case SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY:
		convert_millis(valp, data->remaining_cap);
		break;
	case SENSOR_CHAN_GAUGE_TIME_TO_EMPTY:
		/* Get time in ms */
		if (data->time_to_empty == 0xffff) {
			valp->val1 = 0;
			valp->val2 = 0;
		} else {
			tmp = data->time_to_empty * TIME_MULTIPLIER_MS;
			convert_millis(valp, tmp);
		}
		break;
	case SENSOR_CHAN_GAUGE_TIME_TO_FULL:
		/* Get time in ms */
		if (data->time_to_full == 0xffff) {
			valp->val1 = 0;
			valp->val2 = 0;
		} else {
			tmp = data->time_to_full * TIME_MULTIPLIER_MS;
			convert_millis(valp, tmp);
		}
		break;
	case SENSOR_CHAN_GAUGE_CYCLE_COUNT:
		valp->val1 = data->cycle_count / 100;
		valp->val2 = data->cycle_count % 100 * 10000;
		break;
	case SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY:
		convert_millis(valp, data->design_cap);
		break;
	case SENSOR_CHAN_GAUGE_DESIGN_VOLTAGE:
		convert_millis(valp, config->design_voltage);
		break;
	case SENSOR_CHAN_GAUGE_DESIRED_VOLTAGE:
		convert_millis(valp, config->desired_voltage);
		break;
	case SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT:
		valp->val1 = data->ichg_term;
		valp->val2 = 0;
		break;
	case MAX17262_COULOMB_COUNTER:
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
 * @brief Read register values for supported channels
 *
 * @param dev MAX17262 device to access
 * @return 0 if successful, or negative error code from I2C API
 */
static int max17262_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct max17262_data *data = dev->data;

	/* clang-format off */
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
	/* clang-format on */

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);
	for (size_t i = 0; i < ARRAY_SIZE(regs); i++) {
		int rc;

		rc = max17262_reg_read(dev, regs[i].reg_addr, regs[i].dest);
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
 * @param dev MAX17262 device to access
 * @return 0 for success
 * @return -EINVAL if the I2C controller could not be found
 */
static int max17262_gauge_init(const struct device *dev)
{
	const struct max17262_config *const config = dev->config;
	int16_t tmp, hibcfg;
	int rc;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	/* Read Status register */
	rc = max17262_reg_read(dev, STATUS, &tmp);
	if (rc) {
		return rc;
	}

	if (!(tmp & STATUS_POR)) {
		/*
		 * Status.POR bit is set to 1 when MAX17262 detects that
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
	rc = max17262_reg_read(dev, FSTAT, &tmp);
	if (rc) {
		return rc;
	}

	/* Do not continue until FSTAT.DNR bit is cleared */
	while (tmp & FSTAT_DNR) {
		k_sleep(K_MSEC(10));
		rc = max17262_reg_read(dev, FSTAT, &tmp);
		if (rc) {
			return rc;
		}
	}

	/** STEP 2 */
	/* Store original HibCFG value */
	rc = max17262_reg_read(dev, HIBCFG, &hibcfg);
	if (rc) {
		return rc;
	}

	/* Exit Hibernate Mode step 1 */
	rc = max17262_reg_write(dev, SOFT_WAKEUP, 0x0090);
	if (rc) {
		return rc;
	}

	/* Exit Hibernate Mode step 2 */
	rc = max17262_reg_write(dev, HIBCFG, 0x0000);
	if (rc) {
		return rc;
	}

	/* Exit Hibernate Mode step 3 */
	rc = max17262_reg_write(dev, SOFT_WAKEUP, 0x0000);
	if (rc) {
		return rc;
	}

	/** STEP 2.1 --> OPTION 1 EZ Config (No INI file is needed) */
	/* Write DesignCap */
	rc = max17262_reg_write(dev, DESIGN_CAP, config->design_cap);
	if (rc) {
		return rc;
	}

	/* Write IChgTerm */
	rc = max17262_reg_write(dev, ICHG_TERM, config->desired_charging_current);
	if (rc) {
		return rc;
	}

	/* Write VEmpty */
	rc = max17262_reg_write(dev, VEMPTY,
				((config->empty_voltage / 10) << 7) |
					((config->recovery_voltage / 40) & 0x7F));
	if (rc) {
		return rc;
	}

	/* Write ModelCFG */
	if (config->charge_voltage > 4275) {
		rc = max17262_reg_write(dev, MODELCFG, 0x8400);
	} else {
		rc = max17262_reg_write(dev, MODELCFG, 0x8000);
	}

	if (rc) {
		return rc;
	}

	/*
	 * Read ModelCFG.Refresh (highest bit),
	 * proceed to Step 3 when ModelCFG.Refresh == 0
	 */
	rc = max17262_reg_read(dev, MODELCFG, &tmp);
	if (rc) {
		return rc;
	}

	/* Do not continue until ModelCFG.Refresh == 0 */
	while (tmp & MODELCFG_REFRESH) {
		k_sleep(K_MSEC(10));
		rc = max17262_reg_read(dev, MODELCFG, &tmp);
		if (rc) {
			return rc;
		}
	}

	/* Restore Original HibCFG value */
	rc = max17262_reg_write(dev, HIBCFG, hibcfg);
	if (rc) {
		return rc;
	}

	/** STEP 3 */
	/* Read Status register */
	rc = max17262_reg_read(dev, STATUS, &tmp);
	if (rc) {
		return rc;
	}

	/* Clear PowerOnReset bit */
	tmp &= ~STATUS_POR;
	rc = max17262_reg_write(dev, STATUS, tmp);
	if (rc) {
		return rc;
	}

	return 0;
}

static const struct sensor_driver_api max17262_battery_driver_api = {
	.sample_fetch = max17262_sample_fetch,
	.channel_get = max17262_channel_get,
};

#define MAX17262_INIT(n)                                                                           \
	static struct max17262_data max17262_data_##n;                                             \
                                                                                                   \
	static const struct max17262_config max17262_config_##n = {                                \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
		.design_voltage = DT_INST_PROP(n, design_voltage),                                 \
		.desired_voltage = DT_INST_PROP(n, desired_voltage),                               \
		.desired_charging_current = DT_INST_PROP(n, desired_charging_current),             \
		.design_cap = DT_INST_PROP(n, design_cap),                                         \
		.empty_voltage = DT_INST_PROP(n, empty_voltage),                                   \
		.recovery_voltage = DT_INST_PROP(n, recovery_voltage),                             \
		.charge_voltage = DT_INST_PROP(n, charge_voltage),                                 \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, &max17262_gauge_init, NULL, &max17262_data_##n,            \
				     &max17262_config_##n, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &max17262_battery_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX17262_INIT)
