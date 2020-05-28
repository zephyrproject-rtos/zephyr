/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT winbond_w25q16

#include <errno.h>

#include <drivers/flash.h>
#include <drivers/spi.h>
#include <init.h>
#include <string.h>
#include "spi_flash_w25qxxdv_defs.h"
#include "spi_flash_w25qxxdv.h"
#include "flash_priv.h"

#if defined(CONFIG_MULTITHREADING)
#define SYNC_INIT() k_sem_init( \
		&((struct spi_flash_data *)dev->data)->sem, 1, UINT_MAX)
#define SYNC_LOCK() k_sem_take(&driver_data->sem, K_FOREVER)
#define SYNC_UNLOCK() k_sem_give(&driver_data->sem)
#else
#define SYNC_INIT()
#define SYNC_LOCK()
#define SYNC_UNLOCK()
#endif

static const struct flash_parameters flash_wb_parameters = {
	.write_block_size = 1,
	.erase_value = 0xff,
};

static int spi_flash_wb_access(struct spi_flash_data *ctx,
			       uint8_t cmd, bool addressed, off_t offset,
			       void *data, size_t length, bool write)
{
	uint8_t access[4];
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
		access[1] = (uint8_t) (offset >> 16);
		access[2] = (uint8_t) (offset >> 8);
		access[3] = (uint8_t) offset;

		buf[0].len = 4;
	} else {
		buf[0].len = 1;
	}

	tx.count = length ? 2 : 1;

	if (!write) {
		const struct spi_buf_set rx = {
			.buffers = buf,
			.count = 2
		};

		return spi_transceive(ctx->spi, &ctx->spi_cfg, &tx, &rx);
	}

	return spi_write(ctx->spi, &ctx->spi_cfg, &tx);
}

static inline int spi_flash_wb_id(struct device *dev)
{
	struct spi_flash_data *const driver_data = dev->data;
	uint32_t temp_data;
	uint8_t buf[3];

	if (spi_flash_wb_access(driver_data, W25QXXDV_CMD_RDID,
				false, 0, buf, 3, false) != 0) {
		return -EIO;
	}

	temp_data = ((uint32_t) buf[0]) << 16;
	temp_data |= ((uint32_t) buf[1]) << 8;
	temp_data |= (uint32_t) buf[2];

	if (temp_data != CONFIG_SPI_FLASH_W25QXXDV_DEVICE_ID) {
		return -ENODEV;
	}

	return 0;
}

static uint8_t spi_flash_wb_reg_read(struct device *dev, uint8_t reg)
{
	struct spi_flash_data *const driver_data = dev->data;

	if (spi_flash_wb_access(driver_data, reg,
				false, 0, &reg, 1, false)) {
		return 0;
	}

	return reg;
}

static inline void wait_for_flash_idle(struct device *dev)
{
	uint8_t reg;

	do {
		reg = spi_flash_wb_reg_read(dev, W25QXXDV_CMD_RDSR);
	} while (reg & W25QXXDV_WIP_BIT);
}

static int spi_flash_wb_reg_write(struct device *dev, uint8_t reg)
{
	struct spi_flash_data *const driver_data = dev->data;

	if (spi_flash_wb_access(driver_data, reg, false, 0,
				NULL, 0, true) != 0) {
		return -EIO;
	}

	return 0;
}

static int spi_flash_wb_read(struct device *dev, off_t offset, void *data,
			     size_t len)
{
	struct spi_flash_data *const driver_data = dev->data;
	int ret;

	if (offset < 0) {
		return -ENODEV;
	}

	SYNC_LOCK();

	wait_for_flash_idle(dev);

	ret = spi_flash_wb_access(driver_data, W25QXXDV_CMD_READ,
				  true, offset, data, len, false);

	SYNC_UNLOCK();

	return ret;
}

static int spi_flash_wb_write_protection_set_with_lock(struct device *dev,
						       bool enable, bool lock)
{
	struct spi_flash_data *const driver_data = dev->data;
	uint8_t reg = 0U;
	int ret;

	if (lock) {
		SYNC_LOCK();
	}

	wait_for_flash_idle(dev);

	if (enable) {
		reg = W25QXXDV_CMD_WRDI;
	} else {
		reg = W25QXXDV_CMD_WREN;
	}

	ret = spi_flash_wb_reg_write(dev, reg);

	if (lock) {
		SYNC_UNLOCK();
	}

	return ret;
}

static int spi_flash_wb_write_protection_set(struct device *dev, bool enable)
{
	return spi_flash_wb_write_protection_set_with_lock(dev, enable, true);
}

static int spi_flash_wb_program_page(struct device *dev, off_t offset,
		const void *data, size_t len)
{
	uint8_t reg;
	struct spi_flash_data *const driver_data = dev->data;

	__ASSERT(len <= CONFIG_SPI_FLASH_W25QXXDV_PAGE_PROGRAM_SIZE,
		 "Maximum length is %d for page programming (actual:%d)",
		 CONFIG_SPI_FLASH_W25QXXDV_PAGE_PROGRAM_SIZE, len);

	wait_for_flash_idle(dev);

	reg = spi_flash_wb_reg_read(dev, W25QXXDV_CMD_RDSR);
	if (!(reg & W25QXXDV_WEL_BIT)) {
		return -EIO;
	}

	wait_for_flash_idle(dev);

	/* Assume write protection has been disabled. Note that w25qxxdv
	 * flash automatically turns on write protection at the completion
	 * of each write or erase transaction.
	 */
	return spi_flash_wb_access(driver_data, W25QXXDV_CMD_PP,
				  true, offset, (void *)data, len, true);

}

static int spi_flash_wb_write(struct device *dev, off_t offset,
			      const void *data, size_t len)
{
	int ret;
	off_t page_offset;
	/* Cast `data`  to prevent `void*` arithmetic */
	const uint8_t *data_ptr = data;
	struct spi_flash_data *const driver_data = dev->data;

	if (offset < 0) {
		return -ENOTSUP;
	}

	SYNC_LOCK();

	/* Calculate the offset in the first page we write */
	page_offset = offset % CONFIG_SPI_FLASH_W25QXXDV_PAGE_PROGRAM_SIZE;

	/*
	 * Write all data that does not fit into a single programmable page.
	 * By doing this logic, we can safely disable lock protection in
	 * between pages as in case the user did not disable protection then
	 * it will fail on the first write.
	 */
	while ((page_offset + len) >
			CONFIG_SPI_FLASH_W25QXXDV_PAGE_PROGRAM_SIZE) {
		size_t len_to_write_in_page =
			CONFIG_SPI_FLASH_W25QXXDV_PAGE_PROGRAM_SIZE -
			page_offset;

		ret = spi_flash_wb_program_page(dev, offset,
						data_ptr, len_to_write_in_page);
		if (ret) {
			goto end;
		}

		ret = spi_flash_wb_write_protection_set_with_lock(dev,
				false, false);
		if (ret) {
			goto end;
		}

		len -= len_to_write_in_page;
		offset += len_to_write_in_page;
		data_ptr += len_to_write_in_page;

		/*
		 * For the subsequent pages we always start at the beginning
		 * of a page
		 */
		page_offset = 0;
	}

	ret = spi_flash_wb_program_page(dev, offset, data_ptr, len);

end:
	SYNC_UNLOCK();

	return ret;
}

static inline int spi_flash_wb_erase_internal(struct device *dev,
					      off_t offset, size_t size)
{
	struct spi_flash_data *const driver_data = dev->data;
	bool need_offset = true;
	uint8_t erase_opcode;

	if (offset < 0) {
		return -ENOTSUP;
	}

	wait_for_flash_idle(dev);

	/* write enable */
	spi_flash_wb_reg_write(dev, W25QXXDV_CMD_WREN);

	wait_for_flash_idle(dev);

	switch (size) {
	case W25QXXDV_SECTOR_SIZE:
		erase_opcode = W25QXXDV_CMD_SE;
		break;
	case W25QXXDV_BLOCK32K_SIZE:
		erase_opcode = W25QXXDV_CMD_BE32K;
		break;
	case W25QXXDV_BLOCK_SIZE:
		erase_opcode = W25QXXDV_CMD_BE;
		break;
	case CONFIG_SPI_FLASH_W25QXXDV_FLASH_SIZE:
		erase_opcode = W25QXXDV_CMD_CE;
		need_offset = false;
		break;
	default:
		return -EIO;

	}

	/* Assume write protection has been disabled. Note that w25qxxdv
	 * flash automatically turns on write protection at the completion
	 * of each write or erase transaction.
	 */
	return spi_flash_wb_access(driver_data, erase_opcode,
				   need_offset, offset, NULL, 0, true);
}

static int spi_flash_wb_erase(struct device *dev, off_t offset, size_t size)
{
	struct spi_flash_data *const driver_data = dev->data;
	int ret = 0;
	uint32_t new_offset = offset;
	uint32_t size_remaining = size;
	uint8_t reg;

	if ((offset < 0) || ((offset & W25QXXDV_SECTOR_MASK) != 0) ||
	    ((size + offset) > CONFIG_SPI_FLASH_W25QXXDV_FLASH_SIZE) ||
	    ((size & W25QXXDV_SECTOR_MASK) != 0)) {
		return -ENODEV;
	}

	SYNC_LOCK();

	reg = spi_flash_wb_reg_read(dev, W25QXXDV_CMD_RDSR);

	if (!(reg & W25QXXDV_WEL_BIT)) {
		SYNC_UNLOCK();
		return -EIO;
	}

	while ((size_remaining >= W25QXXDV_SECTOR_SIZE) && (ret == 0)) {
		if (size_remaining == CONFIG_SPI_FLASH_W25QXXDV_FLASH_SIZE) {
			ret = spi_flash_wb_erase_internal(dev, offset, size);
			break;
		}

		if ((size_remaining >= W25QXXDV_BLOCK_SIZE) &&
			((new_offset & (W25QXXDV_BLOCK_SIZE - 1)) == 0)) {
			ret = spi_flash_wb_erase_internal(dev, new_offset,
							W25QXXDV_BLOCK_SIZE);
			new_offset += W25QXXDV_BLOCK_SIZE;
			size_remaining -= W25QXXDV_BLOCK_SIZE;
			continue;
		}

		if ((size_remaining >= W25QXXDV_BLOCK32K_SIZE) &&
			((new_offset & (W25QXXDV_BLOCK32K_SIZE - 1)) == 0)) {
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

	SYNC_UNLOCK();

	return ret;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static struct flash_pages_layout dev_layout;

static void flash_wb_pages_layout(struct device *dev,
				  const struct flash_pages_layout **layout,
				  size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *
flash_wb_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_wb_parameters;
}

static const struct flash_driver_api spi_flash_api = {
	.read = spi_flash_wb_read,
	.write = spi_flash_wb_write,
	.erase = spi_flash_wb_erase,
	.write_protection = spi_flash_wb_write_protection_set,
	.get_parameters = flash_wb_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_wb_pages_layout,
#endif
};

static int spi_flash_wb_configure(struct device *dev)
{
	struct spi_flash_data *data = dev->data;

	data->spi = device_get_binding(DT_INST_BUS_LABEL(0));
	if (!data->spi) {
		return -EINVAL;
	}

	data->spi_cfg.frequency = DT_INST_PROP(0, spi_max_frequency);
	data->spi_cfg.operation = SPI_WORD_SET(8);
	data->spi_cfg.slave = DT_INST_REG_ADDR(0);

#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	data->cs_ctrl.gpio_dev = device_get_binding(
		DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
	if (!data->cs_ctrl.gpio_dev) {
		return -ENODEV;
	}

	data->cs_ctrl.gpio_pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(0);
	data->cs_ctrl.gpio_dt_flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0);
	data->cs_ctrl.delay = CONFIG_SPI_FLASH_W25QXXDV_GPIO_CS_WAIT_DELAY;

	data->spi_cfg.cs = &data->cs_ctrl;
#endif

	return spi_flash_wb_id(dev);
}

static int spi_flash_init(struct device *dev)
{
	int ret;

	SYNC_INIT();

	ret = spi_flash_wb_configure(dev);

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	/*
	 * Note: we use the sector size rather than the page size as some
	 * modules that consumes the flash page layout assume the page
	 * size is the minimal size they can erase.
	 */
	dev_layout.pages_count = (CONFIG_SPI_FLASH_W25QXXDV_FLASH_SIZE / W25QXXDV_SECTOR_SIZE);
	dev_layout.pages_size = W25QXXDV_SECTOR_SIZE;
#endif

	return ret;
}

static struct spi_flash_data spi_flash_memory_data;

DEVICE_AND_API_INIT(spi_flash_memory, CONFIG_SPI_FLASH_W25QXXDV_DRV_NAME,
	    spi_flash_init, &spi_flash_memory_data, NULL, POST_KERNEL,
	    CONFIG_SPI_FLASH_W25QXXDV_INIT_PRIORITY, &spi_flash_api);
