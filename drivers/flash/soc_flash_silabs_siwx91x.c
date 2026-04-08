/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT silabs_siwx91x_flash_controller

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include "siwx91x_nwp_api.h"
#include "device/silabs/si91x/wireless/inc/sl_si91x_protocol_types.h"

LOG_MODULE_REGISTER(siwx91x_soc_flash);

struct siwx91x_config {
	const struct device *dev_nwp;
	uintptr_t base_address;
	uint32_t size;
	uint32_t write_block_size;
	uint32_t erase_block_size;
	struct flash_parameters flash_parameters;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_layout flash_pages_layout;
#endif
};

static bool flash_siwx91x_range_is_in_bounds(const struct device *dev, off_t offset, size_t len)
{
	const struct siwx91x_config *cfg = dev->config;

	/* Note: If offset < __rom_region_end, the user to overwriting the current firmware. User
	 * is probably doing a mistake, but not an error from this driver point of view.
	 */
	if (offset < 0) {
		return false;
	}
	if (offset + len > cfg->size) {
		return false;
	}
	return true;
}

static const struct flash_parameters *flash_siwx91x_get_parameters(const struct device *dev)
{
	const struct siwx91x_config *cfg = dev->config;

	return &cfg->flash_parameters;
}

static int flash_siwx91x_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct siwx91x_config *cfg = dev->config;
	void *location = (void *)(cfg->base_address + offset);

	if (!flash_siwx91x_range_is_in_bounds(dev, offset, len)) {
		return -EINVAL;
	}
	memcpy(buf, location, len);
	return 0;
}

static int flash_siwx91x_write(const struct device *dev, off_t offset, const void *buf, size_t len)
{
	const struct siwx91x_config *cfg = dev->config;
	size_t chunk_len;
	uint32_t ret;

	if (!flash_siwx91x_range_is_in_bounds(dev, offset, len)) {
		return -EINVAL;
	}
	if (offset % cfg->write_block_size) {
		return -EINVAL;
	}
	if (len % cfg->write_block_size) {
		return -EINVAL;
	}
	while (len) {
		chunk_len = MIN(len, MAX_CHUNK_SIZE);
		ret = siwx91x_nwp_flash_write(cfg->dev_nwp,
					      cfg->base_address + offset,
					      (const uint8_t *)buf,
					      chunk_len);
		if (ret) {
			return -EIO;
		}
		len -= chunk_len;
		offset += chunk_len;
		buf = (const uint8_t *)buf + chunk_len;
	};
	return 0;
}

static int flash_siwx91x_erase(const struct device *dev, off_t offset, size_t len)
{
	const struct siwx91x_config *cfg = dev->config;
	size_t chunk_len;
	uint32_t ret;

	if (!flash_siwx91x_range_is_in_bounds(dev, offset, len)) {
		return -EINVAL;
	}
	if (offset % cfg->erase_block_size) {
		return -EINVAL;
	}
	if (len % cfg->erase_block_size) {
		return -EINVAL;
	}
	while (len) {
		chunk_len = MIN(len, FLASH_SECTOR_SIZE);
		ret = siwx91x_nwp_flash_erase(cfg->dev_nwp,
					      cfg->base_address + offset,
					      chunk_len);
		if (ret) {
			return -EIO;
		}
		len -= chunk_len;
		offset += chunk_len;
	};
	return 0;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static void flash_siwx91x_page_layout(const struct device *dev,
				      const struct flash_pages_layout **layout, size_t *layout_size)
{
	const struct siwx91x_config *cfg = dev->config;

	*layout = &cfg->flash_pages_layout;
	*layout_size = 1;
}
#endif

static DEVICE_API(flash, siwx91x_api) = {
	.read = flash_siwx91x_read,
	.write = flash_siwx91x_write,
	.erase = flash_siwx91x_erase,
	.get_parameters = flash_siwx91x_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_siwx91x_page_layout,
#endif
};

#ifdef CONFIG_FLASH_PAGE_LAYOUT
#define SIWX91X_PAGE_LAYOUT_FLASH_INIT(n)                                                          \
	.flash_pages_layout.pages_count = DT_REG_SIZE(n) / DT_PROP(n, erase_block_size),           \
	.flash_pages_layout.pages_size = DT_PROP(n, erase_block_size),
#else
#define SIWX91X_PAGE_LAYOUT_FLASH_INIT(n)
#endif

#define SIWX91X_FLASH_INIT_P(n, p)                                                                 \
	static const struct siwx91x_config flash_siwx91x_config_##p = {                            \
		.dev_nwp = DEVICE_DT_GET(DT_NODELABEL(nwp)),                                       \
		.base_address = DT_REG_ADDR(n),                                                    \
		.size = DT_REG_SIZE(n),                                                            \
		.write_block_size = DT_PROP(n, write_block_size),                                  \
		.erase_block_size = DT_PROP(n, erase_block_size),                                  \
		.flash_parameters.write_block_size = DT_PROP(n, write_block_size),                 \
		.flash_parameters.erase_value = 0xff,                                              \
		SIWX91X_PAGE_LAYOUT_FLASH_INIT(n)                                                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(p, NULL, NULL, NULL, &flash_siwx91x_config_##p,                      \
			      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, &siwx91x_api);

#define SIWX91X_FLASH_INIT(p)                                                                      \
	BUILD_ASSERT(DT_INST_CHILD_NUM_STATUS_OKAY(p) == 1);                                       \
	DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(p, SIWX91X_FLASH_INIT_P, p)

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_FLASH_INIT)
