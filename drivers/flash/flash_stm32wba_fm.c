/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/flash.h>

#define DT_DRV_COMPAT st_stm32wba_flash_controller

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_stm32wba, CONFIG_FLASH_LOG_LEVEL);

#include "flash_stm32.h"
#include "flash_manager.h"
#include "flash_driver.h"

#include <stm32_ll_utils.h>

/* Let's wait for double the max erase time to be sure that the operation is
 * completed.
 */
#define STM32_FLASH_TIMEOUT (2 * DT_PROP(DT_INST(0, st_stm32_nv_flash), max_erase_time))

extern struct k_work_q ble_ctrl_work_q;
struct k_work fm_work;

static const struct flash_parameters flash_stm32_parameters = {
	.write_block_size = FLASH_STM32_WRITE_BLOCK_SIZE,
	.erase_value = 0xff,
};

K_SEM_DEFINE(flash_busy, 0, 1);

static void flash_callback(FM_FlashOp_Status_t status)
{
	LOG_DBG("%d", status);

	k_sem_give(&flash_busy);
}

struct FM_CallbackNode cb_ptr = {.Callback = flash_callback};

void FM_ProcessRequest(void)
{
	k_work_submit_to_queue(&ble_ctrl_work_q, &fm_work);
}

void FM_BackgroundProcess_Entry(struct k_work *work)
{
	ARG_UNUSED(work);

	FM_BackgroundProcess();
}

bool flash_stm32_valid_range(const struct device *dev, off_t offset, uint32_t len, bool write)
{
	if (write && !flash_stm32_valid_write(offset, len)) {
		return false;
	}
	return flash_stm32_range_exists(dev, offset, len);
}

static int flash_stm32_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	if (!flash_stm32_valid_range(dev, offset, len, false)) {
		LOG_ERR("Read range invalid. Offset: %p, len: %zu", (void *)offset, len);
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	flash_stm32_sem_take(dev);

	memcpy(data, (uint8_t *)FLASH_STM32_BASE_ADDRESS + offset, len);

	flash_stm32_sem_give(dev);

	return 0;
}

static int flash_stm32_erase(const struct device *dev, off_t offset, size_t len)
{
	int rc;
	int sect_num;

	if (!flash_stm32_valid_range(dev, offset, len, true)) {
		LOG_ERR("Erase range invalid. Offset: %p, len: %zu", (void *)offset, len);
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	/* len is a multiple of FLASH_PAGE_SIZE */
	sect_num = len / FLASH_PAGE_SIZE;

	flash_stm32_sem_take(dev);

	LOG_DBG("Erase offset: %p, page: %ld, len: %zu, sect num: %d", (void *)offset,
		offset / FLASH_PAGE_SIZE, len, sect_num);

	rc = FM_Erase(offset / FLASH_PAGE_SIZE, sect_num, &cb_ptr);
	if (rc == 0) {
		k_sem_take(&flash_busy, K_FOREVER);
	} else {
		LOG_DBG("Erase operation rejected. err = %d", rc);
	}

	flash_stm32_sem_give(dev);

	return rc;
}

static int flash_stm32_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	int rc;

	if (!flash_stm32_valid_range(dev, offset, len, true)) {
		LOG_ERR("Write range invalid. Offset: %p, len: %zu", (void *)offset, len);
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	flash_stm32_sem_take(dev);

	LOG_DBG("Write offset: %p, len: %zu", (void *)offset, len);

	rc = FM_Write((uint32_t *)data, (uint32_t *)(FLASH_STM32_BASE_ADDRESS + offset),
		      (int32_t)len / 4, &cb_ptr);
	if (rc == 0) {
		k_sem_take(&flash_busy, K_FOREVER);
	} else {
		LOG_DBG("Write operation rejected. err = %d", rc);
	}

	flash_stm32_sem_give(dev);

	return rc;
}
#ifdef CONFIG_FLASH_STM32_ACCEPT_UNALIGNED_WRITES
static int write_unaligned(const struct device *dev, off_t offset, const uint8_t *data, size_t len)
{
	uint8_t temp_data[FLASH_STM32_WRITE_BLOCK_SIZE];
	off_t cur_offset_addr;
	size_t local_offset;

	BUILD_ASSERT(FLASH_STM32_WRITE_BLOCK_SIZE <= 128, "Write block size is above the limit.");
	__ASSERT(len <= FLASH_STM32_WRITE_BLOCK_SIZE, "Length too big.");

	memset(temp_data, flash_stm32_parameters.erase_value, FLASH_STM32_WRITE_BLOCK_SIZE);

	cur_offset_addr = ROUND_DOWN(offset, FLASH_STM32_WRITE_BLOCK_SIZE);
	local_offset = offset - cur_offset_addr;

	__ASSERT(len == (FLASH_STM32_WRITE_BLOCK_SIZE - local_offset), "Length error.");

	memcpy(temp_data + local_offset, data, len);

	return flash_stm32_write(dev, cur_offset_addr, temp_data, FLASH_STM32_WRITE_BLOCK_SIZE);
}

static int flash_stm32_no_align_write(const struct device *dev, off_t offset, const void *data,
				      size_t len)
{
	const uint8_t *data_ptr = data;
	size_t chunk_size, local_offset;
	off_t off = offset;
	size_t remaining = len;
	int rc;

	BUILD_ASSERT(FLASH_STM32_WRITE_BLOCK_SIZE >= 4, "Invalid write size.");

	if (!remaining) {
		return 0;
	}

	if (!flash_stm32_range_exists(dev, off, remaining)) {
		return -EINVAL;
	}

	if (off % FLASH_STM32_WRITE_BLOCK_SIZE != 0) {
		local_offset = off % FLASH_STM32_WRITE_BLOCK_SIZE;
		chunk_size = MIN(remaining, FLASH_STM32_WRITE_BLOCK_SIZE - local_offset);

		rc = write_unaligned(dev, off, data, chunk_size);
		if (rc != 0) {
			return rc;
		}

		remaining -= chunk_size;
		off += chunk_size;
		data_ptr += chunk_size;
	}

	chunk_size = ROUND_DOWN(remaining, FLASH_STM32_WRITE_BLOCK_SIZE);

	if (chunk_size != 0) {
		if (IS_ALIGNED(data_ptr, 4)) {
			rc = flash_stm32_write(dev, off, data_ptr, chunk_size);
			if (rc != 0) {
				return rc;
			}
			off += chunk_size;
			data_ptr += chunk_size;
			remaining -= chunk_size;
		} else {
			/* Workaround quirk in FM-based flash driver which requires word-aligned
			 * input buffer. This should not be necessary once this bug is fixed in
			 * the flash driver.
			 */
			size_t temp_chunk_size = chunk_size;

			while (temp_chunk_size > 0) {
				size_t step = MIN(temp_chunk_size, FLASH_STM32_WRITE_BLOCK_SIZE);

				rc = write_unaligned(dev, off, data_ptr, step);
				if (rc != 0) {
					return rc;
				}

				data_ptr += step;
				off += step;
				temp_chunk_size -= step;
			}
			remaining -= chunk_size;
		}
	}

	/* Write tail for any leftover bytes. */
	if (remaining > 0) {
		rc = write_unaligned(dev, off, data_ptr, remaining);
		if (rc != 0) {
			return rc;
		}
	}
	return 0;
}
#endif /* CONFIG_FLASH_STM32_ACCEPT_UNALIGNED_WRITES */

static const struct flash_parameters *flash_stm32_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_stm32_parameters;
}

/* Gives the total logical device size in bytes and return 0. */
static int flash_stm32wba_get_size(const struct device *dev, uint64_t *size)
{
	ARG_UNUSED(dev);

	*size = (uint64_t)LL_GetFlashSize() * 1024U;

	return 0;
}

static struct flash_stm32_priv flash_data = {
	.regs = (FLASH_TypeDef *)DT_INST_REG_ADDR(0),
};

void flash_stm32wba_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
				size_t *layout_size)
{
	static struct flash_pages_layout stm32wba_flash_layout = {
		.pages_count = 0,
		.pages_size = 0,
	};

	ARG_UNUSED(dev);

	if (stm32wba_flash_layout.pages_count == 0) {
		stm32wba_flash_layout.pages_count = FLASH_SIZE / FLASH_PAGE_SIZE;
		stm32wba_flash_layout.pages_size = FLASH_PAGE_SIZE;
	}

	*layout = &stm32wba_flash_layout;
	*layout_size = 1;
}

static DEVICE_API(flash, flash_stm32_api) = {
	.erase = flash_stm32_erase,
#ifndef CONFIG_FLASH_STM32_ACCEPT_UNALIGNED_WRITES
	.write = flash_stm32_write,
#else
	.write = flash_stm32_no_align_write,
#endif
	.read = flash_stm32_read,
	.get_parameters = flash_stm32_get_parameters,
	.get_size = flash_stm32wba_get_size,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_stm32wba_page_layout,
#endif
};

static int stm32_flash_init(const struct device *dev)
{
	k_sem_init(&FLASH_STM32_PRIV(dev)->sem, 1, 1);

	LOG_DBG("Flash initialized. BS: %zu", flash_stm32_parameters.write_block_size);

	k_work_init(&fm_work, &FM_BackgroundProcess_Entry);

	/* Init the Flash Manager module */
	FM_Init();

	/* Enable flash driver system flag */
	FD_SetStatus(FD_FLASHACCESS_RFTS, LL_FLASH_DISABLE);
	FD_SetStatus(FD_FLASHACCESS_RFTS_BYPASS, LL_FLASH_ENABLE);
	FD_SetStatus(FD_FLASHACCESS_SYSTEM, LL_FLASH_ENABLE);

#if ((CONFIG_FLASH_LOG_LEVEL >= LOG_LEVEL_DBG) && CONFIG_FLASH_PAGE_LAYOUT)
	const struct flash_pages_layout *layout;
	size_t layout_size;

	flash_stm32wba_page_layout(dev, &layout, &layout_size);
	for (size_t i = 0; i < layout_size; i++) {
		LOG_DBG("Block %zu: bs: %zu count: %zu", i, layout[i].pages_size,
			layout[i].pages_count);
	}
#endif

	return 0;
}

DEVICE_DT_INST_DEFINE(0, stm32_flash_init, NULL, &flash_data, NULL, POST_KERNEL,
		      CONFIG_FLASH_INIT_PRIORITY, &flash_stm32_api);
