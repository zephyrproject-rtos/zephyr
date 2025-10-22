/*
 * Copyright (c) 2025 Luna Pes <zephyr@orangemurker.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* This file implements the Infineon AN304 SPI Guide for F-RAM */

#include "zephyr/devicetree.h"
#include "zephyr/kernel.h"
#include "zephyr/sys/byteorder.h"
#include "zephyr/sys/util.h"
#include <zephyr/device.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT infineon_fm25xxx

LOG_MODULE_REGISTER(fm25xxx, CONFIG_EEPROM_LOG_LEVEL);

/* Registers */
#define FM25XXX_WREN  0x06
#define FM25XXX_WRDI  0x04
#define FM25XXX_RDSR  0x05
#define FM25XXX_WRSR  0x01
#define FM25XXX_READ  0x03
#define FM25XXX_WRITE 0x02

struct fm25xxx_config {
	struct spi_dt_spec spi;
	size_t size;
	bool readonly;
};

struct fm25xxx_data {
	struct k_sem lock;
};

static uint8_t eeprom_fm25xxx_size_to_addr_bytes(size_t size)
{
	if (size <= 500) {
		return 1;
	} else if (size <= 64000) {
		return 2;
	} else {
		return 3;
	}
}

static int eeprom_fm25xxx_set_enable_write(const struct device *dev, bool enable_writes)
{
	const struct fm25xxx_config *config = dev->config;
	uint8_t op = enable_writes ? FM25XXX_WREN : FM25XXX_WRDI;

	const struct spi_buf tx_bufs[] = {{
		.buf = &op,
		.len = sizeof(op),
	}};
	const struct spi_buf_set tx_buf_set = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs),
	};

	int ret = spi_write_dt(&config->spi, &tx_buf_set);

	if (ret != 0) {
		LOG_ERR("Failed to %s writes", enable_writes ? "enable" : "disable");
		return ret;
	}

	return 0;
}

int eeprom_fm25xxx_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	int ret;
	const struct fm25xxx_config *config = dev->config;

	if (offset + len > config->size) {
		LOG_ERR("Can not read more data than the device size");
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	uint8_t read_op[4] = {FM25XXX_READ};
	off_t last_offset_bit = (offset >> 8) & 0x01;

	size_t addr_bytes = eeprom_fm25xxx_size_to_addr_bytes(config->size);
	size_t op_len = 1 + addr_bytes;

	switch (addr_bytes) {
	case 1:
		read_op[0] |= (last_offset_bit << 3);
		read_op[1] = offset & 0xff;
		break;
	case 2:
		sys_put_be16(offset, &read_op[1]);
		break;
	case 3:
		sys_put_be24(offset, &read_op[1]);
		break;
	default:
		LOG_ERR("Invalid number of address bytes %zu", addr_bytes);
		return -EINVAL;
	}

	LOG_HEXDUMP_DBG(read_op, 4, "Read op");

	const struct spi_buf tx_bufs[] = {{
		.buf = &read_op,
		.len = op_len,
	}};
	const struct spi_buf_set tx_buf_set = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs),
	};

	const struct spi_buf rx_bufs[2] = {
		{
			.buf = NULL,
			.len = op_len,
		},
		{
			.buf = data,
			.len = len,
		},
	};
	const struct spi_buf_set rx_buf_set = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs),
	};

	ret = spi_transceive_dt(&config->spi, &tx_buf_set, &rx_buf_set);

	if (ret != 0) {
		LOG_ERR("Failed to read from FRAM");
		return ret;
	}

	return 0;
}

int eeprom_fm25xxx_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	int ret;
	const struct fm25xxx_config *config = dev->config;
	struct fm25xxx_data *dev_data = dev->data;

	if (config->readonly) {
		LOG_ERR("Can not write to a readonly device");
		return -EACCES;
	}

	if (offset + len > config->size) {
		LOG_ERR("Can not write more data than the device size");
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	uint8_t write_op[4] = {FM25XXX_WRITE};
	off_t last_offset_bit = (offset >> 8) & 0x01;

	size_t addr_bytes = eeprom_fm25xxx_size_to_addr_bytes(config->size);
	size_t op_len = 1 + addr_bytes;

	switch (addr_bytes) {
	case 1:
		write_op[0] |= (last_offset_bit << 3);
		write_op[1] = offset & 0xff;
		break;
	case 2:
		sys_put_be16(offset, &write_op[1]);
		break;
	case 3:
		sys_put_be24(offset, &write_op[1]);
		break;
	default:
		LOG_ERR("Invalid number of address bytes %zu", addr_bytes);
		return -EINVAL;
	}

	LOG_HEXDUMP_DBG(write_op, 4, "Write op");

	const struct spi_buf tx_bufs[] = {
		{
			.buf = &write_op,
			.len = op_len,
		},
		{
			.buf = (void *)data,
			.len = len,
		},
	};
	const struct spi_buf_set tx_buf_set = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs),
	};

	k_sem_take(&dev_data->lock, K_FOREVER);

	ret = eeprom_fm25xxx_set_enable_write(dev, true);
	if (ret != 0) {
		k_sem_give(&dev_data->lock);
		LOG_ERR("Could not enable writes");
		return ret;
	}

	ret = spi_write_dt(&config->spi, &tx_buf_set);
	if (ret != 0) {
		k_sem_give(&dev_data->lock);
		LOG_ERR("Failed to write to FRAM");
		return ret;
	}

	ret = eeprom_fm25xxx_set_enable_write(dev, false);
	if (ret != 0) {
		k_sem_give(&dev_data->lock);
		LOG_ERR("Could not disable writes");
		return ret;
	}

	k_sem_give(&dev_data->lock);

	return 0;
}

size_t eeprom_fm25xxx_get_size(const struct device *dev)
{
	const struct fm25xxx_config *config = dev->config;

	return config->size;
}

static int eeprom_fm25xxx_init(const struct device *dev)
{
	const struct fm25xxx_config *config = dev->config;
	struct fm25xxx_data *data = dev->data;

	k_sem_init(&data->lock, 1, 1);

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(eeprom, eeprom_fm25xxx_api) = {
	.read = &eeprom_fm25xxx_read,
	.write = &eeprom_fm25xxx_write,
	.size = &eeprom_fm25xxx_get_size,
};

#define FM25XXX_INIT(inst)                                                                         \
	static struct fm25xxx_data fm25xxx_data_##inst;                                            \
	static const struct fm25xxx_config fm25xxx_config_##inst = {                               \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_OP_MODE_MASTER | SPI_WORD_SET(8)),           \
		.size = DT_INST_PROP(inst, size),                                                  \
		.readonly = DT_INST_PROP(inst, read_only),                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, eeprom_fm25xxx_init, NULL, &fm25xxx_data_##inst,               \
			      &fm25xxx_config_##inst, POST_KERNEL, CONFIG_EEPROM_INIT_PRIORITY,    \
			      &eeprom_fm25xxx_api);

DT_INST_FOREACH_STATUS_OKAY(FM25XXX_INIT)
