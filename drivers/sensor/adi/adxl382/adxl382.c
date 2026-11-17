/*
 * Copyright (c) 2026 Deere & Company
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adxl382

#include <errno.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include "adxl382.h"

LOG_MODULE_REGISTER(adxl382, CONFIG_SENSOR_LOG_LEVEL);

static int write_checked(const struct device *dev, uint8_t reg, uint8_t value)
{
	const struct adxl382_data *data = dev->data;
	uint8_t actual;
	int ret = data->hw_tf->write_reg(dev, reg, value);

	if (ret == 0) {
		ret = data->hw_tf->read_reg(dev, reg, &actual);
		if (ret == 0 && actual != value) {
			LOG_ERR("Register 0x%02x: expected 0x%02x, got 0x%02x",
				reg, value, actual);
			ret = -EIO;
		}
	}
	return ret;
}

int adxl382_get_accel_data(const struct device *dev, struct adxl382_xyz_accel_data *accel)
{
	struct adxl382_data *data = dev->data;
	uint8_t raw[6];
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = data->hw_tf->read_reg_multiple(dev, ADXL382_XDATA_H, raw, sizeof(raw));
	k_mutex_unlock(&data->lock);
	if (ret == 0) {
		accel->x = (int16_t)sys_get_be16(raw);
		accel->y = (int16_t)sys_get_be16(raw + 2);
		accel->z = (int16_t)sys_get_be16(raw + 4);
	}
	return ret;
}

static int adxl382_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct adxl382_data *data = dev->data;
	struct adxl382_xyz_accel_data sample;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_ACCEL_XYZ) {
		return -ENOTSUP;
	}
	data->valid = false;
	ret = adxl382_get_accel_data(dev, &sample);
	if (ret == 0) {
		data->sample = sample;
		data->valid = true;
	}
	return ret;
}

static void adxl382_accel_convert(struct sensor_value *value, int16_t raw)
{
	int64_t micro_ms2 = (int64_t)raw * SENSOR_G / ADXL382_COUNTS_PER_G;

	value->val1 = micro_ms2 / 1000000;
	value->val2 = micro_ms2 % 1000000;
}

static int adxl382_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *values)
{
	const struct adxl382_data *data = dev->data;

	if (!data->valid) {
		return -ENODATA;
	}
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		adxl382_accel_convert(values, data->sample.x);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		adxl382_accel_convert(values, data->sample.y);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		adxl382_accel_convert(values, data->sample.z);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		adxl382_accel_convert(values++, data->sample.x);
		adxl382_accel_convert(values++, data->sample.y);
		adxl382_accel_convert(values, data->sample.z);
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static int adxl382_init(const struct device *dev)
{
	const struct adxl382_dev_config *cfg = dev->config;
	struct adxl382_data *data = dev->data;
	uint8_t id[3], misc;
	int ret;

	k_mutex_init(&data->lock);
	data->valid = false;
	ret = cfg->bus_init(dev);
	if (ret != 0) {
		LOG_ERR("Bus initialization failed: %d", ret);
		return ret;
	}
	ret = data->hw_tf->read_reg_multiple(dev, ADXL382_DEVID_AD, id, sizeof(id));
	if (ret != 0) {
		return ret;
	}
	ret = data->hw_tf->read_reg(dev, ADXL382_MISC0, &misc);
	if (ret != 0) {
		return ret;
	}
	if (id[0] != 0xad || id[1] != 0x1d || id[2] != 0x17 ||
	    (misc & 0x80) == 0) {
		LOG_ERR("Not ADXL382: IDs %02x %02x %02x, MISC0 %02x",
			id[0], id[1], id[2], misc);
		return -ENODEV;
	}
	ret = data->hw_tf->write_reg(dev, ADXL382_RESET, ADXL382_RESET_CODE);
	if (ret != 0) {
		return ret;
	}
	k_sleep(K_MSEC(10));
	ret = write_checked(dev, ADXL382_OP_MODE, 0);
	if (ret == 0) {
		ret = write_checked(dev, ADXL382_DIG_EN, ADXL382_DIG_EN_XYZ);
	}
	if (ret == 0) {
		ret = write_checked(dev, ADXL382_OP_MODE, ADXL382_MODE_60G_LP);
	}
	if (ret == 0) {
		k_sleep(K_MSEC(100));
		LOG_INF("ADXL382 ready: +/-60 g, low-power XYZ");
	}
	return ret;
}

static DEVICE_API(sensor, adxl382_api_funcs) = {
	.sample_fetch = adxl382_sample_fetch,
	.channel_get = adxl382_channel_get,
#ifdef CONFIG_SENSOR_ASYNC_API
	.submit = adxl382_submit,
	.get_decoder = adxl382_get_decoder,
#endif
};

#define ADXL382_SPI_CFG (SPI_WORD_SET(8) | SPI_TRANSFER_MSB)

#define ADXL382_DEFINE_SPI(inst) \
	static struct adxl382_data adxl382_data_##inst; \
	static const struct adxl382_dev_config adxl382_config_##inst = { \
		.spi = SPI_DT_SPEC_INST_GET(inst, ADXL382_SPI_CFG), \
		.bus_init = adxl382_spi_init, \
	}; \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, adxl382_init, NULL, \
		&adxl382_data_##inst, &adxl382_config_##inst, POST_KERNEL, \
		CONFIG_SENSOR_INIT_PRIORITY, &adxl382_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(ADXL382_DEFINE_SPI)
