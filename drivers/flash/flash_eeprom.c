/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/flash.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_eeprom, CONFIG_FLASH_LOG_LEVEL);

#define DT_DRV_COMPAT zephyr_eeprom_to_flash

#define FLASH_EEPROM_ERASE_VALUE 0xff

struct flash_eeprom_config {
	const struct device *dev;
	struct flash_pages_layout layout;
};

struct flash_parameters flash_eeprom_params = {
	.write_block_size = 1,
	.erase_value = FLASH_EEPROM_ERASE_VALUE,
};

static int flash_eeprom_write(const struct device *dev,
			     off_t address,
			     const void *buffer,
			     size_t length)
{
	const struct flash_eeprom_config *cfg = dev->config;

	return eeprom_write(cfg->dev, address, buffer, length);
}

static int flash_eeprom_read(const struct device *dev, off_t address, void *buffer, size_t length)
{
	const struct flash_eeprom_config *cfg = dev->config;

	return eeprom_read(cfg->dev, address, buffer, length);
}

/* Provided for compatibility, slow */
static int flash_eeprom_erase(const struct device *dev, off_t start, size_t len)
{
	const struct flash_eeprom_config *cfg = dev->config;
	int ret = 0;
	uint8_t buf = FLASH_EEPROM_ERASE_VALUE;

	for (size_t i = 0; i < len; i++) {
		ret = eeprom_write(cfg->dev, start + i, &buf, 1);
		if (ret < 0) {
			return ret;
		}
	}

	return ret;
}

static const struct flash_parameters *flash_eeprom_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);
	return &flash_eeprom_params;
}

#if CONFIG_FLASH_PAGE_LAYOUT
void flash_eeprom_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	const struct flash_eeprom_config *cfg = dev->config;

	*layout = &cfg->layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static DEVICE_API(flash, flash_eeprom_driver_api) = {
	.read = flash_eeprom_read,
	.write = flash_eeprom_write,
	.erase = flash_eeprom_erase,
	.get_parameters = flash_eeprom_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_eeprom_page_layout,
#endif
};

#define FLASH_EEPROM_INIT(inst)                                                                    \
	static const struct flash_eeprom_config flash_eeprom_config_##inst = {                     \
		.dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, eeprom)),                               \
		.layout = {                                                                        \
			.pages_count = DT_PROP(DT_INST_PHANDLE(inst, eeprom), size),               \
			.pages_size = 1,                                                           \
		}                                                                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL,                                              \
			      &flash_eeprom_config_##inst, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,\
			      &flash_eeprom_driver_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_EEPROM_INIT)
