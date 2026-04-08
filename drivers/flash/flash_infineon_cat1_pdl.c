/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_cat1_flash_controller

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/cache.h>
#include <zephyr/logging/log.h>

#include <cy_pdl.h>
#include <cy_flash.h>
#include <cy_syslib.h>

LOG_MODULE_REGISTER(flash_ifx_cat1_pdl, CONFIG_FLASH_LOG_LEVEL);

struct ifx_cat1_flash_controller_data {
	struct k_mutex lock;
};

struct ifx_cat1_flash_config {
	uint32_t base_addr;
	uint32_t region_size;
	uint32_t write_block_size;
	uint32_t erase_block_size;
};

struct ifx_cat1_flash_data {
	struct ifx_cat1_flash_controller_data *ctrl;
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

static int ifx_cat1_flash_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct ifx_cat1_flash_config *cfg = dev->config;
	uint32_t addr;

	if (len == 0) {
		return 0;
	}

	if (offset < 0) {
		return -EINVAL;
	}

	if (offset > cfg->region_size) {
		return -EINVAL;
	}

	if ((cfg->region_size - offset) < len) {
		return -EINVAL;
	}

	addr = cfg->base_addr + offset;

	ifx_cache_invalidate(cfg->base_addr + offset, (size_t)(addr - (cfg->base_addr + offset)));

	memcpy(data, (const void *)addr, len);

	return 0;
}

static int ifx_cat1_flash_write(const struct device *dev, off_t offset, const void *data,
				size_t len)
{
	const struct ifx_cat1_flash_config *cfg = dev->config;
	struct ifx_cat1_flash_data *dev_data = dev->data;
	const uint8_t *src = data;
	uint32_t addr;
	cy_en_flashdrv_status_t st;
	int ret = 0;

	if (len == 0) {
		return 0;
	}

	if (offset < 0) {
		return -EINVAL;
	}

	if (offset > cfg->region_size) {
		return -EINVAL;
	}

	if ((offset % cfg->write_block_size) != 0 || (len % cfg->write_block_size) != 0) {
		return -EINVAL;
	}

	if ((cfg->region_size - offset) < len) {
		return -EINVAL;
	}

	addr = cfg->base_addr + offset;

	k_mutex_lock(&dev_data->ctrl->lock, K_FOREVER);

	while (len > 0) {
		st = Cy_Flash_ProgramRow(addr, (const uint32_t *)src);
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

static int ifx_cat1_flash_erase(const struct device *dev, off_t offset, size_t size)
{
	const struct ifx_cat1_flash_config *cfg = dev->config;
	struct ifx_cat1_flash_data *dev_data = dev->data;
	uint32_t addr;
	cy_en_flashdrv_status_t st;
	int ret = 0;

	if (size == 0) {
		return 0;
	}

	if (offset < 0) {
		return -EINVAL;
	}

	if (offset > cfg->region_size) {
		return -EINVAL;
	}

	if ((offset % cfg->erase_block_size) != 0 || (size % cfg->erase_block_size) != 0) {
		return -EINVAL;
	}

	if ((cfg->region_size - offset) < size) {
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

static const struct flash_parameters *ifx_cat1_flash_get_parameters(const struct device *dev)
{
	struct ifx_cat1_flash_data *dev_data = dev->data;

	return &dev_data->params;
}

static int ifx_cat1_flash_get_size(const struct device *dev, uint64_t *size)
{
	const struct ifx_cat1_flash_config *cfg = dev->config;

	*size = cfg->region_size;
	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void ifx_cat1_flash_page_layout(const struct device *dev,
				       const struct flash_pages_layout **layout,
				       size_t *layout_size)
{
	struct ifx_cat1_flash_data *dev_data = dev->data;

	*layout = &dev_data->layout;
	*layout_size = 1;
}
#endif

static int ifx_cat1_flash_init(const struct device *dev)
{
	const struct device *ctrl = DEVICE_DT_INST_GET(0);
	const struct ifx_cat1_flash_config *cfg = dev->config;
	struct ifx_cat1_flash_data *dev_data = dev->data;

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

static DEVICE_API(flash, ifx_cat1_flash_api) = {
	.read = ifx_cat1_flash_read,
	.write = ifx_cat1_flash_write,
	.erase = ifx_cat1_flash_erase,
	.get_parameters = ifx_cat1_flash_get_parameters,
	.get_size = ifx_cat1_flash_get_size,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = ifx_cat1_flash_page_layout,
#endif
};

#define IFX_FLASH_REGION_DEFINE(node_id)						\
	static struct ifx_cat1_flash_data ifx_data_##node_id = {			\
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
	static const struct ifx_cat1_flash_config ifx_cfg_##node_id = {			\
		.base_addr        = DT_REG_ADDR(node_id),				\
		.region_size      = DT_REG_SIZE(node_id),				\
		.write_block_size = DT_PROP(node_id, write_block_size),			\
		.erase_block_size = DT_PROP(node_id, erase_block_size),			\
	};										\
											\
	DEVICE_DT_DEFINE(node_id, ifx_cat1_flash_init, NULL,				\
			 &ifx_data_##node_id, &ifx_cfg_##node_id,			\
			 POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,			\
			 &ifx_cat1_flash_api);

#define IFX_FLASH_REGION_IF_NVFLASH(node_id)						\
	IF_ENABLED(DT_NODE_HAS_COMPAT(node_id, soc_nv_flash),				\
		   (IFX_FLASH_REGION_DEFINE(node_id)))

DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(0), IFX_FLASH_REGION_IF_NVFLASH)

static struct ifx_cat1_flash_controller_data ifx_flash_controller_data;

static int ifx_cat1_flash_controller_init(const struct device *dev)
{
	struct ifx_cat1_flash_controller_data *ctrl = dev->data;

	k_mutex_init(&ctrl->lock);

	return 0;
}

DEVICE_DT_INST_DEFINE(0, ifx_cat1_flash_controller_init, NULL,
		      &ifx_flash_controller_data, NULL, PRE_KERNEL_1,
		      CONFIG_FLASH_INIT_PRIORITY, NULL);
