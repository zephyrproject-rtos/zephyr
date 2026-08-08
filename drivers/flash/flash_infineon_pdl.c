/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_flash_controller

#include <string.h>
#include <assert.h>

#include <cy_flash.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include "flash_priv.h"

#define SOC_NV_FLASH_NODE SOC_NV_FLASH_CHILD_NODE(0)

#define IFX_FLASH_BASE DT_REG_ADDR(SOC_NV_FLASH_NODE)
#define IFX_FLASH_SIZE DT_REG_SIZE(SOC_NV_FLASH_NODE)
#define IFX_FLASH_MAX  (IFX_FLASH_BASE + IFX_FLASH_SIZE)
#define IFX_FLASH_ROW_WORDS (DT_PROP(SOC_NV_FLASH_NODE, erase_block_size) / sizeof(uint32_t))

BUILD_ASSERT(IFX_FLASH_SIZE > 0, "Flash size must be greater than 0");

BUILD_ASSERT(IFX_FLASH_MAX > IFX_FLASH_BASE, "Flash max_addr must be greater than base_addr");

BUILD_ASSERT(DT_PROP(SOC_NV_FLASH_NODE, write_block_size) ==
		     DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
	     "Infineon flash: write_block_size must equal erase_block_size");

struct ifx_flash_config {
	uint32_t base_addr;
	uint32_t max_addr;
	const size_t write_block_size;
	const size_t erase_block_size;
};

static K_MUTEX_DEFINE(flash_write_lock);

static struct flash_parameters flash_parameters = {
	.write_block_size = DT_PROP(SOC_NV_FLASH_NODE, write_block_size),
	.erase_value = CONFIG_FLASH_INFINEON_ERASE_VALUE,
	/* clang-format off */
	.caps = {
		.no_explicit_erase = IS_ENABLED(CONFIG_FLASH_HAS_NO_EXPLICIT_ERASE),
	},
	/* clang-format on */
};

static int flash_ifx_write(const struct device *dev, off_t offset, const void *data,
			   size_t remaining_len)
{
	const struct ifx_flash_config *cfg = dev->config;
	uint32_t write_offset;
	const uint32_t row_len = cfg->write_block_size;
	const uint8_t *src_ptr;

	static uint32_t __aligned(4) row_buf[IFX_FLASH_ROW_WORDS];
	cy_en_flashdrv_status_t status;
	int ret = 0;

	if (data == NULL) {
		return -EINVAL;
	}

	if (remaining_len == 0) {
		return 0;
	}

	if (offset < 0) {
		return -EINVAL;
	}

	/* Write offset and size must be aligned to write_block_size */
	if (((offset % row_len) != 0) || ((remaining_len % row_len) != 0)) {
		return -EINVAL;
	}

	if ((cfg->max_addr - cfg->base_addr) < (uint32_t)offset) {
		/* offset does not fit within flash memory range */
		return -EINVAL;
	}

	src_ptr = data;
	write_offset = cfg->base_addr + offset;
	/* check bounds before starting write */
	if ((cfg->max_addr - write_offset) < remaining_len) {
		return -EINVAL;
	}

	k_mutex_lock(&flash_write_lock, K_FOREVER);

	while (remaining_len > 0) {
		memcpy(row_buf, src_ptr, row_len);

#if defined(CONFIG_SOC_SERIES_PSC3)
		/* PSC3 advertises explicit erase (no_explicit_erase = false) */
		status = Cy_Flash_ProgramRow(write_offset, row_buf);
#else
		/* PSoC4 auto-erases the row as part of the write. */
		status = Cy_Flash_WriteRow(write_offset, row_buf);
#endif

		if (status != CY_FLASH_DRV_SUCCESS) {
			ret = -EIO;
			break;
		}

		write_offset += row_len;
		src_ptr += row_len;
		remaining_len -= row_len;
	}

	k_mutex_unlock(&flash_write_lock);

	return ret;
}

static int flash_ifx_read(const struct device *dev, off_t offset, void *data, size_t remaining_len)
{
	const struct ifx_flash_config *cfg = dev->config;
	uint32_t read_offset;

	if (remaining_len == 0) {
		return 0;
	}

	if (offset < 0 || (cfg->max_addr - cfg->base_addr) < offset) {
		/* offset does not fit within range memory range */
		return -EINVAL;
	}

	read_offset = cfg->base_addr + offset;
	if ((cfg->max_addr - read_offset) < remaining_len) {
		return -EINVAL;
	}

	memcpy(data, (const void *)read_offset, remaining_len);

	return 0;
}

/**
 * @note On PSC3: Cy_Flash_EraseRow() performs a true
 * hardware row erase by calling Boot-ROM cyboot_flash_erase_row in blocking
 * mode.  An explicit erase is required before programming, so
 * caps.no_explicit_erase is set to false for PSC3.
 *
 * @note On PSoC4: Cy_Flash_WriteRow() automatically erases rows before
 * programming.  caps.no_explicit_erase is set to true for PSoC4 to reflect
 * that callers do not need to erase before writing.
 */
static int flash_ifx_erase(const struct device *dev, off_t offset, size_t size)
{
	const struct ifx_flash_config *cfg = dev->config;
	uint32_t row_len = cfg->erase_block_size;
	uint32_t erase_offset;
	cy_en_flashdrv_status_t status;
	int ret = 0;

	if (size == 0) {
		return 0;
	}

	/* Offset must be aligned to row boundary */
	if ((offset % row_len) != 0) {
		return -EINVAL;
	}

	/* Size must be a multiple of row size */
	if ((size % row_len) != 0) {
		return -EINVAL;
	}

	if (offset < 0 || (cfg->max_addr - cfg->base_addr) < offset) {
		/* offset does not fit within range memory range */
		return -EINVAL;
	}

	erase_offset = cfg->base_addr + offset;

	if ((cfg->max_addr - erase_offset) < size) {
		return -EINVAL;
	}

	k_mutex_lock(&flash_write_lock, K_FOREVER);

#if defined(CONFIG_SOC_SERIES_PSC3)
	/* PSC3 will use dedicated EraseRow for a true hardware erase */
	while (size > 0) {
		status = Cy_Flash_EraseRow(erase_offset);
		if (status != CY_FLASH_DRV_SUCCESS) {
			ret = -EIO;
			break;
		}

		erase_offset += row_len;
		size -= row_len;
	}
#else
	/* On PSoC4, we write the erase value to the row, as
	 * there is no dedicated erase operation (an erase is
	 * always performed before a write).
	 */
	{
		static uint32_t __aligned(4) row_buf[IFX_FLASH_ROW_WORDS];

		memset(row_buf, CONFIG_FLASH_INFINEON_ERASE_VALUE, sizeof(row_buf));

		while (size > 0) {
			status = Cy_Flash_WriteRow(erase_offset, row_buf);
			if (status != CY_FLASH_DRV_SUCCESS) {
				ret = -EIO;
				break;
			}

			erase_offset += row_len;
			size -= row_len;
		}
	}
#endif

	k_mutex_unlock(&flash_write_lock);

	return ret;
}

static const struct flash_parameters *flash_ifx_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_parameters;
}

static int flash_ifx_get_size(const struct device *dev, uint64_t *size)
{
	const struct ifx_flash_config *cfg = dev->config;

	*size = cfg->max_addr - cfg->base_addr;

	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static const struct flash_pages_layout flash_pages_layout = {
	.pages_count =
		DT_REG_SIZE(SOC_NV_FLASH_NODE) / DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
	.pages_size = DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
};

static void flash_ifx_page_layout(const struct device *dev,
				  const struct flash_pages_layout **layout, size_t *layout_size)
{
	ARG_UNUSED(dev);

	*layout = &flash_pages_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static DEVICE_API(flash, flash_infineon_api) = {
	.read = flash_ifx_read,
	.write = flash_ifx_write,
	.erase = flash_ifx_erase,
	.get_parameters = flash_ifx_get_parameters,
	.get_size = flash_ifx_get_size,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_ifx_page_layout,
#endif
};

static const struct ifx_flash_config ifx_flash_config_parameters = {
	.base_addr = DT_REG_ADDR(SOC_NV_FLASH_NODE),
	.max_addr = DT_REG_ADDR(SOC_NV_FLASH_NODE) + DT_REG_SIZE(SOC_NV_FLASH_NODE),
	.write_block_size = DT_PROP(SOC_NV_FLASH_NODE, write_block_size),
	.erase_block_size = DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, &ifx_flash_config_parameters, POST_KERNEL,
		      CONFIG_FLASH_INIT_PRIORITY, &flash_infineon_api);
