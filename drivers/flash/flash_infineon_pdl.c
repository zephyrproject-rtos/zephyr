/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT     infineon_psoc4_flash
#define SOC_NV_FLASH_NODE DT_INST_CHILD(0, flash_0)

#include <cy_flash.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <string.h>
#include <zephyr/device.h>

struct ifx_cat1_flash_config {
	uint32_t base_addr;
	uint32_t max_addr;
	const size_t write_block_size;
};

static struct flash_parameters flash_psoc4_parameters = {
	.write_block_size = DT_PROP(SOC_NV_FLASH_NODE, write_block_size),
	.erase_value = 0xFF,
	.caps = {
			.no_explicit_erase = true,
		},
};

static int flash_psoc4_write(const struct device *dev, off_t offset, const void *data,
			     size_t data_len)
{
	const struct ifx_cat1_flash_config *cfg = dev->config;
	uint32_t write_offset;
	uint32_t row_base;
	const uint8_t *data_ptr = data;

	uint32_t ROW_LEN = cfg->write_block_size;
	/* Buffer aligned to 4-byte boundary for uint32_t access required by HAL */
	__aligned(4) uint8_t row_buf[ROW_LEN];
	cy_en_flashdrv_status_t status;

	if (offset < 0) {
		return -EINVAL;
	}

	write_offset = cfg->base_addr + offset;

	/* Check bounds before starting write */
	if ((write_offset + data_len) > cfg->max_addr) {
		return -EINVAL;
	}

	while (data_len > 0) {

		/* Align address to row boundary */
		row_base = write_offset & ~(ROW_LEN - 1);

		uint32_t row_offset = write_offset - row_base;

		uint32_t write_len = ROW_LEN - row_offset;

		if (write_len > data_len) {
			write_len = data_len;
		}

		/* Read existing flash row data first to preserve existing content */
		memcpy((void *)row_buf, (const void *)row_base, ROW_LEN);

		/* Copy new data into the row buffer at the correct offset */
		memcpy((uint8_t *)row_buf + row_offset, data_ptr, write_len);

		status = Cy_Flash_WriteRow(row_base, (const uint32_t *)row_buf);
		if (status != CY_FLASH_DRV_SUCCESS) {
			return -EIO;
		}

		write_offset += write_len;
		data_ptr += write_len;
		data_len -= write_len;
	}

	return 0;
}

static int flash_psoc4_read(const struct device *dev, off_t offset, void *data, size_t data_len)
{
	const struct ifx_cat1_flash_config *cfg = dev->config;
	uint32_t read_addr;

	if (offset < 0) {
		return -EINVAL;
	}

	read_addr = cfg->base_addr + offset;

	if ((read_addr + data_len) > cfg->max_addr) {
		return -EINVAL;
	}

	memcpy(data, (const void *)read_addr, data_len);

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
static int flash_psoc4_erase(const struct device *dev, off_t offset, size_t size)
{
	const struct ifx_cat1_flash_config *cfg = dev->config;
	uint32_t ROW_LEN = cfg->write_block_size;
	uint32_t erase_offset;

	uint32_t __aligned(4) row_buf[ROW_LEN / sizeof(uint32_t)];
	cy_en_flashdrv_status_t status;

	/* Basic parameter validation */
	if (offset < 0) {
		return -EINVAL;
	}

	if ((cfg->base_addr + offset + size) > cfg->max_addr) {
		return -EINVAL;
	}

	/* Offset must be aligned to row boundary */
	if ((offset % ROW_LEN) != 0) {
		return -EINVAL;
	}

	/* Size must be a multiple of row size */
	if ((size % ROW_LEN) != 0) {
		return -EINVAL;
	}

	erase_offset = cfg->base_addr + offset;

	/* Erase by writing 0xFF to each row */
	while (size > 0) {
		/* Fill row buffer with erase value (0xFF) */
		memset((void *)row_buf, 0xFF, ROW_LEN);

		status = Cy_Flash_WriteRow(erase_offset, (const uint32_t *)row_buf);
		if (status != CY_FLASH_DRV_SUCCESS) {
			return -EIO;
		}

		erase_offset += ROW_LEN;
		size -= ROW_LEN;
	}

	return 0;
}

static const struct flash_parameters *flash_psoc4_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_psoc4_parameters;
}

static int flash_psoc4_get_size(const struct device *dev, uint64_t *size)
{
	const struct ifx_cat1_flash_config *cfg = dev->config;

	*size = cfg->max_addr - cfg->base_addr;

	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static const struct flash_pages_layout flash_psoc4_pages_layout = {
	.pages_count =
		DT_REG_SIZE(SOC_NV_FLASH_NODE) / DT_PROP(SOC_NV_FLASH_NODE, write_block_size),
	.pages_size = DT_PROP(SOC_NV_FLASH_NODE, write_block_size),
};

static void flash_psoc4_page_layout(const struct device *dev,
				    const struct flash_pages_layout **layout, size_t *layout_size)
{
	ARG_UNUSED(dev);

	*layout = &flash_psoc4_pages_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_driver_api flash_psoc4_api = {
	.read = flash_psoc4_read,
	.write = flash_psoc4_write,
	.erase = flash_psoc4_erase,
	.get_parameters = flash_psoc4_get_parameters,
	.get_size = flash_psoc4_get_size,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_psoc4_page_layout,
#endif
};

static const struct ifx_cat1_flash_config ifx_cat1_flash_config = {
	.base_addr = DT_REG_ADDR(SOC_NV_FLASH_NODE),
	.max_addr = DT_REG_ADDR(SOC_NV_FLASH_NODE) + DT_REG_SIZE(SOC_NV_FLASH_NODE),
	.write_block_size = DT_PROP(SOC_NV_FLASH_NODE, write_block_size),
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, &ifx_cat1_flash_config, POST_KERNEL,
		      CONFIG_FLASH_INIT_PRIORITY, &flash_psoc4_api);
