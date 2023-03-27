/*
 * Copyright (c) 2020 Christian Hirsch
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT maxim_max31855

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/types.h>
#include <zephyr/device.h>

#define THERMOCOUPLE_TEMPERATURE_POS 18
#define INTERNAL_TEMPERATURE_POS     4
#define THERMOCOUPLE_SIGN_BITS	     0xffff2000
#define INTERNAL_SIGN_BITS	     0xfffff800
#define THERMOCOUPLE_RESOLUTION	     25
#define INTERNAL_RESOLUTION	     625

LOG_MODULE_REGISTER(MAX31855, CONFIG_SENSOR_LOG_LEVEL);

struct max31855_config {
	struct spi_dt_spec spi;
};

struct max31855_data {
	uint32_t sample;
};

static int max31855_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct max31855_data *data = dev->data;
	const struct max31855_config *config = dev->config;
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	struct spi_buf rx_buf = {
		.buf = &(data->sample),
		.len = sizeof(data->sample),
	};
	const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1};

	ret = spi_read_dt(&config->spi, &rx);
	if (ret < 0) {
		LOG_ERR("max31855_read FAIL %d", ret);
		return ret;
	}

	return 0;
}

static int max31855_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct max31855_data *data = dev->data;
	uint32_t temp = sys_be32_to_cpu(data->sample);

	if (temp & BIT(16)) {
		return -EIO;
	}

	switch (chan) {

	case SENSOR_CHAN_AMBIENT_TEMP:
		temp = (temp >> THERMOCOUPLE_TEMPERATURE_POS) & 0x3fff;

		/* if sign bit is set, make value negative */
		if (temp & BIT(14)) {
			temp |= THERMOCOUPLE_SIGN_BITS;
		}

		temp = temp * THERMOCOUPLE_RESOLUTION;

		val->val1 = temp / 100;
		val->val2 = (temp - val->val1 * 100) * 10000;
		break;

	case SENSOR_CHAN_DIE_TEMP:
		temp = (temp >> INTERNAL_TEMPERATURE_POS) & 0xfff;

		/* if sign bit is set, make value negative */
		if (temp & BIT(12)) {
			temp |= INTERNAL_SIGN_BITS;
		}

		temp = temp * INTERNAL_RESOLUTION;

		val->val1 = temp / 10000;
		val->val2 = (temp - val->val1 * 10000) * 100;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static const struct sensor_driver_api max31855_api = {
	.sample_fetch = max31855_sample_fetch,
	.channel_get = max31855_channel_get,
};

static int max31855_init(const struct device *dev)
{
	const struct max31855_config *config = dev->config;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus is not ready");
		return -ENODEV;
	}

	return 0;
}

#define MAX31855_INIT(n)                                                                           \
	static struct max31855_data max31855_data_##n;                                             \
	static const struct max31855_config max31855_config_##n = {                                \
		.spi = SPI_DT_SPEC_INST_GET(n, SPI_OP_MODE_MASTER | SPI_WORD_SET(8U), 0U),         \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(n, &max31855_init, NULL, &max31855_data_##n,                  \
				     &max31855_config_##n, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &max31855_api);

DT_INST_FOREACH_STATUS_OKAY(MAX31855_INIT)
