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
	uint32_t row_base;
	const uint8_t *src_ptr = data;

	uint32_t row_len = cfg->write_block_size;
	/* Buffer aligned to 4-byte boundary for uint32_t access required by HAL */
	__aligned(4) uint8_t row_buf[row_len];
	cy_en_flashdrv_status_t status;

	if (remaining_len == 0) {
		return 0;
	}

	if (offset < 0) {
		return -EINVAL;
	}

	if (offset < 0 || (cfg->max_addr - cfg->base_addr) < offset) {
		/* offset does not fit within range memory range */
		return -EINVAL;
	}

	write_offset = cfg->base_addr + offset;
	/* check bounds before starting write */
	if ((cfg->max_addr - write_offset) < remaining_len) {
		return -EINVAL;
	}

	while (remaining_len > 0) {

		/* Align address to row boundary */
		row_base = write_offset & ~(row_len - 1);

		uint32_t row_offset = write_offset - row_base;

		uint32_t chunk_len = row_len - row_offset;

		if (chunk_len > remaining_len) {
			chunk_len = remaining_len;
		}

		if ((row_offset == 0) && (chunk_len == row_len)) {
			/* Row write aligned to base addr & size */
			status = Cy_Flash_WriteRow(row_base, (const uint32_t *)src_ptr);
		} else {
			/* Read existing flash row data first to preserve existing content */
			memcpy(row_buf, (const void *)row_base, row_len);
			/* Copy new data into the row buffer at the correct offset */
			memcpy(row_buf + row_offset, src_ptr, chunk_len);

			status = Cy_Flash_WriteRow(row_base, (const uint32_t *)row_buf);
		}

		write_offset += chunk_len;
		src_ptr += chunk_len;
		remaining_len -= chunk_len;
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
