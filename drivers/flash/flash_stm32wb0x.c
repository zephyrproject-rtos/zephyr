/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32wb0_flash_controller

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/math_extras.h>

/* <soc.h> also brings "stm32wb0x_hal_flash.h"
 * and "system_stm32wb0x.h", which provide macros
 * used by the driver, such as FLASH_PAGE_SIZE or
 * _MEMORY_FLASH_SIZE_ respectively.
 */
#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_system.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_stm32wb0x, CONFIG_FLASH_LOG_LEVEL);

/**
 * Driver private definitions & assertions
 */
#define SYSTEM_FLASH_SIZE	_MEMORY_FLASH_SIZE_
#define PAGES_IN_FLASH		(SYSTEM_FLASH_SIZE / FLASH_PAGE_SIZE)

#define WRITE_BLOCK_SIZE \
	DT_PROP(DT_INST(0, soc_nv_flash), write_block_size)

/* Size of flash words, in bytes (equal to write block size) */
#define WORD_SIZE	WRITE_BLOCK_SIZE

#define ERASE_BLOCK_SIZE \
	DT_PROP(DT_INST(0, soc_nv_flash), erase_block_size)

/**
 * Driver private structures
 */
struct flash_wb0x_data {
	/** Used to serialize write/erase operations */
	struct k_sem write_lock;

	/** Flash size, in bytes */
	size_t flash_size;
};

/**
 * Driver private utility functions
 */
static inline uint32_t read_mem_u32(const uint32_t *ptr)
{
	/**
	 * Fetch word using sys_get_le32, which performs byte-sized
	 * reads instead of word-sized. This is important as ptr may
	 * be unaligned. We also want to use le32 because the data is
	 * stored in little-endian inside the flash.
	 */
	return sys_get_le32((const uint8_t *)ptr);
}

static inline size_t get_flash_size_in_bytes(void)
{
	/* FLASH.SIZE contains the highest flash address supported
	 * on this MCU, which is also the number of words in flash
	 * minus one.
	 */
	const uint32_t words_in_flash =
		READ_BIT(FLASH->SIZE, FLASH_FLASH_SIZE_FLASH_SIZE) + 1;

	return words_in_flash * WORD_SIZE;
}

/**
 * @brief Returns the associated error to IRQ flags.
 *
 * @returns a negative error value
 */
static int error_from_irq_flags(uint32_t flags)
{
	/**
	 * Only two errors are expected:
	 *  - illegal command
	 *  - command error
	 */
	if (flags & FLASH_FLAG_ILLCMD) {
		return -EINVAL;
	}

	if (flags & FLASH_FLAG_CMDERR) {
		return -EIO;
	}

	/*
	 * Unexpected error flag -> "out of domain"
	 * In practice, this should never be reached.
	 */
	return -EDOM;
}

static bool is_valid_flash_range(const struct device *dev,
				off_t offset, uint32_t len)
{
	const struct flash_wb0x_data *data = dev->data;
	uint32_t offset_plus_len;

		/* (offset + len) must not overflow */
	return !u32_add_overflow(offset, len, &offset_plus_len)
		/* offset must be a valid offset in flash */
		&& IN_RANGE(offset, 0, data->flash_size - 1)
		/* (offset + len) must be in [0; flash size]
		 * because it is equal to the last accessed
		 * byte in flash plus one (an access of `len`
		 * bytes starting at `offset` touches bytes
		 * `offset` to `offset + len` EXCLUDED)
		 */
		&& IN_RANGE(offset_plus_len, 0, data->flash_size);
}

static bool is_writeable_flash_range(const struct device *dev,
				off_t offset, uint32_t len)
{
	if ((offset % WRITE_BLOCK_SIZE) != 0
		|| (len % WRITE_BLOCK_SIZE) != 0) {
		return false;
	}

	return is_valid_flash_range(dev, offset, len);
}

static bool is_erasable_flash_range(const struct device *dev,
				off_t offset, uint32_t len)
{
	if ((offset % ERASE_BLOCK_SIZE) != 0
		|| (len % ERASE_BLOCK_SIZE) != 0) {
		return false;
	}

	return is_valid_flash_range(dev, offset, len);
}

/**
 * Driver private functions
 */

static uint32_t poll_flash_controller(void)
{
	uint32_t flags;

	/* Poll until an interrupt flag is raised */
	do {
		flags = FLASH->IRQRAW;
	} while (flags == 0);

	/* Acknowledge the flag(s) we have seen */
	FLASH->IRQRAW = flags;

	return flags;
}

static int execute_flash_command(uint8_t cmd)
{
	uint32_t irq_flags;

	/* Clear all pending interrupt bits */
	FLASH->IRQRAW = FLASH->IRQRAW;

	/* Start command */
	FLASH->COMMAND = cmd;

	/* Wait for CMDSTART */
	irq_flags = poll_flash_controller();

	/* If command didn't start, an error occurred */
	if (!(irq_flags & FLASH_IT_CMDSTART)) {
		return error_from_irq_flags(irq_flags);
	}

	/**
	 * Both CMDSTART and CMDDONE may be set if the command was
	 * executed fast enough. In this case, we're already done.
	 * Otherwise, we need to poll again until CMDDONE/error occurs.
	 */
	if (!(irq_flags & FLASH_IT_CMDDONE)) {
		irq_flags = poll_flash_controller();
	}

	if (!(irq_flags & FLASH_IT_CMDDONE)) {
		return error_from_irq_flags(irq_flags);
	} else {
		return 0;
	}
}

int erase_page_range(uint32_t start_page, uint32_t page_count)
{
	int res = 0;

	__ASSERT_NO_MSG(start_page < PAGES_IN_FLASH);
	__ASSERT_NO_MSG((start_page + page_count - 1) < PAGES_IN_FLASH);

	for (uint32_t i = start_page;
		i < (start_page + page_count);
		i++) {
		/* ADDRESS[16:9] = XADR[10:3] (address of page to erase)
		 * ADDRESS[8:0]  = 0 (row & word address, must be 0)
		 */
		FLASH->ADDRESS = (i << 9);

		res = execute_flash_command(FLASH_CMD_ERASE_PAGES);
		if (res < 0) {
			break;
		}
	}

	return res;
}

int write_word_range(const void *buf, uint32_t start_word, uint32_t num_words)
{
	/* Special value to load in DATAx registers to skip
	 * writing corresponding word with BURSTWRITE command.
	 */
	const uint32_t BURST_IGNORE_VALUE = 0xFFFFFFFF;
	const size_t WORDS_IN_BURST = 4;
	uint32_t dst_addr = start_word;
	uint32_t remaining = num_words;
	/**
	 * Note that @p buf may not be aligned to 32-bit boundary.
	 * However, declaring src_ptr as uint32_t* makes the address
	 * increment by 4 every time we do src_ptr++, which makes it
	 * behave like the other counters in this function.
	 */
	const uint32_t *src_ptr = buf;
	int res = 0;

	/**
	 * Write to flash is performed as a 3 step process:
	 *  - write single words using WRITE commands until the write
	 *    write address is aligned to flash quadword boundary
	 *
	 *  - after write address is aligned to quadword, we can use
	 *    the BURSTWRITE commands to write 4 words at a time
	 *
	 *  - once less than 4 words remain to write, a last BURSTWRITE
	 *    is used with unneeded DATAx registers filled with 0xFFFFFFFF
	 *    (this makes BURSTWRITE ignore write to these addresses)
	 */

	/* (1) Align to quadword boundary with WRITE commands */
	while (remaining > 0 && (dst_addr % WORDS_IN_BURST) != 0) {
		FLASH->ADDRESS = dst_addr;
		FLASH->DATA0 = read_mem_u32(src_ptr);

		res = execute_flash_command(FLASH_CMD_WRITE);
		if (res < 0) {
			return res;
		}

		src_ptr++;
		dst_addr++;
		remaining--;
	}

	/* (2) Write bursts of quadwords */
	while (remaining >= WORDS_IN_BURST) {
		__ASSERT_NO_MSG((dst_addr % WORDS_IN_BURST) == 0);

		FLASH->ADDRESS = dst_addr;
		FLASH->DATA0 = read_mem_u32(src_ptr + 0);
		FLASH->DATA1 = read_mem_u32(src_ptr + 1);
		FLASH->DATA2 = read_mem_u32(src_ptr + 2);
		FLASH->DATA3 = read_mem_u32(src_ptr + 3);

		res = execute_flash_command(FLASH_CMD_BURSTWRITE);
		if (res < 0) {
			return res;
		}

		src_ptr += WORDS_IN_BURST;
		dst_addr += WORDS_IN_BURST;
		remaining -= WORDS_IN_BURST;
	}

	/* (3) Write trailing (between 1 and 3 words) */
	if (remaining > 0) {
		__ASSERT_NO_MSG(remaining < WORDS_IN_BURST);
		__ASSERT_NO_MSG((dst_addr % WORDS_IN_BURST) == 0);

		FLASH->ADDRESS = dst_addr;
		FLASH->DATA0 = read_mem_u32(src_ptr + 0);

		FLASH->DATA1 = (remaining >= 2)
			? read_mem_u32(src_ptr + 1)
			: BURST_IGNORE_VALUE;

		FLASH->DATA2 = (remaining == 3)
			? read_mem_u32(src_ptr + 2)
			: BURST_IGNORE_VALUE;

		FLASH->DATA3 = BURST_IGNORE_VALUE;

		remaining = 0;

		res = execute_flash_command(FLASH_CMD_BURSTWRITE);
	}

	return res;
}

/**
 * Driver subsystem API implementation
 */
int flash_wb0x_read(const struct device *dev, off_t offset,
			void *buffer, size_t len)
{
	if (!len) {
		return 0;
	}

	if (!is_valid_flash_range(dev, offset, len)) {
		return -EINVAL;
	}

	const uint8_t *flash_base = ((void *)DT_REG_ADDR(DT_INST(0, st_stm32_nv_flash)));

	memcpy(buffer, flash_base + (uint32_t)offset, len);

	return 0;
}

int flash_wb0x_write(const struct device *dev, off_t offset,
			const void *buffer, size_t len)
{
	struct flash_wb0x_data *data = dev->data;
	int res;

	if (!len) {
		return 0;
	}

	if (!is_writeable_flash_range(dev, offset, len)) {
		return -EINVAL;
	}

	/* Acquire driver lock */
	res = k_sem_take(&data->write_lock, K_NO_WAIT);
	if (res < 0) {
		return res;
	}

	const uint32_t start_word = (uint32_t)offset / WORD_SIZE;
	const uint32_t num_words = len / WORD_SIZE;

	res = write_word_range(buffer, start_word, num_words);

	/* Release driver lock */
	k_sem_give(&data->write_lock);

	return res;
}

int flash_wb0x_erase(const struct device *dev, off_t offset, size_t size)
{
	struct flash_wb0x_data *data = dev->data;
	int res;

	if (!size) {
		return 0;
	}

	if (!is_erasable_flash_range(dev, offset, size)) {
		return -EINVAL;
	}

	/* Acquire driver lock */
	res = k_sem_take(&data->write_lock, K_NO_WAIT);
	if (res < 0) {
		return res;
	}

	const uint32_t start_page = (uint32_t)offset / ERASE_BLOCK_SIZE;
	const uint32_t page_count = size / ERASE_BLOCK_SIZE;

	res = erase_page_range(start_page, page_count);

	/* Release driver lock */
	k_sem_give(&data->write_lock);

	return res;
}

const struct flash_parameters *flash_wb0x_get_parameters(
					const struct device *dev)
{
	static const struct flash_parameters fp = {
		.write_block_size = WRITE_BLOCK_SIZE,
		.erase_value = 0xff,
	};

	return &fp;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
void flash_wb0x_pages_layout(const struct device *dev,
				const struct flash_pages_layout **layout,
				size_t *layout_size)
{
	/**
	 * STM32WB0 flash: single bank, 2KiB pages
	 * (the number of pages depends on MCU)
	 */
	static const struct flash_pages_layout fpl[] = {{
		.pages_count = PAGES_IN_FLASH,
		.pages_size = FLASH_PAGE_SIZE
	}};

	*layout = fpl;
	*layout_size = ARRAY_SIZE(fpl);
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_driver_api flash_wb0x_api = {
	.erase = flash_wb0x_erase,
	.write = flash_wb0x_write,
	.read = flash_wb0x_read,
	.get_parameters = flash_wb0x_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_wb0x_pages_layout,
#endif
	/* extended operations not supported */
};

int stm32wb0x_flash_init(const struct device *dev)
{
	struct flash_wb0x_data *data = dev->data;

	k_sem_init(&data->write_lock, 1, 1);

	data->flash_size = get_flash_size_in_bytes();

	return 0;
}

/**
 * Driver device instantiation
 */
static struct flash_wb0x_data wb0x_flash_drv_data;

DEVICE_DT_INST_DEFINE(0, stm32wb0x_flash_init, NULL,
		    &wb0x_flash_drv_data, NULL, POST_KERNEL,
		    CONFIG_FLASH_INIT_PRIORITY, &flash_wb0x_api);
