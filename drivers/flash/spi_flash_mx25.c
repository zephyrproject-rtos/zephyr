/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <flash.h>
#include <spi.h>
#include <init.h>
#include <string.h>
#include "spi_flash_mx25_defs.h"
#include "spi_flash_mx25.h"
#include "flash_priv.h"

static int spi_flash_wb_access(struct spi_flash_data *ctx,
			       u8_t cmd, bool addressed, off_t offset,
			       void *data, size_t length, bool write)
{
	u8_t access[4];
	struct spi_buf buf[2] = {
		{
			.buf = access
		},
		{
			.buf = data,
			.len = length
		}
	};
	struct spi_buf_set tx = {
		.buffers = buf,
	};

	access[0] = cmd;

	if (addressed) {
		access[0] = (u8_t) (offset >> 16);
		access[1] = (u8_t) (offset >> 8);
		access[2] = (u8_t) offset;
		access[3] = cmd;

		buf[0].len = 4;
		ctx->spi_cfg.operation = SPI_WORD_SET(32);
	} else {
		buf[0].len = 1;
		ctx->spi_cfg.operation = SPI_WORD_SET(8);
	}

	tx.count = length ? 2 : 1;

	if (!write) {
		const struct spi_buf_set rx = {
			.buffers = (buf + 1),
			.count = 2
		};
		tx.count = 1;
		return spi_transceive(ctx->spi, &ctx->spi_cfg, &tx, &rx);
	}

	return spi_write(ctx->spi, &ctx->spi_cfg, &tx);
}

static inline int spi_flash_wb_id(struct device *dev)
{
	struct spi_flash_data *const driver_data = dev->driver_data;
	u32_t temp_data;
	u8_t buf[5], i;

	if (spi_flash_wb_access(driver_data, MX25_CMD_RDID,
				false, 0, buf, 3, false) != 0) {
		return -EIO;
	}

	temp_data = ((u32_t) buf[0]) << 16;
	temp_data |= ((u32_t) buf[1]) << 8;
	temp_data |= (u32_t) buf[2];

	if (temp_data != MX25_RDID_VALUE) {
		return -ENODEV;
	}

	return 0;
}

static u8_t spi_flash_wb_reg_read(struct device *dev, u8_t reg)
{
	struct spi_flash_data *const driver_data = dev->driver_data;

	if (spi_flash_wb_access(driver_data, reg,
				false, 0, &reg, 1, false)) {
		return 0;
	}

	return reg;
}

static inline void wait_for_flash_idle1(struct device *dev)
{
	u8_t reg;
	struct spi_flash_data *data = dev->driver_data;

	do {
		reg = spi_flash_wb_reg_read(dev, MX25_CMD_RDSR);
	} while (reg & MX25_WEL_BIT);
}

static inline void wait_for_flash_idle(struct device *dev)
{
	u8_t reg;
	struct spi_flash_data *data = dev->driver_data;

	do {
		reg = spi_flash_wb_reg_read(dev, MX25_CMD_RDSR);
	} while (reg & MX25_WIP_BIT);
}

static int spi_flash_wb_reg_write(struct device *dev, u8_t reg)
{
	struct spi_flash_data *const driver_data = dev->driver_data;

	if (spi_flash_wb_access(driver_data, reg, false, 0,
				NULL, 0, true) != 0) {
		return -EIO;
	}

	return 0;
}

static int spi_flash_wb_read(struct device *dev, off_t offset, void *rdata,
			     size_t len)
{
	struct spi_flash_data *const driver_data = dev->driver_data;
	int ret;

	if (len > CONFIG_SPI_FLASH_MX25_MAX_DATA_LEN || offset < 0) {
		return -ENODEV;
	}

	k_sem_take(&driver_data->sem, K_FOREVER);

	wait_for_flash_idle(dev);

	ret = spi_flash_wb_access(driver_data, MX25_CMD_READ,
				  true, offset, rdata, len, false);

	k_sem_give(&driver_data->sem);

	return ret;
}

static int spi_flash_wb_write(struct device *dev, off_t offset,
			      const void *wdata, size_t len)
{
	struct spi_flash_data *const driver_data = dev->driver_data;
	u8_t reg;
	int ret;

	if (len > CONFIG_SPI_FLASH_MX25_MAX_DATA_LEN || offset < 0) {
		return -ENOTSUP;
	}

	k_sem_take(&driver_data->sem, K_FOREVER);

	wait_for_flash_idle(dev);

	spi_flash_wb_reg_write(dev, MX25_CMD_WREN);

	reg = spi_flash_wb_reg_read(dev, MX25_CMD_RDSR);
	if (!(reg & MX25_WEL_BIT)) {
		k_sem_give(&driver_data->sem);
		return -EIO;
	}


	ret = spi_flash_wb_access(driver_data, MX25_CMD_PP,
				  true, offset, (void *)wdata, len, true);

	k_sem_give(&driver_data->sem);

	return ret;
}

static int spi_flash_wb_write_protection_set(struct device *dev, bool enable)
{
	struct spi_flash_data *const driver_data = dev->driver_data;
	u8_t reg = 0;
	int ret;


	k_sem_take(&driver_data->sem, K_FOREVER);

	wait_for_flash_idle(dev);

	if (enable) {
		reg = MX25_CMD_WRDI;
	} else {
		reg = MX25_CMD_WREN;
	}

	ret = spi_flash_wb_reg_write(dev, reg);

	k_sem_give(&driver_data->sem);

	return ret;
}

int spi_flash_wb_erase_internal(struct device *dev,
					      off_t offset, size_t size)
{
	struct spi_flash_data *const driver_data = dev->driver_data;
	u8_t erase_opcode;
	u32_t len;

	if (offset < 0) {
		return -ENOTSUP;
	}

	wait_for_flash_idle(dev);

	/* write enable */
	spi_flash_wb_reg_write(dev, MX25_CMD_WREN);

	wait_for_flash_idle(dev);

	switch (size) {
	case MX25_SECTOR_SIZE:
		erase_opcode = MX25_CMD_SE;
		len = MX25_LEN_CMD_ADDRESS;
		break;
	case MX25_BLOCK32K_SIZE:
		erase_opcode = MX25_CMD_BE32K;
		len = MX25_LEN_CMD_ADDRESS;
		break;
	case MX25_BLOCK_SIZE:
		erase_opcode = MX25_CMD_BE;
		len = MX25_LEN_CMD_ADDRESS;
		break;
	case CONFIG_SPI_FLASH_MX25_FLASH_SIZE:
		erase_opcode = MX25_CMD_CE;
		len = 1;
		break;
	default:
		return -EIO;

	}

	/* Assume write protection has been disabled. Note that w25qxxdv
	 * flash automatically turns on write protection at the completion
	 * of each write or erase transaction.
	 */

	return spi_flash_wb_access(driver_data, erase_opcode,
				   true, offset, NULL, 1, true);
}

static int spi_flash_wb_erase(struct device *dev, off_t offset, size_t size)
{
	struct spi_flash_data *const driver_data = dev->driver_data;
	int ret = 0;
	u32_t new_offset = offset;
	u32_t size_remaining = size;
	u8_t reg;

	if ((offset < 0) || ((offset & MX25_SECTOR_MASK) != 0) ||
	    ((size + offset) > CONFIG_SPI_FLASH_MX25_FLASH_SIZE) ||
	    ((size & MX25_SECTOR_MASK) != 0)) {
		return -ENODEV;
	}

	k_sem_take(&driver_data->sem, K_FOREVER);

	reg = spi_flash_wb_reg_read(dev, MX25_CMD_RDSR);

	if (!(reg & MX25_WEL_BIT)) {
		k_sem_give(&driver_data->sem);
		return -EIO;
	}

	while ((size_remaining >= MX25_SECTOR_SIZE) && (ret == 0)) {

		if (size_remaining == CONFIG_SPI_FLASH_MX25_FLASH_SIZE) {
			ret = spi_flash_wb_erase_internal(dev, offset, size);
			break;
		}

		if (size_remaining >= MX25_BLOCK_SIZE) {
			ret = spi_flash_wb_erase_internal(dev, new_offset,
							  MX25_BLOCK_SIZE);
			new_offset += MX25_BLOCK_SIZE;
			size_remaining -= MX25_BLOCK_SIZE;

			continue;
		}

		if (size_remaining >= MX25_BLOCK32K_SIZE) {
			ret = spi_flash_wb_erase_internal(dev, new_offset,
							  MX25_BLOCK32K_SIZE);
			new_offset += MX25_BLOCK32K_SIZE;
			size_remaining -= MX25_BLOCK32K_SIZE;
			continue;
		}

		if (size_remaining >= MX25_SECTOR_SIZE) {
			ret = spi_flash_wb_erase_internal(dev, new_offset,
							  MX25_SECTOR_SIZE);
			new_offset += MX25_SECTOR_SIZE;
			size_remaining -= MX25_SECTOR_SIZE;
			continue;
		}
	}

	k_sem_give(&driver_data->sem);

	return ret;

}

static const struct flash_driver_api spi_flash_api = {
	.read = spi_flash_wb_read,
	.write = spi_flash_wb_write,
	.erase = spi_flash_wb_erase,
	.write_protection = spi_flash_wb_write_protection_set,
};

static int spi_flash_wb_configure(struct device *dev)
{
	struct spi_flash_data *data = dev->driver_data;
	int ret = -1;

	data->spi = device_get_binding(CONFIG_SPI_FLASH_MX25_SPI_NAME);
	if (!data->spi) {
		return -EINVAL;
	}

	data->spi_cfg.frequency = CONFIG_SPI_FLASH_MX25_SPI_FREQ_0;
	data->spi_cfg.slave = CONFIG_SPI_FLASH_MX25_SPI_SLAVE;

	return spi_flash_wb_id(dev);
}

static int spi_flash_init(struct device *dev)
{
	struct spi_flash_data *data = dev->driver_data;
	int ret;

	k_sem_init(&data->sem, 1, UINT_MAX);

	ret = spi_flash_wb_configure(dev);

	if (!ret) {
		dev->driver_api = &spi_flash_api;
	}

	return ret;
}

static struct spi_flash_data spi_flash_memory_data;

DEVICE_INIT(spi_flash_memory, CONFIG_SPI_FLASH_MX25_DRV_NAME,
	    spi_flash_init, &spi_flash_memory_data, NULL, POST_KERNEL,
	    CONFIG_SPI_FLASH_MX25_INIT_PRIORITY);
