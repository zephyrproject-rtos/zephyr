/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT     infineon_flash_controller
#define SOC_NV_FLASH_NODE DT_INST_CHILD(0, flash_0)

#include <string.h>
#include <assert.h>

#include <cy_flash.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>

#define IFX_FLASH_BASE DT_REG_ADDR(SOC_NV_FLASH_NODE)
#define IFX_FLASH_SIZE DT_REG_SIZE(SOC_NV_FLASH_NODE)
#define IFX_FLASH_MAX  (IFX_FLASH_BASE + IFX_FLASH_SIZE)

BUILD_ASSERT(IFX_FLASH_SIZE > 0, "Flash size must be greater than 0");

BUILD_ASSERT(IFX_FLASH_MAX > IFX_FLASH_BASE, "Flash max_addr must be greater than base_addr");

struct ifx_flash_config {
	uint32_t base_addr;
	uint32_t max_addr;
	const size_t write_block_size;
	const size_t erase_block_size;
};

static struct flash_parameters flash_parameters = {
	.write_block_size = DT_PROP(SOC_NV_FLASH_NODE, write_block_size),
	.erase_value = 0xFF,
	.caps = {
			.no_explicit_erase = true,
		},
};

static int flash_ifx_write(const struct device *dev, off_t offset, const void *data,
			   size_t remaining_len)
{
	const struct ifx_flash_config *cfg = dev->config;
	uint32_t write_offset;
	const uint8_t *src_ptr = data;

	uint32_t row_len = cfg->write_block_size;
	cy_en_flashdrv_status_t status;

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

	if ((cfg->max_addr - cfg->base_addr) < offset) {
		/* offset does not fit within flash memory range */
		return -EINVAL;
	}

	write_offset = cfg->base_addr + offset;
	/* check bounds before starting write */
	if ((cfg->max_addr - write_offset) < remaining_len) {
		return -EINVAL;
	}

	/*
	 * row_len is always a multiple of 4, so source pointer alignment is
	 * invariant across iterations — check once before the loop.
	 */
	if (((uintptr_t)src_ptr & 0x3) != 0) {
		/* Source not 4-byte aligned; stage each row through an aligned buffer */
		uint32_t __aligned(4) row_buf[row_len / sizeof(uint32_t)];

		while (remaining_len > 0) {
			memcpy(row_buf, src_ptr, row_len);
			status = Cy_Flash_WriteRow(write_offset, row_buf);
			if (status != CY_FLASH_DRV_SUCCESS) {
				return -EIO;
			}

			write_offset += row_len;
			src_ptr += row_len;
			remaining_len -= row_len;
		}
	} else {
		while (remaining_len > 0) {
			status = Cy_Flash_WriteRow(write_offset,
						   (const uint32_t *)src_ptr);
			if (status != CY_FLASH_DRV_SUCCESS) {
				return -EIO;
			}

			write_offset += row_len;
			src_ptr += row_len;
			remaining_len -= row_len;
		}
	}

	return 0;
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
 * @note The PSoC4 flash hardware automatically erases rows before writing
 * when using Cy_Flash_WriteRow(). This driver sets caps.no_explicit_erase = true
 * to indicate that explicit erase is not required for write operations.
 *
 * However, this function implements erase by writing the erase value (0xFF)
 * to the flash using the same write mechanism. This ensures API compatibility
 * while leveraging the hardware's auto-erase-on-write behavior.
 */
static int flash_ifx_erase(const struct device *dev, off_t offset, size_t size)
{
	const struct ifx_flash_config *cfg = dev->config;
	uint32_t row_len = cfg->erase_block_size;
	uint32_t erase_offset;

	uint32_t __aligned(4) row_buf[row_len / sizeof(uint32_t)];
	cy_en_flashdrv_status_t status;

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
	/* Erase by writing 0xFF to each row */
	while (size > 0) {
		/* Fill row buffer with erase value (0xFF) */
		memset((void *)row_buf, 0xFF, row_len);

		status = Cy_Flash_WriteRow(erase_offset, (const uint32_t *)row_buf);
		if (status != CY_FLASH_DRV_SUCCESS) {
			return -EIO;
		}

		erase_offset += row_len;
		size -= row_len;
	}

	return 0;
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
		DT_REG_SIZE(SOC_NV_FLASH_NODE) / DT_PROP(SOC_NV_FLASH_NODE, write_block_size),
	.pages_size = DT_PROP(SOC_NV_FLASH_NODE, write_block_size),
};

static void flash_ifx_page_layout(const struct device *dev,
				  const struct flash_pages_layout **layout, size_t *layout_size)
{
	ARG_UNUSED(dev);

	*layout = &flash_pages_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_driver_api flash_infineon_api = {
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
