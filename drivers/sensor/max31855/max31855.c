/*
 * Copyright (c) 2020 Christian Hirsch
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <drivers/sensor.h>
#include <init.h>
#include <drivers/gpio.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <drivers/spi.h>
#include <logging/log.h>

#include "max31855.h"

LOG_MODULE_REGISTER(MAX31855, CONFIG_SENSOR_LOG_LEVEL);

#if defined(DT_INST_0_MAXIM_MAX31855_CS_GPIOS_CONTROLLER)
static struct spi_cs_control max31855_cs_ctrl;
#else
#error "Please specify CS pin for MAX31855"
#endif

static int max31855_read(struct max31855_data *data,
		void *buf, int size)
{
	struct spi_buf rx_buf;
	const struct spi_buf_set rx = {
			.buffers = &rx_buf,
			.count = 1
	};

	rx_buf.buf = buf;
	rx_buf.len = size;

	int ret = spi_read(data->spi, &data->spi_cfg, &rx);

	if (ret) {
		LOG_DBG("spi_read FAIL %d", ret);
		return ret;
	}

	return 0;
}

static int max31855_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct max31855_data *data = dev->driver_data;
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	ret = max31855_read(data, &(data->data), sizeof(data->data));
	if (ret < 0) {
		LOG_DBG("max31855_read FAIL %d", ret);
		return ret;
	}

	return 0;
}

static int max31855_channel_get(struct device *dev,
		enum sensor_channel chan,
		struct sensor_value *val)
{
	struct max31855_data *data = dev->driver_data;
	s32_t temp = sys_be32_to_cpu(data->data);

	if (temp & (0x00010000)) {
		return -EIO;
	}

	switch (chan) {

	case SENSOR_CHAN_AMBIENT_TEMP:
		temp = (temp >> 18) & 0x3fff;

		/* if sign bit is set, make value negative */
		if (temp & 0x2000) {
			temp |= 0xffff2000;
		}

		val->val1 = temp / 4;
		val->val2 = (temp % 4) * 250000;
		break;

	case SENSOR_CHAN_DIE_TEMP:
		temp = (temp >> 4) & 0xfff;

		/* if sign bit is set, make value negative */
		if (temp & 0x800) {
			temp |= 0xfffff800;
		}

		val->val1 = temp / 16;
		val->val2 = (temp % 16) * 62500;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static const struct sensor_driver_api max31855_api_funcs = {
		.sample_fetch = max31855_sample_fetch,
		.channel_get = max31855_channel_get,
};

static inline int max31855_spi_init(struct max31855_data *data)
{
	data->spi = device_get_binding(DT_INST_0_MAXIM_MAX31855_BUS_NAME);
	if (!data->spi) {
		LOG_DBG("spi device not found: %s",
				DT_INST_0_MAXIM_MAX31855_BUS_NAME);
		return -EINVAL;
	}

	data->spi_cfg.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB;
	data->spi_cfg.frequency = DT_INST_0_MAXIM_MAX31855_SPI_MAX_FREQUENCY;
	data->spi_cfg.slave = DT_INST_0_MAXIM_MAX31855_BASE_ADDRESS;

#if defined(DT_INST_0_MAXIM_MAX31855_CS_GPIOS_CONTROLLER)
	max31855_cs_ctrl.gpio_dev = device_get_binding(
		DT_INST_0_MAXIM_MAX31855_CS_GPIOS_CONTROLLER);
	if (!max31855_cs_ctrl.gpio_dev) {
		LOG_ERR("Unable to get GPIO SPI CS device");
		return -ENODEV;
	}

	max31855_cs_ctrl.gpio_pin = DT_INST_0_MAXIM_MAX31855_CS_GPIOS_PIN;
	max31855_cs_ctrl.delay = 100U;

	data->spi_cfg.cs = &max31855_cs_ctrl;
	LOG_DBG("SPI GPIO CS configured on %s:%u",
			DT_INST_0_MAXIM_MAX31855_CS_GPIOS_CONTROLLER,
			DT_INST_0_MAXIM_MAX31855_CS_GPIOS_PIN);
#endif

	return 0;
}

int max31855_init(struct device *dev)
{
	struct max31855_data *data = dev->driver_data;

	if (max31855_spi_init(data) < 0) {
		LOG_DBG("spi master not found: %s",
				DT_INST_0_MAXIM_MAX31855_LABEL);
		return -EINVAL;
	}

	return 0;
}

static struct max31855_data max31855_data;

DEVICE_AND_API_INIT(max31855, DT_INST_0_MAXIM_MAX31855_LABEL,
		max31855_init, &max31855_data, NULL, POST_KERNEL,
		CONFIG_SENSOR_INIT_PRIORITY, &max31855_api_funcs);
