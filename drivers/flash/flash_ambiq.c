/*
 * Copyright (c) 2023 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_flash_controller

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>

#include <am_mcu_apollo.h>

LOG_MODULE_REGISTER(flash_ambiq, CONFIG_FLASH_LOG_LEVEL);

#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)
#define SOC_NV_FLASH_ADDR DT_REG_ADDR(SOC_NV_FLASH_NODE)
#define SOC_NV_FLASH_SIZE DT_REG_SIZE(SOC_NV_FLASH_NODE)
#if (CONFIG_SOC_SERIES_APOLLO4X)
#define MIN_WRITE_SIZE 16
#else
#define MIN_WRITE_SIZE 4
#endif /* CONFIG_SOC_SERIES_APOLLO4X */
#define FLASH_WRITE_BLOCK_SIZE MAX(DT_PROP(SOC_NV_FLASH_NODE, write_block_size), MIN_WRITE_SIZE)
#define FLASH_ERASE_BLOCK_SIZE DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)

BUILD_ASSERT((FLASH_WRITE_BLOCK_SIZE & (MIN_WRITE_SIZE - 1)) == 0,
	     "The flash write block size must be a multiple of MIN_WRITE_SIZE!");

#define FLASH_ERASE_BYTE 0xFF
#define FLASH_ERASE_WORD                                                                           \
	(((uint32_t)(FLASH_ERASE_BYTE << 24)) | ((uint32_t)(FLASH_ERASE_BYTE << 16)) |             \
	 ((uint32_t)(FLASH_ERASE_BYTE << 8)) | ((uint32_t)FLASH_ERASE_BYTE))

#if defined(CONFIG_MULTITHREADING)
static struct k_sem flash_ambiq_sem;
#define FLASH_SEM_INIT() k_sem_init(&flash_ambiq_sem, 1, 1)
#define FLASH_SEM_TAKE() k_sem_take(&flash_ambiq_sem, K_FOREVER)
#define FLASH_SEM_GIVE() k_sem_give(&flash_ambiq_sem)
#else
#define FLASH_SEM_INIT()
#define FLASH_SEM_TAKE()
#define FLASH_SEM_GIVE()
#endif /* CONFIG_MULTITHREADING */

static const struct flash_parameters flash_ambiq_parameters = {
	.write_block_size = FLASH_WRITE_BLOCK_SIZE,
	.erase_value = FLASH_ERASE_BYTE,
#if defined(CONFIG_SOC_SERIES_APOLLO4X)
	.caps = {
		.no_explicit_erase = true,
	},
#endif
};

static bool flash_ambiq_valid_range(off_t offset, size_t len)
{
	if ((offset < 0) || offset >= SOC_NV_FLASH_SIZE || (SOC_NV_FLASH_SIZE - offset) < len) {
		return false;
	}

	return true;
}

static int flash_ambiq_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	ARG_UNUSED(dev);

	if (!flash_ambiq_valid_range(offset, len)) {
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	memcpy(data, (uint8_t *)(SOC_NV_FLASH_ADDR + offset), len);

	return 0;
}

static int flash_ambiq_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	ARG_UNUSED(dev);

	int ret = 0;
	unsigned int key = 0;
	uint32_t aligned[FLASH_WRITE_BLOCK_SIZE / sizeof(uint32_t)] = {0};
	uint32_t *src = (uint32_t *)data;

	/* write address must be block size aligned and the write length must be multiple of block
	 * size.
	 */
	if (!flash_ambiq_valid_range(offset, len) ||
	    ((uint32_t)offset & (FLASH_WRITE_BLOCK_SIZE - 1)) ||
	    (len & (FLASH_WRITE_BLOCK_SIZE - 1))) {
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	FLASH_SEM_TAKE();

	key = irq_lock();

	for (int i = 0; i < len / FLASH_WRITE_BLOCK_SIZE; i++) {
		for (int j = 0; j < FLASH_WRITE_BLOCK_SIZE / sizeof(uint32_t); j++) {
			/* Make sure the source data is 4-byte aligned. */
			aligned[j] = UNALIGNED_GET((uint32_t *)src);
			src++;
		}
#if (CONFIG_SOC_SERIES_APOLLO4X)
		ret = am_hal_mram_main_program(
			AM_HAL_MRAM_PROGRAM_KEY, aligned,
			(uint32_t *)(SOC_NV_FLASH_ADDR + offset + i * FLASH_WRITE_BLOCK_SIZE),
			FLASH_WRITE_BLOCK_SIZE / sizeof(uint32_t));
#elif (CONFIG_SOC_SERIES_APOLLO3X)
		ret = am_hal_flash_program_main(
			AM_HAL_FLASH_PROGRAM_KEY, aligned,
			(uint32_t *)(SOC_NV_FLASH_ADDR + offset + i * FLASH_WRITE_BLOCK_SIZE),
			FLASH_WRITE_BLOCK_SIZE / sizeof(uint32_t));
#endif /* CONFIG_SOC_SERIES_APOLLO4X */
		if (ret) {
			break;
		}
	}

	irq_unlock(key);

	FLASH_SEM_GIVE();

	return ret;
}

static int flash_ambiq_erase(const struct device *dev, off_t offset, size_t len)
{
	ARG_UNUSED(dev);

	int ret = 0;

	if (!flash_ambiq_valid_range(offset, len)) {
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

#if (CONFIG_SOC_SERIES_APOLLO4X)
	/* The erase address and length alignment check will be done in HAL.*/
#elif (CONFIG_SOC_SERIES_APOLLO3X)
	if ((offset % FLASH_ERASE_BLOCK_SIZE) != 0) {
		LOG_ERR("offset 0x%lx is not on a page boundary", (long)offset);
		return -EINVAL;
	}

	if ((len % FLASH_ERASE_BLOCK_SIZE) != 0) {
		LOG_ERR("len %zu is not multiple of a page size", len);
		return -EINVAL;
	}
#endif /* CONFIG_SOC_SERIES_APOLLO4X */

	FLASH_SEM_TAKE();

#if (CONFIG_SOC_SERIES_APOLLO4X)
	ret = am_hal_mram_main_fill(AM_HAL_MRAM_PROGRAM_KEY, FLASH_ERASE_WORD,
				    (uint32_t *)(SOC_NV_FLASH_ADDR + offset),
				    (len / sizeof(uint32_t)));
#elif (CONFIG_SOC_SERIES_APOLLO3X)
	unsigned int key = 0;

	key = irq_lock();

	ret = am_hal_flash_page_erase(
		AM_HAL_FLASH_PROGRAM_KEY,
		AM_HAL_FLASH_ADDR2INST(((uint32_t)SOC_NV_FLASH_ADDR + offset)),
		AM_HAL_FLASH_ADDR2PAGE(((uint32_t)SOC_NV_FLASH_ADDR + offset)));

	irq_unlock(key);
#endif /* CONFIG_SOC_SERIES_APOLLO4X */

	FLASH_SEM_GIVE();

	return ret;
}

static const struct flash_parameters *flash_ambiq_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_ambiq_parameters;
}

#if CONFIG_FLASH_PAGE_LAYOUT
static const struct flash_pages_layout pages_layout = {
	.pages_count = SOC_NV_FLASH_SIZE / FLASH_ERASE_BLOCK_SIZE,
	.pages_size = FLASH_ERASE_BLOCK_SIZE,
};

static void flash_ambiq_pages_layout(const struct device *dev,
				     const struct flash_pages_layout **layout, size_t *layout_size)
{
	ARG_UNUSED(dev);

	*layout = &pages_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_driver_api flash_ambiq_driver_api = {
	.read = flash_ambiq_read,
	.write = flash_ambiq_write,
	.erase = flash_ambiq_erase,
	.get_parameters = flash_ambiq_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_ambiq_pages_layout,
#endif
};

static int flash_ambiq_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	FLASH_SEM_INIT();

	return 0;
}

DEVICE_DT_INST_DEFINE(0, flash_ambiq_init, NULL, NULL, NULL, POST_KERNEL,
		      CONFIG_FLASH_INIT_PRIORITY, &flash_ambiq_driver_api);
