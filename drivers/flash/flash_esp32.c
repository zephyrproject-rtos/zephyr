/*
 * Copyright (c) 2021-2025 Espressif Systems (Shanghai) Co., Ltd.
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
#include <zephyr/devicetree.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/drivers/flash.h>
#include <soc.h>

#ifdef CONFIG_ESP_FLASH_ASYNC_IPM
#include <zephyr/drivers/ipm.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_esp32, CONFIG_FLASH_LOG_LEVEL);

#define FLASH_SEM_TIMEOUT (k_is_in_isr() ? K_NO_WAIT : K_FOREVER)

#ifdef CONFIG_ESP32_EFUSE_VIRTUAL_KEEP_IN_FLASH
#define ENCRYPTION_IS_VIRTUAL (!efuse_hal_flash_encryption_enabled())
#else
#define ENCRYPTION_IS_VIRTUAL 0
#endif

#ifndef ALIGN_OFFSET
#define ALIGN_OFFSET(num, align) ((num) & ((align) - 1))
#endif

struct flash_esp32_dev_config {
	spi_dev_t *controller;
};

#ifdef CONFIG_ESP_FLASH_ASYNC

#ifdef CONFIG_ESP_FLASH_ASYNC_WORK
K_THREAD_STACK_DEFINE(esp_flash_workqueue_stack, CONFIG_ESP_FLASH_ASYNC_WORK_STACK_SIZE);
static struct k_work_q esp_flash_workqueue;
#endif

enum flash_op {
	FLASH_OP_NONE,
	FLASH_OP_READ,
	FLASH_OP_WRITE,
	FLASH_OP_ERASE
};

typedef void (*flash_done_cb_t)(void *);

struct flash_req {
	enum flash_op op;
	off_t addr;
	size_t len;
	void *buf;
	int result;
};

#ifdef CONFIG_ESP_FLASH_ASYNC_IPM
enum host_remote_cmd {
	CMD_NONE,
	CMD_REQUEST,
	CMD_RESPONSE
};
#endif
#endif /* CONFIG_ESP_FLASH_ASYNC */

struct flash_esp32_dev_data {
#ifdef CONFIG_MULTITHREADING
#ifdef CONFIG_ESP_FLASH_ASYNC
	const struct device *dev;
	struct k_mutex lock;
	struct flash_req req;
	struct k_work work;
	struct k_sem sync;
#ifdef CONFIG_ESP_FLASH_ASYNC_IPM
	const struct device *ipm;
	struct k_work remote_work;
	struct flash_req remote_req;
	struct k_sem remote_sync;
#endif
#endif
	struct k_sem sem;
#endif /* CONFIG_MULTITHREADING */
};

static const struct flash_parameters flash_esp32_parameters = {
	.write_block_size = FLASH_WRITE_BLK_SZ,
	.erase_value = 0xff,
};

#if defined(CONFIG_MULTITHREADING) && !defined(CONFIG_ESP_FLASH_ASYNC)
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
#else /* CONFIG_MULTITHREADING && !CONFIG_ESP_FLASH_ASYNC */

#define flash_esp32_sem_take(dev) do {} while (0)
#define flash_esp32_sem_give(dev) do {} while (0)

#endif /* CONFIG_MULTITHREADING && !CONFIG_ESP_FLASH_ASYNC */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <stdint.h>
#include <string.h>

#ifdef CONFIG_ESP_FLASH_HOST
#ifndef CONFIG_MCUBOOT
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

#ifdef CONFIG_ESP_FLASH_ENCRYPTION
#define FLASH_BUFFER_SIZE 32

static bool aligned_flash_write(size_t dest_addr, const void *src, size_t size, bool erase);
static bool aligned_flash_erase(size_t addr, size_t size);

/* Auxiliar buffer to store the sector that will be partially written */
static uint8_t write_aux_buf[FLASH_SECTOR_SIZE] = {0};

/* Auxiliar buffer to store the sector that will be partially erased */
static uint8_t erase_aux_buf[FLASH_SECTOR_SIZE] = {0};

static bool aligned_flash_write(size_t dest_addr, const void *src, size_t size, bool erase)
{
	bool flash_encryption_enabled = esp_flash_encryption_enabled();

	/* When flash encryption is enabled, write alignment is 32 bytes, however to avoid
	 * inconsistences the region may be erased right before writing, thus the alignment
	 * is set to the erase required alignment (FLASH_SECTOR_SIZE).
	 * When flash encryption is not enabled, regular write alignment is 4 bytes.
	 */
	size_t alignment = flash_encryption_enabled ? (erase ? FLASH_SECTOR_SIZE : 32) : 4;

	if (IS_ALIGNED(dest_addr, alignment) && IS_ALIGNED((uintptr_t)src, 4) &&
	    IS_ALIGNED(size, alignment)) {
		/* A single write operation is enough when all parameters are aligned */

		if (flash_encryption_enabled && erase) {
			if (esp_flash_erase_region(NULL, dest_addr, size) != ESP_OK) {
				LOG_ERR("%s: Flash erase failed at 0x%08lx", __func__,
					(uintptr_t)dest_addr);
				return false;
			}
		}
		return flash_esp32_write_check_enc(dest_addr, (void *)src, size) == ESP_OK;
	}

	LOG_DBG("%s: forcing unaligned write dest_addr: 0x%08lx src: 0x%08lx size: 0x%x erase: %c",
		__func__, (uintptr_t)dest_addr, (uintptr_t)src, size, erase ? 't' : 'f');

	size_t write_addr = dest_addr;
	size_t bytes_remaining = size;
	size_t src_offset = 0;

	while (bytes_remaining > 0) {
		size_t aligned_curr_addr = ROUND_DOWN(write_addr, alignment);
		size_t curr_buf_off = write_addr - aligned_curr_addr;
		size_t chunk_len = MIN(bytes_remaining, FLASH_SECTOR_SIZE - curr_buf_off);

		/* Read data before modifying */
		if (flash_esp32_read_check_enc(aligned_curr_addr, write_aux_buf,
					       ROUND_UP(chunk_len, alignment)) != ESP_OK) {
			LOG_ERR("%s: Flash read failed at 0x%08lx", __func__,
				(uintptr_t)aligned_curr_addr);
			return false;
		}

		/* Erase if needed */
		if (flash_encryption_enabled && erase) {
			if (esp_flash_erase_region(NULL, aligned_curr_addr,
						   ROUND_UP(chunk_len, FLASH_SECTOR_SIZE)) !=
			    ESP_OK) {
				LOG_ERR("%s: Flash erase failed at 0x%08lx", __func__,
					(uintptr_t)aligned_curr_addr);
				return false;
			}
		}

		/* Merge data into buffer */
		memcpy(&write_aux_buf[curr_buf_off], &((const uint8_t *)src)[src_offset],
		       chunk_len);

		/* Write back aligned chunk */
		if (flash_esp32_write_check_enc(aligned_curr_addr, write_aux_buf,
						ROUND_UP(chunk_len, alignment)) != ESP_OK) {
			LOG_ERR("%s: Flash write failed at 0x%08lx", __func__,
				(uintptr_t)aligned_curr_addr);
			return false;
		}

		write_addr += chunk_len;
		src_offset += chunk_len;
		bytes_remaining -= chunk_len;
	}

	return true;
}

static bool erase_partial_sector(size_t addr, size_t sector_size, size_t erase_start,
								 size_t erase_end)
{
	/* Read full sector before erasing */
	if (flash_esp32_read_check_enc(addr, erase_aux_buf, sector_size) != ESP_OK) {
		LOG_ERR("%s: Flash read failed at 0x%08lx", __func__, (uintptr_t)addr);
		return false;
	}
	/* Erase full sector */
	if (esp_flash_erase_region(NULL, addr, sector_size) != ESP_OK) {
		LOG_ERR("%s: Flash erase failed at 0x%08lx", __func__, (uintptr_t)addr);
		return false;
	}
	/* Write back preserved head data up to erase_start */
	if (erase_start > 0) {
		if (!aligned_flash_write(addr, erase_aux_buf, erase_start, false)) {
			LOG_ERR("%s: Flash write failed at 0x%08lx", __func__, (uintptr_t)addr);
			return false;
		}
	}
	/* Write back preserved tail data from erase_end up to sector end */
	if (erase_end < sector_size) {
		if (!aligned_flash_write(addr + erase_end, &erase_aux_buf[erase_end],
								sector_size - erase_end, false)) {
			LOG_ERR("%s: Flash write failed at 0x%08lx", __func__,
					(uintptr_t)(addr + erase_end));
			return false;
		}
	}
	return true;
}

static bool aligned_flash_erase(size_t addr, size_t size)
{
	if (IS_ALIGNED(addr, FLASH_SECTOR_SIZE) && IS_ALIGNED(size, FLASH_SECTOR_SIZE)) {
		/* A single erase operation is enough when all parameters are aligned */
		return esp_flash_erase_region(NULL, addr, size) == ESP_OK;
	}

	const size_t sector_size = FLASH_SECTOR_SIZE;
	const size_t start_addr = ROUND_DOWN(addr, sector_size);
	const size_t end_addr = ROUND_UP(addr + size, sector_size);
	const size_t total_len = end_addr - start_addr;

	LOG_DBG("%s: forcing unaligned erase on sector Offset: "
		"0x%08lx Length: 0x%x total_len: 0x%x",
		__func__, (uintptr_t)addr, (int)size, total_len);

	size_t current_addr = start_addr;

	while (current_addr < end_addr) {
		bool preserve_head = (addr > current_addr);
		bool preserve_tail = ((addr + size) < (current_addr + sector_size));

		if (preserve_head || preserve_tail) {
			size_t erase_start = preserve_head ? (addr - current_addr) : 0;
			size_t erase_end =
				MIN(current_addr + sector_size, addr + size) - current_addr;

			LOG_DBG("%s: partial sector erase from: 0x%08lx to: 0x%08lx Length: 0x%x",
				__func__, (uintptr_t)(current_addr + erase_start),
				(uintptr_t)(current_addr + erase_end), erase_end - erase_start);

			if (!erase_partial_sector(current_addr, sector_size, erase_start,
						  erase_end)) {
				return false;
			}

			current_addr += sector_size;
		} else {
			/* Full sector erase is safe, erase the next consecutive full sectors */
			size_t contiguous_size =
				ROUND_DOWN(addr + size, sector_size) - current_addr;

			LOG_DBG("%s: sectors erased from: 0x%08lx length: 0x%x", __func__,
				(uintptr_t)current_addr, contiguous_size);

			if (esp_flash_erase_region(NULL, current_addr, contiguous_size) != ESP_OK) {
				LOG_ERR("%s: Flash erase failed at 0x%08lx", __func__,
					(uintptr_t)current_addr);
				return false;
			}

			current_addr += contiguous_size;
		}
	}

	return true;
}
#endif /* CONFIG_ESP_FLASH_ENCRYPTION */
#endif /* !CONFIG_MCUBOOT */

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

	if (length == 0U) {
		return 0;
	}

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
#endif /* CONFIG_MCUBOOT */

	if (ret != 0) {
		LOG_ERR("Flash read error: %d", ret);
		return -EIO;
	}

	return 0;
}

static int flash_esp32_write(const struct device *dev, off_t address, const void *buffer,
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

#ifdef CONFIG_ESP_FLASH_ENCRYPTION
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
#else
	ret = flash_esp32_write_check_enc(address, buffer, length);
#endif /* CONFIG_ESP_FLASH_ENCRYPTION */

	flash_esp32_sem_give(dev);
#endif /* CONFIG_MCUBOOT */

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

#ifdef CONFIG_ESP_FLASH_ENCRYPTION
	if (!aligned_flash_erase(start, len)) {
		ret = -EIO;
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
#else
	ret = esp_flash_erase_region(NULL, start, len);
#endif /* CONFIG_ESP_FLASH_ENCRYPTION */

	flash_esp32_sem_give(dev);
#endif /* CONFIG_MCUBOOT */

	if (ret != 0) {
		LOG_ERR("Flash erase error: %d", ret);
		return -EIO;
	}
	return 0;
}
#endif /* CONFIG_ESP_FLASH_HOST */

#ifdef CONFIG_ESP_FLASH_ASYNC
static IRAM_ATTR int flash_esp32_read_async(const struct device *dev, off_t address,
					    void *buffer, size_t length)
{
	struct flash_esp32_dev_data *data = dev->data;
	struct flash_req *req = &data->req;

	if (k_is_in_isr()) {
		return -EINVAL;
	}
	if (k_mutex_lock(&data->lock, K_TIMEOUT_ABS_SEC(CONFIG_ESP_FLASH_ASYNC_TIMEOUT))) {
		return -ETIMEDOUT;
	}
	req->op = FLASH_OP_READ;
	req->addr = address;
	req->len = length;
	req->buf = buffer;

	k_work_submit(&data->work);
	k_sem_take(&data->sync, FLASH_SEM_TIMEOUT);
	k_mutex_unlock(&data->lock);

	return req->result;
}

static IRAM_ATTR int flash_esp32_write_async(const struct device *dev, off_t address,
					     const void *buffer, size_t length)
{
	struct flash_esp32_dev_data *data = dev->data;
	struct flash_req *req = &data->req;

	if (k_is_in_isr()) {
		return -EINVAL;
	}
	if (k_mutex_lock(&data->lock, K_TIMEOUT_ABS_SEC(CONFIG_ESP_FLASH_ASYNC_TIMEOUT))) {
		return -ETIMEDOUT;
	}
	req->op = FLASH_OP_WRITE;
	req->addr = address;
	req->len = length;
	req->buf = (void *)buffer;

	k_work_submit(&data->work);
	k_sem_take(&data->sync, FLASH_SEM_TIMEOUT);
	k_mutex_unlock(&data->lock);

	return req->result;
}

static IRAM_ATTR int flash_esp32_erase_async(const struct device *dev, off_t start, size_t length)
{
	struct flash_esp32_dev_data *data = dev->data;
	struct flash_req *req = &data->req;

	if (k_is_in_isr()) {
		return -EINVAL;
	}
	if (k_mutex_lock(&data->lock, K_TIMEOUT_ABS_SEC(CONFIG_ESP_FLASH_ASYNC_TIMEOUT))) {
		return -ETIMEDOUT;
	}
	req->op = FLASH_OP_ERASE;
	req->addr = start;
	req->len = length;
	req->buf = NULL;

	k_work_submit(&data->work);
	k_sem_take(&data->sync, FLASH_SEM_TIMEOUT);
	k_mutex_unlock(&data->lock);

	return req->result;
}

#ifdef CONFIG_ESP_FLASH_HOST
static void flash_process_request(const struct device *dev, struct flash_req *req)
{
	switch (req->op) {
	case FLASH_OP_READ:
		req->result = flash_esp32_read(dev, req->addr, req->buf, req->len);
		break;
	case FLASH_OP_WRITE:
		req->result = flash_esp32_write(dev, req->addr, req->buf, req->len);
		break;
	case FLASH_OP_ERASE:
		req->result = flash_esp32_erase(dev, req->addr, req->len);
		break;
	default:
		req->result = -EINVAL;
		break;
	}
}
#endif

static IRAM_ATTR void flash_worker(struct k_work *work)
{
	struct flash_esp32_dev_data *data = CONTAINER_OF(work, struct flash_esp32_dev_data, work);

#ifdef CONFIG_ESP_FLASH_HOST
	if (data->req.op) {
		flash_process_request(data->dev, &data->req);
		data->req.op = FLASH_OP_NONE;
		k_sem_give(&data->sync);
	}
#else /* CONFIG_ESP_FLASH_REMOTE */
#ifdef CONFIG_ESP_FLASH_ASYNC_IPM
	/* remote cpu -> host cpu request */
	ipm_send(data->ipm, -1, CMD_REQUEST, &data->req, sizeof(struct flash_req));
#else
	ARG_UNUSED(data);
#endif
#endif
}

#ifdef CONFIG_ESP_FLASH_ASYNC_IPM
#ifdef CONFIG_ESP_FLASH_HOST
static IRAM_ATTR void flash_remote_worker(struct k_work *work)
{
	struct flash_esp32_dev_data *data = CONTAINER_OF(work, struct flash_esp32_dev_data,
							remote_work);
	if (data->remote_req.op) {
		/* TODO: without waiting here the subsequent IPM operations would fail */
		k_sleep(K_USEC(1));
		flash_process_request(data->dev, &data->remote_req);
		data->remote_req.op = FLASH_OP_NONE;
		/* host cpu -> remote cpu response */
		ipm_send(data->ipm, -1, CMD_RESPONSE, &data->remote_req, sizeof(struct flash_req));
	}
}
#endif

static void flash_cpu01_receive_cb(const struct device *ipm, void *user_data, uint32_t id,
				volatile void *shm)
{
	struct flash_esp32_dev_data *data = (struct flash_esp32_dev_data *) user_data;
	struct flash_req *req = (struct flash_req *) shm;

#ifdef CONFIG_ESP_FLASH_HOST
	if (id == CMD_REQUEST) {
		data->remote_req = *req;
		k_work_submit(&data->remote_work);
	}
#else
	if (id == CMD_RESPONSE) {
		data->req.result = req->result;
		k_sem_give(&data->sync);
	}
#endif
}
#endif
#endif /* CONFIG_ESP_FLASH_ASYNC */

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
#endif

static const struct flash_parameters *flash_esp32_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_esp32_parameters;
}

static int flash_esp32_init(const struct device *dev)
{
#ifdef CONFIG_MULTITHREADING
	struct flash_esp32_dev_data *const data = dev->data;

#ifdef CONFIG_ESP_FLASH_ASYNC
	k_mutex_init(&data->lock);
	k_sem_init(&data->sem, 0, 1);
	k_sem_init(&data->sync, 0, 1);
	k_work_init(&data->work, flash_worker);

#ifdef CONFIG_ESP_FLASH_ASYNC_WORK
	k_work_queue_init(&esp_flash_workqueue);
	k_work_queue_start(&esp_flash_workqueue, esp_flash_workqueue_stack,
			   K_THREAD_STACK_SIZEOF(esp_flash_workqueue_stack),
			   CONFIG_ESP_FLASH_ASYNC_WORK_PRIORITY, NULL);
	k_work_submit_to_queue(&esp_flash_workqueue, &data->work);
#endif

#ifdef CONFIG_ESP_FLASH_ASYNC_IPM
	data->ipm = DEVICE_DT_GET(DT_NODELABEL(ipm0));

	if (data->ipm) {
		ipm_register_callback(data->ipm, flash_cpu01_receive_cb, data);
	} else {
		LOG_ERR("Failed to get ipm0 device");
		return -ENODEV;
	}
#ifdef CONFIG_ESP_FLASH_HOST
	k_sem_init(&data->remote_sync, 0, 1);
	k_work_init(&data->remote_work, flash_remote_worker);
#endif
#endif

#else /* CONFIG_ESP_FLASH_ASYNC */

	k_sem_init(&data->sem, 1, 1);

#endif /* CONFIG_ESP_FLASH_ASYNC */
#endif /* CONFIG_MULTITHREADING */
	return 0;
}

static DEVICE_API(flash, flash_esp32_driver_api) = {
#ifdef CONFIG_ESP_FLASH_ASYNC
	.read = flash_esp32_read_async,
	.write = flash_esp32_write_async,
	.erase = flash_esp32_erase_async,
#else
	.read = flash_esp32_read,
	.write = flash_esp32_write,
	.erase = flash_esp32_erase,
#endif
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
