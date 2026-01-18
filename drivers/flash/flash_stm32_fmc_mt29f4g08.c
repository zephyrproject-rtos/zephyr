/*
 * Copyright (c) 2025 Endress+Hauser GmbH+Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT micron_mt29f4g08

#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/flash/nand_flash.h>
#include <zephyr/sys/math_extras.h>

#include "flash_stm32_fmc_nand.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_stm32_fmc_mt29f4g08, CONFIG_FLASH_LOG_LEVEL);

#define ECC_FEATURE_ADDR 0x90
#define ECC_FEATURE_DATA {0x08, 0x00, 0x00, 0x00}

struct flash_mt29f4g08_config {
	const struct device *controller;
	struct flash_parameters parameters;
	uint8_t bank;
	size_t page_size;
	size_t spare_area_size;
	size_t block_size;
	size_t plane_size;
	size_t flash_size;
	uint8_t setup_time;
	uint8_t wait_setup_time;
	uint8_t hold_setup_time;
	uint8_t hiz_setup_time;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_layout layout;
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
};

static struct nand_flash_address
flash_mt29f4g08_calculate_address(const struct flash_mt29f4g08_config *config, off_t offset)
{
	off_t page_index = offset / config->page_size;
	off_t block_index = offset / config->block_size;
	off_t plane_index = offset / config->plane_size;

	return (struct nand_flash_address){
		.page = page_index % (config->block_size / config->page_size),
		.block = block_index % (config->plane_size / config->block_size),
		.plane = plane_index % (config->flash_size / config->plane_size),
	};
}

static int flash_mt29f4g08_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct flash_mt29f4g08_config *config = dev->config;
	const struct device *controller = config->controller;
	int ret;

	if ((offset < 0) || (offset >= config->flash_size) ||
	    (len > (config->flash_size - offset))) {
		return -EINVAL;
	}

	while (len > 0) {
		off_t page_offset = offset % config->page_size;
		size_t chunk = ((page_offset + len) < config->page_size)
				       ? len
				       : (config->page_size - page_offset);
		struct nand_flash_address address =
			flash_mt29f4g08_calculate_address(config, offset);

		ret = flash_stm32_fmc_nand_read_page_chunk(controller, &address, page_offset, chunk,
							   (uint8_t *)data);
		if (ret != 0) {
			LOG_ERR("Reading page %d at block %d/plane %d failed with error %d",
				address.page, address.block, address.plane, ret);
			return ret;
		}
		data = (uint8_t *)data + chunk;
		offset += chunk;
		len -= chunk;
	}

	return 0;
}

static int flash_mt29f4g08_write(const struct device *dev, off_t offset, const void *data,
				 size_t len)
{
	const struct flash_mt29f4g08_config *config = dev->config;
	const struct device *controller = config->controller;
	int ret;

	if ((offset < 0) || (offset >= config->flash_size) ||
	    (len > (config->flash_size - offset))) {
		return -EINVAL;
	}

	if (((offset % config->page_size) != 0) || ((len % config->page_size) != 0)) {
		return -EINVAL;
	}

	while (len > 0) {
		struct nand_flash_address address =
			flash_mt29f4g08_calculate_address(config, offset);

		ret = flash_stm32_fmc_nand_write_page(controller, &address, (const uint8_t *)data);
		if (ret != 0) {
			LOG_ERR("Writing page %d at block %d/plane %d failed with error %d",
				address.page, address.block, address.plane, ret);
			return ret;
		}
		data = (const uint8_t *)data + config->page_size;
		offset += config->page_size;
		len -= config->page_size;
	}

	return 0;
}

static int flash_mt29f4g08_erase(const struct device *dev, off_t offset, size_t size)
{
	const struct flash_mt29f4g08_config *config = dev->config;
	const struct device *controller = config->controller;
	int ret;

	if ((offset < 0) || (offset >= config->flash_size) ||
	    (size > (config->flash_size - offset))) {
		return -EINVAL;
	}

	if (((offset % config->block_size) != 0) || ((size % config->block_size) != 0)) {
		return -EINVAL;
	}

	while (size > 0) {
		struct nand_flash_address address =
			flash_mt29f4g08_calculate_address(config, offset);

		ret = flash_stm32_fmc_nand_erase_block(controller, &address);
		if (ret != 0) {
			LOG_ERR("Erasing block %d at plane %d failed with error %d", address.block,
				address.plane, ret);
			return ret;
		}
		offset += config->block_size;
		size -= config->block_size;
	}

	return 0;
}

static const struct flash_parameters *flash_mt29f4g08_get_parameters(const struct device *dev)
{
	const struct flash_mt29f4g08_config *config = dev->config;

	return &config->parameters;
}

static int flash_mt29f4g08_get_size(const struct device *dev, uint64_t *size)
{
	const struct flash_mt29f4g08_config *config = dev->config;

	*size = config->flash_size;

	return 0;
}

#if CONFIG_FLASH_PAGE_LAYOUT
static void flash_mt29f4g08_page_layout(const struct device *dev,
					const struct flash_pages_layout **layout,
					size_t *layout_size)
{
	const struct flash_mt29f4g08_config *config = dev->config;

	*layout = &config->layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

#if CONFIG_FLASH_EX_OP_ENABLED
static int flash_mt29f4g08_is_bad_block(const struct device *dev, const off_t *offset,
					enum flash_block_status *status)
{
	const struct flash_mt29f4g08_config *config = dev->config;
	struct nand_flash_address address;
	uint8_t spare_area[config->spare_area_size];
	int ret;

	if (status == NULL) {
		return -EINVAL;
	}

	if ((offset == NULL) || (*offset < 0) || (*offset >= config->flash_size) ||
	    ((*offset % config->block_size) != 0)) {
		*status = FLASH_BLOCK_BAD;
		return -EINVAL;
	}

	/* Check bad block marker in first page */
	memset(spare_area, 0x00, config->spare_area_size);
	address = flash_mt29f4g08_calculate_address(config, *offset);
	ret = flash_stm32_fmc_nand_read_spare_area(config->controller, &address, spare_area);
	if (ret == 0) {
		*status = (spare_area[0] != 0xFF) ? FLASH_BLOCK_BAD : FLASH_BLOCK_GOOD;
	} else {
		*status = FLASH_BLOCK_BAD;
	}

	return ret;
}

static int flash_mt29f4g08_mark_bad_block(const struct device *dev, const off_t *offset)
{
	const struct flash_mt29f4g08_config *config = dev->config;
	struct nand_flash_address address;
	uint8_t spare_area[config->spare_area_size];
	int ret;

	if ((offset == NULL) || (*offset < 0) || (*offset >= config->flash_size) ||
	    ((*offset % config->block_size) != 0)) {
		return -EINVAL;
	}

	/* Mark bad block in first page */
	memset(spare_area, 0x00, config->spare_area_size);
	address = flash_mt29f4g08_calculate_address(config, *offset);
	ret = flash_stm32_fmc_nand_read_spare_area(config->controller, &address, spare_area);
	if (ret == 0) {
		spare_area[0] = 0x00;
		ret = flash_stm32_fmc_nand_write_spare_area(config->controller, &address,
							    spare_area);
	}

	return ret;
}

int flash_mt29f4g08_ex_op(const struct device *dev, uint16_t code, const uintptr_t in, void *out)
{
	switch (code) {
	case FLASH_IS_BAD_BLOCK:
		return flash_mt29f4g08_is_bad_block(dev, (const off_t *)in,
						    (enum flash_block_status *)out);

	case FLASH_MARK_BAD_BLOCK:
		return flash_mt29f4g08_mark_bad_block(dev, (const off_t *)in);

	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_FLASH_EX_OP_ENABLED */

static int flash_stm32_fmc_mt29f4g08_init(const struct device *dev)
{
	const struct flash_mt29f4g08_config *config = dev->config;
	const struct device *controller = config->controller;
	int ret;

	if (!device_is_ready(controller)) {
		LOG_ERR("Parent flash controller %s is not ready", controller->name);
		return -ENODEV;
	}

	struct flash_stm32_fmc_nand_init init = {
		.bank = config->bank,
		.page_size = config->page_size,
		.spare_area_size = config->spare_area_size,
		.block_size = config->block_size,
		.plane_size = config->plane_size,
		.flash_size = config->flash_size,
		.setup_time = config->setup_time,
		.wait_setup_time = config->wait_setup_time,
		.hold_setup_time = config->hold_setup_time,
		.hiz_setup_time = config->hiz_setup_time,
	};

	/* Initialise NAND bank */
	ret = flash_stm32_fmc_nand_init_bank(controller, &init);
	if (ret != 0) {
		LOG_ERR("NAND bank initialisation failed with error %d", ret);
		return -EIO;
	}

	/* Reset NAND flash */
	ret = flash_stm32_fmc_nand_reset(controller);
	if (ret != 0) {
		LOG_ERR("NAND flash reset failed with error %d", ret);
		return -EIO;
	}

#ifdef CONFIG_FLASH_MT29F4G08_ECC
	/* Enable on-die ECC feature */
	struct nand_flash_feature ecc_feature = {
		.feature_addr = ECC_FEATURE_ADDR,
		.feature_data = ECC_FEATURE_DATA,
	};

	ret = flash_stm32_fmc_nand_set_feature(controller, &ecc_feature);
	if (ret != 0) {
		LOG_ERR("Enabling on-die ECC failed with error %d", ret);
		return -EIO;
	}
#endif /* CONFIG_FLASH_MT29F4G08_ECC */

	LOG_INF("MT29F4G08 flash initialised with FMC controller %s", controller->name);

	return 0;
}

static DEVICE_API(flash, flash_stm32_fmc_mt29f4g08_api) = {
	.read = flash_mt29f4g08_read,
	.write = flash_mt29f4g08_write,
	.erase = flash_mt29f4g08_erase,
	.get_parameters = flash_mt29f4g08_get_parameters,
	.get_size = flash_mt29f4g08_get_size,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_mt29f4g08_page_layout,
#endif
#ifdef CONFIG_FLASH_EX_OP_ENABLED
	.ex_op = flash_mt29f4g08_ex_op,
#endif /* CONFIG_FLASH_EX_OP_ENABLED */
};

/* A page in this context corresponds to the smallest erasable area which is a block */
#define LAYOUT_PAGES_PROP(n)                                                                       \
	IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT,                                                       \
		(.layout = {                                                                       \
			.pages_count = DT_INST_PROP(n, flash_size) / DT_INST_PROP(n, block_size),  \
			.pages_size = DT_INST_PROP(n, block_size),                                 \
		},))

#define FLASH_STM32_FMC_MT29F4G08_INIT(n)                                                          \
	static const struct flash_mt29f4g08_config flash_stm32_fmc_mt29f4g08_config_##n = {        \
		.controller = DEVICE_DT_GET(DT_INST_PARENT(n)),                                    \
		.parameters = {                                                                    \
			.write_block_size = DT_INST_PROP(n, page_size),                            \
			.erase_value = 0xff,                                                       \
		},                                                                                 \
		.bank = DT_INST_PROP(n, reg),                                                      \
		.page_size = DT_INST_PROP(n, page_size),                                           \
		.spare_area_size = DT_INST_PROP(n, spare_area_size),                               \
		.block_size = DT_INST_PROP(n, block_size),                                         \
		.plane_size = DT_INST_PROP(n, plane_size),                                         \
		.flash_size = DT_INST_PROP(n, flash_size),                                         \
		.setup_time = DT_INST_PROP(n, setup_time),                                         \
		.wait_setup_time = DT_INST_PROP(n, wait_setup_time),                               \
		.hold_setup_time = DT_INST_PROP(n, hold_setup_time),                               \
		.hiz_setup_time = DT_INST_PROP(n, hiz_setup_time),                                 \
		LAYOUT_PAGES_PROP(n)};                                                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, flash_stm32_fmc_mt29f4g08_init, NULL, NULL,                       \
			      &flash_stm32_fmc_mt29f4g08_config_##n, POST_KERNEL,                  \
			      CONFIG_FLASH_INIT_PRIORITY, &flash_stm32_fmc_mt29f4g08_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_STM32_FMC_MT29F4G08_INIT)
