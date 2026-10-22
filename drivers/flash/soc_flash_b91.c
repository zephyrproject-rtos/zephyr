/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_b91_flash_controller
#define FLASH_SIZE   DT_REG_SIZE(DT_INST(0, soc_nv_flash))
#define FLASH_ORIGIN DT_REG_ADDR(DT_INST(0, soc_nv_flash))

#include "flash.h"
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>

/* driver definitions */
#define BLOCK_64K_SIZE         (0x10000u)
#define BLOCK_64K_PAGES        (BLOCK_64K_SIZE / PAGE_SIZE)
#define BLOCK_32K_SIZE         (0x8000u)
#define BLOCK_32K_PAGES        (BLOCK_32K_SIZE / PAGE_SIZE)
#define SECTOR_SIZE            (0x1000u)
#define SECTOR_PAGES           (SECTOR_SIZE / PAGE_SIZE)


/* driver data structure */
struct flash_b91_data {
	struct k_sem write_lock;
};

/* driver parameters structure */
static const struct flash_parameters flash_b91_parameters = {
	.write_block_size = DT_PROP(DT_INST(0, soc_nv_flash), write_block_size),
	.erase_value = 0xff,
};


/* Check for correct offset and length */
static bool flash_b91_is_range_valid(off_t offset, size_t len)
{
	/* check for min value */
	if ((offset < 0) || (len < 1)) {
		return false;
	}

	/* check for max value */
	if ((offset + len) > FLASH_SIZE) {
		return false;
	}

	return true;
}

/* API implementation: driver initialization */
static int flash_b91_init(const struct device *dev)
{
	struct flash_b91_data *dev_data = dev->data;

	k_sem_init(&dev_data->write_lock, 1, 1);

	return 0;
}

/* API implementation: erase */
static int flash_b91_erase(const struct device *dev, off_t offset, size_t len)
{
	int page_nums = len / PAGE_SIZE;
	struct flash_b91_data *dev_data = dev->data;

	/* return SUCCESS if len equals 0 (required by tests/drivers/flash) */
	if (!len) {
		return 0;
	}

	/* check for valid range */
	if (!flash_b91_is_range_valid(offset, len)) {
		return -EINVAL;
	}

	/* erase can be done only by pages */
	if (((offset % PAGE_SIZE) != 0) || ((len % PAGE_SIZE) != 0)) {
		return -EINVAL;
	}

	/* take semaphore */
	if (k_sem_take(&dev_data->write_lock, K_NO_WAIT)) {
		return -EACCES;
	}

	while (page_nums) {
		/* check for 64K erase possibility, then check for 32K and so on.. */
		if ((page_nums >= BLOCK_64K_PAGES) && ((offset % BLOCK_64K_SIZE) == 0)) {
			/* erase 64K block */
			flash_erase_64kblock(offset);
			page_nums -= BLOCK_64K_PAGES;
			offset += BLOCK_64K_SIZE;
		} else if ((page_nums >= BLOCK_32K_PAGES) && ((offset % BLOCK_32K_SIZE) == 0)) {
			/* erase 32K block */
			flash_erase_32kblock(offset);
			page_nums -= BLOCK_32K_PAGES;
			offset += BLOCK_32K_SIZE;
		} else if ((page_nums >= SECTOR_PAGES) && ((offset % SECTOR_SIZE) == 0)) {
			/* erase sector */
			flash_erase_sector(offset);
			page_nums -= SECTOR_PAGES;
			offset += SECTOR_SIZE;
		} else {
			/* erase page */
			flash_erase_page(offset);
			page_nums--;
			offset += PAGE_SIZE;
		}
	}

	/* release semaphore */
	k_sem_give(&dev_data->write_lock);

	return 0;
}

/* API implementation: write */
static int flash_b91_write(const struct device *dev, off_t offset,
			   const void *data, size_t len)
{
	void *buf = NULL;
	struct flash_b91_data *dev_data = dev->data;

	/* return SUCCESS if len equals 0 (required by tests/drivers/flash) */
	if (!len) {
		return 0;
	}

	/* check for valid range */
	if (!flash_b91_is_range_valid(offset, len)) {
		return -EINVAL;
	}

	/* take semaphore */
	if (k_sem_take(&dev_data->write_lock, K_NO_WAIT)) {
		return -EACCES;
	}

	/* need to store data in intermediate RAM buffer in case from flash to flash write */
	if (((uint32_t)data >= FLASH_ORIGIN) &&
		((uint32_t)data < (FLASH_ORIGIN + FLASH_SIZE))) {

		buf = k_malloc(len);
		if (buf == NULL) {
			k_sem_give(&dev_data->write_lock);
			return -ENOMEM;
		}

		/* copy Flash data to RAM */
		memcpy(buf, data, len);

		/* substitute data with allocated buffer */
		data = buf;
	}

	/* write flash */
	flash_write_page(offset, len, (unsigned char *)data);

	/* if ram memory is allocated for flash writing it should be free */
	if (buf != NULL) {
		k_free(buf);
	}

	/* release semaphore */
	k_sem_give(&dev_data->write_lock);

	return 0;
}

/* API implementation: read */
static int flash_b91_read(const struct device *dev, off_t offset,
			  void *data, size_t len)
{
	ARG_UNUSED(dev);

	/* return SUCCESS if len equals 0 (required by tests/drivers/flash) */
	if (!len) {
		return 0;
	}

	/* check for valid range */
	if (!flash_b91_is_range_valid(offset, len)) {
		return -EINVAL;
	}

	/* read flash */
	flash_read_page(offset, len, (unsigned char *)data);

	return 0;
}

/* API implementation: get_parameters */
static const struct flash_parameters *
flash_b91_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_b91_parameters;
}

/* API implementation: page_layout */
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static const struct flash_pages_layout dev_layout = {
	.pages_count = FLASH_SIZE / PAGE_SIZE,
	.pages_size = PAGE_SIZE,
};

static void flash_b91_pages_layout(const struct device *dev,
				   const struct flash_pages_layout **layout,
				   size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static struct flash_b91_data flash_data;

static DEVICE_API(flash, flash_b91_api) = {
	.erase = flash_b91_erase,
	.write = flash_b91_write,
	.read = flash_b91_read,
	.get_parameters = flash_b91_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_b91_pages_layout,
#endif
};

/* Driver registration */
DEVICE_DT_INST_DEFINE(0, flash_b91_init,
		      NULL, &flash_data, NULL, POST_KERNEL,
		      CONFIG_FLASH_INIT_PRIORITY, &flash_b91_api);
