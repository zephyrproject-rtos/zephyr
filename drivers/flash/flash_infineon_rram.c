/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_rram_controller

#define RRAM_COMPAT_NODE(node_id) \
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, soc_nv_flash), (node_id), ())

#define SOC_RRAM_NODE \
	DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(0), RRAM_COMPAT_NODE)

#include <string.h>

#include <infineon_kconfig.h>
#include <cy_rram.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>

#define IFX_RRAM_START      DT_REG_ADDR(SOC_RRAM_NODE)
#define IFX_RRAM_SIZE       DT_REG_SIZE(SOC_RRAM_NODE)
#define IFX_RRAM_STOP       (IFX_RRAM_START + IFX_RRAM_SIZE)
#define IFX_RRAM_WRITE_BLOCK_SIZE DT_PROP(SOC_RRAM_NODE, write_block_size)
#define IFX_ERASE_VALUE     0xFF

BUILD_ASSERT(IFX_RRAM_SIZE > 0, "RRAM size must be greater than 0");
BUILD_ASSERT(IFX_RRAM_STOP > IFX_RRAM_START, "RRAM stop addr must be greater than start addr");

static uint8_t erase_buffer[IFX_RRAM_WRITE_BLOCK_SIZE];

LOG_MODULE_REGISTER(rram_infineon, CONFIG_FLASH_LOG_LEVEL);

struct ifx_rram_config {
	RRAMC_Type *base;
};

struct ifx_rram_data {
	struct k_mutex lock;
};

static int ifx_rram_validate_range(off_t offset, size_t len)
{
	if (offset < 0 || offset > IFX_RRAM_SIZE || len > (IFX_RRAM_SIZE - offset)) {
		LOG_ERR("Offset %ld is out of bounds", (long)offset);
		return -EINVAL;
	}

	return 0;
}

static int ifx_rram_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	const struct ifx_rram_config *cfg = dev->config;
	struct ifx_rram_data *dev_data = dev->data;
	uint32_t write_offset;
	int ret;

	if (len == 0) {
		LOG_DBG("Write: Zero-length write, nothing to do");
		return 0;
	}

	ret = ifx_rram_validate_range(offset, len);
	if (ret) {
		return ret;
	}

	write_offset = IFX_RRAM_START + offset;

	k_mutex_lock(&dev_data->lock, K_FOREVER);
	cy_en_rram_status_t status =
		Cy_RRAM_NvmWriteByteArray(cfg->base, write_offset, (const uint8_t *)data, len);
	k_mutex_unlock(&dev_data->lock);

	return (status == CY_RRAM_SUCCESS) ? 0 : -EIO;
}

static int ifx_rram_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct ifx_rram_config *cfg = dev->config;
	struct ifx_rram_data *dev_data = dev->data;
	uint32_t read_offset;
	int ret;

	if (len == 0) {
		LOG_DBG("Read: Zero-length read, nothing to do");
		return 0;
	}

	ret = ifx_rram_validate_range(offset, len);
	if (ret) {
		return ret;
	}

	read_offset = IFX_RRAM_START + offset;

	k_mutex_lock(&dev_data->lock, K_FOREVER);
	cy_en_rram_status_t status =
		Cy_RRAM_ReadByteArray(cfg->base, read_offset, (uint8_t *)data, len);
	k_mutex_unlock(&dev_data->lock);

	return (status == CY_RRAM_SUCCESS) ? 0 : -EIO;
}

static int ifx_rram_erase(const struct device *dev, off_t offset, size_t size)
{
	const struct ifx_rram_config *cfg = dev->config;
	struct ifx_rram_data *dev_data = dev->data;
	cy_en_rram_status_t status = CY_RRAM_SUCCESS;
	size_t remaining = size;
	uint32_t erase_offset;
	int ret;

	if (size == 0) {
		LOG_DBG("Erase: Zero-length erase, nothing to do");
		return 0;
	}

	ret = ifx_rram_validate_range(offset, size);
	if (ret) {
		return ret;
	}

	erase_offset = IFX_RRAM_START + offset;

	k_mutex_lock(&dev_data->lock, K_FOREVER);
	while (remaining > 0 && status == CY_RRAM_SUCCESS) {
		size_t chunk = MIN(remaining, sizeof(erase_buffer));

		status = Cy_RRAM_NvmWriteByteArray(cfg->base, erase_offset, erase_buffer,
						   chunk);
		erase_offset += chunk;
		remaining -= chunk;
	}
	k_mutex_unlock(&dev_data->lock);

	return (status == CY_RRAM_SUCCESS) ? 0 : -EIO;
}

static const struct flash_parameters rram_parameters = {
	.write_block_size = DT_PROP(SOC_RRAM_NODE, write_block_size),
	.erase_value = IFX_ERASE_VALUE,
	.caps = {
		.no_explicit_erase = true,
	},
};
static const struct flash_parameters *ifx_rram_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);
	return &rram_parameters;
}

static int ifx_rram_get_size(const struct device *dev, uint64_t *size)
{
	ARG_UNUSED(dev);
	*size = IFX_RRAM_SIZE;
	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static const struct flash_pages_layout rram_pages_layout = {
	.pages_count = DT_REG_SIZE(SOC_RRAM_NODE) / DT_PROP(SOC_RRAM_NODE, write_block_size),
	.pages_size = DT_PROP(SOC_RRAM_NODE, write_block_size),
};

static void ifx_rram_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
				 size_t *layout_size)
{
	ARG_UNUSED(dev);
	*layout = &rram_pages_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static int ifx_rram_init(const struct device *dev)
{
	struct ifx_rram_data *dev_data = dev->data;

	k_mutex_init(&dev_data->lock);
	memset(erase_buffer, IFX_ERASE_VALUE, sizeof(erase_buffer));

	return 0;
}

DEVICE_API(flash, rram_infineon_api) = {
	.read = ifx_rram_read,
	.write = ifx_rram_write,
	.erase = ifx_rram_erase,
	.get_parameters = ifx_rram_get_parameters,
	.get_size = ifx_rram_get_size,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = ifx_rram_page_layout,
#endif
};

static struct ifx_rram_data ifx_rram_data_instance;

static const struct ifx_rram_config ifx_rram_config_parameters = {
	.base = (RRAMC_Type *)DT_REG_ADDR(DT_DRV_INST(0)),
};

DEVICE_DT_INST_DEFINE(0, ifx_rram_init, NULL, &ifx_rram_data_instance, &ifx_rram_config_parameters,
		      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, &rram_infineon_api);
