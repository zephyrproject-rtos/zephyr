/*
 * Copyright (c) 2025, Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ams_as5048

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(ams_as5048, CONFIG_SENSOR_LOG_LEVEL);

#define AS5048_REG_ANGLE        0x3FFF
#define AS5048_READ_BIT         BIT(14)
#define AS5048_PARITY_BIT       BIT(15)
#define AS5048_DATA_MASK        GENMASK(13, 0)
#define AS5048_ERROR_BIT        BIT(14)
#define AS5048_MAX_STEPS        16384
#define AS5048_FULL_ANGLE_DEG   360
#define AS5048_MICRO_DEGREE     1000000

struct as5048_config {
	struct spi_dt_spec spi;
};

struct as5048_data {
	uint16_t angle_raw;
};

static uint16_t as5048_build_read_command(uint16_t reg)
{
	uint16_t cmd = reg & AS5048_DATA_MASK;

	cmd |= AS5048_READ_BIT;

	/* Even parity across bits 0..14 (read bit + address). */
	if (__builtin_parity(cmd)) {
		cmd |= AS5048_PARITY_BIT;
	}

	return cmd;
}

static int as5048_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct as5048_data *data = dev->data;
	const struct as5048_config *cfg = dev->config;
	uint16_t tx_buf[2];
	uint16_t rx_buf[2];
	struct spi_buf tx[] = {
		{
			.buf = &tx_buf[0],
			.len = sizeof(tx_buf[0]),
		},
		{
			.buf = &tx_buf[1],
			.len = sizeof(tx_buf[1]),
		},
	};

	const struct spi_buf_set tx_set = {
		.buffers = tx,
		.count = ARRAY_SIZE(tx),
	};

	struct spi_buf rx[] = {
		{
			.buf = &rx_buf[0],
			.len = sizeof(rx_buf[0]),
		},
		{
			.buf = &rx_buf[1],
			.len = sizeof(rx_buf[1]),
		},
	};

	const struct spi_buf_set rx_set = {
		.buffers = rx,
		.count = ARRAY_SIZE(rx),
	};

	int ret;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_ROTATION)) {
		return -ENOTSUP;
	}

	tx_buf[0] = sys_cpu_to_be16(as5048_build_read_command(AS5048_REG_ANGLE));
	tx_buf[1] = sys_cpu_to_be16(0x0000);

	ret = spi_transceive_dt(&cfg->spi, &tx_set, &rx_set);
	if (ret < 0) {
		LOG_ERR("spi transceive failed (%d)", ret);
		return ret;
	}

	uint16_t raw = sys_be16_to_cpu(rx_buf[1]);

	if (raw & AS5048_ERROR_BIT) {
		LOG_ERR("AS5048 reported error (0x%04x)", raw);
		return -EIO;
	}

	data->angle_raw = raw & AS5048_DATA_MASK;

	return 0;
}

static int as5048_channel_get(const struct device *dev,
				 enum sensor_channel chan,
				 struct sensor_value *val)
{
	struct as5048_data *data = dev->data;

	if (chan != SENSOR_CHAN_ROTATION) {
		return -ENOTSUP;
	}

	int32_t scaled = (int32_t)data->angle_raw * AS5048_FULL_ANGLE_DEG;

	val->val1 = scaled / AS5048_MAX_STEPS;
	val->val2 = ((scaled % AS5048_MAX_STEPS) * AS5048_MICRO_DEGREE) / AS5048_MAX_STEPS;

	return 0;
}

static int as5048_init(const struct device *dev)
{
	const struct as5048_config *cfg = dev->config;

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(sensor, as5048_api) = {
	.sample_fetch = as5048_sample_fetch,
	.channel_get = as5048_channel_get,
};

#define AS5048_INIT(inst) \
	static struct as5048_data as5048_data_##inst; \
	static const struct as5048_config as5048_config_##inst = { \
		.spi = SPI_DT_SPEC_INST_GET(inst, \
				SPI_WORD_SET(16) | SPI_TRANSFER_MSB), \
	}; \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, as5048_init, NULL, \
				 &as5048_data_##inst, &as5048_config_##inst, \
				 POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
				 &as5048_api);

DT_INST_FOREACH_STATUS_OKAY(AS5048_INIT)
