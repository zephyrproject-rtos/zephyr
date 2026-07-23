/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensirion_scd30

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/scd30.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <string.h>

#include "scd30.h"

LOG_MODULE_REGISTER(SCD30, CONFIG_SENSOR_LOG_LEVEL);

static uint8_t scd30_crc(const uint8_t *data, size_t len)
{
	return crc8(data, len, SCD30_CRC_POLY, SCD30_CRC_INIT, false);
}

static int scd30_write_cmd(const struct device *dev, uint16_t cmd)
{
	const struct scd30_config *cfg = dev->config;
	uint8_t buf[2];
	int ret;

	sys_put_be16(cmd, buf);
	ret = i2c_write_dt(&cfg->bus, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	k_msleep(SCD30_CMD_DELAY_MS);
	return 0;
}

static int scd30_write_cmd_arg(const struct device *dev, uint16_t cmd, uint16_t arg)
{
	const struct scd30_config *cfg = dev->config;
	uint8_t buf[5];
	int ret;

	sys_put_be16(cmd, buf);
	sys_put_be16(arg, &buf[2]);
	buf[4] = scd30_crc(&buf[2], 2);

	ret = i2c_write_dt(&cfg->bus, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	k_msleep(SCD30_CMD_DELAY_MS);
	return 0;
}

static int scd30_read_words(const struct device *dev, uint8_t *rx_buf, size_t rx_len)
{
	const struct scd30_config *cfg = dev->config;
	int ret;

	/* SCD30 does not support repeated start: separate write then read. */
	ret = i2c_read_dt(&cfg->bus, rx_buf, rx_len);
	if (ret < 0) {
		LOG_ERR("Failed to read I2C data (%d)", ret);
		return ret;
	}

	for (size_t i = 0; i < rx_len; i += 3) {
		if (scd30_crc(&rx_buf[i], 2) != rx_buf[i + 2]) {
			LOG_ERR("Invalid CRC");
			return -EIO;
		}
	}

	return 0;
}

static int scd30_cmd_read_words(const struct device *dev, uint16_t cmd, uint8_t *rx_buf,
				size_t rx_len)
{
	int ret;

	ret = scd30_write_cmd(dev, cmd);
	if (ret < 0) {
		return ret;
	}

	return scd30_read_words(dev, rx_buf, rx_len);
}

static float scd30_be_float(const uint8_t *word_pair)
{
	uint8_t raw[4] = {word_pair[0], word_pair[1], word_pair[3], word_pair[4]};
	uint32_t u32 = sys_get_be32(raw);
	float value;

	memcpy(&value, &u32, sizeof(value));
	return value;
}

static int scd30_stop_measurement(const struct device *dev)
{
	struct scd30_data *data = dev->data;
	int ret;

	ret = scd30_write_cmd(dev, SCD30_CMD_STOP_PERIODIC_MEASUREMENT);
	if (ret < 0) {
		return ret;
	}

	k_msleep(SCD30_STOP_DELAY_MS);
	data->measuring = false;
	return 0;
}

static int scd30_start_measurement(const struct device *dev)
{
	struct scd30_data *data = dev->data;
	int ret;

	ret = scd30_write_cmd_arg(dev, SCD30_CMD_START_PERIODIC_MEASUREMENT,
				  data->ambient_pressure);
	if (ret < 0) {
		return ret;
	}

	data->measuring = true;
	return 0;
}

static int scd30_set_idle(const struct device *dev)
{
	struct scd30_data *data = dev->data;

	if (!data->measuring) {
		return 0;
	}

	return scd30_stop_measurement(dev);
}

static int scd30_data_ready_gpio(const struct device *dev, bool *ready)
{
	const struct scd30_config *cfg = dev->config;
	int level;

	*ready = false;

	if (cfg->ready_gpio.port == NULL) {
		return -ENOTSUP;
	}

	level = gpio_pin_get_dt(&cfg->ready_gpio);
	if (level < 0) {
		return level;
	}

	*ready = (level != 0);
	return 0;
}

static int scd30_data_ready_i2c(const struct device *dev, bool *ready)
{
	uint8_t rx[3];
	int ret;

	*ready = false;

	ret = scd30_cmd_read_words(dev, SCD30_CMD_GET_DATA_READY, rx, sizeof(rx));
	if (ret < 0) {
		return ret;
	}

	*ready = (sys_get_be16(rx) == 1);
	return 0;
}

static int scd30_data_ready(const struct device *dev, bool *ready)
{
	const struct scd30_config *cfg = dev->config;

	/* Prefer RDY pin when wired; otherwise use I2C get-data-ready. */
	if (cfg->ready_gpio.port != NULL) {
		return scd30_data_ready_gpio(dev, ready);
	}

	return scd30_data_ready_i2c(dev, ready);
}

static int scd30_read_sample(const struct device *dev)
{
	struct scd30_data *data = dev->data;
	uint8_t rx[18];
	int ret;

	ret = scd30_cmd_read_words(dev, SCD30_CMD_READ_MEASUREMENT, rx, sizeof(rx));
	if (ret < 0) {
		return ret;
	}

	data->co2_sample = scd30_be_float(&rx[0]);
	data->temp_sample = scd30_be_float(&rx[6]);
	data->humi_sample = scd30_be_float(&rx[12]);

	return 0;
}

static int scd30_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	bool ready;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_CO2 &&
	    chan != SENSOR_CHAN_AMBIENT_TEMP && chan != SENSOR_CHAN_HUMIDITY) {
		return -ENOTSUP;
	}

	ret = scd30_data_ready(dev, &ready);
	if (ret < 0) {
		return ret;
	}

	if (!ready) {
		return 0;
	}

	return scd30_read_sample(dev);
}

static int scd30_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	const struct scd30_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_CO2:
		return sensor_value_from_float(val, data->co2_sample);
	case SENSOR_CHAN_AMBIENT_TEMP:
		return sensor_value_from_float(val, data->temp_sample);
	case SENSOR_CHAN_HUMIDITY:
		return sensor_value_from_float(val, data->humi_sample);
	default:
		return -ENOTSUP;
	}
}

static int scd30_attr_set(const struct device *dev, enum sensor_channel chan,
			  enum sensor_attribute attr, const struct sensor_value *val)
{
	struct scd30_data *data = dev->data;
	int ret;

	ARG_UNUSED(chan);

	if (val->val1 < 0) {
		return -EINVAL;
	}

	switch ((enum sensor_attribute_scd30)attr) {
	case SENSOR_ATTR_SCD30_AMBIENT_PRESSURE: {
		uint16_t pressure = (uint16_t)val->val1;

		if (pressure != 0 &&
		    (pressure < SCD30_AMBIENT_PRESSURE_MIN ||
		     pressure > SCD30_AMBIENT_PRESSURE_MAX)) {
			return -EINVAL;
		}
		data->ambient_pressure = pressure;
		/* May be written while measuring; restarts continuous mode. */
		return scd30_start_measurement(dev);
	}
	case SENSOR_ATTR_SCD30_TEMPERATURE_OFFSET: {
		int32_t offset_cdeg;

		ret = scd30_set_idle(dev);
		if (ret < 0) {
			return ret;
		}
		offset_cdeg = val->val1 * 100 + val->val2 / 10000;
		if (offset_cdeg < 0 || offset_cdeg > UINT16_MAX) {
			return -EINVAL;
		}
		ret = scd30_write_cmd_arg(dev, SCD30_CMD_SET_TEMPERATURE_OFFSET,
					  (uint16_t)offset_cdeg);
		break;
	}
	case SENSOR_ATTR_SCD30_SENSOR_ALTITUDE:
		ret = scd30_set_idle(dev);
		if (ret < 0) {
			return ret;
		}
		if (val->val1 > UINT16_MAX) {
			return -EINVAL;
		}
		ret = scd30_write_cmd_arg(dev, SCD30_CMD_SET_ALTITUDE, (uint16_t)val->val1);
		break;
	case SENSOR_ATTR_SCD30_AUTOMATIC_CALIB_ENABLE:
		ret = scd30_set_idle(dev);
		if (ret < 0) {
			return ret;
		}
		if (val->val1 > 1) {
			return -EINVAL;
		}
		ret = scd30_write_cmd_arg(dev, SCD30_CMD_SET_ASC, (uint16_t)val->val1);
		break;
	case SENSOR_ATTR_SCD30_MEASUREMENT_INTERVAL:
		if (val->val1 < SCD30_MEASUREMENT_INTERVAL_MIN_S ||
		    val->val1 > SCD30_MEASUREMENT_INTERVAL_MAX_S) {
			return -EINVAL;
		}
		ret = scd30_set_idle(dev);
		if (ret < 0) {
			return ret;
		}
		ret = scd30_write_cmd_arg(dev, SCD30_CMD_SET_MEASUREMENT_INTERVAL,
					  (uint16_t)val->val1);
		break;
	default:
		return -ENOTSUP;
	}

	if (ret < 0) {
		return ret;
	}

	return scd30_start_measurement(dev);
}

static int scd30_attr_get(const struct device *dev, enum sensor_channel chan,
			  enum sensor_attribute attr, struct sensor_value *val)
{
	uint8_t rx[3];
	uint16_t word;
	int ret;
	bool was_measuring;
	struct scd30_data *data = dev->data;

	ARG_UNUSED(chan);

	was_measuring = data->measuring;

	switch ((enum sensor_attribute_scd30)attr) {
	case SENSOR_ATTR_SCD30_AMBIENT_PRESSURE:
		val->val1 = data->ambient_pressure;
		val->val2 = 0;
		return 0;
	case SENSOR_ATTR_SCD30_TEMPERATURE_OFFSET:
		ret = scd30_set_idle(dev);
		if (ret < 0) {
			return ret;
		}
		ret = scd30_cmd_read_words(dev, SCD30_CMD_SET_TEMPERATURE_OFFSET, rx, sizeof(rx));
		if (ret < 0) {
			goto out_restart;
		}
		word = sys_get_be16(rx);
		val->val1 = word / 100;
		val->val2 = (word % 100) * 10000;
		break;
	case SENSOR_ATTR_SCD30_SENSOR_ALTITUDE:
		ret = scd30_set_idle(dev);
		if (ret < 0) {
			return ret;
		}
		ret = scd30_cmd_read_words(dev, SCD30_CMD_SET_ALTITUDE, rx, sizeof(rx));
		if (ret < 0) {
			goto out_restart;
		}
		val->val1 = sys_get_be16(rx);
		val->val2 = 0;
		break;
	case SENSOR_ATTR_SCD30_AUTOMATIC_CALIB_ENABLE:
		ret = scd30_set_idle(dev);
		if (ret < 0) {
			return ret;
		}
		ret = scd30_cmd_read_words(dev, SCD30_CMD_SET_ASC, rx, sizeof(rx));
		if (ret < 0) {
			goto out_restart;
		}
		val->val1 = sys_get_be16(rx);
		val->val2 = 0;
		break;
	case SENSOR_ATTR_SCD30_MEASUREMENT_INTERVAL:
		ret = scd30_set_idle(dev);
		if (ret < 0) {
			return ret;
		}
		ret = scd30_cmd_read_words(dev, SCD30_CMD_SET_MEASUREMENT_INTERVAL, rx, sizeof(rx));
		if (ret < 0) {
			goto out_restart;
		}
		val->val1 = sys_get_be16(rx);
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

out_restart:
	if (was_measuring) {
		int start_ret = scd30_start_measurement(dev);

		if (ret == 0) {
			ret = start_ret;
		}
	}

	return ret;
}

int scd30_forced_recalibration(const struct device *dev, uint16_t target_concentration)
{
	bool was_measuring;
	struct scd30_data *data = dev->data;
	int ret;

	if (target_concentration < SCD30_FRC_CONCENTRATION_MIN ||
	    target_concentration > SCD30_FRC_CONCENTRATION_MAX) {
		return -EINVAL;
	}

	was_measuring = data->measuring;
	ret = scd30_set_idle(dev);
	if (ret < 0) {
		return ret;
	}

	ret = scd30_write_cmd_arg(dev, SCD30_CMD_SET_FRC, target_concentration);
	if (ret < 0) {
		goto out;
	}

out:
	if (was_measuring) {
		int start_ret = scd30_start_measurement(dev);

		if (ret == 0) {
			ret = start_ret;
		}
	}

	return ret;
}

int scd30_soft_reset(const struct device *dev)
{
	struct scd30_data *data = dev->data;
	const struct scd30_config *cfg = dev->config;
	int ret;

	ret = scd30_write_cmd(dev, SCD30_CMD_SOFT_RESET);
	if (ret < 0) {
		return ret;
	}

	k_msleep(SCD30_BOOT_TIME_MS);
	data->measuring = false;
	data->ambient_pressure = 0;

	ret = scd30_write_cmd_arg(dev, SCD30_CMD_SET_MEASUREMENT_INTERVAL,
				  cfg->measurement_interval);
	if (ret < 0) {
		return ret;
	}

	return scd30_start_measurement(dev);
}

static int scd30_probe(const struct device *dev)
{
	uint8_t fw[3];
	int ret;

	ret = scd30_cmd_read_words(dev, SCD30_CMD_GET_FIRMWARE_VERSION, fw, sizeof(fw));
	if (ret < 0) {
		return ret;
	}

	LOG_INF("SCD30 firmware %u.%u", fw[0], fw[1]);
	return 0;
}

static int scd30_init(const struct device *dev)
{
	const struct scd30_config *cfg = dev->config;
	struct scd30_data *data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->bus.bus);
		return -ENODEV;
	}

	if (cfg->measurement_interval < SCD30_MEASUREMENT_INTERVAL_MIN_S ||
	    cfg->measurement_interval > SCD30_MEASUREMENT_INTERVAL_MAX_S) {
		LOG_ERR("Invalid measurement-interval %u", cfg->measurement_interval);
		return -EINVAL;
	}

	if (cfg->ready_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->ready_gpio)) {
			LOG_ERR_DEVICE_NOT_READY(cfg->ready_gpio.port);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->ready_gpio, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Failed to configure ready GPIO (%d)", ret);
			return ret;
		}
	}

	data->ambient_pressure = 0;
	data->measuring = false;

	ret = scd30_write_cmd(dev, SCD30_CMD_SOFT_RESET);
	if (ret < 0) {
		LOG_ERR("Soft reset failed (%d)", ret);
		return ret;
	}

	k_msleep(SCD30_BOOT_TIME_MS);

	ret = scd30_probe(dev);
	if (ret < 0) {
		LOG_ERR("Failed to read firmware version (%d)", ret);
		return ret;
	}

	(void)scd30_stop_measurement(dev);

	ret = scd30_write_cmd_arg(dev, SCD30_CMD_SET_MEASUREMENT_INTERVAL,
				  cfg->measurement_interval);
	if (ret < 0) {
		LOG_ERR("Failed to set measurement interval (%d)", ret);
		return ret;
	}

	ret = scd30_start_measurement(dev);
	if (ret < 0) {
		LOG_ERR("Failed to start measurement (%d)", ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(sensor, scd30_api_funcs) = {
	.sample_fetch = scd30_sample_fetch,
	.channel_get = scd30_channel_get,
	.attr_set = scd30_attr_set,
	.attr_get = scd30_attr_get,
};

#define SCD30_INIT(inst)                                                                           \
	static struct scd30_data scd30_data_##inst;                                                \
	static const struct scd30_config scd30_config_##inst = {                                   \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.ready_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, ready_gpios, {0}),                    \
		.measurement_interval = DT_INST_PROP(inst, measurement_interval),                  \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, scd30_init, NULL, &scd30_data_##inst,                   \
				     &scd30_config_##inst, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &scd30_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(SCD30_INIT)
