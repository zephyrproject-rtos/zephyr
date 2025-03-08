/*
 * Original license from ina23x driver:
 *  Copyright 2021 The Chromium OS Authors
 *  Copyright 2021 Grinn
 *
 * Copyright 2024, Remie Lowik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_ina700

#include "ina700.h"
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ina700.h>
#include <zephyr/sys/byteorder.h>
#include <stdint.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(INA700, CONFIG_SENSOR_LOG_LEVEL);

/** @brief The LSB value for the bus voltage register, in microvolts/LSB. */
#define INA700_BUS_VOLTAGE_UV_LSB 3125U

/** @brief The LSB value for the temperature register, in microcelsius/LSB. */
#define INA700_TEMPERATURE_UC_LSB 125000U

/** @brief The LSB value for the current register, in microAmpere/LSB. */
#define INA700_CURRENT_UA_LSB 480U

/** @brief The LSB value for the power register, in microWatt/LSB. */
#define INA700_POWER_UW_LSB 96U

/** @brief The LSB value for the energy register, in microJoule/LSB. */
#define INA700_ENERGY_UJ_LSB 1536U

/** @brief The LSB value for the charge register, in microCoulomb/LSB. */
#define INA700_CHARGE_UC_LSB 30U

#define BIT40_SIGN_BIT_POSITION 39

/**
 * @brief Get a reading from the INA700 sensor
 * See sensor_channel_get in sensor.h for more details.
 * Available channels are:
 * SENSOR_CHAN_VOLTAGE
 * SENSOR_CHAN_DIE_TEMP
 * SENSOR_CHAN_CURRENT
 * SENSOR_CHAN_POWER
 * SENSOR_CHAN_ENERGY
 * SENSOR_CHAN_CHARGE
 * @param dev Pointer to the sensor device
 * @param chan The channel to read
 * @param val Where to store the value
 *
 * @return 0 if successful, negative errno code if failure.
 */
static int ina700_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct ina700_data *data = dev->data;
	int32_t bus_uv, temperature, current;
	uint32_t power;
	uint64_t energy;
	int64_t charge;
	enum sensor_channel_ina700 ina_chan = (enum sensor_channel_ina700)chan;

	if (chan == SENSOR_CHAN_VOLTAGE) {
		bus_uv = data->bus_voltage * INA700_BUS_VOLTAGE_UV_LSB;

		val->val1 = bus_uv / 1000000U;
		val->val2 = bus_uv % 1000000U;
	} else if (chan == SENSOR_CHAN_DIE_TEMP) {
		/** The bit shift is required because the first 4 bits are reserved. */
		temperature = (data->temperature >> 4) * INA700_TEMPERATURE_UC_LSB;

		val->val1 = temperature / 1000000L;
		val->val2 = temperature % 1000000L;
	} else if (chan == SENSOR_CHAN_CURRENT) {
		current = data->current * INA700_CURRENT_UA_LSB;

		val->val1 = current / 1000000L;
		val->val2 = current % 1000000L;
	} else if (chan == SENSOR_CHAN_POWER) {
		power = data->power * INA700_POWER_UW_LSB;

		val->val1 = power / 1000000U;
		val->val2 = power % 1000000U;
	} else if (ina_chan == SENSOR_CHAN_ENERGY) {
		energy = data->energy * INA700_ENERGY_UJ_LSB;

		val->val1 = energy / 1000000U;
		val->val2 = energy % 1000000U;
	} else if (ina_chan == SENSOR_CHAN_CHARGE) {
		charge = data->charge * INA700_CHARGE_UC_LSB;

		val->val1 = charge / 1000000L;
		val->val2 = charge % 1000000L;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief Fetch a sample from the INA700 sensor and store it in an internal
 * driver buffer
 * See sensor_sample_fetch in sensor.h for more details.
 * @param dev Pointer to the sensor device
 *
 * @return 0 if successful, negative errno code if failure.
 */
static int ina700_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ina700_data *data = dev->data;
	const struct ina700_config *config = dev->config;
	int ret;
	enum sensor_channel_ina700 ina_chan = (enum sensor_channel_ina700)chan;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE &&
	    chan != SENSOR_CHAN_DIE_TEMP && chan != SENSOR_CHAN_CURRENT &&
	    chan != SENSOR_CHAN_POWER && ina_chan != SENSOR_CHAN_ENERGY &&
	    ina_chan != SENSOR_CHAN_CHARGE) {
		return -ENOTSUP;
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_VOLTAGE)) {
		ret = ina700_reg_read_16(&config->bus, INA700_REG_BUS_VOLT, &data->bus_voltage);
		if (ret < 0) {
			LOG_ERR("Failed to read bus voltage");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_DIE_TEMP)) {
		ret = ina700_reg_read_16(&config->bus, INA700_REG_TEMPERATURE, &data->temperature);
		if (ret < 0) {
			LOG_ERR("Failed to read temperature");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_CURRENT)) {
		ret = ina700_reg_read_16(&config->bus, INA700_REG_CURRENT, &data->current);
		if (ret < 0) {
			LOG_ERR("Failed to read current");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_POWER)) {
		ret = ina700_reg_read_24(&config->bus, INA700_REG_POWER, &data->power);
		if (ret < 0) {
			LOG_ERR("Failed to read power");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (ina_chan == SENSOR_CHAN_ENERGY)) {
		ret = ina700_reg_read_40(&config->bus, INA700_REG_ENERGY, &data->energy);
		if (ret < 0) {
			LOG_ERR("Failed to read energy");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (ina_chan == SENSOR_CHAN_CHARGE)) {
		ret = ina700_reg_read_40(&config->bus, INA700_REG_CHARGE, &data->charge);
		if (ret < 0) {
			LOG_ERR("Failed to read charge");
			return ret;
		}
	}

	return 0;
}

/**
 * @brief Read the data from inside the device structure
 * @return 0 if successful, negative errno code if failure.
 */
static int ina700_read_data(const struct device *dev)
{
	struct ina700_data *data = dev->data;

	return ina700_sample_fetch(dev, data->chan);
}

/**
 * @brief Set an attribute for the iNA700 ensor
 * @param dev Pointer to the sensor device
 * @param chan The channel the attribute belongs to, if any.  Some
 * attributes may only be set for all channels of a device, depending on
 * device capabilities.
 * @param attr The attribute to set
 * @param val The value to set the attribute to
 *
 * @return 0 if successful, negative errno code if failure.
 */
static int ina700_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct ina700_config *config = dev->config;
	uint16_t data = val->val1;
	enum sensor_attribute_ina700 ina_attr = (enum sensor_attribute_ina700)attr;

	if (attr == SENSOR_ATTR_CONFIGURATION) {
		return ina700_reg_write(&config->bus, INA700_REG_CONFIG, data);
	} else if (attr == SENSOR_ATTR_ALERT) {
		return ina700_reg_write(&config->bus, INA700_REG_ALERT_DIAG, data);
	} else if (ina_attr == SENSOR_ATTR_CURRENT_OVER_LIMIT_THRESHOLD) {
		return ina700_reg_write(&config->bus, INA700_REG_COL, data);
	} else if (ina_attr == SENSOR_ATTR_CURRENT_UNDER_LIMIT_THRESHOLD) {
		return ina700_reg_write(&config->bus, INA700_REG_CUL, data);
	} else if (ina_attr == SENSOR_ATTR_BUS_OVERVOLTAGE_THRESHOLD) {
		return ina700_reg_write(&config->bus, INA700_REG_BOVL, data);
	} else if (ina_attr == SENSOR_ATTR_BUS_UNDERVOLTAGE_THRESHOLD) {
		return ina700_reg_write(&config->bus, INA700_REG_BUVL, data);
	} else if (ina_attr == SENSOR_ATTR_TEMPERATURE_OVER_LIMIT_THRESHOLD) {
		return ina700_reg_write(&config->bus, INA700_REG_TEMP_LIMIT, data);
	} else if (ina_attr == SENSOR_ATTR_POWER_OVER_LIMIT_THRESHOLD) {
		return ina700_reg_write(&config->bus, INA700_REG_PWR_LIMIT, data);
	}
	LOG_ERR("INA700 attribute not supported.");
	return -ENOTSUP;
}

/**
 * @brief Get an attribute for the iNA700 sensor
 *
 * @param dev Pointer to the sensor device
 * @param chan The channel the attribute belongs to, if any.  Some
 * attributes may only be set for all channels of a device, depending on
 * device capabilities.
 * @param attr The attribute to get
 * @param val Pointer to where to store the attribute
 *
 * @return 0 if successful, negative errno code if failure.
 */
static int ina700_attr_get(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, struct sensor_value *val)
{
	const struct ina700_config *config = dev->config;
	uint16_t data;
	int ret;
	enum sensor_attribute_ina700 ina_attr = (enum sensor_attribute_ina700)attr;

	if (attr == SENSOR_ATTR_CONFIGURATION) {
		ret = ina700_reg_read_16(&config->bus, INA700_REG_CONFIG, &data);
		if (ret < 0) {
			return ret;
		}
	} else if (attr == SENSOR_ATTR_ALERT) {
		ret = ina700_reg_read_16(&config->bus, INA700_REG_ALERT_DIAG, &data);
		if (ret < 0) {
			return ret;
		}
	} else if (ina_attr == SENSOR_ATTR_CURRENT_OVER_LIMIT_THRESHOLD) {
		ret = ina700_reg_read_16(&config->bus, INA700_REG_COL, &data);
		if (ret < 0) {
			return ret;
		}
	} else if (ina_attr == SENSOR_ATTR_CURRENT_UNDER_LIMIT_THRESHOLD) {
		ret = ina700_reg_read_16(&config->bus, INA700_REG_CUL, &data);
		if (ret < 0) {
			return ret;
		}
	} else if (ina_attr == SENSOR_ATTR_BUS_OVERVOLTAGE_THRESHOLD) {
		ret = ina700_reg_read_16(&config->bus, INA700_REG_BOVL, &data);
		if (ret < 0) {
			return ret;
		}
	} else if (ina_attr == SENSOR_ATTR_BUS_UNDERVOLTAGE_THRESHOLD) {
		ret = ina700_reg_read_16(&config->bus, INA700_REG_BUVL, &data);
		if (ret < 0) {
			return ret;
		}
	} else if (ina_attr == SENSOR_ATTR_TEMPERATURE_OVER_LIMIT_THRESHOLD) {
		ret = ina700_reg_read_16(&config->bus, INA700_REG_TEMP_LIMIT, &data);
		if (ret < 0) {
			return ret;
		}
	} else if (ina_attr == SENSOR_ATTR_POWER_OVER_LIMIT_THRESHOLD) {
		ret = ina700_reg_read_16(&config->bus, INA700_REG_PWR_LIMIT, &data);
		if (ret < 0) {
			return ret;
		}
	} else {
		LOG_ERR("INA700 attribute not supported.");
		return -ENOTSUP;
	}

	val->val1 = data;
	val->val2 = 0;

	return 0;
}

/**
 * @brief The work handler for the IN700 sensor trigger
 * @param work a work item to retrieve the trigger from
 * @return 0 if successful, negative errno code if failure.
 */
static void ina700_trigger_work_handler(struct k_work *work)
{
	struct ina700_trigger *trigg = CONTAINER_OF(work, struct ina700_trigger, conversion_work);
	struct ina700_data *data = CONTAINER_OF(trigg, struct ina700_data, trigger);
	const struct ina700_config *config = data->dev->config;
	int ret;
	uint16_t reg_alert;

	/* Read reg alert to clear alerts */
	ret = ina700_reg_read_16(&config->bus, INA700_REG_ALERT_DIAG, &reg_alert);
	if (ret < 0) {
		LOG_ERR("Failed to read alert register!");
		return;
	}

	ret = ina700_read_data(data->dev);
	if (ret < 0) {
		LOG_WRN("Unable to read data, ret %d", ret);
	}

	if (data->trigger.handler_alert) {
		data->trigger.handler_alert(data->dev, data->trigger.trig_alert);
	}
}

/**
 * @brief sensor operation mode check
 *
 * @retval true if set or false if not set
 */
static bool ina700_is_triggered_mode_set(const struct device *dev)
{
	const struct ina700_config *config = dev->config;

	uint8_t mode = (config->adc_config & GENMASK(15, 12)) >> 12;

	switch (mode) {
	case INA700_OPER_MODE_TRIGGERED_BUS_VOLTAGE:
	case INA700_OPER_MODE_TRIGGERED_TEMPERATURE:
	case INA700_OPER_MODE_TRIGGERED_TEMPERATURE_BUS_VOLTAGE:
	case INA700_OPER_MODE_TRIGGERED_TEMPERATURE_CURRENT:
	case INA700_OPER_MODE_TRIGGERED_TEMPERATURE_CURRENT_BUS_VOLTAGE:
		return true;
	default:
		return false;
	}
}

/**
 * @brief initializes the INA700 sensor
 * @param dev the device to inititalize
 * @return 0 if successful, negative errno code if failure.
 */
static int ina700_init(const struct device *dev)
{
	struct ina700_data *data = dev->data;
	const struct ina700_config *config = dev->config;
	uint16_t id;
	int ret;

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C bus %s is not ready", config->bus.bus->name);
		return -ENODEV;
	}

	data->dev = dev;

	ret = ina700_reg_read_16(&config->bus, INA700_REG_MANUFACTURER_ID, &id);
	if (ret < 0) {
		LOG_ERR("Failed to read manufacturer register!");
		return ret;
	}

	if (id != INA700_MANUFACTURER_ID) {
		LOG_ERR("Manufacturer ID doesn't match!");
		return -ENODEV;
	}

	ret = ina700_reg_write(&config->bus, INA700_REG_ADC_CONFIG, config->adc_config);
	if (ret < 0) {
		LOG_ERR("Failed to write ADC configuration register!");
		return ret;
	}

	ret = ina700_reg_write(&config->bus, INA700_REG_CONFIG, config->config);
	if (ret < 0) {
		LOG_ERR("Failed to write configuration register!");
		return ret;
	}

	if (ina700_is_triggered_mode_set(dev)) {
		if ((config->alert_diag & GENMASK(15, 14)) != GENMASK(15, 14)) {
			LOG_ERR("ALATCH and CNVR bits must be enabled in triggered mode!");
			return -ENODEV;
		}

		ret = ina700_reg_write(&config->bus, INA700_REG_COL,
				       config->current_over_limit_threshold);
		if (ret < 0) {
			LOG_ERR("Failed to write current over limit threshold register!");
			return ret;
		}

		ret = ina700_reg_write(&config->bus, INA700_REG_CUL,
				       config->current_under_limit_threshold);
		if (ret < 0) {
			LOG_ERR("Failed to write current under limit threshold register!");
			return ret;
		}

		ret = ina700_reg_write(&config->bus, INA700_REG_BOVL,
				       config->bus_overvoltage_threshold);
		if (ret < 0) {
			LOG_ERR("Failed to write bus overvoltage threshold register!");
			return ret;
		}

		ret = ina700_reg_write(&config->bus, INA700_REG_BUVL,
				       config->bus_undervoltage_threshold);
		if (ret < 0) {
			LOG_ERR("Failed to write bus undervoltage threshold register!");
			return ret;
		}

		ret = ina700_reg_write(&config->bus, INA700_REG_TEMP_LIMIT,
				       config->temperature_over_limit_threshold);
		if (ret < 0) {
			LOG_ERR("Failed to write temperature over limit threshold register!");
			return ret;
		}

		ret = ina700_reg_write(&config->bus, INA700_REG_PWR_LIMIT,
				       config->power_over_limit_threshold);
		if (ret < 0) {
			LOG_ERR("Failed to write power over limit threshold register!");
			return ret;
		}

		k_work_init(&data->trigger.conversion_work, ina700_trigger_work_handler);

		ret = ina700_trigger_mode_init(&data->trigger, &config->alert_gpio);
		if (ret < 0) {
			LOG_ERR("Failed to init trigger mode");
			return ret;
		}

		ret = ina700_reg_write(&config->bus, INA700_REG_ALERT_DIAG, config->alert_diag);
		if (ret < 0) {
			LOG_ERR("Failed to write alert configuration register!");
			return ret;
		}
	}

	return 0;
}

/**
 * @brief reads 16 bits from the selected register on the selected i2c bus.
 * @param bus the complete I2C DT information
 * @param reg the register address
 * @param val the value to store the read data into
 * @return 0 if successful, negative errno code if failure.
 */
int ina700_reg_read_16(const struct i2c_dt_spec *bus, uint8_t reg, uint16_t *val)
{
	uint8_t data[2];
	int ret;

	ret = i2c_burst_read_dt(bus, reg, data, sizeof(data));
	if (ret < 0) {
		return ret;
	}

	*val = sys_get_be16(data);

	return ret;
}

/**
 * @brief reads 24 bits from the selected register on the selected i2c bus.
 * @param bus the complete I2C DT information
 * @param reg the register address
 * @param val the value to store the read data into
 * @return 0 if successful, negative errno code if failure.
 */
int ina700_reg_read_24(const struct i2c_dt_spec *bus, uint8_t reg, uint32_t *val)
{
	uint8_t data[3];
	int ret;

	ret = i2c_burst_read_dt(bus, reg, data, sizeof(data));
	if (ret < 0) {
		return ret;
	}

	*val = sys_get_be24(data);

	return ret;
}

/**
 * @brief reads 40 bits from the selected register on the selected i2c bus.
 * @param bus the complete I2C DT information
 * @param reg the register address
 * @param val the value to store the read data into
 * @return 0 if successful, negative errno code if failure.
 */
int ina700_reg_read_40(const struct i2c_dt_spec *bus, uint8_t reg, uint64_t *val)
{
	uint8_t data[5];
	int ret;
	uint64_t tmp_val;

	ret = i2c_burst_read_dt(bus, reg, data, sizeof(data));
	if (ret < 0) {
		return ret;
	}

	tmp_val = sys_get_be40(data);
	/*
	 * As the charge can be negative, the sign needs to be preserved when the 40 bits
	 * are placed in the 64 bit variable.
	 * This is done by using a 64 bit bitmask with the leftmost 24 bits filled with 1's.
	 * This bitmask value is then bitwise AND with the negative tmp_val contents
	 * that have been bitshifted 39 places to the right.
	 * This value is either 0 if the tmp_val is positive or -1 if the tmp_val is negative.
	 * The & operation will then turn the 1's back to 0s or keep them as is respectively.
	 * The | bitwise operation will then fill the empty spots before the data,
	 * with either the 0' s or the 1' s. Thus preserving the sign before the data
	 */
	*val = (((0xffffff0000000000) & (-(tmp_val >> (BIT40_SIGN_BIT_POSITION)))) | tmp_val);

	return ret;
}

/**
 * @brief Writes the specified value to the selected register on the specified i2c bus.
 * @param bus the complete I2C DT information
 * @param reg the register address
 * @param val the value to write to the register
 */
int ina700_reg_write(const struct i2c_dt_spec *bus, uint8_t reg, uint16_t val)
{
	uint8_t tx_buf[3];

	tx_buf[0] = reg;
	sys_put_be16(val, &tx_buf[1]);

	return i2c_write_dt(bus, tx_buf, sizeof(tx_buf));
}

/**
 * @brief Activate a sensor's trigger and set the trigger handler
 * @param dev Pointer to the sensor device
 * @param trig The trigger to activate
 * @param handler The function that should be called when the trigger
 * fires
 *
 * @return 0 if successful, negative errno code if failure.
 */
static int ina700_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			      sensor_trigger_handler_t handler)
{
	ARG_UNUSED(trig);
	struct ina700_data *ina700 = dev->data;

	if (!ina700_is_triggered_mode_set(dev)) {
		return -ENOTSUP;
	}

	ina700->trigger.handler_alert = handler;
	ina700->trigger.trig_alert = trig;

	return 0;
}

static const struct sensor_driver_api ina700_driver_api = {
	.attr_set = ina700_attr_set,
	.attr_get = ina700_attr_get,
	.trigger_set = ina700_trigger_set,
	.sample_fetch = ina700_sample_fetch,
	.channel_get = ina700_channel_get,
};

#define INA700_CFG_IRQ(inst)                                                                       \
	.trig_enabled = true, .mask = DT_INST_PROP(inst, mask),                                    \
	.alert_limit = DT_INST_PROP(inst, alert_limit),                                            \
	.alert_gpio = GPIO_DT_SPEC_INST_GET(inst, alert_gpios)

#define INA700_DRIVER_INIT(inst)                                                                   \
	static struct ina700_data drv_data_##inst;                                                 \
	static const struct ina700_config drv_config_##inst = {                                    \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.config = (DT_INST_PROP(inst, conversion_delay) << 6),                             \
		.adc_config = (DT_INST_ENUM_IDX(inst, adc_mode) << 12) |                           \
			      (DT_INST_ENUM_IDX(inst, vbus_conversion_time_us) << 9) |             \
			      (DT_INST_ENUM_IDX(inst, vshunt_conversion_time_us) << 6) |           \
			      (DT_INST_ENUM_IDX(inst, temperature_conversion_time_us) << 3) |      \
			      DT_INST_ENUM_IDX(inst, adc_mode),                                    \
		.alert_diag = (DT_INST_ENUM_IDX(inst, alatch) << 15) |                             \
			      (DT_INST_ENUM_IDX(inst, conversion_ready) << 14) |                   \
			      (DT_INST_ENUM_IDX(inst, slow_alert) << 13) |                         \
			      (DT_INST_ENUM_IDX(inst, alert_polarity) << 12),                      \
		.current_over_limit_threshold = DT_INST_PROP(inst, current_over_limit_threshold),  \
		.current_under_limit_threshold =                                                   \
			DT_INST_PROP(inst, current_under_limit_threshold),                         \
		.bus_overvoltage_threshold = DT_INST_PROP(inst, bus_overvoltage_threshold),        \
		.bus_undervoltage_threshold = DT_INST_PROP(inst, bus_undervoltage_threshold),      \
		.temperature_over_limit_threshold =                                                \
			DT_INST_PROP(inst, temperature_over_limit_threshold),                      \
		.power_over_limit_threshold = DT_INST_PROP(inst, power_over_limit_threshold),      \
		.alert_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, alert_gpios, {0}),                    \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &ina700_init, NULL, &drv_data_##inst,                   \
				     &drv_config_##inst, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
				     &ina700_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INA700_DRIVER_INIT)
