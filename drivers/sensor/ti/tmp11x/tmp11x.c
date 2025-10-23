/*
 * Copyright (c) 2019 Centaur Analytics, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmp11x

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/sensor/tmp11x.h>
#include <zephyr/dt-bindings/sensor/tmp11x.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include "tmp11x.h"

#define EEPROM_SIZE_REG        sizeof(uint16_t)
#define EEPROM_TMP117_RESERVED (2 * sizeof(uint16_t))
#define EEPROM_MIN_BUSY_MS     7
#define RESET_MIN_BUSY_MS      2

LOG_MODULE_REGISTER(TMP11X, CONFIG_SENSOR_LOG_LEVEL);

int tmp11x_reg_read(const struct device *dev, uint8_t reg, uint16_t *val)
{
	const struct tmp11x_dev_config *cfg = dev->config;

	if (i2c_burst_read_dt(&cfg->bus, reg, (uint8_t *)val, 2) < 0) {
		return -EIO;
	}

	*val = sys_be16_to_cpu(*val);

	return 0;
}

int tmp11x_reg_write(const struct device *dev, uint8_t reg, uint16_t val)
{
	const struct tmp11x_dev_config *cfg = dev->config;
	uint8_t tx_buf[3] = {reg, val >> 8, val & 0xFF};

	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

int tmp11x_write_config(const struct device *dev, uint16_t mask, uint16_t conf)
{
	uint16_t config = 0;
	int result;

	result = tmp11x_reg_read(dev, TMP11X_REG_CFGR, &config);

	if (result < 0) {
		return result;
	}

	config &= ~mask;
	config |= conf;

	return tmp11x_reg_write(dev, TMP11X_REG_CFGR, config);
}

static inline bool tmp11x_is_offset_supported(const struct tmp11x_data *drv_data)
{
	return drv_data->id == TMP117_DEVICE_ID || drv_data->id == TMP119_DEVICE_ID;
}

/**
 * @brief Convert sensor_value temperature to TMP11X register format
 *
 * This function converts a temperature from sensor_value format (val1 in degrees C,
 * val2 in micro-degrees C) to the TMP11X register format. It uses 64-bit arithmetic
 * to prevent overflow and clamps the result to the valid int16_t range.
 *
 * @param val Pointer to sensor_value containing temperature
 * @return Temperature value in TMP11X register format (int16_t)
 */
static inline int16_t tmp11x_sensor_value_to_reg_format(const struct sensor_value *val)
{
	int64_t temp_micro = ((int64_t)val->val1 * 1000000) + val->val2;
	int64_t temp_scaled = (temp_micro * 10) / TMP11X_RESOLUTION;

	/* Clamp to int16_t range */
	if (temp_scaled > INT16_MAX) {
		return INT16_MAX;
	} else if (temp_scaled < INT16_MIN) {
		return INT16_MIN;
	} else {
		return (int16_t)temp_scaled;
	}
}

static bool check_eeprom_bounds(const struct device *dev, off_t offset, size_t len)
{
	struct tmp11x_data *drv_data = dev->data;

	if ((offset + len) > EEPROM_TMP11X_SIZE || offset % EEPROM_SIZE_REG != 0 ||
	    len % EEPROM_SIZE_REG != 0) {
		return false;
	}

	/* TMP117 and TMP119 uses EEPROM[2] as temperature offset register */
	if ((drv_data->id == TMP117_DEVICE_ID || drv_data->id == TMP119_DEVICE_ID) &&
	    offset <= EEPROM_TMP117_RESERVED && (offset + len) > EEPROM_TMP117_RESERVED) {
		return false;
	}

	return true;
}

int tmp11x_eeprom_await(const struct device *dev)
{
	int res;
	uint16_t val;

	k_sleep(K_MSEC(EEPROM_MIN_BUSY_MS));

	WAIT_FOR((res = tmp11x_reg_read(dev, TMP11X_REG_EEPROM_UL, &val)) != 0 ||
			 val & TMP11X_EEPROM_UL_BUSY,
		 100, k_msleep(1));

	return res;
}

int tmp11x_eeprom_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	uint8_t reg;
	const uint16_t *src = data;
	int res;

	if (!check_eeprom_bounds(dev, offset, len)) {
		return -EINVAL;
	}

	res = tmp11x_reg_write(dev, TMP11X_REG_EEPROM_UL, TMP11X_EEPROM_UL_UNLOCK);
	if (res) {
		return res;
	}

	for (reg = (offset / 2); reg < offset / 2 + len / 2; reg++) {
		uint16_t val = *src;

		res = tmp11x_reg_write(dev, reg + TMP11X_REG_EEPROM1, val);
		if (res != 0) {
			break;
		}

		res = tmp11x_eeprom_await(dev);
		src++;

		if (res != 0) {
			break;
		}
	}

	res = tmp11x_reg_write(dev, TMP11X_REG_EEPROM_UL, 0);

	return res;
}

int tmp11x_eeprom_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	uint8_t reg;
	uint16_t *dst = data;
	int res = 0;

	if (!check_eeprom_bounds(dev, offset, len)) {
		return -EINVAL;
	}

	for (reg = (offset / 2); reg < offset / 2 + len / 2; reg++) {
		res = tmp11x_reg_read(dev, reg + TMP11X_REG_EEPROM1, dst);
		if (res != 0) {
			break;
		}
		dst++;
	}

	return res;
}

/**
 * @brief Check the Device ID
 *
 * @param[in]   dev     Pointer to the device structure
 * @param[in]	id	Pointer to the variable for storing the device id
 *
 * @retval 0 on success
 * @retval -EIO Otherwise
 */
static inline int tmp11x_device_id_check(const struct device *dev, uint16_t *id)
{
	if (tmp11x_reg_read(dev, TMP11X_REG_DEVICE_ID, id) != 0) {
		LOG_ERR("%s: Failed to get Device ID register!", dev->name);
		return -EIO;
	}

	if ((*id != TMP116_DEVICE_ID) && (*id != TMP117_DEVICE_ID) && (*id != TMP119_DEVICE_ID)) {
		LOG_ERR("%s: Failed to match the device IDs!", dev->name);
		return -EINVAL;
	}

	return 0;
}

static int tmp11x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct tmp11x_data *drv_data = dev->data;
	uint16_t value;
	uint16_t cfg_reg = 0;
	int rc;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP);

	/* clear sensor values */
	drv_data->sample = 0U;

	/* Make sure that a data is available */
	rc = tmp11x_reg_read(dev, TMP11X_REG_CFGR, &cfg_reg);
	if (rc < 0) {
		LOG_ERR("%s, Failed to read from CFGR register", dev->name);
		return rc;
	}

	if ((cfg_reg & TMP11X_CFGR_DATA_READY) == 0) {
		LOG_DBG("%s: no data ready", dev->name);
		return -EBUSY;
	}

	/* Get the most recent temperature measurement */
	rc = tmp11x_reg_read(dev, TMP11X_REG_TEMP, &value);
	if (rc < 0) {
		LOG_ERR("%s: Failed to read from TEMP register!", dev->name);
		return rc;
	}

	/* store measurements to the driver */
	drv_data->sample = (int16_t)value;

	return 0;
}

/*
 * See datasheet "Temperature Results and Limits" section for more
 * details on processing sample data.
 */
static void tmp11x_temperature_to_sensor_value(int16_t temperature, struct sensor_value *val)
{
	int32_t tmp;

	tmp = (temperature * (int32_t)TMP11X_RESOLUTION) / 10;
	val->val1 = tmp / 1000000; /* uCelsius */
	val->val2 = tmp % 1000000;
}

static int tmp11x_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct tmp11x_data *drv_data = dev->data;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	tmp11x_temperature_to_sensor_value(drv_data->sample, val);

	return 0;
}

static int16_t tmp11x_conv_value(const struct sensor_value *val)
{
	uint32_t freq_micro = sensor_value_to_micro(val);

	switch (freq_micro) {
	case 64000000: /* 1 / 15.5 ms has been rounded down */
		return TMP11X_DT_ODR_15_5_MS;
	case 8000000:
		return TMP11X_DT_ODR_125_MS;
	case 4000000:
		return TMP11X_DT_ODR_250_MS;
	case 2000000:
		return TMP11X_DT_ODR_500_MS;
	case 1000000:
		return TMP11X_DT_ODR_1000_MS;
	case 250000:
		return TMP11X_DT_ODR_4000_MS;
	case 125000:
		return TMP11X_DT_ODR_8000_MS;
	case 62500:
		return TMP11X_DT_ODR_16000_MS;
	default:
		LOG_ERR("%" PRIu32 " uHz not supported", freq_micro);
		return -EINVAL;
	}
}

static bool tmp11x_is_attr_store_supported(enum sensor_attribute attr)
{
	switch ((int)attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
	case SENSOR_ATTR_LOWER_THRESH:
	case SENSOR_ATTR_UPPER_THRESH:
	case SENSOR_ATTR_OFFSET:
	case SENSOR_ATTR_OVERSAMPLING:
	case SENSOR_ATTR_TMP11X_SHUTDOWN_MODE:
	case SENSOR_ATTR_TMP11X_CONTINUOUS_CONVERSION_MODE:
	case SENSOR_ATTR_TMP11X_ALERT_PIN_POLARITY:
	case SENSOR_ATTR_TMP11X_ALERT_MODE:
		return true;
	case SENSOR_ATTR_TMP11X_ONE_SHOT_MODE:
		return false;
	}

	return false;
}

static int tmp11x_attr_store_reload(const struct device *dev)
{
	int await_res = tmp11x_eeprom_await(dev);
	int reset_res = tmp11x_reg_write(dev, TMP11X_REG_CFGR, TMP11X_CFGR_RESET);

	k_sleep(K_MSEC(RESET_MIN_BUSY_MS));

	return await_res != 0 ? await_res : reset_res;
}

static int tmp11x_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct tmp11x_dev_config *cfg = dev->config;
	struct tmp11x_data *drv_data = dev->data;
	int16_t value;
	int res = 0;
	bool store;
	int store_res = 0;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	store = cfg->store_attr_values && tmp11x_is_attr_store_supported(attr);
	if (store) {
		store_res = tmp11x_reg_write(dev, TMP11X_REG_EEPROM_UL, TMP11X_EEPROM_UL_UNLOCK);
		if (store_res != 0) {
			return store_res;
		}
	}

	switch ((int)attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		value = tmp11x_conv_value(val);
		if (value < 0) {
			return value;
		}

		res = tmp11x_write_config(dev, TMP11X_CFGR_CONV, value);
		break;

	case SENSOR_ATTR_OFFSET:
		if (!tmp11x_is_offset_supported(drv_data)) {
			LOG_ERR("%s: Offset is not supported", dev->name);
			return -EINVAL;
		}
		/*
		 * The offset is encoded into the temperature register format.
		 */
		value = tmp11x_sensor_value_to_reg_format(val);

		res = tmp11x_reg_write(dev, TMP117_REG_TEMP_OFFSET, value);
		break;

	case SENSOR_ATTR_OVERSAMPLING:
		/* sensor supports averaging 1, 8, 32 and 64 samples */
		switch (val->val1) {
		case 1:
			value = TMP11X_AVG_1_SAMPLE;
			break;

		case 8:
			value = TMP11X_AVG_8_SAMPLES;
			break;

		case 32:
			value = TMP11X_AVG_32_SAMPLES;
			break;

		case 64:
			value = TMP11X_AVG_64_SAMPLES;
			break;

		default:
			res = -EINVAL;
			break;
		}

		if (res == 0) {
			res = tmp11x_write_config(dev, TMP11X_CFGR_AVG, value);
		}

		break;

	case SENSOR_ATTR_TMP11X_SHUTDOWN_MODE:
		res = tmp11x_write_config(dev, TMP11X_CFGR_MODE, TMP11X_MODE_SHUTDOWN);
		break;

	case SENSOR_ATTR_TMP11X_CONTINUOUS_CONVERSION_MODE:
		res = tmp11x_write_config(dev, TMP11X_CFGR_MODE, TMP11X_MODE_CONTINUOUS);
		break;

	case SENSOR_ATTR_TMP11X_ONE_SHOT_MODE:
		res = tmp11x_write_config(dev, TMP11X_CFGR_MODE, TMP11X_MODE_ONE_SHOT);
		break;

#ifdef CONFIG_TMP11X_TRIGGER
	case SENSOR_ATTR_TMP11X_ALERT_PIN_POLARITY:
		if (val->val1 == TMP11X_ALERT_PIN_ACTIVE_HIGH) {
			res = tmp11x_write_config(dev, TMP11X_CFGR_ALERT_PIN_POL,
						  TMP11X_CFGR_ALERT_PIN_POL);
		} else {
			res = tmp11x_write_config(dev, TMP11X_CFGR_ALERT_PIN_POL, 0);
		}
		break;

	case SENSOR_ATTR_TMP11X_ALERT_MODE:
		if (val->val1 == TMP11X_ALERT_THERM_MODE) {
			res = tmp11x_write_config(dev, TMP11X_CFGR_ALERT_MODE,
						  TMP11X_CFGR_ALERT_MODE);
		} else {
			res = tmp11x_write_config(dev, TMP11X_CFGR_ALERT_MODE, 0);
		}
		break;

	case SENSOR_ATTR_UPPER_THRESH:
		/* Convert temperature to register format */
		value = tmp11x_sensor_value_to_reg_format(val);
		res = tmp11x_reg_write(dev, TMP11X_REG_HIGH_LIM, value);
		break;

	case SENSOR_ATTR_LOWER_THRESH:
		/* Convert temperature to register format */
		value = tmp11x_sensor_value_to_reg_format(val);
		res = tmp11x_reg_write(dev, TMP11X_REG_LOW_LIM, value);
		break;

	case SENSOR_ATTR_TMP11X_ALERT_PIN_SELECT:
		if (val->val1 == TMP11X_ALERT_PIN_ALERT_SEL) {
			res = tmp11x_write_config(dev, TMP11X_CFGR_ALERT_DR_SEL, 0);
		} else {
			res = tmp11x_write_config(dev, TMP11X_CFGR_ALERT_DR_SEL,
						  TMP11X_CFGR_ALERT_DR_SEL);
		}
		break;
#endif /* CONFIG_TMP11X_TRIGGER */

	default:
		res = -ENOTSUP;
		break;
	}

	if (store) {
		store_res = tmp11x_attr_store_reload(dev);
	}

	return res != 0 ? res : store_res;
}

static int tmp11x_attr_get(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, struct sensor_value *val)
{
	uint16_t data;
	int rc;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_CONFIGURATION:
		rc = tmp11x_reg_read(dev, TMP11X_REG_CFGR, &data);

		if (rc == 0) {
			val->val1 = data;
			val->val2 = 0;
		}

		return rc;

	case SENSOR_ATTR_OFFSET:
		if (!tmp11x_is_offset_supported(dev->data)) {
			LOG_ERR("%s: Offset is not supported", dev->name);
			return -EINVAL;
		}

		rc = tmp11x_reg_read(dev, TMP117_REG_TEMP_OFFSET, &data);
		if (rc == 0) {
			tmp11x_temperature_to_sensor_value(data, val);
		}

		return rc;

#ifdef CONFIG_TMP11X_TRIGGER
	case SENSOR_ATTR_UPPER_THRESH:
		rc = tmp11x_reg_read(dev, TMP11X_REG_HIGH_LIM, &data);
		if (rc == 0) {
			tmp11x_temperature_to_sensor_value(data, val);
		}

		return rc;

	case SENSOR_ATTR_LOWER_THRESH:
		rc = tmp11x_reg_read(dev, TMP11X_REG_LOW_LIM, &data);
		if (rc == 0) {
			tmp11x_temperature_to_sensor_value(data, val);
		}

		return rc;
#endif /* CONFIG_TMP11X_TRIGGER */

	default:
		return -ENOTSUP;
	}
}

static DEVICE_API(sensor, tmp11x_driver_api) = {
	.attr_set = tmp11x_attr_set,
	.attr_get = tmp11x_attr_get,
	.sample_fetch = tmp11x_sample_fetch,
	.channel_get = tmp11x_channel_get,
#ifdef CONFIG_TMP11X_TRIGGER
	.trigger_set = tmp11x_trigger_set,
#endif
};

static int tmp11x_init(const struct device *dev)
{
	struct tmp11x_data *drv_data = dev->data;
	const struct tmp11x_dev_config *cfg = dev->config;
	int rc;
	uint16_t id;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("I2C dev %s not ready", cfg->bus.bus->name);
		return -EINVAL;
	}

	/* Check the Device ID */
	rc = tmp11x_device_id_check(dev, &id);
	if (rc < 0) {
		return rc;
	}
	LOG_DBG("Got device ID: %x", id);
	drv_data->id = id;

	rc = tmp11x_write_config(dev, TMP11X_CFGR_CONV, cfg->odr);
	if (rc < 0) {
		return rc;
	}

	rc = tmp11x_write_config(dev, TMP11X_CFGR_AVG, cfg->oversampling);
	if (rc < 0) {
		return rc;
	}

	int8_t value = cfg->alert_pin_polarity ? TMP11X_CFGR_ALERT_PIN_POL : 0;

	rc = tmp11x_write_config(dev, TMP11X_CFGR_ALERT_PIN_POL, value);
	if (rc < 0) {
		return rc;
	}

	value = cfg->alert_mode ? TMP11X_CFGR_ALERT_MODE : 0;
	rc = tmp11x_write_config(dev, TMP11X_CFGR_ALERT_MODE, value);
	if (rc < 0) {
		return rc;
	}

	value = cfg->alert_dr_sel ? TMP11X_CFGR_ALERT_DR_SEL : 0;
	rc = tmp11x_write_config(dev, TMP11X_CFGR_ALERT_DR_SEL, value);
	if (rc < 0) {
		return rc;
	}

#ifdef CONFIG_TMP11X_TRIGGER
	drv_data->dev = dev;
	rc = tmp11x_init_interrupt(dev);
	if (rc < 0) {
		LOG_ERR("%s: Failed to initialize alert pin", dev->name);
		return rc;
	}
#endif /* CONFIG_TMP11X_TRIGGER */

	return rc;
}

#ifdef CONFIG_PM_DEVICE
BUILD_ASSERT(!DT_INST_NODE_HAS_PROP(_num, power_domains), "Driver does not support power domain");
static int tmp11x_pm_control(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME: {
		const struct tmp11x_dev_config *cfg = dev->config;

		ret = tmp11x_write_config(dev, TMP11X_CFGR_CONV, cfg->odr);
		if (ret < 0) {
			LOG_ERR("Failed to resume TMP11X");
		}
		break;
	}
	case PM_DEVICE_ACTION_SUSPEND: {
		ret = tmp11x_write_config(dev, TMP11X_CFGR_MODE, TMP11X_MODE_SHUTDOWN);
		if (ret < 0) {
			LOG_ERR("Failed to suspend TMP11X");
		}
		break;
	}
	default:
		ret = -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

#ifdef CONFIG_TMP11X_TRIGGER
#define DEFINE_TMP11X_TRIGGER(_num) .alert_gpio = GPIO_DT_SPEC_INST_GET_OR(_num, alert_gpios, {}),
#else
#define DEFINE_TMP11X_TRIGGER(_num)
#endif

#define DEFINE_TMP11X(_num)                                                                        \
	static struct tmp11x_data tmp11x_data_##_num;                                              \
	static const struct tmp11x_dev_config tmp11x_config_##_num = {                             \
		.bus = I2C_DT_SPEC_INST_GET(_num),                                                 \
		.odr = DT_INST_PROP(_num, odr),                                                    \
		.oversampling = DT_INST_PROP(_num, oversampling),                                  \
		.alert_pin_polarity = DT_INST_PROP(_num, alert_polarity),                          \
		.alert_mode = DT_INST_PROP(_num, alert_mode),                                      \
		.alert_dr_sel = DT_INST_PROP(_num, alert_dr_sel),                                  \
		.store_attr_values = DT_INST_PROP(_num, store_attr_values),                        \
		DEFINE_TMP11X_TRIGGER(_num)};                                                      \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(_num, tmp11x_pm_control);                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(_num, tmp11x_init, PM_DEVICE_DT_INST_GET(_num),               \
				     &tmp11x_data_##_num, &tmp11x_config_##_num, POST_KERNEL,      \
				     CONFIG_SENSOR_INIT_PRIORITY, &tmp11x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_TMP11X)
