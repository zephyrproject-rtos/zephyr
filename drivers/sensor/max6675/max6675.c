/*
 * Copyright (c) 2021 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max6675

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(max6675, CONFIG_SENSOR_LOG_LEVEL);

/** Thermocouple input bit (goes high if thermocouple is disconnected). */
#define THERMOCOUPLE_INPUT BIT(2)
/** Temperature position. */
#define TEMPERATURE_POS 3U
/** Temperature resolution (cDeg/LSB). */
#define TEMPERATURE_RES 25U

struct max6675_config {
	struct spi_dt_spec spi;
};

struct max6675_data {
	uint16_t sample;
};

static int max6675_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct max6675_data *data = dev->data;
	const struct max6675_config *config = dev->config;

	int ret;
	uint8_t buf_rx[2];
	const struct spi_buf rx_buf = {
		.buf = buf_rx,
		.len = ARRAY_SIZE(buf_rx)
	};
	const struct spi_buf_set rx_bufs = {
		.buffers = &rx_buf,
		.count = 1U
	};

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	ret = spi_read_dt(&config->spi, &rx_bufs);
	if (ret < 0) {
		return ret;
	}

	data->sample = sys_get_be16(buf_rx);

	if (data->sample & THERMOCOUPLE_INPUT) {
		LOG_INF("Thermocouple not connected");
		return -ENOENT;
	}

	return 0;
}

static int max6675_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct max6675_data *data = dev->data;

	int32_t temperature;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	temperature = (data->sample >> TEMPERATURE_POS) * TEMPERATURE_RES;
	val->val1 = temperature / 100;
	val->val2 = (temperature - val->val1 * 100) * 10000;

	return 0;
}

static const struct sensor_driver_api max6675_api = {
	.sample_fetch = &max6675_sample_fetch,
	.channel_get = &max6675_channel_get,
};

static int max6675_init(const struct device *dev)
{
	const struct max6675_config *config = dev->config;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus is not ready");
		return -ENODEV;
	}

	return 0;
}

#define MAX6675_INIT(n)							\
	static struct max6675_data max6675_data_##n;			\
	static const struct max6675_config max6675_config_##n = {	\
		.spi = SPI_DT_SPEC_INST_GET(n,				\
					    SPI_OP_MODE_MASTER |	\
					    SPI_WORD_SET(8U),		\
					    0U),			\
	};								\
	SENSOR_DEVICE_DT_INST_DEFINE(n, &max6675_init, NULL,		\
			      &max6675_data_##n, &max6675_config_##n,	\
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,	\
			      &max6675_api);

DT_INST_FOREACH_STATUS_OKAY(MAX6675_INIT)
