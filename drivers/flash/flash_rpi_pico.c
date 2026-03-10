/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * Copyright (c) 2022 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <stddef.h>

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#include <hardware/flash.h>

LOG_MODULE_REGISTER(flash_rpi_pico, CONFIG_FLASH_LOG_LEVEL);

#define DT_DRV_COMPAT raspberrypi_pico_flash_controller

#define PAGE_SIZE   256
#define SECTOR_SIZE DT_PROP(DT_CHOSEN(zephyr_flash), erase_block_size)
#define ERASE_VALUE 0xff
#define FLASH_SIZE  KB(CONFIG_FLASH_SIZE)

static const struct flash_parameters flash_rpi_parameters = {
	.write_block_size = 1,
	.erase_value = ERASE_VALUE,
};

static uint8_t flash_ram_buffer[PAGE_SIZE];

static bool is_valid_range(off_t offset, uint32_t size)
{
	return (offset >= 0) && ((offset + size) <= FLASH_SIZE);
}

static int flash_rpi_read(const struct device *dev, off_t offset, void *data, size_t size)
{
	ARG_UNUSED(dev);

	if (size == 0) {
		return 0;
	}

	if (!is_valid_range(offset, size)) {
		LOG_ERR("Read range exceeds the flash boundaries");
		return -EINVAL;
	}

	memcpy(data, (uint8_t *)(CONFIG_FLASH_BASE_ADDRESS + offset), size);

	return 0;
}

static int flash_rpi_write(const struct device *dev, off_t offset, const void *data, size_t size)
{
	uint32_t key;
	size_t bytes_to_write;
	uint8_t *data_pointer = (uint8_t *)data;

	ARG_UNUSED(dev);

	if (size == 0) {
		return 0;
	}

	if (!is_valid_range(offset, size)) {
		LOG_ERR("Write range exceeds the flash boundaries. Offset=%#lx, Size=%u", offset,
			size);
		return -EINVAL;
	}

	key = irq_lock();

	if ((offset & (PAGE_SIZE - 1)) > 0) {
		bytes_to_write = MIN(PAGE_SIZE - (offset & (PAGE_SIZE - 1)), size);
		memcpy(flash_ram_buffer, data_pointer, bytes_to_write);
		flash_write_partial(offset, flash_ram_buffer, bytes_to_write);
		data_pointer += bytes_to_write;
		size -= bytes_to_write;
		offset += bytes_to_write;
	}

	while (size >= PAGE_SIZE) {
		bytes_to_write = PAGE_SIZE;
		memcpy(flash_ram_buffer, data_pointer, bytes_to_write);
		flash_range_program(offset, flash_ram_buffer, bytes_to_write);
		data_pointer += bytes_to_write;
		size -= bytes_to_write;
		offset += bytes_to_write;
	}

	if (size > 0) {
		memcpy(flash_ram_buffer, data_pointer, size);
		flash_write_partial(offset, flash_ram_buffer, size);
	}

	irq_unlock(key);

	return 0;
}

static int flash_rpi_erase(const struct device *dev, off_t offset, size_t size)
{
	uint32_t key;

	if (size == 0) {
		return 0;
	}

	if (!is_valid_range(offset, size)) {
		LOG_ERR("Erase range exceeds the flash boundaries. Offset=%#lx, Size=%u", offset,
			size);
		return -EINVAL;
	}

	if ((offset % SECTOR_SIZE) || (size % SECTOR_SIZE)) {
		LOG_ERR("Erase range is not a multiple of the sector size. Offset=%#lx, Size=%u",
			offset, size);
		return -EINVAL;
	}

	key = irq_lock();

	flash_range_erase(offset, size);

	irq_unlock(key);

	return 0;
}

static const struct flash_parameters *flash_rpi_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_rpi_parameters;
}

#if CONFIG_FLASH_PAGE_LAYOUT

static const struct flash_pages_layout flash_rpi_pages_layout = {
	.pages_count = FLASH_SIZE / SECTOR_SIZE,
	.pages_size = SECTOR_SIZE,
};

void flash_rpi_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
			   size_t *layout_size)
{
	*layout = &flash_rpi_pages_layout;
	*layout_size = 1;
}

#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static DEVICE_API(flash, flash_rpi_driver_api) = {
	.read = flash_rpi_read,
	.write = flash_rpi_write,
	.erase = flash_rpi_erase,
	.get_parameters = flash_rpi_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_rpi_page_layout,
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,
		      &flash_rpi_driver_api);
