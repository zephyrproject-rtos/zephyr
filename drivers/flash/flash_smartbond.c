/*
 * Copyright (c) 2022 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_smartbond_flash_controller
#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)
#define QSPIF_NODE DT_NODELABEL(qspif)

#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/sys/byteorder.h>
#include <DA1469xAB.h>

#define FLASH_ERASE_SIZE	DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)
#define FLASH_PAGE_SIZE		256

struct flash_smartbond_config {
	uint32_t qspif_base_address;
};

static const struct flash_parameters flash_smartbond_parameters = {
	.write_block_size = DT_PROP(SOC_NV_FLASH_NODE, write_block_size),
	.erase_value = 0xff,
};

static bool range_is_valid(off_t offset, uint32_t size)
{
	return (offset + size) <= (CONFIG_FLASH_SIZE * 1024);
}

static ALWAYS_INLINE void qspic_data_write8(uint8_t data)
{
	volatile uint8_t *reg8 = (uint8_t *)&QSPIC->QSPIC_WRITEDATA_REG;

	*reg8 = data;
}

static ALWAYS_INLINE void qspic_data_write32(uint32_t data)
{
	volatile uint32_t *reg32 = (uint32_t *)&QSPIC->QSPIC_WRITEDATA_REG;

	*reg32 = data;
}

static ALWAYS_INLINE uint8_t qspic_data_read8(void)
{
	volatile uint8_t *reg8 = (uint8_t *)&QSPIC->QSPIC_READDATA_REG;

	return *reg8;
}

static __ramfunc uint8_t qspic_read_status(void)
{
	uint8_t status;

	QSPIC->QSPIC_CTRLBUS_REG = QSPIC_QSPIC_CTRLBUS_REG_QSPIC_EN_CS_Msk;
	qspic_data_write8(0x05);
	status = qspic_data_read8();
	QSPIC->QSPIC_CTRLBUS_REG = QSPIC_QSPIC_CTRLBUS_REG_QSPIC_DIS_CS_Msk;

	return status;
}


static __ramfunc void qspic_wait_busy(void)
{
	do {
	} while (qspic_read_status() & 0x01);
}

static __ramfunc void qspic_automode_exit(void)
{
	QSPIC->QSPIC_CTRLMODE_REG &= ~QSPIC_QSPIC_CTRLMODE_REG_QSPIC_AUTO_MD_Msk;
	QSPIC->QSPIC_CTRLBUS_REG = QSPIC_QSPIC_CTRLBUS_REG_QSPIC_SET_SINGLE_Msk;
	QSPIC->QSPIC_CTRLBUS_REG = QSPIC_QSPIC_CTRLBUS_REG_QSPIC_EN_CS_Msk;
	qspic_data_write8(0xff);
	qspic_data_write8(0xff);
	QSPIC->QSPIC_CTRLBUS_REG = QSPIC_QSPIC_CTRLBUS_REG_QSPIC_DIS_CS_Msk;
}

static __ramfunc void qspic_write_enable(void)
{
	uint8_t status;

	do {
		QSPIC->QSPIC_CTRLBUS_REG = QSPIC_QSPIC_CTRLBUS_REG_QSPIC_EN_CS_Msk;
		qspic_data_write8(0x06);
		QSPIC->QSPIC_CTRLBUS_REG = QSPIC_QSPIC_CTRLBUS_REG_QSPIC_DIS_CS_Msk;

		do {
			status = qspic_read_status();
		} while (status & 0x01);
	} while (!(status & 0x02));
}

static __ramfunc size_t qspic_write_page(uint32_t address, const uint8_t *data, size_t size)
{
	size_t written;

	/* Make sure we write up to page boundary */
	size = MIN(size, FLASH_PAGE_SIZE - (address & (FLASH_PAGE_SIZE - 1)));
	written = size;

	QSPIC->QSPIC_CTRLBUS_REG = QSPIC_QSPIC_CTRLBUS_REG_QSPIC_EN_CS_Msk;

	address = sys_cpu_to_be32(address);
	qspic_data_write32(address | 0x02);

	while (size >= 4) {
		qspic_data_write32(*(uint32_t *) data);
		data += 4;
		size -= 4;
	}

	while (size) {
		qspic_data_write8(*data);
		data++;
		size--;
	}

	QSPIC->QSPIC_CTRLBUS_REG = QSPIC_QSPIC_CTRLBUS_REG_QSPIC_DIS_CS_Msk;

	return written;
}

static __ramfunc void qspic_write(uint32_t address, const uint8_t *data, size_t size)
{
	size_t written;

	while (size) {
		qspic_write_enable();

		written = qspic_write_page(address, data, size);
		address += written;
		data += written;
		size -= written;

		qspic_wait_busy();
	}
}

static int flash_smartbond_read(const struct device *dev, off_t offset,
			      void *data, size_t size)
{
	const struct flash_smartbond_config *config = dev->config;

	if (!range_is_valid(offset, size)) {
		return -EINVAL;
	}

	if (!size) {
		return 0;
	}

	memcpy(data, (uint8_t *)(config->qspif_base_address + offset), size);

	return 0;
}

static __ramfunc int flash_smartbond_write(const struct device *dev,
					 off_t offset, const void *data,
					 size_t size)
{
	unsigned int key;
	uint32_t ctrlmode;

	if (!range_is_valid(offset, size)) {
		return -EINVAL;
	}

	if (!size) {
		return 0;
	}

	key = irq_lock();

	ctrlmode = QSPIC->QSPIC_CTRLMODE_REG;
	qspic_automode_exit();
	qspic_wait_busy();

	qspic_write(offset, data, size);

	QSPIC->QSPIC_CTRLMODE_REG = ctrlmode;
	CACHE->CACHE_CTRL1_REG |= CACHE_CACHE_CTRL1_REG_CACHE_FLUSH_Msk;

	irq_unlock(key);

	return 0;
}

static __ramfunc int flash_smartbond_erase(const struct device *dev, off_t offset,
					 size_t size)
{
	unsigned int key;
	uint32_t ctrlmode;
	uint32_t address;

	if (!range_is_valid(offset, size)) {
		return -EINVAL;
	}

	if ((offset % FLASH_ERASE_SIZE) != 0) {
		return -EINVAL;
	}

	if ((size % FLASH_ERASE_SIZE) != 0) {
		return -EINVAL;
	}

	if (!size) {
		return 0;
	}

	key = irq_lock();

	ctrlmode = QSPIC->QSPIC_CTRLMODE_REG;
	qspic_automode_exit();
	qspic_wait_busy();

	while (size) {
		qspic_write_enable();

		QSPIC->QSPIC_CTRLBUS_REG = QSPIC_QSPIC_CTRLBUS_REG_QSPIC_EN_CS_Msk;

		address = sys_cpu_to_be32(offset);
		qspic_data_write32(address | 0x20);
		QSPIC->QSPIC_CTRLBUS_REG = QSPIC_QSPIC_CTRLBUS_REG_QSPIC_DIS_CS_Msk;

		qspic_wait_busy();

		offset += FLASH_ERASE_SIZE;
		size -= FLASH_ERASE_SIZE;
	}

	QSPIC->QSPIC_CTRLMODE_REG = ctrlmode;
	CACHE->CACHE_CTRL1_REG |= CACHE_CACHE_CTRL1_REG_CACHE_FLUSH_Msk;

	irq_unlock(key);

	return 0;
}

static const struct flash_parameters *
flash_smartbond_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_smartbond_parameters;
}

#if CONFIG_FLASH_PAGE_LAYOUT
static const struct flash_pages_layout flash_smartbond_0_pages_layout = {
	.pages_count = DT_REG_SIZE(SOC_NV_FLASH_NODE) /
				DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
	.pages_size = DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
};

void flash_smartbond_page_layout(const struct device *dev,
			       const struct flash_pages_layout **layout,
			       size_t *layout_size)
{
	*layout = &flash_smartbond_0_pages_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static int flash_smartbond_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static const struct flash_driver_api flash_smartbond_driver_api = {
	.read = flash_smartbond_read,
	.write = flash_smartbond_write,
	.erase = flash_smartbond_erase,
	.get_parameters = flash_smartbond_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_smartbond_page_layout,
#endif
};

static const struct flash_smartbond_config flash_smartbond_0_config = {
	.qspif_base_address = DT_REG_ADDR(QSPIF_NODE),
};

DEVICE_DT_INST_DEFINE(0, flash_smartbond_init, NULL, NULL, &flash_smartbond_0_config,
		      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, &flash_smartbond_driver_api);
