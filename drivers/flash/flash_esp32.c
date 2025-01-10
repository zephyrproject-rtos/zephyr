/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_flash_controller
#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)

#define FLASH_WRITE_BLK_SZ DT_PROP(SOC_NV_FLASH_NODE, write_block_size)
#define FLASH_ERASE_BLK_SZ DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)

/*
 * HAL includes go first to
 * avoid BIT macro redefinition
 */
#include <esp_flash.h>
#include <spi_flash_mmap.h>
#include <soc/spi_struct.h>
#include <esp_flash_encrypt.h>
#include <esp_flash_internal.h>
#include <bootloader_flash_priv.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/drivers/flash.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_esp32, CONFIG_FLASH_LOG_LEVEL);

#define FLASH_SEM_TIMEOUT (k_is_in_isr() ? K_NO_WAIT : K_FOREVER)

struct flash_esp32_dev_config {
	spi_dev_t *controller;
};

struct flash_esp32_dev_data {
#ifdef CONFIG_MULTITHREADING
	struct k_sem sem;
#endif
};

static const struct flash_parameters flash_esp32_parameters = {
	.write_block_size = FLASH_WRITE_BLK_SZ,
	.erase_value = 0xff,
};

#ifdef CONFIG_MULTITHREADING
static inline void flash_esp32_sem_take(const struct device *dev)
{
	struct flash_esp32_dev_data *data = dev->data;

	k_sem_take(&data->sem, FLASH_SEM_TIMEOUT);
}

static inline void flash_esp32_sem_give(const struct device *dev)
{
	struct flash_esp32_dev_data *data = dev->data;

	k_sem_give(&data->sem);
}
#else

#define flash_esp32_sem_take(dev) do {} while (0)
#define flash_esp32_sem_give(dev) do {} while (0)

#endif /* CONFIG_MULTITHREADING */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <stdint.h>
#include <string.h>

#ifdef CONFIG_MCUBOOT
#define READ_BUFFER_SIZE 32
static bool flash_esp32_is_aligned(off_t address, void *buffer, size_t length)
{
	/* check if address, buffer pointer, and length are 4-byte aligned */
	return ((address & 3) == 0) && (((uintptr_t)buffer & 3) == 0) && ((length & 3) == 0);
}
#endif

static int flash_esp32_read(const struct device *dev, off_t address, void *buffer, size_t length)
{
	int ret = 0;

#ifdef CONFIG_MCUBOOT
	uint8_t *dest_ptr = (uint8_t *)buffer;
	size_t remaining = length;
	size_t copy_size = 0;
	size_t aligned_size = 0;
	bool allow_decrypt = esp_flash_encryption_enabled();

	if (flash_esp32_is_aligned(address, buffer, length)) {
		ret = esp_rom_flash_read(address, buffer, length, allow_decrypt);
		return (ret == ESP_OK) ? 0 : -EIO;
	}

	/* handle unaligned reading */
	uint8_t __aligned(4) temp_buf[READ_BUFFER_SIZE + 8];
	while (remaining > 0) {
		size_t addr_offset = address & 3;
		size_t buf_offset = (uintptr_t)dest_ptr & 3;

		copy_size = (remaining > READ_BUFFER_SIZE) ? READ_BUFFER_SIZE : remaining;

		if (addr_offset == 0 && buf_offset == 0 && copy_size >= 4) {
			aligned_size = copy_size & ~3;
			ret = esp_rom_flash_read(address, dest_ptr, aligned_size, allow_decrypt);
			if (ret != ESP_OK) {
				return -EIO;
			}

			address += aligned_size;
			dest_ptr += aligned_size;
			remaining -= aligned_size;
		} else {
			size_t start_addr = address - addr_offset;

			aligned_size = (copy_size + addr_offset + 3) & ~3;

			ret = esp_rom_flash_read(start_addr, temp_buf, aligned_size, allow_decrypt);
			if (ret != ESP_OK) {
				return -EIO;
			}

			memcpy(dest_ptr, temp_buf + addr_offset, copy_size);

			address += copy_size;
			dest_ptr += copy_size;
			remaining -= copy_size;
		}
	}
#else
	flash_esp32_sem_take(dev);

	if (esp_flash_encryption_enabled()) {
		ret = esp_flash_read_encrypted(NULL, address, buffer, length);
	} else {
		ret = esp_flash_read(NULL, buffer, address, length);
	}

	flash_esp32_sem_give(dev);
#endif

	if (ret != 0) {
		LOG_ERR("Flash read error: %d", ret);
		return -EIO;
	}

	return 0;
}

static int flash_esp32_write(const struct device *dev,
			     off_t address,
			     const void *buffer,
			     size_t length)
{
	int ret = 0;

#ifdef CONFIG_MCUBOOT
	if (!flash_esp32_is_aligned(address, (void *)buffer, length)) {
		LOG_ERR("Unaligned flash write is not supported");
		return -EINVAL;
	}

	bool encrypt = esp_flash_encryption_enabled();

	ret = esp_rom_flash_write(address, (void *)buffer, length, encrypt);
#else
	flash_esp32_sem_take(dev);

	if (esp_flash_encryption_enabled()) {
		ret = esp_flash_write_encrypted(NULL, address, buffer, length);
	} else {
		ret = esp_flash_write(NULL, buffer, address, length);
	}

	flash_esp32_sem_give(dev);
#endif

	if (ret != 0) {
		LOG_ERR("Flash write error: %d", ret);
		return -EIO;
	}

	return 0;
}

static int flash_esp32_erase(const struct device *dev, off_t start, size_t len)
{
	int ret = 0;

#ifdef CONFIG_MCUBOOT
	ret = esp_rom_flash_erase_range(start, len);
#else
	flash_esp32_sem_take(dev);
	ret = esp_flash_erase_region(NULL, start, len);
	flash_esp32_sem_give(dev);
#endif
	if (ret != 0) {
		LOG_ERR("Flash erase error: %d", ret);
		return -EIO;
	}
	return 0;
}

#if CONFIG_FLASH_PAGE_LAYOUT
static const struct flash_pages_layout flash_esp32_pages_layout = {
	.pages_count = DT_REG_SIZE(SOC_NV_FLASH_NODE) / FLASH_ERASE_BLK_SZ,
	.pages_size = DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
};

void flash_esp32_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	*layout = &flash_esp32_pages_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *
flash_esp32_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_esp32_parameters;
}

static int flash_esp32_init(const struct device *dev)
{
#ifdef CONFIG_MULTITHREADING
	struct flash_esp32_dev_data *const dev_data = dev->data;

	k_sem_init(&dev_data->sem, 1, 1);
#endif /* CONFIG_MULTITHREADING */

	return 0;
}

static DEVICE_API(flash, flash_esp32_driver_api) = {
	.read = flash_esp32_read,
	.write = flash_esp32_write,
	.erase = flash_esp32_erase,
	.get_parameters = flash_esp32_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_esp32_page_layout,
#endif
};

static struct flash_esp32_dev_data flash_esp32_data;

static const struct flash_esp32_dev_config flash_esp32_config = {
	.controller = (spi_dev_t *) DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, flash_esp32_init,
		      NULL,
		      &flash_esp32_data, &flash_esp32_config,
		      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,
		      &flash_esp32_driver_api);
