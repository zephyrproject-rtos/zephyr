/*
 * Copyright (c) 2021 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max6675

#include <drivers/sensor.h>
#include <drivers/spi.h>
#include <sys/byteorder.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(max6675, CONFIG_SENSOR_LOG_LEVEL);

/** Thermocouple input bit (goes high if thermocouple is disconnected). */
#define THERMOCOUPLE_INPUT BIT(2)
/** Temperature position. */
#define TEMPERATURE_POS 3U
/** Temperature resolution (cDeg/LSB). */
#define TEMPERATURE_RES 25U

struct max6675_config {
	const char *spi_label;
	struct spi_config spi_config;
	const char *spi_cs_label;
	gpio_pin_t spi_cs_pin;
	gpio_dt_flags_t spi_cs_flags;
};

struct max6675_data {
	const struct device *spi;
	struct spi_cs_control cs_ctrl;
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

	ret = spi_read(data->spi, &config->spi_config, &rx_bufs);
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
	struct max6675_data *data = dev->data;
	const struct max6675_config *config = dev->config;

	data->spi = device_get_binding(config->spi_label);
	if (data->spi == NULL) {
		LOG_ERR("Could not get SPI device %s", config->spi_label);
		return -ENODEV;
	}

	data->cs_ctrl.gpio_dev = device_get_binding(config->spi_cs_label);
	if (data->cs_ctrl.gpio_dev != NULL) {
		data->cs_ctrl.gpio_pin = config->spi_cs_pin;
		data->cs_ctrl.gpio_dt_flags = config->spi_cs_flags;
		data->cs_ctrl.delay = 0U;
	}

	return 0;
}

#define MAX6675_INIT(n)							       \
	static struct max6675_data max6675_data_##n;			       \
									       \
	static const struct max6675_config max6675_config_##n = {	       \
		.spi_label = DT_INST_BUS_LABEL(n),			       \
		.spi_config = {						       \
			.frequency = DT_INST_PROP_OR(n, spi_max_frequency, 0U),\
			.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8U),    \
			.slave = DT_INST_REG_ADDR(n),			       \
			.cs = &max6675_data_##n.cs_ctrl,		       \
		},							       \
		.spi_cs_label = UTIL_AND(				       \
			DT_INST_SPI_DEV_HAS_CS_GPIOS(n),		       \
			DT_INST_SPI_DEV_CS_GPIOS_LABEL(n)),		       \
		.spi_cs_pin = UTIL_AND(					       \
			DT_INST_SPI_DEV_HAS_CS_GPIOS(n),		       \
			DT_INST_SPI_DEV_CS_GPIOS_PIN(n)),		       \
		.spi_cs_flags = UTIL_AND(				       \
			DT_INST_SPI_DEV_HAS_CS_GPIOS(n),		       \
			DT_INST_SPI_DEV_CS_GPIOS_FLAGS(n)),		       \
	};								       \
									       \
	DEVICE_DT_INST_DEFINE(n, &max6675_init, device_pm_control_nop,	       \
			      &max6675_data_##n, &max6675_config_##n,	       \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,	       \
			      &max6675_api);

DT_INST_FOREACH_STATUS_OKAY(MAX6675_INIT)
