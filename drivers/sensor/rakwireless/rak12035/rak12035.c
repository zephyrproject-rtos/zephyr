/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT rakwireless_rak12035

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/rak12035.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "rak12035_reg.h"

LOG_MODULE_REGISTER(RAK12035, CONFIG_SENSOR_LOG_LEVEL);

/*
 * Match the Arduino RAK12035 library: command STOP, then wait before read.
 * Capacitance conversion continues after the read returns the previous sample;
 * do not back-to-back double-read — that races the soft-I2C measurement.
 */
#define RAK12035_COMMAND_DELAY	K_MSEC(CONFIG_RAK12035_COMMAND_DELAY_MS)
#define RAK12035_RESET_TIME	K_MSEC(500)
#define RAK12035_POWER_SETTLE	K_MSEC(250)
#define RAK12035_BOOT_TIMEOUT	K_SECONDS(5)
#define RAK12035_BOOT_RETRY	K_MSEC(250)

struct rak12035_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec reset;
	const struct device *vin_supply;
};

struct rak12035_data {
	int16_t temperature;
	uint16_t capacitance;
	uint16_t dry_calibration;
	uint16_t wet_calibration;
	uint8_t moisture;
	uint8_t version;
};

static int rak12035_read(const struct device *dev, uint8_t command, uint8_t *buf, size_t len)
{
	const struct rak12035_config *config = dev->config;
	int ret;

	ret = i2c_write_dt(&config->i2c, &command, sizeof(command));
	if (ret < 0) {
		return ret;
	}

	k_sleep(RAK12035_COMMAND_DELAY);

	return i2c_read_dt(&config->i2c, buf, len);
}

static int rak12035_read_u8(const struct device *dev, uint8_t command, uint8_t *value)
{
	return rak12035_read(dev, command, value, sizeof(*value));
}

static int rak12035_read_be16(const struct device *dev, uint8_t command, uint16_t *value)
{
	uint8_t buf[2];
	int ret;

	ret = rak12035_read(dev, command, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	*value = sys_get_be16(buf);

	return 0;
}

static int rak12035_write_be16(const struct device *dev, uint8_t command, uint16_t value)
{
	const struct rak12035_config *config = dev->config;
	uint8_t buf[3] = {command};

	sys_put_be16(value, &buf[1]);

	return i2c_write_dt(&config->i2c, buf, sizeof(buf));
}

static int rak12035_calculate_moisture(uint16_t capacitance, uint16_t dry, uint16_t wet,
				      uint8_t *moisture)
{
	int32_t denominator = (int32_t)wet - dry;
	int32_t value;

	if (denominator == 0) {
		return -EINVAL;
	}

	if (dry < wet) {
		capacitance = CLAMP(capacitance, dry, wet);
	} else {
		capacitance = CLAMP(capacitance, wet, dry);
	}

	value = (100 * ((int32_t)capacitance - dry)) / denominator;
	*moisture = (uint8_t)CLAMP(value, 0, 100);

	return 0;
}

static int rak12035_fetch_temperature(const struct device *dev, int16_t *temperature)
{
	uint16_t raw;
	int ret;

	ret = rak12035_read_be16(dev, RAK12035_CMD_GET_TEMPERATURE, &raw);
	if (ret < 0) {
		return ret;
	}

	/* Firmware stores tenths of °C as a signed big-endian int16. */
	*temperature = (int16_t)raw;

	return 0;
}

static int rak12035_fetch_moisture(const struct device *dev, uint16_t *capacitance,
				   uint8_t *moisture, bool fetch_capacitance)
{
	struct rak12035_data *data = dev->data;
	uint16_t cap = data->capacitance;
	uint8_t humidity;
	int ret;

	if (fetch_capacitance || data->version < RAK12035_VERSION_DEVICE_HUMIDITY_MIN) {
		ret = rak12035_read_be16(dev, RAK12035_CMD_GET_CAPACITANCE, &cap);
		if (ret < 0) {
			return ret;
		}
	}

	if (data->version >= RAK12035_VERSION_DEVICE_HUMIDITY_MIN) {
		ret = rak12035_read_u8(dev, RAK12035_CMD_GET_HUMIDITY, &humidity);
		if (ret < 0) {
			return ret;
		}

		if (humidity > 100U) {
			return -ERANGE;
		}

		*moisture = humidity;
	} else {
		ret = rak12035_calculate_moisture(cap, data->dry_calibration,
						 data->wet_calibration, moisture);
		if (ret < 0) {
			return ret;
		}
	}

	*capacitance = cap;

	return 0;
}

static int rak12035_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct rak12035_data *data = dev->data;
	int16_t temperature = data->temperature;
	uint16_t capacitance = data->capacitance;
	uint8_t moisture = data->moisture;
	bool fetch_temperature;
	bool fetch_capacitance;
	bool fetch_moisture;
	int ret;

	fetch_temperature = chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP;
	fetch_capacitance =
		chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_RAK12035_CAPACITANCE_RAW;
	fetch_moisture = chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_HUMIDITY;

	if (!fetch_temperature && !fetch_capacitance && !fetch_moisture) {
		return -ENOTSUP;
	}

	if (fetch_temperature) {
		ret = rak12035_fetch_temperature(dev, &temperature);
		if (ret < 0) {
			return ret;
		}
	}

	if (fetch_moisture) {
		ret = rak12035_fetch_moisture(dev, &capacitance, &moisture, fetch_capacitance);
		if (ret < 0) {
			return ret;
		}
	} else if (fetch_capacitance) {
		ret = rak12035_read_be16(dev, RAK12035_CMD_GET_CAPACITANCE, &capacitance);
		if (ret < 0) {
			return ret;
		}
	}

	data->temperature = temperature;
	data->capacitance = capacitance;
	data->moisture = moisture;

	return 0;
}

static int rak12035_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	const struct rak12035_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		val->val1 = data->temperature / 10;
		val->val2 = (data->temperature % 10) * 100000;
		return 0;
	case SENSOR_CHAN_HUMIDITY:
		val->val1 = data->moisture;
		val->val2 = 0;
		return 0;
	case SENSOR_CHAN_RAK12035_CAPACITANCE_RAW:
		val->val1 = data->capacitance;
		val->val2 = 0;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int rak12035_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	struct rak12035_data *data = dev->data;
	uint16_t *cached;
	uint16_t readback;
	uint8_t set_command;
	uint8_t get_command;
	int ret;

	if (chan != SENSOR_CHAN_HUMIDITY) {
		return -ENOTSUP;
	}

	if (val->val1 < 0 || val->val1 > UINT16_MAX || val->val2 != 0) {
		return -EINVAL;
	}

	switch ((int)attr) {
	case SENSOR_ATTR_RAK12035_CALIBRATION_DRY:
		set_command = RAK12035_CMD_SET_HUMIDITY_ZERO;
		get_command = RAK12035_CMD_GET_HUMIDITY_ZERO;
		cached = &data->dry_calibration;
		break;
	case SENSOR_ATTR_RAK12035_CALIBRATION_WET:
		set_command = RAK12035_CMD_SET_HUMIDITY_FULL;
		get_command = RAK12035_CMD_GET_HUMIDITY_FULL;
		cached = &data->wet_calibration;
		break;
	default:
		return -ENOTSUP;
	}

	ret = rak12035_write_be16(dev, set_command, (uint16_t)val->val1);
	if (ret < 0) {
		return ret;
	}

	k_sleep(RAK12035_COMMAND_DELAY);

	ret = rak12035_read_be16(dev, get_command, &readback);
	if (ret < 0) {
		return ret;
	}

	if (readback != (uint16_t)val->val1) {
		LOG_ERR("Calibration readback mismatch: wrote %u, read %u",
			(uint16_t)val->val1, readback);
		return -EIO;
	}

	*cached = readback;

	return 0;
}

static int rak12035_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	const struct rak12035_data *data = dev->data;

	if (chan != SENSOR_CHAN_HUMIDITY) {
		return -ENOTSUP;
	}

	switch ((int)attr) {
	case SENSOR_ATTR_RAK12035_CALIBRATION_DRY:
		val->val1 = data->dry_calibration;
		break;
	case SENSOR_ATTR_RAK12035_CALIBRATION_WET:
		val->val1 = data->wet_calibration;
		break;
	default:
		return -ENOTSUP;
	}

	val->val2 = 0;

	return 0;
}

static DEVICE_API(sensor, rak12035_api) = {
	.attr_set = rak12035_attr_set,
	.attr_get = rak12035_attr_get,
	.sample_fetch = rak12035_sample_fetch,
	.channel_get = rak12035_channel_get,
};

static int rak12035_wait_ready(const struct device *dev, uint8_t *version)
{
	k_timepoint_t timeout = sys_timepoint_calc(RAK12035_BOOT_TIMEOUT);
	int ret;

	do {
		ret = rak12035_read_u8(dev, RAK12035_CMD_GET_VERSION, version);
		if (ret == 0) {
			return 0;
		}

		k_sleep(RAK12035_BOOT_RETRY);
	} while (!sys_timepoint_expired(timeout));

	return ret;
}

static int rak12035_init(const struct device *dev)
{
	const struct rak12035_config *config = dev->config;
	struct rak12035_data *data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus is not ready");
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_REGULATOR) && config->vin_supply != NULL) {
		if (!device_is_ready(config->vin_supply)) {
			LOG_ERR("VIN regulator is not ready");
			return -ENODEV;
		}

		ret = regulator_enable(config->vin_supply);
		if (ret < 0) {
			LOG_ERR("Failed to enable VIN supply: %d", ret);
			return ret;
		}

		/* Match Arduino sensor_on(): wait after WB_IO2 / 3V3_S rises. */
		k_sleep(RAK12035_POWER_SETTLE);
	}

	if (config->reset.port != NULL) {
		if (!gpio_is_ready_dt(&config->reset)) {
			LOG_ERR("Reset GPIO is not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure reset GPIO: %d", ret);
			return ret;
		}

		ret = gpio_pin_set_dt(&config->reset, 1);
		if (ret < 0) {
			LOG_ERR("Failed to assert reset: %d", ret);
			return ret;
		}

		k_sleep(RAK12035_RESET_TIME);

		ret = gpio_pin_set_dt(&config->reset, 0);
		if (ret < 0) {
			LOG_ERR("Failed to deassert reset: %d", ret);
			return ret;
		}
	}

	ret = rak12035_wait_ready(dev, &data->version);
	if (ret < 0) {
		LOG_ERR("Sensor did not become ready (I2C addr 0x%02x): %d",
			config->i2c.addr, ret);
		return ret;
	}

	if (config->reset.port != NULL) {
		k_sleep(RAK12035_RESET_TIME);
	}

	if (data->version < RAK12035_VERSION_DEVICE_HUMIDITY_MIN) {
		ret = rak12035_read_be16(dev, RAK12035_CMD_GET_HUMIDITY_ZERO,
					&data->dry_calibration);
		if (ret < 0) {
			LOG_ERR("Failed to read dry calibration: %d", ret);
			return ret;
		}

		ret = rak12035_read_be16(dev, RAK12035_CMD_GET_HUMIDITY_FULL,
					&data->wet_calibration);
		if (ret < 0) {
			LOG_ERR("Failed to read wet calibration: %d", ret);
			return ret;
		}

		if (data->dry_calibration == data->wet_calibration ||
		    data->dry_calibration == 0U || data->wet_calibration == 0U) {
			LOG_WRN("Invalid soil moisture calibration (dry=%u wet=%u); "
				"calibrate with the vendor tool before using humidity",
				data->dry_calibration, data->wet_calibration);
		}

		LOG_INF("FW 0x%02x, dry_cal=%u, wet_cal=%u", data->version,
			data->dry_calibration, data->wet_calibration);
	} else {
		LOG_INF("Detected RAK12035 firmware version 0x%02x", data->version);
	}

	return 0;
}

#define RAK12035_VIN_SUPPLY(inst)                                                                \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, vin_supply),                                       \
		    (DEVICE_DT_GET(DT_INST_PHANDLE(inst, vin_supply))), (NULL))

#define RAK12035_DEFINE(inst)                                                                    \
	static struct rak12035_data rak12035_data_##inst;                                         \
                                                                                                   \
	static const struct rak12035_config rak12035_config_##inst = {                            \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                         \
		.vin_supply = RAK12035_VIN_SUPPLY(inst),                                           \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, rak12035_init, NULL,                                    \
				     &rak12035_data_##inst, &rak12035_config_##inst, POST_KERNEL,  \
				     CONFIG_SENSOR_INIT_PRIORITY, &rak12035_api);

DT_INST_FOREACH_STATUS_OKAY(RAK12035_DEFINE)
