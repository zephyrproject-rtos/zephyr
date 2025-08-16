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

#ifdef CONFIG_ESP32_EFUSE_VIRTUAL_KEEP_IN_FLASH
#define ENCRYPTION_IS_VIRTUAL (!efuse_hal_flash_encryption_enabled())
#else
#define ENCRYPTION_IS_VIRTUAL 0
#endif

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef ALIGN_UP
#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))
#endif

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(num, align) ((num) & ~((align) - 1))
#endif

#ifndef ALIGN_OFFSET
#define ALIGN_OFFSET(num, align) ((num) & ((align) - 1))
#endif

#ifndef IS_ALIGNED
#define IS_ALIGNED(num, align) (ALIGN_OFFSET((num), (align)) == 0)
#endif

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

#ifndef CONFIG_MCUBOOT
#define FLASH_BUFFER_SIZE 32

static bool aligned_flash_write(size_t dest_addr, const void *src, size_t size, bool erase);
static bool aligned_flash_erase(size_t addr, size_t size);
#endif

#ifdef CONFIG_MCUBOOT
#define READ_BUFFER_SIZE 32
static bool flash_esp32_is_aligned(off_t address, void *buffer, size_t length)
{
	/* check if address, buffer pointer, and length are 4-byte aligned */
	return ((address & 3) == 0) && (((uintptr_t)buffer & 3) == 0) && ((length & 3) == 0);
}
#endif

static int flash_esp32_read_check_enc(off_t address, void *buffer, size_t length)
{
	int ret = 0;

	if (esp_flash_encryption_enabled()) {
		LOG_DBG("Flash read ENCRYPTED - address 0x%lx size 0x%x", address, length);
		ret = esp_flash_read_encrypted(NULL, address, buffer, length);
	} else {
		LOG_DBG("Flash read RAW - address 0x%lx size 0x%x", address, length);
		ret = esp_flash_read(NULL, buffer, address, length);
	}

	if (ret != 0) {
		LOG_ERR("Flash read error: %d", ret);
		return -EIO;
	}

	return 0;
}

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

	ret = flash_esp32_read_check_enc(address, buffer, length);

	flash_esp32_sem_give(dev);
#endif

	if (ret != 0) {
		LOG_ERR("Flash read error: %d", ret);
		return -EIO;
	}

	return 0;
}

static int flash_esp32_write_check_enc(off_t address, const void *buffer, size_t length)
{
	int ret = 0;

	if (esp_flash_encryption_enabled() && !ENCRYPTION_IS_VIRTUAL) {
		LOG_DBG("Flash write ENCRYPTED - address 0x%lx size 0x%x", address, length);
		ret = esp_flash_write_encrypted(NULL, address, buffer, length);
	} else {
		LOG_DBG("Flash write RAW - address 0x%lx size 0x%x", address, length);
		ret = esp_flash_write(NULL, buffer, address, length);
	}

	if (ret != 0) {
		LOG_ERR("Flash write error: %d", ret);
		return -EIO;
	}

	return 0;
}

#ifndef CONFIG_MCUBOOT

/* Auxiliar buffer to store the sector that will be partially written */
static uint8_t __aligned(32) write_aux_buf[FLASH_SECTOR_SIZE] = {0};

/* Auxiliar buffer to store the sector that will be partially erased */
static uint8_t __aligned(32) erase_aux_buf[FLASH_SECTOR_SIZE] = {0};

static bool aligned_flash_write(size_t dest_addr, const void *src, size_t size, bool erase)
{
	bool flash_encryption_enabled = esp_flash_encryption_enabled();

	/* When flash encryption is enabled, write alignment is 32 bytes, however to avoid
	 * inconsistences the region may be erased right before writing, thus the alignment
	 * is set to the erase required alignment (FLASH_SECTOR_SIZE).
	 * When flash encryption is not enabled, regular write alignment is 4 bytes.
	 */
	size_t alignment = flash_encryption_enabled ? (erase ? FLASH_SECTOR_SIZE : 32) : 4;

	if (IS_ALIGNED(dest_addr, alignment) && IS_ALIGNED((uintptr_t)src, 4)
		&& IS_ALIGNED(size, alignment)) {
		/* A single write operation is enough when all parameters are aligned */

		if (flash_encryption_enabled && erase) {
			if (esp_flash_erase_region(NULL, dest_addr, size) != ESP_OK) {
				return false;
			}
		}
		return flash_esp32_write_check_enc(dest_addr, (void *)src, size) == ESP_OK;
	}

	LOG_DBG("%s: forcing unaligned write dest_addr: 0x%08x src: 0x%08x size: 0x%x erase: %c",
			__func__, (uint32_t)dest_addr, (uint32_t)src, size, erase ? 't' : 'f');

	const uint32_t aligned_addr = ALIGN_DOWN(dest_addr, alignment);
	const uint32_t addr_offset = ALIGN_OFFSET(dest_addr, alignment);
	uint32_t bytes_remaining = size;

	/* Perform a read operation considering an offset not aligned to 4-byte boundary */

	uint32_t bytes = MIN(bytes_remaining + addr_offset, sizeof(write_aux_buf));

	if (flash_esp32_read_check_enc(aligned_addr, write_aux_buf, ALIGN_UP(bytes, alignment)) !=
		ESP_OK) {
		return false;
	}

	if (flash_encryption_enabled && erase) {
		if (esp_flash_erase_region(NULL, aligned_addr,
			ALIGN_UP(bytes, FLASH_SECTOR_SIZE)) != ESP_OK) {
			return false;
		}
	}

	uint32_t bytes_written = bytes - addr_offset;

	memcpy(&write_aux_buf[addr_offset], src, bytes_written);

	if (flash_esp32_write_check_enc(aligned_addr, write_aux_buf, ALIGN_UP(bytes, alignment)) !=
		ESP_OK) {
		return false;
	}

	bytes_remaining -= bytes_written;

	/* Write remaining data to Flash if any */

	uint32_t offset = bytes;

	while (bytes_remaining != 0) {
		bytes = MIN(bytes_remaining, sizeof(write_aux_buf));
		if (flash_esp32_read_check_enc(aligned_addr + offset, write_aux_buf,
			ALIGN_UP(bytes, alignment)) != ESP_OK) {
			return false;
		}

		if (flash_encryption_enabled && erase) {
			if (esp_flash_erase_region(NULL, aligned_addr + offset,
				ALIGN_UP(bytes, FLASH_SECTOR_SIZE)) != ESP_OK) {
				return false;
			}
		}

		memcpy(write_aux_buf, &((uint8_t *)src)[bytes_written], bytes);

		if (flash_esp32_write_check_enc(aligned_addr + offset, write_aux_buf,
			ALIGN_UP(bytes, alignment)) != ESP_OK) {
			return false;
		}

		offset += bytes;
		bytes_written += bytes;
		bytes_remaining -= bytes;
	}

	return true;
}

static bool aligned_flash_erase(size_t addr, size_t size)
{
	if (IS_ALIGNED(addr, FLASH_SECTOR_SIZE) && IS_ALIGNED(size, FLASH_SECTOR_SIZE)) {
		/* A single erase operation is enough when all parameters are aligned */

		return esp_flash_erase_region(NULL, addr, size) == ESP_OK;
	}

	LOG_DBG("%s: forcing unaligned erase on sector Offset: 0x%x Length: 0x%x",
					__func__, (int)addr, (int)size);

	const uint32_t aligned_addr = ALIGN_DOWN(addr, FLASH_SECTOR_SIZE);
	const uint32_t addr_offset = ALIGN_OFFSET(addr, FLASH_SECTOR_SIZE);
	uint32_t bytes_remaining = size;

	/* Perform a read operation considering an offset not aligned to 4-byte boundary */

	uint32_t bytes = MIN(bytes_remaining + addr_offset, sizeof(erase_aux_buf));

	if (flash_esp32_read_check_enc(aligned_addr, erase_aux_buf,
				       ALIGN_UP(bytes, FLASH_SECTOR_SIZE)) != ESP_OK) {
		return false;
	}

	if (esp_flash_erase_region(NULL, aligned_addr, ALIGN_UP(bytes, FLASH_SECTOR_SIZE)) !=
	    ESP_OK) {
		LOG_ERR("%s: Flash erase failed", __func__);
		return -1;
	}

	uint32_t bytes_written = bytes - addr_offset;

	/* Write first part of non-erased data */
	if (addr_offset > 0) {
		if (!aligned_flash_write(aligned_addr, erase_aux_buf, addr_offset, false)) {
			return false;
		}
	}

	if (bytes < sizeof(erase_aux_buf)) {
		if (!aligned_flash_write(aligned_addr + bytes, erase_aux_buf + bytes,
					    sizeof(erase_aux_buf) - bytes, false)) {
			return false;
		}
	}

	bytes_remaining -= bytes_written;

	/* Write remaining data to Flash if any */

	uint32_t offset = bytes;

	while (bytes_remaining != 0) {
		bytes = MIN(bytes_remaining, sizeof(erase_aux_buf));
		if (flash_esp32_read_check_enc(aligned_addr + offset, erase_aux_buf,
					       ALIGN_UP(bytes, FLASH_SECTOR_SIZE)) != ESP_OK) {
			return false;
		}

		if (esp_flash_erase_region(NULL, aligned_addr + offset,
					   ALIGN_UP(bytes, FLASH_SECTOR_SIZE)) != ESP_OK) {
			LOG_ERR("%s: Flash erase failed", __func__);
			return -1;
		}

		if (bytes < sizeof(erase_aux_buf)) {
			if (!aligned_flash_write(aligned_addr + offset + bytes,
				    erase_aux_buf + bytes, sizeof(erase_aux_buf) - bytes, false)) {
				return false;
			}
		}

		offset += bytes;
		bytes_written += bytes;
		bytes_remaining -= bytes;
	}

	return true;
}
#endif

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
	bool erase = false;

	if (esp_flash_encryption_enabled()) {
		/* Ensuring flash region has been erased before writing in order to
		 * avoid inconsistences when hardware flash encryption is enabled.
		 */
		erase = true;
	}

	if (!aligned_flash_write(address, buffer, length, erase)) {
		LOG_ERR("%s: Flash erase before write failed", __func__);
		ret = -1;
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

	if ((len % FLASH_SECTOR_SIZE) != 0 || (start % FLASH_SECTOR_SIZE) != 0) {
		LOG_DBG("%s: Not aligned on sector Offset: 0x%x Length: 0x%x", __func__, (int)start,
			(int)len);

		if (!aligned_flash_erase(start, len)) {
			ret = -EIO;
		}
	} else {
		LOG_DBG("%s: Aligned Addr: 0x%08x Length: %d", __func__, (int)start, (int)len);

		ret = esp_flash_erase_region(NULL, start, len);
	}

	if (esp_flash_encryption_enabled()) {
		uint8_t erased_val_buf[FLASH_BUFFER_SIZE];
		uint32_t bytes_remaining = len;
		uint32_t offset = start;
		uint32_t bytes_written = MIN(sizeof(erased_val_buf), len);

		memset(erased_val_buf,
			flash_get_parameters(dev)->erase_value, sizeof(erased_val_buf));

		/* When hardware flash encryption is enabled, force expected erased
		 * value (0xFF) into flash when erasing a region.
		 *
		 * This is handled on this implementation because MCUboot's state
		 * machine relies on erased valued data (0xFF) readed from a
		 * previously erased region that was not written yet, however when
		 * hardware flash encryption is enabled, the flash read always
		 * decrypts whats being read from flash, thus a region that was
		 * erased would not be read as what MCUboot expected (0xFF).
		 */
		while (bytes_remaining != 0) {
			if (!aligned_flash_write(offset, erased_val_buf, bytes_written, false)) {
				LOG_ERR("%s: Flash erase failed", __func__);
				return -1;
			}
			offset += bytes_written;
			bytes_remaining -= bytes_written;
		}
	}

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
