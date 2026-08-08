/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Infineon SMIF NOR flash device driver.
 *
 * Implements the Zephyr flash API for a NOR device attached to an
 * infineon,smif controller. This driver is a thin shim: the parent controller
 * (infineon,smif) performs the SMIF/PDL enumeration and owns the shared PDL
 * memory context; this driver issues per-slot read / program / erase against
 * that context using the device's chip-select as the PDL memory number.
 *
 * The command set is described statically by the controller from devicetree,
 * so this driver only advertises geometry (size, erase-block and write-block
 * sizes) taken from devicetree.
 */

#define DT_DRV_COMPAT infineon_smif_nor

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>

#include "cy_pdl.h"

#include "flash_infineon_smif.h"

LOG_MODULE_REGISTER(flash_infineon_smif_nor, CONFIG_FLASH_LOG_LEVEL);

struct ifx_smif_nor_config {
	const struct device *ctrl;
	uint8_t mem_num;
	uint32_t total_size;
	uint32_t erase_block_size;
	struct flash_parameters params;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	const struct flash_pages_layout *pages_layout;
	size_t pages_layout_size;
#endif
};

static int ifx_smif_nor_validate_range(const struct ifx_smif_nor_config *cfg, off_t offset,
				       size_t len)
{
	if (offset < 0 ||  offset > (off_t)cfg->total_size) {
		LOG_ERR("Offset %ld is out of bounds", (long)offset);
		return -EINVAL;
	}

	if (len > cfg->total_size - (size_t)offset) {
		LOG_ERR("Length %zu exceeds memory bounds at offset %ld", len, (long)offset);
		return -EINVAL;
	}

	return 0;
}

static int ifx_smif_nor_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct ifx_smif_nor_config *cfg = dev->config;
	struct ifx_smif_controller_data *ctrl = cfg->ctrl->data;
	cy_en_smif_status_t status;
	int ret;

	if (len == 0) {
		return 0;
	}

	ret = ifx_smif_nor_validate_range(cfg, offset, len);
	if (ret) {
		return ret;
	}

	k_sem_take(&ctrl->lock, K_FOREVER);
	status = Cy_SMIF_MemNumRead(&ctrl->mem_context, cfg->mem_num, (uint32_t)offset,
				    (uint8_t *)buf, (uint32_t)len);
	k_sem_give(&ctrl->lock);

	if (status != CY_SMIF_SUCCESS) {
		LOG_ERR("Error reading @ %ld (Err:0x%x)", (long)offset, status);
		return -EIO;
	}

	return 0;
}

static int ifx_smif_nor_write(const struct device *dev, off_t offset, const void *buf, size_t len)
{
	const struct ifx_smif_nor_config *cfg = dev->config;
	struct ifx_smif_controller_data *ctrl = cfg->ctrl->data;
	cy_en_smif_status_t status;
	int ret;

	if (len == 0) {
		return 0;
	}

	ret = ifx_smif_nor_validate_range(cfg, offset, len);
	if (ret) {
		return ret;
	}

	k_sem_take(&ctrl->lock, K_FOREVER);
	status = Cy_SMIF_MemNumWrite(&ctrl->mem_context, cfg->mem_num, (uint32_t)offset,
				     (const uint8_t *)buf, (uint32_t)len);
	k_sem_give(&ctrl->lock);

	if (status != CY_SMIF_SUCCESS) {
		LOG_ERR("Error writing @ %ld (Err:0x%x)", (long)offset, status);
		return -EIO;
	}

	return 0;
}

/* Require the erase range to cover whole sectors. On a hybrid device the sector
 * size varies by region, so walk the page layout and check offset and size
 * against the sector boundaries of the regions they span; without a layout use
 * the uniform erase-block size.
 */
static int ifx_smif_nor_check_erase_align(const struct ifx_smif_nor_config *cfg, off_t offset,
					  size_t size)
{
	/* A whole-device erase covers every sector and is trivially aligned;
	 * return early instead of walking the region layout.
	 */
	if ((offset == 0) && (size == cfg->total_size)) {
		return 0;
	}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
	off_t region_start = 0;
	off_t cur = offset;
	size_t remaining = size;

	for (size_t i = 0; (i < cfg->pages_layout_size) && (remaining > 0); i++) {
		size_t sector = cfg->pages_layout[i].pages_size;
		off_t region_end =
			region_start + (off_t)cfg->pages_layout[i].pages_count * (off_t)sector;

		while ((cur >= region_start) && (cur < region_end) && (remaining > 0)) {
			if ((((size_t)(cur - region_start)) % sector) != 0) {
				return -EINVAL;
			}
			if (remaining < sector) {
				return -EINVAL;
			}
			cur += (off_t)sector;
			remaining -= sector;
		}
		region_start = region_end;
	}

	return (remaining == 0) ? 0 : -EINVAL;
#else
	if (((offset % cfg->erase_block_size) != 0) || ((size % cfg->erase_block_size) != 0)) {
		return -EINVAL;
	}

	return 0;
#endif
}

static int ifx_smif_nor_erase(const struct device *dev, off_t offset, size_t size)
{
	const struct ifx_smif_nor_config *cfg = dev->config;
	struct ifx_smif_controller_data *ctrl = cfg->ctrl->data;
	cy_en_smif_status_t status;
	int ret;

	if (size == 0) {
		return 0;
	}

	ret = ifx_smif_nor_validate_range(cfg, offset, size);
	if (ret) {
		return ret;
	}

	ret = ifx_smif_nor_check_erase_align(cfg, offset, size);
	if (ret) {
		LOG_ERR("Erase range (offset %ld, size %zu) not aligned to sector boundaries",
			(long)offset, size);
		return ret;
	}

	k_sem_take(&ctrl->lock, K_FOREVER);
	if ((offset == 0) && (size == cfg->total_size)) {
		status = Cy_SMIF_MemNumEraseChip(&ctrl->mem_context, cfg->mem_num);
	} else {
		status = Cy_SMIF_MemNumEraseSector(&ctrl->mem_context, cfg->mem_num,
						   (uint32_t)offset, (uint32_t)size);
	}
	k_sem_give(&ctrl->lock);

	if (status != CY_SMIF_SUCCESS) {
		LOG_ERR("Error erasing @ %ld (Err:0x%x)", (long)offset, status);
		return -EIO;
	}

	return 0;
}

static const struct flash_parameters *ifx_smif_nor_get_parameters(const struct device *dev)
{
	const struct ifx_smif_nor_config *cfg = dev->config;

	return &cfg->params;
}

static int ifx_smif_nor_get_size(const struct device *dev, uint64_t *size)
{
	const struct ifx_smif_nor_config *cfg = dev->config;

	*size = cfg->total_size;

	return 0;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static void ifx_smif_nor_page_layout(const struct device *dev,
				     const struct flash_pages_layout **layout, size_t *layout_size)
{
	const struct ifx_smif_nor_config *cfg = dev->config;

	*layout = cfg->pages_layout;
	*layout_size = cfg->pages_layout_size;
}
#endif

static int ifx_smif_nor_init(const struct device *dev)
{
	const struct ifx_smif_nor_config *cfg = dev->config;
	const struct ifx_smif_controller_data *ctrl = cfg->ctrl->data;

	if (!device_is_ready(cfg->ctrl)) {
		LOG_ERR("SMIF controller %s not ready", cfg->ctrl->name);
		return -ENODEV;
	}

	if (!ctrl->ready) {
		LOG_ERR("SMIF controller %s enumeration failed", cfg->ctrl->name);
		return -EIO;
	}

	return 0;
}

static DEVICE_API(flash, ifx_smif_nor_driver_api) = {
	.read = ifx_smif_nor_read,
	.write = ifx_smif_nor_write,
	.erase = ifx_smif_nor_erase,
	.get_parameters = ifx_smif_nor_get_parameters,
	.get_size = ifx_smif_nor_get_size,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = ifx_smif_nor_page_layout,
#endif
};

#ifdef CONFIG_FLASH_PAGE_LAYOUT
/* One flash page-layout entry per hybrid erase region: a device with the
 * "infineon,hybrid-region-*" arrays has non-uniform sectors, so describe each
 * region by its sector count and sector size. A uniform device emits a single
 * entry covering the whole array in erase-block-size sectors.
 */
#define IFX_SMIF_NOR_HYBRID_LAYOUT_ENTRY(node_id, prop, idx)                                        \
	{                                                                                          \
		.pages_count = DT_PROP_BY_IDX(node_id, infineon_hybrid_region_sectors, idx),        \
		.pages_size = DT_PROP_BY_IDX(node_id, infineon_hybrid_region_erase_size, idx),      \
	}

#define IFX_SMIF_NOR_UNIFORM_LAYOUT_ENTRY(n)                                                        \
	{                                                                                          \
		.pages_count = DT_INST_REG_SIZE(n) / DT_INST_PROP(n, erase_block_size),             \
		.pages_size = DT_INST_PROP(n, erase_block_size),                                    \
	}

#define IFX_SMIF_NOR_LAYOUT_DEFINE(n)                                                               \
	static const struct flash_pages_layout ifx_smif_nor_layout_##n[] = {                       \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(n, infineon_hybrid_region_sectors),              \
			    (DT_INST_FOREACH_PROP_ELEM_SEP(n, infineon_hybrid_region_sectors,      \
							   IFX_SMIF_NOR_HYBRID_LAYOUT_ENTRY, (,))), \
			    (IFX_SMIF_NOR_UNIFORM_LAYOUT_ENTRY(n)))                                 \
	};

#define IFX_SMIF_NOR_PAGES_LAYOUT_FIELDS(n)                                                         \
	.pages_layout = ifx_smif_nor_layout_##n,                                                    \
	.pages_layout_size = ARRAY_SIZE(ifx_smif_nor_layout_##n),
#else
#define IFX_SMIF_NOR_LAYOUT_DEFINE(n)
#define IFX_SMIF_NOR_PAGES_LAYOUT_FIELDS(n)
#endif

#define IFX_SMIF_NOR_INST(n)                                                                        \
	IFX_SMIF_NOR_LAYOUT_DEFINE(n)                                                               \
	static const struct ifx_smif_nor_config ifx_smif_nor_config_##n = {                        \
		.ctrl = DEVICE_DT_GET(DT_INST_PARENT(n)),                                           \
		.mem_num = DT_INST_PROP(n, infineon_chip_select),                                   \
		.total_size = DT_INST_REG_SIZE(n),                                                  \
		.erase_block_size = DT_INST_PROP(n, erase_block_size),                              \
		.params = {                                                                         \
			.write_block_size = DT_INST_PROP(n, write_block_size),                      \
			.erase_value = 0xFF,                                                        \
		},                                                                                  \
		IFX_SMIF_NOR_PAGES_LAYOUT_FIELDS(n)                                                 \
	};                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ifx_smif_nor_init, NULL, NULL, &ifx_smif_nor_config_##n,           \
			      POST_KERNEL, CONFIG_FLASH_INFINEON_SMIF_NOR_INIT_PRIORITY,            \
			      &ifx_smif_nor_driver_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_SMIF_NOR_INST)
