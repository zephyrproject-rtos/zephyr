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
#include "spi_flash_w25qxxdv_defs.h"
#include "spi_flash_w25qxxdv.h"
#include "flash_priv.h"

static inline int spi_flash_wb_id(struct device *dev)
{
	struct spi_flash_data *const driver_data = dev->driver_data;
	u8_t buf[W25QXXDV_LEN_CMD_AND_ID];
	u32_t temp_data;

	buf[0] = W25QXXDV_CMD_RDID;

	if (spi_transceive(driver_data->spi, buf, W25QXXDV_LEN_CMD_AND_ID,
			   buf, W25QXXDV_LEN_CMD_AND_ID) != 0) {
		return -EIO;
	}

	temp_data = ((u32_t) buf[1]) << 16;
	temp_data |= ((u32_t) buf[2]) << 8;
	temp_data |= (u32_t) buf[3];

	if (temp_data != W25QXXDV_RDID_VALUE) {
		return -ENODEV;
	}

	return 0;
}

static int spi_flash_wb_config(struct device *dev)
{
	struct spi_flash_data *const driver_data = dev->driver_data;
	struct spi_config config;

	config.max_sys_freq = CONFIG_SPI_FLASH_W25QXXDV_SPI_FREQ_0;

	config.config = SPI_WORD(8);

	if (spi_slave_select(driver_data->spi,
			     CONFIG_SPI_FLASH_W25QXXDV_SPI_SLAVE) !=
			     0) {
		return -EIO;
	}

	if (spi_configure(driver_data->spi, &config) != 0) {
		return -EIO;
	}

	return spi_flash_wb_id(dev);
}

static int spi_flash_wb_reg_read(struct device *dev, u8_t *data)
{
	struct spi_flash_data *const driver_data = dev->driver_data;
	u8_t buf[2];

	if (spi_transceive(driver_data->spi, data, 2, buf, 2) != 0) {
		return -EIO;
	}

	memcpy(data, buf, 2);

	return 0;
}

static inline void wait_for_flash_idle(struct device *dev)
{
	u8_t buf[2];

	buf[0] = W25QXXDV_CMD_RDSR;
	spi_flash_wb_reg_read(dev, buf);

	while (buf[1] & W25QXXDV_WIP_BIT) {
		buf[0] = W25QXXDV_CMD_RDSR;
		spi_flash_wb_reg_read(dev, buf);
	}
}

static int spi_flash_wb_reg_write(struct device *dev, u8_t *data)
{
	struct spi_flash_data *const driver_data = dev->driver_data;
	u8_t buf;

	wait_for_flash_idle(dev);

	if (spi_transceive(driver_data->spi, data, 1,
			   &buf /*dummy */, 1) != 0) {
		return -EIO;
	}

	return 0;
}

static int spi_flash_wb_read(struct device *dev, off_t offset, void *data,
			     size_t len)
{
	struct spi_flash_data *const driver_data = dev->driver_data;
	u8_t *buf = driver_data->buf;

	if (len > CONFIG_SPI_FLASH_W25QXXDV_MAX_DATA_LEN || offset < 0) {
		return -ENODEV;
	}

	k_sem_take(&driver_data->sem, K_FOREVER);

	if (spi_flash_wb_config(dev) != 0) {
		k_sem_give(&driver_data->sem);
		return -EIO;
	}

	wait_for_flash_idle(dev);

	buf[0] = W25QXXDV_CMD_READ;
	buf[1] = (u8_t) (offset >> 16);
	buf[2] = (u8_t) (offset >> 8);
	buf[3] = (u8_t) offset;

	memset(buf + W25QXXDV_LEN_CMD_ADDRESS, 0, len);

	if (spi_transceive(driver_data->spi, buf, len + W25QXXDV_LEN_CMD_ADDRESS,
			   buf, len + W25QXXDV_LEN_CMD_ADDRESS) != 0) {
		k_sem_give(&driver_data->sem);
		return -EIO;
	}

	memcpy(data, buf + W25QXXDV_LEN_CMD_ADDRESS, len);

	k_sem_give(&driver_data->sem);

	return 0;
}

static int spi_flash_wb_write(struct device *dev, off_t offset,
			      const void *data, size_t len)
{
	struct spi_flash_data *const driver_data = dev->driver_data;
	u8_t *buf = driver_data->buf;

	if (len > CONFIG_SPI_FLASH_W25QXXDV_MAX_DATA_LEN || offset < 0) {
		return -ENOTSUP;
	}

	k_sem_take(&driver_data->sem, K_FOREVER);

	if (spi_flash_wb_config(dev) != 0) {
		k_sem_give(&driver_data->sem);
		return -EIO;
	}

	wait_for_flash_idle(dev);

	buf[0] = W25QXXDV_CMD_RDSR;
	spi_flash_wb_reg_read(dev, buf);

	if (!(buf[1] & W25QXXDV_WEL_BIT)) {
		k_sem_give(&driver_data->sem);
		return -EIO;
	}

	wait_for_flash_idle(dev);

	buf[0] = W25QXXDV_CMD_PP;
	buf[1] = (u8_t) (offset >> 16);
	buf[2] = (u8_t) (offset >> 8);
	buf[3] = (u8_t) offset;

	memcpy(buf + W25QXXDV_LEN_CMD_ADDRESS, data, len);

	/* Assume write protection has been disabled. Note that w25qxxdv
	 * flash automatically turns on write protection at the completion
	 * of each write or erase transaction.
	 */
	if (spi_write(driver_data->spi, buf, len + W25QXXDV_LEN_CMD_ADDRESS) != 0) {
		k_sem_give(&driver_data->sem);
		return -EIO;
	}

	k_sem_give(&driver_data->sem);

	return 0;
}

static int spi_flash_wb_write_protection_set(struct device *dev, bool enable)
{
	struct spi_flash_data *const driver_data = dev->driver_data;
	u8_t buf = 0;

	k_sem_take(&driver_data->sem, K_FOREVER);

	if (spi_flash_wb_config(dev) != 0) {
		k_sem_give(&driver_data->sem);
		return -EIO;
	}

	wait_for_flash_idle(dev);

	if (enable) {
		buf = W25QXXDV_CMD_WRDI;
	} else {
		buf = W25QXXDV_CMD_WREN;
	}

	if (spi_flash_wb_reg_write(dev, &buf) != 0) {
		k_sem_give(&driver_data->sem);
		return -EIO;
	}

	k_sem_give(&driver_data->sem);

	return 0;
}

static inline int spi_flash_wb_erase_internal(struct device *dev,
					      off_t offset, size_t size)
{
	struct spi_flash_data *const driver_data = dev->driver_data;
	u8_t buf[W25QXXDV_LEN_CMD_ADDRESS];
	u8_t erase_opcode;
	u32_t len;

	if (offset < 0) {
		return -ENOTSUP;
	}

	wait_for_flash_idle(dev);

	/* write enable */
	buf[0] = W25QXXDV_CMD_WREN;
	spi_flash_wb_reg_write(dev, buf);

	wait_for_flash_idle(dev);

	switch (size) {
	case W25QXXDV_SECTOR_SIZE:
		erase_opcode = W25QXXDV_CMD_SE;
		len = W25QXXDV_LEN_CMD_ADDRESS;
		break;
	case W25QXXDV_BLOCK32K_SIZE:
		erase_opcode = W25QXXDV_CMD_BE32K;
		len = W25QXXDV_LEN_CMD_ADDRESS;
		break;
	case W25QXXDV_BLOCK_SIZE:
		erase_opcode = W25QXXDV_CMD_BE;
		len = W25QXXDV_LEN_CMD_ADDRESS;
		break;
	case CONFIG_SPI_FLASH_W25QXXDV_FLASH_SIZE:
		erase_opcode = W25QXXDV_CMD_CE;
		len = 1;
		break;
	default:
		return -EIO;

	}

	buf[0] = erase_opcode;
	buf[1] = (u8_t) (offset >> 16);
	buf[2] = (u8_t) (offset >> 8);
	buf[3] = (u8_t) offset;

	/* Assume write protection has been disabled. Note that w25qxxdv
	 * flash automatically turns on write protection at the completion
	 * of each write or erase transaction.
	 */
	return spi_write(driver_data->spi, buf, len);
}

static int spi_flash_wb_erase(struct device *dev, off_t offset, size_t size)
{
	struct spi_flash_data *const driver_data = dev->driver_data;
	u8_t *buf = driver_data->buf;
	int ret = 0;
	u32_t new_offset = offset;
	u32_t size_remaining = size;

	if ((offset < 0) || ((offset & W25QXXDV_SECTOR_MASK) != 0) ||
	    ((size + offset) > CONFIG_SPI_FLASH_W25QXXDV_FLASH_SIZE) ||
	    ((size & W25QXXDV_SECTOR_MASK) != 0)) {
		return -ENODEV;
	}

	k_sem_take(&driver_data->sem, K_FOREVER);

	if (spi_flash_wb_config(dev) != 0) {
		k_sem_give(&driver_data->sem);
		return -EIO;
	}

	buf[0] = W25QXXDV_CMD_RDSR;
	spi_flash_wb_reg_read(dev, buf);

	if (!(buf[1] & W25QXXDV_WEL_BIT)) {
		k_sem_give(&driver_data->sem);
		return -EIO;
	}

	while ((size_remaining >= W25QXXDV_SECTOR_SIZE) && (ret == 0)) {
		if (size_remaining == CONFIG_SPI_FLASH_W25QXXDV_FLASH_SIZE) {
			ret = spi_flash_wb_erase_internal(dev, offset, size);
			break;
		}

		if (size_remaining >= W25QXXDV_BLOCK_SIZE) {
			ret = spi_flash_wb_erase_internal(dev, new_offset,
							  W25QXXDV_BLOCK_SIZE);
			new_offset += W25QXXDV_BLOCK_SIZE;
			size_remaining -= W25QXXDV_BLOCK_SIZE;
			continue;
		}

		if (size_remaining >= W25QXXDV_BLOCK32K_SIZE) {
			ret = spi_flash_wb_erase_internal(dev, new_offset,
							  W25QXXDV_BLOCK32K_SIZE);
			new_offset += W25QXXDV_BLOCK32K_SIZE;
			size_remaining -= W25QXXDV_BLOCK32K_SIZE;
			continue;
		}

		if (size_remaining >= W25QXXDV_SECTOR_SIZE) {
			ret = spi_flash_wb_erase_internal(dev, new_offset,
							  W25QXXDV_SECTOR_SIZE);
			new_offset += W25QXXDV_SECTOR_SIZE;
			size_remaining -= W25QXXDV_SECTOR_SIZE;
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
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = (flash_api_pages_layout)
		       flash_page_layout_not_implemented,
#endif
	.write_block_size = 1,
};

static int spi_flash_init(struct device *dev)
{
	struct device *spi_dev;
	struct spi_flash_data *data = dev->driver_data;
	int ret;

	spi_dev = device_get_binding(CONFIG_SPI_FLASH_W25QXXDV_SPI_NAME);
	if (!spi_dev) {
		return -EIO;
	}

	data->spi = spi_dev;

	k_sem_init(&data->sem, 1, UINT_MAX);

	ret = spi_flash_wb_config(dev);
	if (!ret) {
		dev->driver_api = &spi_flash_api;
	}

	return ret;
}

static struct spi_flash_data spi_flash_memory_data;

DEVICE_INIT(spi_flash_memory, CONFIG_SPI_FLASH_W25QXXDV_DRV_NAME, spi_flash_init,
	    &spi_flash_memory_data, NULL, POST_KERNEL,
	    CONFIG_SPI_FLASH_W25QXXDV_INIT_PRIORITY);
