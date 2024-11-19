/*
 * Copyright (c) 2024 Jan Fäh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensirion_sts4x

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/devicetree.h>

LOG_MODULE_REGISTER(STS4X, CONFIG_SENSOR_LOG_LEVEL);

#define STS4X_CMD_RESET 0x94

#define STS4X_RESET_TIME 1

#define STS4X_CRC_POLY 0x31
#define STS4X_CRC_INIT 0xFF

#define STS4X_MAX_TEMP 175
#define STS4X_MIN_TEMP -45

struct sts4x_config {
	struct i2c_dt_spec bus;
	uint8_t repeatability;
};

struct sts4x_data {
	uint16_t temp_sample;
};

static const uint8_t measure_cmds[3] = {0xE0, 0xF6, 0xFD};
static const uint16_t measure_time_us[3] = {1600, 4500, 8300};

static int sts4x_crc_check(uint16_t value, uint8_t sensor_crc)
{
	uint8_t buf[2];

	sys_put_be16(value, buf);

	uint8_t calculated_crc = crc8(buf, 2, STS4X_CRC_POLY, STS4X_CRC_INIT, false);

	if (calculated_crc == sensor_crc) {
		return 0;
	}

	return -EIO;
}

static int sts4x_write_command(const struct device *dev, uint8_t cmd)
{
	const struct sts4x_config *cfg = dev->config;
	uint8_t tx_buf = cmd;

	return i2c_write_dt(&cfg->bus, &tx_buf, 1);
}

static int sts4x_read_sample(const struct device *dev, uint16_t *temp_sample)
{
	const struct sts4x_config *cfg = dev->config;
	uint8_t rx_buf[3];
	int ret;

	ret = i2c_read_dt(&cfg->bus, rx_buf, sizeof(rx_buf));
	if (ret < 0) {
		LOG_ERR("Failed to read data.");
		return ret;
	}

	*temp_sample = sys_get_be16(rx_buf);
	ret = sts4x_crc_check(*temp_sample, rx_buf[2]);
	if (ret < 0) {
		LOG_ERR("Invalid CRC.");
		return ret;
	}

	return 0;
}

static int sts4x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct sts4x_data *data = dev->data;
	const struct sts4x_config *cfg = dev->config;
	int ret;

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP) {
		ret = sts4x_write_command(dev, measure_cmds[cfg->repeatability]);
		if (ret < 0) {
			LOG_ERR("Failed to write measure command.");
			return ret;
		}

		k_usleep(measure_time_us[cfg->repeatability]);

		ret = sts4x_read_sample(dev, &data->temp_sample);
		if (ret < 0) {
			LOG_ERR("Failed to get temperature data.");
			return ret;
		}

		return 0;
	} else {
		return -ENOTSUP;
	}
}

static int sts4x_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	const struct sts4x_data *data = dev->data;

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		int64_t temp;

		temp = data->temp_sample * STS4X_MAX_TEMP;
		val->val1 = (int32_t)(temp / 0xFFFF) + STS4X_MIN_TEMP;
		val->val2 = ((temp % 0xFFFF) * 1000000) / 0xFFFF;
	} else {
		return -ENOTSUP;
	}
	return 0;
}

static int sts4x_init(const struct device *dev)
{
	const struct sts4x_config *cfg = dev->config;
	int ret;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("Device not ready.");
		return -ENODEV;
	}

	ret = sts4x_write_command(dev, STS4X_CMD_RESET);
	if (ret < 0) {
		LOG_ERR("Failed to reset the device.");
		return ret;
	}

	k_msleep(STS4X_RESET_TIME);

	return 0;
}

static const struct sensor_driver_api sts4x_api_funcs = {
	.sample_fetch = sts4x_sample_fetch,
	.channel_get = sts4x_channel_get,
};

#define STS4X_INIT(inst)                                                                           \
	static struct sts4x_data sts4x_data_##inst;                                                \
	static const struct sts4x_config sts4x_config_##inst = {                                   \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.repeatability = DT_INST_PROP(inst, repeatability),                                \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, sts4x_init, NULL, &sts4x_data_##inst,                   \
				     &sts4x_config_##inst, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &sts4x_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(STS4X_INIT)
