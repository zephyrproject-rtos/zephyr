/*
 * Copyright (c) 2026 Zephyr Project Developers
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT avia_hx711_spi

#include <zephyr/drivers/sensor/hx711.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include "hx711_spi.h"

LOG_MODULE_REGISTER(HX711, CONFIG_SENSOR_LOG_LEVEL);

static int hx711_spi_read_sample(const struct device *dev, int32_t *sample)
{
	const struct hx711_config *config = dev->config;

	/*
	 * Send out a clock pulse sequence through MOSI to HX711.
	 * The first 48 bits are 101010... repeating 24 times, for reading
	 * out the last 24-bit sample.
	 * The last 8 bits are trailing clock pulses that set the gain of
	 * the next sample.
	 */

	uint8_t tx_buffer[7] = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x00};
	uint8_t rx_buffer[6] = {0};
	int ret;

	switch (config->gain) {
	case 64:
		tx_buffer[6] = HX711_CHA_GAIN_64;
		break;
	case 128:
		tx_buffer[6] = HX711_CHA_GAIN_128;
		break;
	default:
		LOG_ERR("Unsupported gain.");
		return -ENOTSUP;
	}

	const struct spi_buf tx_buf = {.buf = tx_buffer, .len = sizeof(tx_buffer)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	struct spi_buf rx_buf[2] = {{.buf = rx_buffer, .len = sizeof(rx_buffer)},
				    {.buf = NULL, .len = 1}};
	const struct spi_buf_set rx = {.buffers = rx_buf, .count = 2};

	ret = spi_transceive_dt(&config->spi, &tx, &rx);

	if (ret < 0) {
		return ret;
	}

	/*
	 * The received data in the buffer has each bit doubled, i.e.
	 * 11110011... should be 1101...
	 * Need to "squash" it to recover the actual value.
	 */

	if (sample) {
		*sample = 0;

		for (int i = 0; i < sizeof(rx_buffer); ++i) {
			uint8_t b = rx_buffer[i];

			*sample <<= 4;
			*sample |= (b & 0x1) | (((b >> 3) & 0x1) << 1) | (((b >> 5) & 0x1) << 2) |
				   (((b >> 7) & 0x1) << 3);
		}

		/* Discard unfilled bits and recover the sign */
		*sample = (*sample << 8) >> 8;
	}

	return 0;
}

static int hx711_sample_wait(const struct device *dev)
{
	const struct hx711_config *config = dev->config;

	uint8_t tx_buffer = 0;
	uint8_t rx_buffer = 0;
	int ret;

	const struct spi_buf tx_buf = {.buf = &tx_buffer, .len = 1};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	struct spi_buf rx_buf = {.buf = &rx_buffer, .len = 1};
	const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1};

	/* Don't wait forever */
	k_timepoint_t end = sys_timepoint_calc(K_SECONDS(1));

	while (true) {
		ret = spi_transceive_dt(&config->spi, &tx, &rx);

		if (ret < 0) {
			return ret;
		}

		if (rx_buffer == 0) {
			/* MISO low => data ready */
			break;
		}

		if (sys_timepoint_expired(end)) {
			return -EAGAIN;
		}

		/* Max sample rate of HX711 is 145 Hz
		 * (assuming max external clock speed 20 MHz)
		 */
		k_msleep(6);
	}

	return 0;
}

static int hx711_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct hx711_config *config = dev->config;
	struct hx711_data *data = dev->data;
	int32_t sample;
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_VOLTAGE chan ==
			(enum sensor_channel)SENSOR_CHAN_HX711_MASS);

	ret = hx711_sample_wait(dev);

	if (ret < 0) {
		return ret;
	}

	ret = hx711_spi_read_sample(dev, &sample);

	if (ret < 0) {
		LOG_ERR("Failed to fetch sample.");
		return ret;
	}

	data->sample_uv = ((int64_t)sample * config->avdd_uv / HX711_SAMPLE_MAX) / 2 / config->gain;

	return 0;
}

static int hx711_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	struct hx711_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		val->val1 = 0;
		val->val2 = data->sample_uv;
		break;
	case SENSOR_CHAN_HX711_MASS:
		val->val1 = data->sample_uv * data->conv_factor_uv_to_g;
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int hx711_attr_set(const struct device *dev, enum sensor_channel chan,
			  enum sensor_attribute attr, const struct sensor_value *val)
{
	struct hx711_data *data = dev->data;
	int ret = 0;

	switch ((uint32_t)attr) {
	case SENSOR_ATTR_GAIN:
		if (val->val1 == HX711_GAIN_128) {
			data->gain = 128;
		} else if (val->val1 == HX711_GAIN_64) {
			data->gain = 64;
		} else {
			return -ENOTSUP;
		}
		break;
	case SENSOR_ATTR_HX711_CONV_FACTOR:
		data->conv_factor_uv_to_g = val->val1;
		break;
	case SENSOR_ATTR_HX711_OFFSET:
		data->offset = val->val1;
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}

static DEVICE_API(sensor, hx711_driver_api) = {
	.sample_fetch = hx711_sample_fetch,
	.channel_get = hx711_channel_get,
	.attr_set = hx711_attr_set,
};

static int hx711_init(const struct device *dev)
{
	const struct hx711_config *config = dev->config;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_DBG("SPI bus not ready");
		return -ENODEV;
	}

	/* Perform the first read to set the gain */

	if (hx711_sample_wait(dev) < 0) {
		return -ENODEV;
	}

	if (hx711_spi_read_sample(dev, NULL) < 0) {
		return -ENODEV;
	}

	return 0;
}

#define HX711_SPI_OPERATION                                                                        \
	(SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_TRANSFER_MSB)

#define HX711_DEFINE(inst)                                                                         \
	static struct hx711_data hx711_data_##inst;                                                \
	static const struct hx711_config hx711_config_##inst = {                                   \
		.spi = SPI_DT_SPEC_INST_GET(inst, HX711_SPI_OPERATION),                            \
		.gain = DT_INST_PROP(inst, gain),                                                  \
		.avdd_uv = DT_INST_PROP(inst, avdd) * 1000,                                        \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, hx711_init, NULL,                                       \
				     &hx711_data_##inst, &hx711_config_##inst, POST_KERNEL,        \
				     CONFIG_SENSOR_INIT_PRIORITY, &hx711_driver_api);

DT_INST_FOREACH_STATUS_OKAY(HX711_DEFINE)
