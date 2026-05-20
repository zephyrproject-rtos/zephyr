/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT meas_htu31d

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/htu31d.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

#include "htu31d.h"

LOG_MODULE_REGISTER(HTU31D, CONFIG_SENSOR_LOG_LEVEL);

static const uint8_t htu31d_conv_time[] = {
	HTU31D_CONV_TIME_OSR_0,
	HTU31D_CONV_TIME_OSR_1,
	HTU31D_CONV_TIME_OSR_2,
	HTU31D_CONV_TIME_OSR_3,
};

static bool htu31d_check_crc(const uint8_t *data, uint8_t data_len, uint8_t expected)
{
	uint8_t crc = crc8(data, data_len, HTU31D_CRC_POLY, HTU31D_CRC_INIT, false);

	if (crc != expected) {
		LOG_ERR("CRC mismatch (expected 0x%02x, got 0x%02x)", crc, expected);
		return false;
	}

	return true;
}

static bool htu31d_validate_response(const uint8_t *buf, uint8_t word_count)
{
	for (uint8_t i = 0; i < word_count; i++) {
		const uint8_t *word = &buf[i * HTU31D_WORD_SIZE];

		if (!htu31d_check_crc(word, HTU31D_CRC_DATA_LEN, word[2])) {
			return false;
		}
	}

	return true;
}

static int htu31d_read_with_crc(const struct device *dev, uint8_t cmd, uint8_t *buf,
				uint8_t buf_len)
{
	const struct htu31d_config *cfg = dev->config;
	int ret;

	ret = i2c_burst_read_dt(&cfg->bus, cmd, buf, buf_len);
	if (ret < 0) {
		return ret;
	}

	if (!htu31d_check_crc(buf, buf_len - 1U, buf[buf_len - 1U])) {
		return -EIO;
	}

	return 0;
}

static int htu31d_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct htu31d_config *cfg = dev->config;
	struct htu31d_data *data = dev->data;
	uint8_t conv_cmd;
	uint8_t read_cmd;
	uint8_t rx_buf[HTU31D_T_RH_BUF_LEN];
	uint8_t conv_delay;
	int ret;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_AMBIENT_TEMP) &&
	    (chan != SENSOR_CHAN_HUMIDITY)) {
		return -ENOTSUP;
	}

	conv_cmd = HTU31D_CMD_CONV_BASE + (cfg->osr_humidity << HTU31D_OSR_RH_SHIFT) +
		   (cfg->osr_temp << HTU31D_OSR_T_SHIFT);

	ret = i2c_write_dt(&cfg->bus, &conv_cmd, sizeof(conv_cmd));
	if (ret < 0) {
		LOG_ERR("Conversion trigger failed: %d", ret);
		return ret;
	}

	/* Wait for the slower of the two conversions to complete */
	conv_delay = MAX(htu31d_conv_time[cfg->osr_temp], htu31d_conv_time[cfg->osr_humidity]);
	k_msleep(conv_delay);

	read_cmd = HTU31D_CMD_READ_T_RH;

	ret = i2c_burst_read_dt(&cfg->bus, read_cmd, rx_buf, sizeof(rx_buf));
	if (ret < 0) {
		LOG_ERR("I2C read failed: %d", ret);
		return ret;
	}

	if (!htu31d_validate_response(rx_buf, HTU31D_T_RH_WORDS)) {
		return -EIO;
	}

	data->temp_sample = sys_get_be16(&rx_buf[0]);
	data->humi_sample = sys_get_be16(&rx_buf[HTU31D_WORD_SIZE]);

	return 0;
}

static int htu31d_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct htu31d_data *data = dev->data;
	int64_t micro;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		micro = HTU31D_TEMP_OFFSET_MICRO +
			(HTU31D_TEMP_SCALE_MICRO * (int64_t)data->temp_sample) / HTU31D_ADC_DIVISOR;
		sensor_value_from_micro(val, micro);
		break;
	case SENSOR_CHAN_HUMIDITY:
		micro = (HTU31D_HUMI_SCALE_MICRO * (int64_t)data->humi_sample) / HTU31D_ADC_DIVISOR;
		sensor_value_from_micro(val, micro);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

int htu31d_heater_set(const struct device *dev, bool enable)
{
	const struct htu31d_config *cfg = dev->config;
	uint8_t cmd = enable ? HTU31D_CMD_HEATER_ON : HTU31D_CMD_HEATER_OFF;

	return i2c_write_dt(&cfg->bus, &cmd, sizeof(cmd));
}

int htu31d_read_diagnostics(const struct device *dev, uint8_t *diag)
{
	uint8_t buf[HTU31D_DIAG_BUF_LEN];
	int ret;

	ret = htu31d_read_with_crc(dev, HTU31D_CMD_READ_DIAG, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	*diag = buf[0];
	return 0;
}

int htu31d_read_serial(const struct device *dev, uint32_t *serial)
{
	uint8_t buf[HTU31D_SERIAL_BUF_LEN];
	int ret;

	ret = htu31d_read_with_crc(dev, HTU31D_CMD_READ_SERIAL, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	*serial = sys_get_be24(buf);
	return 0;
}

static int htu31d_soft_reset(const struct device *dev)
{
	const struct htu31d_config *cfg = dev->config;
	uint8_t cmd = HTU31D_CMD_RESET;
	int ret;

	ret = i2c_write_dt(&cfg->bus, &cmd, sizeof(cmd));
	if (ret < 0) {
		LOG_ERR("Soft reset failed: %d", ret);
		return ret;
	}

	k_msleep(HTU31D_RESET_TIME_MS);

	return 0;
}

static int htu31d_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_TURN_ON:
	case PM_DEVICE_ACTION_RESUME:
		return htu31d_soft_reset(dev);
	case PM_DEVICE_ACTION_SUSPEND:
	case PM_DEVICE_ACTION_TURN_OFF:
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int htu31d_init(const struct device *dev)
{
	const struct htu31d_config *cfg = dev->config;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->bus.bus);
		return -ENODEV;
	}

	return pm_device_driver_init(dev, htu31d_pm_action);
}

static DEVICE_API(sensor, htu31d_api) = {
	.sample_fetch = htu31d_sample_fetch,
	.channel_get = htu31d_channel_get,
};

#define HTU31D_INIT(inst)                                                                          \
	static struct htu31d_data htu31d_data_##inst;                                              \
	static const struct htu31d_config htu31d_config_##inst = {                                 \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.osr_temp = DT_INST_PROP(inst, osr_temp),                                          \
		.osr_humidity = DT_INST_PROP(inst, osr_humidity),                                  \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(inst, htu31d_pm_action);                                          \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, htu31d_init, PM_DEVICE_DT_INST_GET(inst),               \
				     &htu31d_data_##inst, &htu31d_config_##inst, POST_KERNEL,      \
				     CONFIG_SENSOR_INIT_PRIORITY, &htu31d_api);

DT_INST_FOREACH_STATUS_OKAY(HTU31D_INIT)
