/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_ect_flash_controller

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/cache.h>
#include <zephyr/logging/log.h>

#include <cy_pdl.h>
#include <cy_flash.h>
#include <cy_syslib.h>

LOG_MODULE_REGISTER(flash_infineon_ect, CONFIG_FLASH_LOG_LEVEL);

struct ifx_ect_flash_controller_data {
	struct k_mutex lock;
};

struct ifx_ect_flash_config {
	uint32_t base_addr;
	uint32_t region_size;
	uint32_t write_block_size;
	uint32_t erase_block_size;
};

struct ifx_ect_flash_data {
	struct ifx_ect_flash_controller_data *ctrl;
	struct flash_parameters params;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
};

static inline bool is_main_flash_region(uint32_t base)
{
	return ((base >= CY_FLASH_LG_SBM_TOP) && (base < CY_FLASH_LG_SBM_END)) ||
	       ((base >= CY_FLASH_SM_SBM_TOP) && (base < CY_FLASH_SM_SBM_END));
}

static inline bool is_work_flash_region(uint32_t base)
{
	return ((base >= CY_WFLASH_LG_SBM_TOP) && (base < CY_WFLASH_LG_SBM_END)) ||
	       ((base >= CY_WFLASH_SM_SBM_TOP) && (base < CY_WFLASH_SM_SBM_END));
}

static void ifx_cache_invalidate(uint32_t addr, size_t len)
{
	FLASHC_FLASH_CMD = _VAL2FLD(FLASHC_FLASH_CMD_INV, 1U);

#if defined(CONFIG_DCACHE)
	sys_cache_data_invd_range((void *)addr, len);
#else
	ARG_UNUSED(addr);
	ARG_UNUSED(len);
#endif
}

static cy_en_flashdrv_status_t ifx_ect_flash_program(const struct ifx_ect_flash_config *cfg,
						     uint32_t addr, const uint8_t *src)
{
	/* SROM data size code is log2 of the program size in bytes */
	cy_stc_flash_programrow_config_t pcfg = {
		.destAddr = (const uint32_t *)addr,
		.dataAddr = (const uint32_t *)src,
		.blocking = CY_FLASH_PROGRAMROW_BLOCKING,
		.skipBC = CY_FLASH_PROGRAMROW_SKIP_BLANK_CHECK,
		.dataSize = (cy_en_flash_programrow_datasize_t)LOG2(cfg->write_block_size),
		.dataLoc = CY_FLASH_PROGRAMROW_DATA_LOCATION_SRAM,
		.intrMask = CY_FLASH_PROGRAMROW_NOT_SET_INTR_MASK,
	};

	/* Cy_Flash_ProgramRow() always programs a 512 byte row */
	if (!is_work_flash_region(cfg->base_addr)) {
		return Cy_Flash_ProgramRow(addr, (const uint32_t *)src);
	}

	return Cy_Flash_Program_WorkFlash(&pcfg);
}

/* Erased work flash reads back undefined data, so blank check each write block first */
static int ifx_ect_work_flash_read(const struct device *dev, uint32_t addr, uint8_t *dst,
				   size_t len)
{
	const struct ifx_ect_flash_config *cfg = dev->config;
	struct ifx_ect_flash_data *dev_data = dev->data;
	cy_stc_flash_blankcheck_config_t bcfg = {
		.numOfWordsToBeChecked = cfg->write_block_size / sizeof(uint32_t),
	};
	cy_en_flashdrv_status_t st;
	uint32_t block;
	size_t chunk;

	while (len > 0) {
		block = ROUND_DOWN(addr, cfg->write_block_size);
		chunk = MIN(len, block + cfg->write_block_size - addr);
		bcfg.addrToBeChecked = (const uint32_t *)block;

		st = Cy_Flash_BlankCheck(&bcfg, CY_FLASH_DRIVER_BLOCKING);
		if (st == CY_FLASH_DRV_SUCCESS) {
			memset(dst, dev_data->params.erase_value, chunk);
		} else if (st == CY_FLASH_DRV_FLASH_NOT_ERASED) {
			memcpy(dst, (const void *)addr, chunk);
		} else {
			LOG_ERR("Blank check failed at 0x%08x (err 0x%x)", addr, st);
			return -EIO;
		}

		addr += chunk;
		dst += chunk;
		len -= chunk;
	}

	return 0;
}

static int ifx_ect_flash_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct ifx_ect_flash_config *cfg = dev->config;
	struct ifx_ect_flash_data *dev_data = dev->data;
	uint32_t addr;
	int ret;

	if (len == 0) {
		return 0;
	}

	if (offset < 0) {
		LOG_DBG("Negative offset %ld", (long)offset);
		return -EINVAL;
	}

	if (offset > cfg->region_size) {
		LOG_DBG("Offset 0x%lx past region size 0x%x", (long)offset, cfg->region_size);
		return -EINVAL;
	}

	if ((cfg->region_size - offset) < len) {
		LOG_DBG("Len %zu at 0x%lx past region size 0x%x", len, (long)offset,
			cfg->region_size);
		return -EINVAL;
	}

	addr = cfg->base_addr + offset;

	ifx_cache_invalidate(addr, len);

	if (!is_work_flash_region(cfg->base_addr)) {
		memcpy(data, (const void *)addr, len);
		return 0;
	}

	k_mutex_lock(&dev_data->ctrl->lock, K_FOREVER);
	ret = ifx_ect_work_flash_read(dev, addr, data, len);
	k_mutex_unlock(&dev_data->ctrl->lock);

	return ret;
}

static int ifx_ect_flash_write(const struct device *dev, off_t offset, const void *data,
				size_t len)
{
	const struct ifx_ect_flash_config *cfg = dev->config;
	struct ifx_ect_flash_data *dev_data = dev->data;
	const uint8_t *src = data;
	uint32_t addr;
	cy_en_flashdrv_status_t st;
	int ret = 0;

	if (len == 0) {
		return 0;
	}

	if (offset < 0) {
		LOG_DBG("Negative offset %ld", (long)offset);
		return -EINVAL;
	}

	if (offset > cfg->region_size) {
		LOG_DBG("Offset 0x%lx past region size 0x%x", (long)offset, cfg->region_size);
		return -EINVAL;
	}

	if ((offset % cfg->write_block_size) != 0 || (len % cfg->write_block_size) != 0) {
		LOG_DBG("Offset 0x%lx or len %zu not aligned to %u", (long)offset, len,
			cfg->write_block_size);
		return -EINVAL;
	}

	if ((cfg->region_size - offset) < len) {
		LOG_DBG("Len %zu at 0x%lx past region size 0x%x", len, (long)offset,
			cfg->region_size);
		return -EINVAL;
	}

	addr = cfg->base_addr + offset;

	k_mutex_lock(&dev_data->ctrl->lock, K_FOREVER);

	while (len > 0) {
		st = ifx_ect_flash_program(cfg, addr, src);
		if (st != CY_FLASH_DRV_SUCCESS) {
			LOG_ERR("Write failed at 0x%08x (err 0x%x)", addr, st);
			ret = -EIO;
			goto out;
		}

		addr += cfg->write_block_size;
		src += cfg->write_block_size;
		len -= cfg->write_block_size;
	}

out:
	k_mutex_unlock(&dev_data->ctrl->lock);
	return ret;
}

static int ifx_ect_flash_erase(const struct device *dev, off_t offset, size_t size)
{
	const struct ifx_ect_flash_config *cfg = dev->config;
	struct ifx_ect_flash_data *dev_data = dev->data;
	uint32_t addr;
	cy_en_flashdrv_status_t st;
	int ret = 0;

	if (size == 0) {
		return 0;
	}

	if (offset < 0) {
		LOG_DBG("Negative offset %ld", (long)offset);
		return -EINVAL;
	}

	if (offset > cfg->region_size) {
		LOG_DBG("Offset 0x%lx past region size 0x%x", (long)offset, cfg->region_size);
		return -EINVAL;
	}

	if ((offset % cfg->erase_block_size) != 0 || (size % cfg->erase_block_size) != 0) {
		LOG_DBG("Offset 0x%lx or size %zu not aligned to %u", (long)offset, size,
			cfg->erase_block_size);
		return -EINVAL;
	}

	if ((cfg->region_size - offset) < size) {
		LOG_DBG("Size %zu at 0x%lx past region size 0x%x", size, (long)offset,
			cfg->region_size);
		return -EINVAL;
	}

	addr = cfg->base_addr + offset;

	k_mutex_lock(&dev_data->ctrl->lock, K_FOREVER);

	while (size > 0) {
		st = Cy_Flash_EraseSector(addr);
		if (st != CY_FLASH_DRV_SUCCESS) {
			LOG_ERR("Erase failed at 0x%08x (err 0x%x)", addr, st);
			ret = -EIO;
			goto out;
		}

		addr += cfg->erase_block_size;
		size -= cfg->erase_block_size;
	}

out:
	k_mutex_unlock(&dev_data->ctrl->lock);
	return ret;
}

static const struct flash_parameters *ifx_ect_flash_get_parameters(const struct device *dev)
{
	struct ifx_ect_flash_data *dev_data = dev->data;

	return &dev_data->params;
}

static int ifx_ect_flash_get_size(const struct device *dev, uint64_t *size)
{
	const struct ifx_ect_flash_config *cfg = dev->config;

	*size = cfg->region_size;
	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void ifx_ect_flash_page_layout(const struct device *dev,
				      const struct flash_pages_layout **layout,
				      size_t *layout_size)
{
	struct ifx_ect_flash_data *dev_data = dev->data;

	*layout = &dev_data->layout;
	*layout_size = 1;
}
#endif

static int ifx_ect_flash_init(const struct device *dev)
{
	const struct device *ctrl = DEVICE_DT_INST_GET(0);
	const struct ifx_ect_flash_config *cfg = dev->config;
	struct ifx_ect_flash_data *dev_data = dev->data;

	if (!device_is_ready(ctrl)) {
		return -ENODEV;
	}

	dev_data->ctrl = ctrl->data;

	/* Enable programming for this region's bank. */
	if (is_main_flash_region(cfg->base_addr)) {
		Cy_Flashc_MainWriteEnable();
	} else if (is_work_flash_region(cfg->base_addr)) {
		Cy_Flashc_WorkWriteEnable();
	} else {
		return -EINVAL;
	}

	return 0;
}

static DEVICE_API(flash, ifx_ect_flash_api) = {
	.read = ifx_ect_flash_read,
	.write = ifx_ect_flash_write,
	.erase = ifx_ect_flash_erase,
	.get_parameters = ifx_ect_flash_get_parameters,
	.get_size = ifx_ect_flash_get_size,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = ifx_ect_flash_page_layout,
#endif
};

#define IFX_FLASH_REGION_DEFINE(node_id)						\
	static struct ifx_ect_flash_data ifx_data_##node_id = {				\
		.params = {								\
			.write_block_size =						\
				DT_PROP(node_id, write_block_size),			\
			.erase_value = 0xFF,						\
		},									\
		IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT,					\
			(.layout = {							\
				.pages_count = DT_REG_SIZE(node_id) /			\
					DT_PROP(node_id, erase_block_size),		\
				.pages_size  =						\
					DT_PROP(node_id, erase_block_size),		\
			},))								\
	};										\
											\
	static const struct ifx_ect_flash_config ifx_cfg_##node_id = {			\
		.base_addr        = DT_REG_ADDR(node_id),				\
		.region_size      = DT_REG_SIZE(node_id),				\
		.write_block_size = DT_PROP(node_id, write_block_size),			\
		.erase_block_size = DT_PROP(node_id, erase_block_size),			\
	};										\
											\
	DEVICE_DT_DEFINE(node_id, ifx_ect_flash_init, NULL,				\
			 &ifx_data_##node_id, &ifx_cfg_##node_id,			\
			 POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,			\
			 &ifx_ect_flash_api);

#define IFX_FLASH_REGION_IF_NVFLASH(node_id)						\
	IF_ENABLED(DT_NODE_HAS_COMPAT(node_id, soc_nv_flash),				\
		   (IFX_FLASH_REGION_DEFINE(node_id)))

DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(0), IFX_FLASH_REGION_IF_NVFLASH)

static struct ifx_ect_flash_controller_data ifx_flash_controller_data;

static int ifx_ect_flash_controller_init(const struct device *dev)
{
	struct ifx_ect_flash_controller_data *ctrl = dev->data;

	k_mutex_init(&ctrl->lock);

	return 0;
}

DEVICE_DT_INST_DEFINE(0, ifx_ect_flash_controller_init, NULL,
		      &ifx_flash_controller_data, NULL, PRE_KERNEL_1,
		      CONFIG_FLASH_INIT_PRIORITY, NULL);
