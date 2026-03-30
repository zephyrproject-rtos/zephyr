/*
 * Copyright (c) 2026 Filip Stojanovic <filipembedded@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_ade7978

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "ade7978.h"

LOG_MODULE_REGISTER(ADE7978, CONFIG_SENSOR_LOG_LEVEL);

static inline int ade7978_read_reg8(const struct device *dev, uint16_t addr, uint8_t *value)
{
	const struct ade7978_config *cfg = dev->config;
	uint8_t tx_buf[4] = {ADE7978_CMD_READ, (addr >> 8) & 0xFF, addr & 0xFF, ADE7978_DUMMY};
	uint8_t rx_buf[4];

	const struct spi_buf tx = {.buf = tx_buf, .len = sizeof(tx_buf)};
	const struct spi_buf rx = {.buf = rx_buf, .len = sizeof(rx_buf)};

	const struct spi_buf_set tx_set = {.buffers = &tx, .count = 1};
	const struct spi_buf_set rx_set = {.buffers = &rx, .count = 1};

	int ret = spi_transceive_dt(&cfg->spi, &tx_set, &rx_set);

	if (ret < 0) {
		return ret;
	}

	*value = rx_buf[3];

	return 0;
}

static inline int __maybe_unused ade7978_read_reg16(const struct device *dev, uint16_t addr,
						    uint16_t *value)
{
	const struct ade7978_config *cfg = dev->config;
	uint8_t tx_buf[5] = {ADE7978_CMD_READ, (addr >> 8) & 0xFF, addr & 0xFF, ADE7978_DUMMY,
			     ADE7978_DUMMY};
	uint8_t rx_buf[5];

	const struct spi_buf tx = {.buf = tx_buf, .len = sizeof(tx_buf)};
	const struct spi_buf rx = {.buf = rx_buf, .len = sizeof(rx_buf)};

	const struct spi_buf_set tx_set = {.buffers = &tx, .count = 1};
	const struct spi_buf_set rx_set = {.buffers = &rx, .count = 1};

	int ret = spi_transceive_dt(&cfg->spi, &tx_set, &rx_set);

	if (ret < 0) {
		return ret;
	}

	*value = ((uint16_t)rx_buf[3] << 8) | rx_buf[4];

	return 0;
}

static inline int ade7978_read_reg32(const struct device *dev, uint16_t addr, uint32_t *value)
{
	const struct ade7978_config *cfg = dev->config;
	uint8_t tx_buf[7] = {ADE7978_CMD_READ, (addr >> 8) & 0xFF, addr & 0xFF,  ADE7978_DUMMY,
			     ADE7978_DUMMY,    ADE7978_DUMMY,      ADE7978_DUMMY};
	uint8_t rx_buf[7];

	const struct spi_buf tx = {.buf = tx_buf, .len = sizeof(tx_buf)};
	const struct spi_buf rx = {.buf = rx_buf, .len = sizeof(rx_buf)};

	const struct spi_buf_set tx_set = {.buffers = &tx, .count = 1};
	const struct spi_buf_set rx_set = {.buffers = &rx, .count = 1};

	int ret = spi_transceive_dt(&cfg->spi, &tx_set, &rx_set);

	if (ret < 0) {
		return ret;
	}

	*value = ((uint32_t)rx_buf[3] << 24) | ((uint32_t)rx_buf[4] << 16) |
		 ((uint32_t)rx_buf[5] << 8) | ((uint32_t)rx_buf[6]);

	return 0;
}

static inline int __maybe_unused ade7978_write_reg8(const struct device *dev, uint16_t addr,
						    uint8_t value)
{
	const struct ade7978_config *cfg = dev->config;
	uint8_t tx_buf[4] = {ADE7978_CMD_WRITE, (addr >> 8) & 0xFF, addr & 0xFF, value};

	const struct spi_buf tx = {.buf = tx_buf, .len = sizeof(tx_buf)};

	const struct spi_buf_set tx_set = {.buffers = &tx, .count = 1};

	int ret = spi_transceive_dt(&cfg->spi, &tx_set, NULL);

	if (ret < 0) {
		return ret;
	}

	return 0;
}

static inline int ade7978_write_reg16(const struct device *dev, uint16_t addr, uint16_t value)
{
	const struct ade7978_config *cfg = dev->config;
	uint8_t tx_buf[5] = {ADE7978_CMD_WRITE, (addr >> 8) & 0xFF, addr & 0xFF,
			     (value >> 8) & 0xFF, value & 0xFF};

	const struct spi_buf tx = {.buf = tx_buf, .len = sizeof(tx_buf)};

	const struct spi_buf_set tx_set = {.buffers = &tx, .count = 1};

	int ret = spi_transceive_dt(&cfg->spi, &tx_set, NULL);

	if (ret < 0) {
		return ret;
	}

	return 0;
}

static inline int __maybe_unused ade7978_write_reg32(const struct device *dev, uint16_t addr,
						     uint32_t value)
{
	const struct ade7978_config *cfg = dev->config;
	uint8_t tx_buf[7] = {ADE7978_CMD_WRITE,    (addr >> 8) & 0xFF,   addr & 0xFF,
			     (value >> 24) & 0xFF, (value >> 16) & 0xFF, (value >> 8) & 0xFF,
			     value & 0xFF};

	const struct spi_buf tx = {.buf = tx_buf, .len = sizeof(tx_buf)};

	const struct spi_buf_set tx_set = {.buffers = &tx, .count = 1};

	int ret = spi_transceive_dt(&cfg->spi, &tx_set, NULL);

	if (ret < 0) {
		return ret;
	}

	return 0;
}

static inline int32_t ade7978_sign_extend_24(uint32_t value)
{
	value = value & ADE7978_24BIT_MASK;

	if (value & ADE7978_24BIT_SIGN) {
		/* Negative number - extend */
		return (int32_t)(value | ADE7978_24BIT_EXTEND);
	}

	/* Positive number */
	return (int32_t)value;
}

static int ade7978_init(const struct device *dev)
{
	const struct ade7978_config *cfg = dev->config;
	uint8_t version;
	int ret;

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("SPI bus not ready!");
		return -ENODEV;
	}

	ret = ade7978_read_reg8(dev, ADE7978_REG_VERSION, &version);
	if (ret < 0) {
		LOG_ERR("Failed to read VERSION register");
		return ret;
	}

	LOG_INF("ADE7978 version: 0x%02X", version);

	ret = ade7978_write_reg16(dev, ADE7978_REG_RUN, 0x01);
	if (ret < 0) {
		LOG_ERR("Failed to start DSP");
		return ret;
	}

	LOG_INF("ADE7978 initialized successfully!");
	return 0;
}

static int ade7978_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ade7978_data *data = dev->data;
	uint32_t raw_airms, raw_avrms;
	int ret;

	switch (chan) {
	case SENSOR_CHAN_ALL:
		ret = ade7978_read_reg32(dev, ADE7978_REG_AIRMS, &raw_airms);
		if (ret < 0) {
			return ret;
		}
		ret = ade7978_read_reg32(dev, ADE7978_REG_AVRMS, &raw_avrms);
		if (ret < 0) {
			return ret;
		}
		data->airms = ade7978_sign_extend_24(raw_airms);
		data->avrms = ade7978_sign_extend_24(raw_avrms);
		break;
	case SENSOR_CHAN_CURRENT:
		ret = ade7978_read_reg32(dev, ADE7978_REG_AIRMS, &raw_airms);
		if (ret < 0) {
			return ret;
		}
		data->airms = ade7978_sign_extend_24(raw_airms);
		break;
	case SENSOR_CHAN_VOLTAGE:
		ret = ade7978_read_reg32(dev, ADE7978_REG_AVRMS, &raw_avrms);
		if (ret < 0) {
			return ret;
		}
		data->avrms = ade7978_sign_extend_24(raw_avrms);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int ade7978_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct ade7978_data *data = dev->data;
	int ret;

	switch (chan) {
	case SENSOR_CHAN_CURRENT:
		ret = sensor_value_from_double(val, (double)data->airms / ADE7978_IRATIO);
		break;
	case SENSOR_CHAN_VOLTAGE:
		ret = sensor_value_from_double(val, (double)data->avrms / ADE7978_VRATIO);
		break;
	default:
		return -ENOTSUP;
	}
	return ret;
}

static DEVICE_API(sensor, ade7978_api) = {
	.sample_fetch = ade7978_sample_fetch,
	.channel_get = ade7978_channel_get,
};

#define ADE7978_DEFINE(inst)                                                                       \
	static struct ade7978_data ade7978_data_##inst;                                            \
                                                                                                   \
	static const struct ade7978_config ade7978_config_##inst = {                               \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |          \
							  SPI_WORD_SET(8))};                       \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, ade7978_init, NULL, &ade7978_data_##inst,               \
				     &ade7978_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &ade7978_api);

DT_INST_FOREACH_STATUS_OKAY(ADE7978_DEFINE)
