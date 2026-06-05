/*
 * Copyright (c) 2025, Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for Microchip 47L16 I2C SRAM with EEPROM backup (EERAM).
 *
 * The 47L16 provides 2KB of SRAM accessible via I2C with automatic EEPROM
 * backup on power loss. SRAM writes are immediate with no page boundaries.
 * The EEPROM backup occurs automatically or can be triggered via the
 * control register at I2C address 0x18 (not implemented in this driver).
 */

#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_EEPROM_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eeprom_microchip_47l16);

#define DT_DRV_COMPAT microchip_47l16

struct eeprom_47l16_config {
	struct i2c_dt_spec i2c;
	size_t size;
	size_t pagesize;
	uint8_t addr_width;
};

struct eeprom_47l16_data {
	struct k_mutex lock;
};

static int eeprom_47l16_read(const struct device *dev, off_t offset,
			     void *buf, size_t len)
{
	const struct eeprom_47l16_config *config = dev->config;
	struct eeprom_47l16_data *data = dev->data;
	uint8_t addr[2];
	uint8_t *pbuf = buf;
	int ret;

	if (!len) {
		return 0;
	}

	if ((offset + len) > config->size) {
		LOG_WRN("attempt to read past device boundary");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	while (len) {
		size_t chunk_len = len;
		size_t remainder = BIT(config->addr_width) - offset;

		if (chunk_len > remainder) {
			chunk_len = remainder;
		}

		if (config->addr_width == 16) {
			sys_put_be16(offset, addr);
			ret = i2c_write_read_dt(&config->i2c, addr, 2U,
						pbuf, chunk_len);
		} else {
			addr[0] = offset & 0xFFU;
			ret = i2c_write_read_dt(&config->i2c, addr, 1U,
						pbuf, chunk_len);
		}

		if (ret < 0) {
			LOG_ERR("failed to read EERAM (err %d)", ret);
			k_mutex_unlock(&data->lock);
			return ret;
		}

		pbuf += chunk_len;
		offset += chunk_len;
		len -= chunk_len;
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

static int eeprom_47l16_write(const struct device *dev, off_t offset,
			      const void *buf, size_t len)
{
	const struct eeprom_47l16_config *config = dev->config;
	struct eeprom_47l16_data *data = dev->data;
	const uint8_t *pbuf = buf;
	uint8_t block[258]; /* 2-byte addr + max 256-byte chunk */
	int ret;

	if (!len) {
		return 0;
	}

	if ((offset + len) > config->size) {
		LOG_WRN("attempt to write past device boundary");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	while (len) {
		size_t chunk_len = len;
		size_t addr_len;

		/* Limit chunk to fit in block buffer */
		if (chunk_len > 256U) {
			chunk_len = 256U;
		}

		/* Build write buffer: address + data */
		if (config->addr_width == 16) {
			sys_put_be16(offset, block);
			addr_len = 2U;
		} else {
			block[0] = offset & 0xFFU;
			addr_len = 1U;
		}

		memcpy(&block[addr_len], pbuf, chunk_len);

		ret = i2c_write_dt(&config->i2c, block, addr_len + chunk_len);
		if (ret < 0) {
			LOG_ERR("failed to write EERAM (err %d)", ret);
			k_mutex_unlock(&data->lock);
			return ret;
		}

		/* SRAM writes are immediate — no delay needed */
		pbuf += chunk_len;
		offset += chunk_len;
		len -= chunk_len;
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

static size_t eeprom_47l16_size(const struct device *dev)
{
	const struct eeprom_47l16_config *config = dev->config;

	return config->size;
}

static int eeprom_47l16_init(const struct device *dev)
{
	const struct eeprom_47l16_config *config = dev->config;
	struct eeprom_47l16_data *data = dev->data;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	k_mutex_init(&data->lock);

	return 0;
}

static DEVICE_API(eeprom, eeprom_47l16_api) = {
	.read  = eeprom_47l16_read,
	.write = eeprom_47l16_write,
	.size  = eeprom_47l16_size,
};

#define EEPROM_47L16_DEVICE(inst)					\
	static struct eeprom_47l16_data eeprom_47l16_data_##inst;	\
									\
	static const struct eeprom_47l16_config				\
		eeprom_47l16_config_##inst = {				\
		.i2c         = I2C_DT_SPEC_INST_GET(inst),		\
		.size        = DT_INST_PROP(inst, size),		\
		.pagesize    = DT_INST_PROP(inst, pagesize),		\
		.addr_width  = DT_INST_PROP(inst, address_width),	\
	};								\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			      eeprom_47l16_init,			\
			      NULL,					\
			      &eeprom_47l16_data_##inst,		\
			      &eeprom_47l16_config_##inst,		\
			      POST_KERNEL,				\
			      CONFIG_EEPROM_MICROCHIP_47L16_INIT_PRIORITY, \
			      &eeprom_47l16_api);

DT_INST_FOREACH_STATUS_OKAY(EEPROM_47L16_DEVICE)
