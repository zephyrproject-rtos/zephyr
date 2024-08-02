/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 */

#define DT_DRV_COMPAT nuvoton_numaker_rmc

#include <string.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include "flash_priv.h"
#include <NuMicro.h>

LOG_MODULE_REGISTER(flash_numaker_rmc, CONFIG_FLASH_LOG_LEVEL);

#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)
#define SOC_NV_FLASH_WRITE_BLOCK_SIZE DT_PROP_OR(SOC_NV_FLASH_NODE, write_block_size, 0x04)

struct flash_numaker_data {
	RMC_T *rmc;
	struct k_sem write_lock;
	uint32_t flash_block_base;
};

static const struct flash_parameters flash_numaker_parameters = {
	.write_block_size = SOC_NV_FLASH_WRITE_BLOCK_SIZE,
	.erase_value = 0xff,
	.caps = {
		.no_explicit_erase = true,
	},
};

/* Validate offset and length */
static bool flash_numaker_is_range_valid(off_t offset, size_t len)
{
	uint32_t aprom_size = RMC_APROM_END - RMC_APROM_BASE;

	/* check for min value */
	if ((offset < 0) || (len == 0)) {
		return false;
	}

	/* check for max value */
	if (offset >= aprom_size || len > aprom_size || (aprom_size - offset) < len) {
		return false;
	}

	return true;
}

/*
 * Erase a flash memory area.
 *
 * param dev       Device struct
 * param offset    The address's offset
 * param len       The size of the buffer
 * return 0       on success
 * return -EINVAL erroneous code
 */

static int flash_numaker_erase(const struct device *dev, off_t offset, size_t len)
{
	struct flash_numaker_data *dev_data = dev->data;
	uint32_t rc = 0;
	unsigned int key;
	int page_nums = len / RMC_FLASH_PAGE_SIZE;
	uint32_t addr = dev_data->flash_block_base + offset;

	/* return SUCCESS for len == 0 (required by tests/drivers/flash) */
	if (len == 0) {
		return 0;
	}

	/* Validate range */
	if (!flash_numaker_is_range_valid(offset, len)) {
		return -EINVAL;
	}

	/* check alignment and erase only by pages */
	if (((addr % RMC_FLASH_PAGE_SIZE) != 0) || ((len % RMC_FLASH_PAGE_SIZE) != 0)) {
		return -EINVAL;
	}

	/* take semaphore */
	if (k_sem_take(&dev_data->write_lock, K_NO_WAIT)) {
		return -EACCES;
	}

	SYS_UnlockReg();
	key = irq_lock();
	while (page_nums) {
		/* erase page */
		if (RMC_Erase(addr)) {
			LOG_ERR("Erase flash page failed or erase time-out");
			rc = -EIO;
			goto done;
		}
		page_nums--;
		addr += RMC_FLASH_PAGE_SIZE;
	}

done:
	SYS_LockReg();
	irq_unlock(key);
	/* release semaphore */
	k_sem_give(&dev_data->write_lock);

	return rc;
}

/*
 * Read a flash memory area.
 *
 * param dev       Device struct
 * param offset    The address's offset
 * param data      The buffer to store or read the value
 * param length    The size of the buffer
 * return 0       on success,
 * return -EIO     erroneous code
 */
static int flash_numaker_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	struct flash_numaker_data *dev_data = dev->data;
	uint32_t addr = dev_data->flash_block_base + offset;

	/* return SUCCESS for len == 0 (required by tests/drivers/flash) */
	if (len == 0) {
		return 0;
	}

	/* Validate range */
	if (!flash_numaker_is_range_valid(offset, len)) {
		return -EINVAL;
	}

	/* read flash */
	memcpy(data, (void *)addr, len);

	return 0;
}

static int32_t flash_numaker_block_write(uint32_t u32_addr, const uint8_t *pu8_data, int block_size)
{
	int32_t retval;
	const uint32_t *pu32_data = (const uint32_t *)pu8_data;

	SYS_UnlockReg();
	if (block_size == 4) {
		retval = RMC_Write(u32_addr, *pu32_data);
	} else if (block_size == 8) {
		retval = RMC_Write(u32_addr, *pu32_data) |
			 RMC_Write(u32_addr + 4, *(pu32_data + 1));
	} else {
		retval = -1;
	}
	SYS_LockReg();

	return retval;
}

static int flash_numaker_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	struct flash_numaker_data *dev_data = dev->data;
	uint32_t rc = 0;
	unsigned int key;
	uint32_t addr = dev_data->flash_block_base + offset;
	int block_size = flash_numaker_parameters.write_block_size;
	int blocks = len / flash_numaker_parameters.write_block_size;
	const uint8_t *pu8_data = (const uint8_t *)data;

	/* return SUCCESS for len == 0 (required by tests/drivers/flash) */
	if (len == 0) {
		return 0;
	}

	/* Validate range */
	if (!flash_numaker_is_range_valid(offset, len)) {
		return -EINVAL;
	}

	/* Validate address alignment */
	if ((addr % flash_numaker_parameters.write_block_size) != 0) {
		return -EINVAL;
	}

	/* Validate write size be multiples of the write block size */
	if ((len % block_size) != 0) {
		return -EINVAL;
	}

	/* Validate offset be multiples of the write block size */
	if ((offset % block_size) != 0) {
		return -EINVAL;
	}

	if (k_sem_take(&dev_data->write_lock, K_FOREVER)) {
		return -EACCES;
	}

	key = irq_lock();

	while (blocks) {
		if (flash_numaker_block_write(addr, pu8_data, block_size)) {
			rc = -EIO;
			goto done;
		}
		pu8_data += block_size;
		addr += block_size;
		blocks--;
	}

done:
	irq_unlock(key);

	k_sem_give(&dev_data->write_lock);

	return rc;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static const struct flash_pages_layout dev_layout = {
	.pages_count = DT_REG_SIZE(SOC_NV_FLASH_NODE) /
		       DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
	.pages_size = DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
};

static void flash_numaker_pages_layout(const struct device *dev,
				       const struct flash_pages_layout **layout,
				       size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *flash_numaker_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_numaker_parameters;
}

static struct flash_numaker_data flash_data;

static const struct flash_driver_api flash_numaker_api = {
	.erase = flash_numaker_erase,
	.write = flash_numaker_write,
	.read = flash_numaker_read,
	.get_parameters = flash_numaker_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_numaker_pages_layout,
#endif
};

static int flash_numaker_init(const struct device *dev)
{
	struct flash_numaker_data *dev_data = dev->data;

	k_sem_init(&dev_data->write_lock, 1, 1);

	/* Enable RMC ISP function */
	SYS_UnlockReg();
	RMC_Open();
	/* Enable APROM update. */
	RMC_ENABLE_AP_UPDATE();
	SYS_LockReg();
	dev_data->flash_block_base = (uint32_t)RMC_APROM_BASE;
	dev_data->rmc = (RMC_T *)DT_REG_ADDR(DT_NODELABEL(rmc));

	return 0;
}

DEVICE_DT_INST_DEFINE(0, flash_numaker_init, NULL, &flash_data, NULL, POST_KERNEL,
		      CONFIG_FLASH_INIT_PRIORITY, &flash_numaker_api);
