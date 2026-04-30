/*
 * Copyright (c) 2026 Alex Fabre
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Flash adapter for the STM32 system bootloader driver.
 *
 * Each @c st,stm32-bootloader-flash DT node (a child of an
 * @c st,stm32-bootloader node) is instantiated as a Zephyr flash device.
 * The adapter translates flash_driver_api offsets into absolute target
 * addresses and delegates every call to the parent bootloader device:
 *
 *   flash_read  (offset, ...) -> stm32_bootloader_read_memory (base+off, ...)
 *   flash_write (offset, ...) -> stm32_bootloader_write_memory(base+off, ...)
 *   flash_erase (offset, size) -> stm32_bootloader_erase_pages(...)
 *
 * The adapter holds no session state: callers must call
 * stm32_bootloader_enter() on the parent before any flash op, and exit
 * when done. If the parent is not entered, the parent returns -ENOTCONN
 * and the adapter propagates that.
 *
 * The adapter never issues a mass-erase on behalf of the caller: that
 * would be wrong when the adapter's region is smaller than the target's
 * total flash. Callers that want mass-erase invoke
 * stm32_bootloader_mass_erase() directly on the parent.
 */

#define DT_DRV_COMPAT st_stm32_bootloader_flash

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/misc/stm32_bootloader/stm32_bootloader.h>

LOG_MODULE_REGISTER(stm32_sys_bl_flash, CONFIG_STM32_BOOTLOADER_LOG_LEVEL);

struct stm32_bl_flash_config {
	/** Parent bootloader device (transport-agnostic). */
	const struct device *parent;
	/** Absolute target address of region start. */
	uint32_t base_address;
	/** Region size in bytes. */
	size_t size;
	/** Uniform erase granularity. */
	size_t erase_block_size;
	/** Write alignment (AN3155 mandates 4). */
	size_t write_block_size;
	bool read_only;
	/** flash_driver_api static parameters. */
	struct flash_parameters params;
	/** Uniform page layout. */
	struct flash_pages_layout page_layout;
};

static int adapter_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct stm32_bl_flash_config *cfg = dev->config;

	if (len == 0) {
		return 0;
	}
	if (buf == NULL || offset < 0) {
		return -EINVAL;
	}
	if ((size_t)offset + len > cfg->size) {
		return -EINVAL;
	}

	return stm32_bootloader_read_memory(cfg->parent, cfg->base_address + (uint32_t)offset, buf,
					    len);
}

static int adapter_write(const struct device *dev, off_t offset, const void *buf, size_t len)
{
	const struct stm32_bl_flash_config *cfg = dev->config;

	if (cfg->read_only) {
		return -EACCES;
	}
	if (len == 0) {
		return 0;
	}
	if (buf == NULL || offset < 0) {
		return -EINVAL;
	}
	if ((size_t)offset + len > cfg->size) {
		return -EINVAL;
	}
	if (((size_t)offset % cfg->write_block_size) != 0 || (len % cfg->write_block_size) != 0) {
		LOG_ERR("Write not %zu-byte aligned", cfg->write_block_size);
		return -EINVAL;
	}

	return stm32_bootloader_write_memory(cfg->parent, cfg->base_address + (uint32_t)offset, buf,
					     len);
}

static int adapter_erase(const struct device *dev, off_t offset, size_t size)
{
	const struct stm32_bl_flash_config *cfg = dev->config;

	if (cfg->read_only) {
		return -EACCES;
	}
	if (size == 0) {
		return 0;
	}
	if (offset < 0) {
		return -EINVAL;
	}
	if ((size_t)offset + size > cfg->size) {
		return -EINVAL;
	}
	if (((size_t)offset % cfg->erase_block_size) != 0 || (size % cfg->erase_block_size) != 0) {
		LOG_ERR("Erase not %zu-byte aligned", cfg->erase_block_size);
		return -EINVAL;
	}

	/*
	 * Convert the (offset, size) range into a list of AN3155 page indices
	 * relative to the target's flash base. The parent bootloader driver
	 * maps those indices to the right Standard or Extended Erase command.
	 *
	 * A small on-stack buffer keeps the translation allocation-free;
	 * STM32 flash sizes in circulation need at most a few hundred pages,
	 * well within the AN3155 per-command limit.
	 */
	size_t first = (size_t)offset / cfg->erase_block_size;
	size_t count = size / cfg->erase_block_size;
	uint16_t pages[CONFIG_STM32_BOOTLOADER_FLASH_MAX_PAGES_PER_ERASE];

	if (count > ARRAY_SIZE(pages)) {
		LOG_ERR("Erase covers %zu pages, max is %u", count,
			(unsigned int)ARRAY_SIZE(pages));
		return -ENOMEM;
	}

	/*
	 * The AN3155 page index is a region-relative counter: page 0 is at
	 * the region's declared reg base. The parent driver forwards it as
	 * raw page number — the target's bootloader interprets it against
	 * its own flash base (e.g. 0x08000000), which is what we want as
	 * long as the region actually begins at the target's flash base.
	 *
	 * For non-main-flash regions (option bytes, system memory) AN3155
	 * page erase is typically not applicable; such regions should be
	 * declared read-only.
	 */
	for (size_t i = 0; i < count; i++) {
		pages[i] = (uint16_t)(first + i);
	}

	return stm32_bootloader_erase_pages(cfg->parent, pages, count);
}

static const struct flash_parameters *adapter_get_parameters(const struct device *dev)
{
	const struct stm32_bl_flash_config *cfg = dev->config;

	return &cfg->params;
}

static int adapter_get_size(const struct device *dev, uint64_t *size)
{
	const struct stm32_bl_flash_config *cfg = dev->config;

	*size = (uint64_t)cfg->size;
	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void adapter_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
				size_t *layout_size)
{
	const struct stm32_bl_flash_config *cfg = dev->config;

	*layout = &cfg->page_layout;
	*layout_size = 1;
}
#endif

static DEVICE_API(flash, stm32_bl_flash_api) = {
	.read = adapter_read,
	.write = adapter_write,
	.erase = adapter_erase,
	.get_parameters = adapter_get_parameters,
	.get_size = adapter_get_size,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = adapter_page_layout,
#endif
};

static int stm32_bl_flash_init(const struct device *dev)
{
	const struct stm32_bl_flash_config *cfg = dev->config;

	if (!device_is_ready(cfg->parent)) {
		LOG_ERR("Parent bootloader device not ready");
		return -ENODEV;
	}
	return 0;
}

#define STM32_BL_FLASH_DEFINE(inst)                                                                \
	BUILD_ASSERT(DT_INST_REG_SIZE(inst) % DT_INST_PROP(inst, erase_block_size) == 0,           \
		     "region size must be a multiple of erase-block-size");                        \
                                                                                                   \
	static const struct stm32_bl_flash_config stm32_bl_flash_cfg_##inst = {                    \
		.parent = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                     \
		.base_address = (uint32_t)DT_INST_REG_ADDR(inst),                                  \
		.size = (size_t)DT_INST_REG_SIZE(inst),                                            \
		.erase_block_size = DT_INST_PROP(inst, erase_block_size),                          \
		.write_block_size = DT_INST_PROP(inst, write_block_size),                          \
		.read_only = DT_INST_PROP(inst, read_only),                                        \
		.params =                                                                          \
			{                                                                          \
				.write_block_size = DT_INST_PROP(inst, write_block_size),          \
				.erase_value = 0xFF,                                               \
			},                                                                         \
		.page_layout =                                                                     \
			{                                                                          \
				.pages_count = DT_INST_REG_SIZE(inst) /                            \
					       DT_INST_PROP(inst, erase_block_size),               \
				.pages_size = DT_INST_PROP(inst, erase_block_size),                \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, stm32_bl_flash_init, NULL, NULL, &stm32_bl_flash_cfg_##inst,   \
			      POST_KERNEL, CONFIG_STM32_BOOTLOADER_FLASH_INIT_PRIORITY,            \
			      &stm32_bl_flash_api);

DT_INST_FOREACH_STATUS_OKAY(STM32_BL_FLASH_DEFINE)
