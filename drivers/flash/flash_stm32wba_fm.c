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


#if !CONFIG_FLASH_STM32WBA_BYTE_WRITE_EMULATION
#define BYTE_WRITE_EMULATE 0
#else
#define BYTE_WRITE_EMULATE 1
#endif


#ifdef BYTE_WRITE_EMULATE
#define BYTE_WRITE_EMULATE_MAX 1024
static uint8_t middle_workbuf[BYTE_WRITE_EMULATE_MAX] __aligned(4);
#endif


#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_stm32wba, CONFIG_FLASH_LOG_LEVEL);

#include "flash_stm32.h"
#include "flash_manager.h"
#include "flash_driver.h"

#include <stm32_ll_utils.h>

/* Let's wait for double the max erase time to be sure that the operation is
 * completed.
 */
#define STM32_FLASH_TIMEOUT	\
	(2 * DT_PROP(DT_INST(0, st_stm32_nv_flash), max_erase_time))

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

struct FM_CallbackNode cb_ptr = {
	.Callback = flash_callback
};

void FM_ProcessRequest(void)
{
	k_work_submit_to_queue(&ble_ctrl_work_q, &fm_work);
}

void FM_BackgroundProcess_Entry(struct k_work *work)
{
	ARG_UNUSED(work);

	FM_BackgroundProcess();
}

bool flash_stm32_valid_range(const struct device *dev, off_t offset,
				    uint32_t len, bool write)
{
	if (write && !flash_stm32_valid_write(offset, len)) {
		return false;
	}
	return flash_stm32_range_exists(dev, offset, len);
}

static int flash_stm32_read(const struct device *dev, off_t offset,
			    void *data,
			    size_t len)
{
	if (!flash_stm32_valid_range(dev, offset, len, false)) {
		LOG_ERR("Read range invalid. Offset: %p, len: %zu",
			(void *) offset, len);
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	flash_stm32_sem_take(dev);

	memcpy(data, (uint8_t *) FLASH_STM32_BASE_ADDRESS + offset, len);

	flash_stm32_sem_give(dev);

	return 0;
}

static int flash_stm32_erase(const struct device *dev, off_t offset,
			     size_t len)
{
	int rc;
	int sect_num;

	if (!flash_stm32_valid_range(dev, offset, len, true)) {
		LOG_ERR("Erase range invalid. Offset: %p, len: %zu",
			(void *)offset, len);
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	/* len is a multiple of FLASH_PAGE_SIZE */
	sect_num = len / FLASH_PAGE_SIZE;

	flash_stm32_sem_take(dev);

	LOG_DBG("Erase offset: %p, page: %ld, len: %zu, sect num: %d",
		(void *)offset, offset / FLASH_PAGE_SIZE, len, sect_num);

	rc = FM_Erase(offset / FLASH_PAGE_SIZE, sect_num, &cb_ptr);
	if (rc == 0) {
		k_sem_take(&flash_busy, K_FOREVER);
	} else {
		LOG_DBG("Erase operation rejected. err = %d", rc);
	}

	flash_stm32_sem_give(dev);

	return rc;
}

static uint8_t num_write_blocks(size_t write_len)
{
	const uint8_t mod_16 = write_len % 16U;
	uint8_t rc = 0;

	if (write_len == 0) {
		LOG_DBG("Write ready.");
		rc = 1;
	} else {
		if (mod_16 == 0) {
			rc = 2;
		} else if (mod_16 > 0) {
			if (write_len > 16) {
				rc = 3;
			} else {
				rc = 4;
			}
		}
	}

	return rc;
}

static int flash_stm32_write(const struct device *dev, off_t offset,
			     const void *data, size_t len)
{
	int rc;
	bool unaligned_flag = 0;

	if (!flash_stm32_valid_range(dev, offset, len, true)) {
#if !BYTE_WRITE_EMULATE
		LOG_ERR("Write range invalid. Offset: %p, len: %zu",
			(void *)offset, len);
		return -EINVAL;
#else
		unaligned_flag = 1;
#endif
	}

	if (!len) {
		return 0;
	}

	if (!unaligned_flag) {
		flash_stm32_sem_take(dev);

		LOG_DBG("Write offset: %p, len: %zu", (void *)offset, len);

		rc = FM_Write((uint32_t *)data,
			      (uint32_t *)(FLASH_STM32_BASE_ADDRESS + offset),
			      (int32_t)len / 4, &cb_ptr);
		if (rc == 0) {
			k_sem_take(&flash_busy, K_FOREVER);
		} else {
			LOG_DBG("Write operation rejected. err = %d", rc);
		}

		flash_stm32_sem_give(dev);
	}

	if (unaligned_flag == 1) {
		uint8_t start_buffer[16];
		uint8_t end_buffer[16];

		size_t current_write_length = 0;
		size_t middle_block_length = 0;
		size_t final_block_length = 0;

		off_t first_block_start_address = 0;
		off_t middle_block_start_address = 0;
		off_t final_block_start_address = 0;
		uint8_t offset_in_block = 0;

		size_t data_buffer_ctr = 0;
		uint8_t write_counter = 0;
		size_t final_buffer_ctr = 0;

		uint8_t remaining_bytes = 0;

		uint8_t *middle_block_data_buffer = NULL;

		int retval = 0;

		/* 1. Start address of first block to write */
		first_block_start_address = offset & ~0xF;

		/* 1.b. Calculate offset within the first block */
		offset_in_block = offset & 0xF;

		/* 1.c. Calculate present length. */
		current_write_length = len;

		/* 1.d. Calculate remaining bytes before hitting next block */
		remaining_bytes = 16 - offset_in_block;

		if (remaining_bytes > len) {
			remaining_bytes = len;
		}

		LOG_DBG("First block address : %p, rem bytes = %d and local offset: %d",
			(void *)first_block_start_address, remaining_bytes, offset_in_block);

		/* 2. Read the first block of memory. */
		retval = flash_stm32_read(dev, first_block_start_address,
					  (void *)start_buffer, 16);
		if (retval != 0) {
			LOG_ERR("Failed to read target region into SRAM - Write will not continue.");
			return -EINVAL;
		}

		/* 3. Modify buffer with user data. */
		for (uint8_t i = offset_in_block;
		     i < (remaining_bytes + offset_in_block); i++) {
			/* Fill start buffer and maintain metadata */
			start_buffer[i] = ((const uint8_t *)data)[data_buffer_ctr];
			data_buffer_ctr++;
			current_write_length--;
		}

		/* Use local helper function to decide which write logic to use. */
		write_counter = num_write_blocks(current_write_length);

		switch (write_counter) {
		case 1:
		{
			LOG_INF("Entered the block A only section.");
			middle_block_length = 0;
			final_block_length = 0;
			goto write_op;
		}
		break;

		case 2:
		{
			LOG_INF("Entered the block A and B section.");
			middle_block_start_address = first_block_start_address + 16;
			middle_block_length = current_write_length;
			final_block_length = 0;
		}
		break;

		case 3:
		{
			LOG_INF("Entered the block for A, B and C");
			middle_block_start_address = first_block_start_address + 16;
			middle_block_length = (current_write_length & ~0xF);

			final_block_length = current_write_length - middle_block_length;
			final_block_start_address =
				middle_block_start_address + middle_block_length;

			/* Read the final 16 bytes to write into SRAM. */
			retval = flash_stm32_read(dev, final_block_start_address,
						  (void *)end_buffer, 16);
			if (retval != 0) {
				LOG_ERR("Failed to read target region into SRAM - Write will not continue.");
				return -EINVAL;
			}
		}
		break;

		case 4:
		{
			LOG_INF("Entered the block A and C section");
			middle_block_length = 0;

			final_block_length = current_write_length;
			final_block_start_address = first_block_start_address + 16;

			retval = flash_stm32_read(dev, final_block_start_address,
						  (void *)end_buffer, 16);
			if (retval != 0) {
				LOG_ERR("Failed to read target region into SRAM - Write will not continue.");
				return -EINVAL;
			}
		}
		break;

		default:
			LOG_DBG("Invalid length input. Cannot peform write.");
			return -EINVAL;
		break;

			LOG_INF("First block: %d, Middle block: %d, FInal Block: %d",
				data_buffer_ctr, middle_block_length, final_block_length);
		}

		/* 6. Driver is limiting unaligned writes to 1KB for safety. */
		if (middle_block_length > 0) {
			if (middle_block_length > BYTE_WRITE_EMULATE_MAX) {
				LOG_ERR("Write size too large :%zu > %d."
					"Use 16-byte aligned writes for large transfers.",
					middle_block_length, BYTE_WRITE_EMULATE_MAX);

				/* flash_stm32_sem_give(dev); */
				return -EINVAL;
			}

			middle_block_data_buffer =
				(uint8_t *)data + data_buffer_ctr;

			/* In case source buffer is not 16-byte aligned, move it to a different buffer. */
			if (((uintptr_t)middle_block_data_buffer & 0x3) != 0) {
				memcpy(middle_workbuf, middle_block_data_buffer,
				       middle_block_length);
				middle_block_data_buffer = middle_workbuf;
			}
		}

		/* 5. Modify the final write buffer if used. */
		if (final_block_length > 0) {
			final_buffer_ctr = data_buffer_ctr + middle_block_length;

			for (uint8_t i = 0; i < final_block_length; i++) {
				end_buffer[i] =
					((const uint8_t *)data)[final_buffer_ctr];
				final_buffer_ctr++;
			}
		}

write_op:
		flash_stm32_sem_take(dev);

		/* 7. Perform the actual write operation(s). */
		rc = FM_Write((uint32_t *)&start_buffer[0],
			      (uint32_t *)(FLASH_STM32_BASE_ADDRESS +
					   first_block_start_address),
			      (int32_t)4, &cb_ptr);
		if (rc == 0) {
			k_sem_take(&flash_busy, K_FOREVER);
		} else {
			LOG_ERR("Write operation rejected, first time. err = %d",
				rc);
			flash_stm32_sem_give(dev);
			return -EINVAL;
		}

		if (middle_block_length > 0) {
			LOG_INF("Writing middle_block_len: %d, address: %p",
				middle_block_length,
				(void *)middle_block_start_address);
			rc = FM_Write((uint32_t *)middle_block_data_buffer,
				      (uint32_t *)(FLASH_STM32_BASE_ADDRESS +
						   middle_block_start_address),
				      (int32_t)(middle_block_length / 4), &cb_ptr);
			if (rc == 0) {
				k_sem_take(&flash_busy, K_FOREVER);
			} else {
				LOG_ERR("Write operation rejected, second time. err = %d",
					rc);
				flash_stm32_sem_give(dev);
				return -EINVAL;
			}
		}

		if (final_block_length > 0) {
			rc = FM_Write((uint32_t *)&end_buffer[0],
				      (uint32_t *)(FLASH_STM32_BASE_ADDRESS +
						   final_block_start_address),
				      (int32_t)4, &cb_ptr);
			if (rc == 0) {
				k_sem_take(&flash_busy, K_FOREVER);
			} else {
				LOG_ERR("Write operation rejected, third time. err = %d",
					rc);
				flash_stm32_sem_give(dev);
				return -EINVAL;
			}
		}

		flash_stm32_sem_give(dev);
	}

	return rc;
}


static const struct flash_parameters *
			flash_stm32_get_parameters(const struct device *dev)
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
	.regs = (FLASH_TypeDef *) DT_INST_REG_ADDR(0),
};

void flash_stm32wba_page_layout(const struct device *dev,
				const struct flash_pages_layout **layout,
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
	.write = flash_stm32_write,
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

	LOG_DBG("Flash initialized. BS: %zu",
		flash_stm32_parameters.write_block_size);

	k_work_init(&fm_work, &FM_BackgroundProcess_Entry);

	/* Enable flash driver system flag */
	FD_SetStatus(FD_FLASHACCESS_RFTS, LL_FLASH_DISABLE);
	FD_SetStatus(FD_FLASHACCESS_RFTS_BYPASS, LL_FLASH_ENABLE);
	FD_SetStatus(FD_FLASHACCESS_SYSTEM, LL_FLASH_ENABLE);

#if ((CONFIG_FLASH_LOG_LEVEL >= LOG_LEVEL_DBG) && CONFIG_FLASH_PAGE_LAYOUT)
	const struct flash_pages_layout *layout;
	size_t layout_size;

	flash_stm32wba_page_layout(dev, &layout, &layout_size);
	for (size_t i = 0; i < layout_size; i++) {
		LOG_DBG("Block %zu: bs: %zu count: %zu", i,
			layout[i].pages_size, layout[i].pages_count);
	}
#endif

	return 0;
}

DEVICE_DT_INST_DEFINE(0, stm32_flash_init, NULL,
		    &flash_data, NULL, POST_KERNEL,
		    CONFIG_FLASH_INIT_PRIORITY, &flash_stm32_api);
