/*
 * Copyright (c) 2023 Andriy Gelman <andriy.gelman@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adt7310

#include <string.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "adt7310.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ADT7310, CONFIG_SENSOR_LOG_LEVEL);

#define ADT7310_READ_CMD  BIT(6)
#define ADT7310_WRITE_CMD 0

#define ADT7310_REG_STATUS	0x00
#define ADT7310_REG_CONFIG	0x01
#define ADT7310_REG_TEMP	0x02
#define ADT7310_REG_ID		0x03
#define ADT7310_REG_HYST	0x05
#define ADT7310_REG_THRESH_HIGH 0x06
#define ADT7310_REG_THRESH_LOW	0x07

#define ADT7310_ID			  0xc0
#define ADT7310_CONFIG_OP_MODE_MASK	  (0x3 << 5)
#define ADT7310_CONFIG_OP_MODE_CONTINUOUS (0x0 << 5)
#define ADT7310_CONFIG_OP_MODE_1SPS	  (0x2 << 5)

#define ADT7310_HYSTERESIS_TEMP_MAX	   15
#define ADT7310_CONFIG_RESOLUTION_16BIT	   BIT(7)
#define ADT7310_CONFIG_INT_COMPARATOR_MODE BIT(4)

/* Continuous conversion time = 240ms -> 1/0.240*1000000 */
#define ADT7310_MAX_SAMPLE_RATE 4166666

/* The quantization step size at 16-bit resolution is 0.0078125. */
/* Ref ADT7310 Reference manual */
#define ADT7310_SAMPLE_TO_MICRO_DEG(x) ((x) * 15625 >> 1)
#define ADT7310_MICRO_DEG_TO_SAMPLE(x) ((x) / 15625 << 1)

static int adt7310_temp_reg_read(const struct device *dev, uint8_t reg, int16_t *val)
{
	const struct adt7310_dev_config *cfg = dev->config;
	uint8_t cmd_buf[3] = { ADT7310_READ_CMD | (reg << 3) };
	int ret;
	const struct spi_buf tx_buf = { .buf = cmd_buf, .len = sizeof(cmd_buf) };
	const struct spi_buf rx_buf = { .buf = cmd_buf, .len = sizeof(cmd_buf) };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };
	const struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };

	ret = spi_transceive_dt(&cfg->bus, &tx, &rx);
	if (ret < 0) {
		return ret;
	}

	memcpy(val, cmd_buf + 1, 2);
	*val = sys_be16_to_cpu(*val);

	return 0;
}

static int adt7310_temp_reg_write(const struct device *dev, uint8_t reg, int16_t val)
{
	const struct adt7310_dev_config *cfg = dev->config;
	uint8_t cmd_buf[3];
	const struct spi_buf tx_buf = { .buf = cmd_buf, .len = sizeof(cmd_buf) };
	const struct spi_buf rx_buf = { .buf = cmd_buf, .len = sizeof(cmd_buf) };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };
	const struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };

	cmd_buf[0] = ADT7310_WRITE_CMD | (reg << 3);
	val = sys_cpu_to_be16(val);
	memcpy(&cmd_buf[1], &val, sizeof(val));

	return spi_transceive_dt(&cfg->bus, &tx, &rx);
}

static int adt7310_reg_read(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct adt7310_dev_config *cfg = dev->config;
	uint8_t cmd_buf[2] = { ADT7310_READ_CMD | (reg << 3) };
	const struct spi_buf tx_buf = { .buf = cmd_buf, .len = sizeof(cmd_buf) };
	const struct spi_buf rx_buf = { .buf = cmd_buf, .len = sizeof(cmd_buf) };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };
	const struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };
	int ret;

	ret = spi_transceive_dt(&cfg->bus, &tx, &rx);
	if (ret < 0) {
		return ret;
	}

	*val = cmd_buf[1];

	return 0;
}

static int adt7310_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct adt7310_dev_config *cfg = dev->config;
	uint8_t cmd_buf[2] = { ADT7310_WRITE_CMD | (reg << 3), val };
	const struct spi_buf tx_buf = { .buf = cmd_buf, .len = sizeof(cmd_buf) };
	const struct spi_buf rx_buf = { .buf = cmd_buf, .len = sizeof(cmd_buf) };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };
	const struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };

	return spi_transceive_dt(&cfg->bus, &tx, &rx);
}

static int adt7310_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct adt7310_data *drv_data = dev->data;
	int16_t value;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	ret = adt7310_temp_reg_read(dev, ADT7310_REG_TEMP, &value);
	if (ret < 0) {
		return ret;
	}

	drv_data->sample = value;

	return 0;
}

static int adt7310_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	int32_t value;
	struct adt7310_data *drv_data = dev->data;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	value = ADT7310_SAMPLE_TO_MICRO_DEG((int32_t)drv_data->sample);
	val->val1 = value / 1000000;
	val->val2 = value % 1000000;

	return 0;
}

static int adt7310_update_reg(const struct device *dev, uint8_t reg, uint8_t value, uint8_t mask)
{
	int ret;
	uint8_t reg_value;

	ret = adt7310_reg_read(dev, reg, &reg_value);
	if (ret < 0) {
		return ret;
	}

	reg_value &= ~mask;
	reg_value |= value;

	return adt7310_reg_write(dev, reg, reg_value);
}

static int adt7310_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	int32_t rate, value;
	uint8_t reg = 0;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	if (val->val1 > INT32_MAX/1000000 - 1 || val->val1 < INT32_MIN/1000000 + 1) {
		return -EINVAL;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		rate = val->val1 * 1000000 + val->val2;

		if (rate > ADT7310_MAX_SAMPLE_RATE || rate < 0) {
			return -EINVAL;
		}

		if (rate > 1000000) {
			return adt7310_update_reg(dev, ADT7310_REG_CONFIG,
						  ADT7310_CONFIG_OP_MODE_CONTINUOUS,
						  ADT7310_CONFIG_OP_MODE_MASK);
		} else {
			return adt7310_update_reg(dev, ADT7310_REG_CONFIG,
						  ADT7310_CONFIG_OP_MODE_1SPS,
						  ADT7310_CONFIG_OP_MODE_MASK);
		}
	case SENSOR_ATTR_HYSTERESIS:
		if (val->val1 < 0 || val->val1 > ADT7310_HYSTERESIS_TEMP_MAX || val->val2 != 0) {
			return -EINVAL;
		}
		return adt7310_reg_write(dev, ADT7310_REG_HYST, val->val1);
	case SENSOR_ATTR_UPPER_THRESH:
		reg = ADT7310_REG_THRESH_HIGH;
		__fallthrough;
	case SENSOR_ATTR_LOWER_THRESH:
		if (!reg) {
			reg = ADT7310_REG_THRESH_LOW;
		}

		value = val->val1 * 1000000 + val->val2;
		value = ADT7310_MICRO_DEG_TO_SAMPLE(value);

		if (value < INT16_MIN || value > INT16_MAX) {
			return -EINVAL;
		}
		return adt7310_temp_reg_write(dev, reg, value);
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int adt7310_probe(const struct device *dev)
{
	uint8_t value;
	int ret;

	ret = adt7310_reg_read(dev, ADT7310_REG_ID, &value);

	if (ret) {
		return ret;
	}

	value &= 0xf8;
	if (value != ADT7310_ID) {
		LOG_ERR("Invalid device ID");
		return -ENODEV;
	}

	return adt7310_reg_write(dev, ADT7310_REG_CONFIG,
				 ADT7310_CONFIG_RESOLUTION_16BIT |
				 ADT7310_CONFIG_INT_COMPARATOR_MODE);
}


static int adt7310_init(const struct device *dev)
{
	const struct adt7310_dev_config *cfg = dev->config;
	int ret;

	if (!spi_is_ready_dt(&cfg->bus)) {
		LOG_ERR("SPI bus %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	ret = adt7310_probe(dev);
	if (ret) {
		return ret;
	}

#if defined(CONFIG_ADT7310_TRIGGER)
	if (cfg->int_gpio.port) {
		ret = adt7310_init_interrupt(dev);
		if (ret) {
			LOG_ERR("Failed to initialize interrupt");
			return ret;
		}
	}
#endif
	return ret;
}

static const struct sensor_driver_api adt7310_driver_api = {
	.attr_set = adt7310_attr_set,
	.sample_fetch = adt7310_sample_fetch,
	.channel_get  = adt7310_channel_get,
#if defined(CONFIG_ADT7310_TRIGGER)
	.trigger_set = adt7310_trigger_set,
#endif
};

#define ADT7310_DEFINE(inst)                                                                       \
	static struct adt7310_data adt7310_data_##inst;                                            \
                                                                                                   \
	static const struct adt7310_dev_config adt7310_config_##inst = {                           \
		.bus = SPI_DT_SPEC_INST_GET(                                                       \
			inst,                                                                      \
			(SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA), 0),  \
                                                                                                   \
		IF_ENABLED(CONFIG_ADT7310_TRIGGER,                                                 \
			   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),))};        \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, adt7310_init, NULL, &adt7310_data_##inst,               \
				     &adt7310_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &adt7310_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ADT7310_DEFINE)
