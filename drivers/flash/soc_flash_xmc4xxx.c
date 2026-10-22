/*
 * Copyright (c) 2022 Schlumberger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_xmc4xxx_flash_controller

#define FLASH_WRITE_BLK_SZ DT_PROP(DT_INST(0, infineon_xmc4xxx_nv_flash), write_block_size)

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>

#include <xmc_flash.h>

struct flash_xmc4xxx_data {
	struct k_sem sem;
};

struct flash_xmc4xxx_config {
	uint32_t base;
	uint32_t size;
	struct flash_parameters parameters;
};

static inline bool is_aligned_32(uint32_t data)
{
	return (data & 0x3) ? false : true;
}

static int flash_xmc4xxx_init(const struct device *dev)
{
	struct flash_xmc4xxx_data *dev_data = dev->data;

	k_sem_init(&dev_data->sem, 1, 1);
	return 0;
}

#define SET_PAGES(node_id)    \
	{.pages_count = DT_PROP(node_id, pages_count), .pages_size = DT_PROP(node_id, pages_size)},

#if CONFIG_FLASH_PAGE_LAYOUT
static const struct flash_pages_layout flash_xmc4xxx_pages_layout[] = {
	DT_FOREACH_CHILD(DT_NODELABEL(pages_layout), SET_PAGES)};

static void flash_xmc4xxx_page_layout(const struct device *dev,
				      const struct flash_pages_layout **layout, size_t *layout_size)
{
	*layout = &flash_xmc4xxx_pages_layout[0];
	*layout_size = ARRAY_SIZE(flash_xmc4xxx_pages_layout);
}
#endif

static int flash_xmc4xxx_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct flash_xmc4xxx_config *dev_config = dev->config;

	if (offset < 0 || offset + len > dev_config->size) {
		return -1;
	}
	memcpy(data, (void *)(dev_config->base + offset), len);
	return 0;
}

static __aligned(4) uint8_t
	aligned_page[DT_PROP(DT_INST(0, infineon_xmc4xxx_nv_flash), write_block_size)];

static int flash_xmc4xxx_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	struct flash_xmc4xxx_data *dev_data = dev->data;
	const struct flash_xmc4xxx_config *dev_config = dev->config;
	uint32_t irq_key;
	uint32_t flash_addr = dev_config->base;
	const uint8_t *src = data;
	int num_pages;

	if (offset < 0 || offset + len > dev_config->size) {
		return -1;
	}

	if (len % dev_config->parameters.write_block_size ||
	    offset % dev_config->parameters.write_block_size > 0) {
		return -1;
	}

	k_sem_take(&dev_data->sem, K_FOREVER);

	/* erase and write operations must be on the uncached base address */
	flash_addr |= 0xc000000;
	flash_addr += offset;

	num_pages = len / dev_config->parameters.write_block_size;
	for (int i = 0; i < num_pages; i++) {
		uint32_t *src_ptr = (uint32_t *)src;

		/* XMC_FLASH_ProgramPage() needs a 32 bit aligned input. */
		/* Copy the data to an aligned array if needed.  */
		if (!is_aligned_32((uint32_t)src)) {
			memcpy(aligned_page, src, dev_config->parameters.write_block_size);
			src_ptr = (uint32_t *)aligned_page;
		}

		irq_key = irq_lock();
		XMC_FLASH_ProgramPage((uint32_t *)flash_addr, src_ptr);
		irq_unlock(irq_key);
		flash_addr += dev_config->parameters.write_block_size;
		src += dev_config->parameters.write_block_size;
	}

	k_sem_give(&dev_data->sem);
	return 0;
}

#if CONFIG_FLASH_PAGE_LAYOUT
static int flash_xmc4xxx_erase(const struct device *dev, off_t offset, size_t size)
{
	struct flash_xmc4xxx_data *dev_data = dev->data;
	const struct flash_xmc4xxx_config *dev_config = dev->config;
	uint32_t irq_key;
	uint32_t offset_page = 0;
	int ret = 0;

	if (offset < 0 || offset > dev_config->size) {
		return -1;
	}

	k_sem_take(&dev_data->sem, K_FOREVER);

	for (int i = 0; i < ARRAY_SIZE(flash_xmc4xxx_pages_layout); i++) {
		for (int k = 0; k < flash_xmc4xxx_pages_layout[i].pages_count; k++) {
			uint32_t pages_size = flash_xmc4xxx_pages_layout[i].pages_size;
			/* erase and write operations must be on the uncached base address */
			uint32_t flash_addr = dev_config->base | 0xc000000;

			if (offset == offset_page && size >= pages_size) {
				flash_addr += offset;
				irq_key = irq_lock();
				XMC_FLASH_EraseSector((uint32_t *)flash_addr);
				irq_unlock(irq_key);

				size -= pages_size;
				offset += pages_size;
			}
			offset_page += pages_size;

			if (size == 0) {
				ret = 0;
				goto finish;
			}

			/* page not aligned with offset address */
			if (offset_page > offset) {
				ret = -1;
				goto finish;
			}
		}
	}

finish:
	k_sem_give(&dev_data->sem);
	return ret;
}
#else
static int flash_xmc4xxx_erase(const struct device *dev, off_t offset, size_t size)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(offset);
	ARG_UNUSED(size);

	return -ENOTSUP;
}
#endif

static const struct flash_parameters *flash_xmc4xxx_get_parameters(const struct device *dev)
{
	const struct flash_xmc4xxx_config *dev_config = dev->config;

	return &dev_config->parameters;
}

static DEVICE_API(flash, flash_xmc4xxx_api) = {
	.erase = flash_xmc4xxx_erase,
	.write = flash_xmc4xxx_write,
	.read = flash_xmc4xxx_read,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_xmc4xxx_page_layout,
#endif
	.get_parameters = flash_xmc4xxx_get_parameters,
};

static struct flash_xmc4xxx_data flash_xmc4xxx_data_0;
static struct flash_xmc4xxx_config flash_xmc4xxx_cfg_0 = {
	.base = DT_REG_ADDR(DT_INST(0, infineon_xmc4xxx_nv_flash)),
	.size = DT_REG_SIZE(DT_INST(0, infineon_xmc4xxx_nv_flash)),
	.parameters = {.write_block_size = FLASH_WRITE_BLK_SZ, .erase_value = 0}};

DEVICE_DT_INST_DEFINE(0, flash_xmc4xxx_init, NULL, &flash_xmc4xxx_data_0, &flash_xmc4xxx_cfg_0,
		      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, &flash_xmc4xxx_api);
