/*
 * Copyright (c) 2023 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_flash_controller

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <zephyr/cache.h>
#include <errno.h>

#include <soc.h>

LOG_MODULE_REGISTER(flash_ambiq, CONFIG_FLASH_LOG_LEVEL);

#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)
#define SOC_NV_FLASH_ADDR DT_REG_ADDR(SOC_NV_FLASH_NODE)
#define SOC_NV_FLASH_SIZE DT_REG_SIZE(SOC_NV_FLASH_NODE)

/* Apollo2x and Apollo3x both use traditional NOR flash (am_hal_flash_* HAL, 4-byte writes) */
#if defined(CONFIG_SOC_SERIES_APOLLO2X) || defined(CONFIG_SOC_SERIES_APOLLO3X)
#define AMBIQ_NOR_FLASH 1
#define MIN_WRITE_SIZE 4
#else
#define MIN_WRITE_SIZE 16
#endif
#ifdef CONFIG_DCACHE
#define FLASH_WRITE_BLOCK_SIZE                                                                     \
	MAX(CONFIG_DCACHE_LINE_SIZE, DT_PROP(SOC_NV_FLASH_NODE, write_block_size))
#else
#define FLASH_WRITE_BLOCK_SIZE MAX(DT_PROP(SOC_NV_FLASH_NODE, write_block_size), MIN_WRITE_SIZE)
#endif
#define FLASH_ERASE_BLOCK_SIZE DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)

BUILD_ASSERT((FLASH_WRITE_BLOCK_SIZE & (MIN_WRITE_SIZE - 1)) == 0,
	     "The flash write block size must be a multiple of MIN_WRITE_SIZE!");

#define FLASH_ERASE_BYTE 0xFF
#define FLASH_ERASE_WORD                                                                           \
	(((uint32_t)(FLASH_ERASE_BYTE << 24)) | ((uint32_t)(FLASH_ERASE_BYTE << 16)) |             \
	 ((uint32_t)(FLASH_ERASE_BYTE << 8)) | ((uint32_t)FLASH_ERASE_BYTE))

/*
 * Write optimization: write in chunks to allow ISRs to run between chunks.
 * Target 128 bytes but ensure it's always a multiple of FLASH_WRITE_BLOCK_SIZE.
 * This calculates the smallest multiple of FLASH_WRITE_BLOCK_SIZE that is >= 128 bytes.
 */
#define FLASH_WRITE_CHUNK_TARGET 128
#define FLASH_WRITE_CHUNK_SIZE                                                                     \
	(FLASH_WRITE_BLOCK_SIZE * DIV_ROUND_UP(FLASH_WRITE_CHUNK_TARGET, FLASH_WRITE_BLOCK_SIZE))

BUILD_ASSERT(FLASH_WRITE_CHUNK_SIZE >= FLASH_WRITE_BLOCK_SIZE,
	     "Chunk size must be at least one block size!");
BUILD_ASSERT((FLASH_WRITE_CHUNK_SIZE % FLASH_WRITE_BLOCK_SIZE) == 0,
	     "Chunk size must be a multiple of block size!");

/* Retry logic for erase/write operations to handle voltage drops */
#define FLASH_OPERATION_MAX_RETRIES 3

struct flash_ambiq_data {
#if defined(CONFIG_MULTITHREADING)
	struct k_mutex lock;
#endif
};

#if defined(CONFIG_MULTITHREADING)
static inline int flash_ambiq_lock_take(struct flash_ambiq_data *data)
{
	return k_mutex_lock(&data->lock, K_FOREVER);
}

#define FLASH_LOCK_INIT(data) k_mutex_init(&(data)->lock)
#define FLASH_LOCK_TAKE(data) flash_ambiq_lock_take(data)
#define FLASH_UNLOCK(data)    k_mutex_unlock(&(data)->lock)
#else
#define FLASH_LOCK_INIT(data)
#define FLASH_LOCK_TAKE(data) 0
#define FLASH_UNLOCK(data)
#endif /* CONFIG_MULTITHREADING */

static const struct flash_parameters flash_ambiq_parameters = {
	.write_block_size = FLASH_WRITE_BLOCK_SIZE,
	.erase_value = FLASH_ERASE_BYTE,
#if !defined(AMBIQ_NOR_FLASH)
	.caps = {
		.no_explicit_erase = true,
	},
#endif
};

/* Map Ambiq HAL status codes to negative errno codes.
 *
 * Apollo2x HAL predates the unified AM_HAL_STATUS_* enum: its am_hal_flash_*
 * functions return int with 0 for success and non-zero for failure.
 * Apollo3x/4p HAL functions return int (can be -1 for alignment errors).
 * Apollo5x HAL functions return uint32_t (AM_HAL_MRAM_* codes >= 0x08000100).
 * AM_HAL_STATUS_* enum values 0-9 are shared across Apollo3x/4x/5x.
 */
static int flash_ambiq_hal_status_to_errno(int hal_status)
{
	if (hal_status < 0) {
		LOG_ERR("HAL returned error: %d", hal_status);
		return -EIO;
	}

#if defined(CONFIG_SOC_SERIES_APOLLO2X)
	/* Apollo2x has no AM_HAL_STATUS_* codes; non-zero simply means failure. */
	if (hal_status != 0) {
		LOG_ERR("Ambiq HAL error code: %d", hal_status);
		return -EIO;
	}

	return 0;
#else
	switch ((uint32_t)hal_status) {
	case AM_HAL_STATUS_SUCCESS:
		return 0;
	case AM_HAL_STATUS_FAIL:
		return -EIO;
	case AM_HAL_STATUS_INVALID_HANDLE:
		return -EINVAL;
	case AM_HAL_STATUS_IN_USE:
		return -EBUSY;
	case AM_HAL_STATUS_TIMEOUT:
		return -ETIMEDOUT;
	case AM_HAL_STATUS_OUT_OF_RANGE:
		return -ERANGE;
	case AM_HAL_STATUS_INVALID_ARG:
		return -EINVAL;
	case AM_HAL_STATUS_INVALID_OPERATION:
		return -EPERM;
	default:
		LOG_ERR("Unknown Ambiq HAL error code: 0x%x", (uint32_t)hal_status);
		return -EIO;
	}
#endif
}

static bool flash_ambiq_valid_range(off_t offset, size_t len)
{
	if ((offset < 0) || offset >= SOC_NV_FLASH_SIZE || (SOC_NV_FLASH_SIZE - offset) < len) {
		return false;
	}

	return true;
}

static int flash_ambiq_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	ARG_UNUSED(dev);

	if (!flash_ambiq_valid_range(offset, len)) {
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	memcpy(data, (uint8_t *)(SOC_NV_FLASH_ADDR + offset), len);

	return 0;
}

static int flash_ambiq_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	struct flash_ambiq_data *dev_data = dev->data;
	int ret = 0;
	unsigned int key = 0;
	uint32_t aligned[FLASH_WRITE_CHUNK_SIZE / sizeof(uint32_t)] = {0};
	const uint8_t *src = (const uint8_t *)data;
	size_t remaining = len;
	size_t current_offset = offset;
	int retry_count;

	/* write address must be block size aligned and the write length must be multiple of block
	 * size.
	 */
	if (!flash_ambiq_valid_range(offset, len) ||
	    ((uint32_t)offset & (FLASH_WRITE_BLOCK_SIZE - 1)) ||
	    (len & (FLASH_WRITE_BLOCK_SIZE - 1))) {
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	/* Lock device for write operation - use mutex for priority inheritance */
	ret = FLASH_LOCK_TAKE(dev_data);
	if (ret != 0) {
		return ret;
	}

	/*
	 * Write in chunks to allow timing-critical ISRs to run between chunks.
	 * Since input validation ensures len is a multiple of FLASH_WRITE_BLOCK_SIZE,
	 * and FLASH_WRITE_CHUNK_SIZE is guaranteed (via BUILD_ASSERT) to be a multiple
	 * of FLASH_WRITE_BLOCK_SIZE, all chunk_size values will be properly aligned.
	 */
	while (remaining > 0) {
		size_t chunk_size = MIN(remaining, FLASH_WRITE_CHUNK_SIZE);
		size_t words = chunk_size / sizeof(uint32_t);
		bool write_verified = false;

		/* Prepare aligned buffer */
		for (size_t j = 0; j < words; j++) {
			aligned[j] = UNALIGNED_GET((uint32_t *)(src + j * sizeof(uint32_t)));
		}

		/* Retry logic: attempt write up to MAX_RETRIES times */
		for (retry_count = 0; retry_count < FLASH_OPERATION_MAX_RETRIES; retry_count++) {
			/* Lock interrupts for write operation */
			key = irq_lock();

#if defined(AMBIQ_NOR_FLASH)
			ret = am_hal_flash_program_main(
				AM_HAL_FLASH_PROGRAM_KEY, aligned,
				(uint32_t *)(SOC_NV_FLASH_ADDR + current_offset), words);
#else
			ret = am_hal_mram_main_program(
				AM_HAL_MRAM_PROGRAM_KEY, aligned,
				(uint32_t *)(SOC_NV_FLASH_ADDR + current_offset), words);
#endif
			/*
			 * Invalidate instruction cache before re-enabling interrupts
			 * so an ISR cannot execute stale code from the just-modified
			 * flash region. Apollo2x/3x have no standard ARM caches; their
			 * proprietary cache is not a concern for data writes.
			 */
#if defined(CONFIG_SOC_SERIES_APOLLO4X)
			if (ret == AM_HAL_STATUS_SUCCESS) {
				am_hal_cachectrl_control(
					AM_HAL_CACHECTRL_CONTROL_MRAM_CACHE_INVALIDATE, NULL);
			}
#elif !defined(AMBIQ_NOR_FLASH)
			if (ret == AM_HAL_STATUS_SUCCESS) {
				sys_cache_data_invd_range(
					(void *)(SOC_NV_FLASH_ADDR + current_offset),
					chunk_size);
				sys_cache_instr_flush_range(
					(void *)(SOC_NV_FLASH_ADDR + current_offset),
					chunk_size);
			}
#endif

			/* Unlock interrupts to allow timing-critical ISRs */
			irq_unlock(key);

			/* Map HAL error code to errno */
			ret = flash_ambiq_hal_status_to_errno(ret);

			if (ret != 0) {
				LOG_WRN("Flash write failed at offset 0x%lx (attempt %d/%d): %d",
					(long)current_offset, retry_count + 1,
					FLASH_OPERATION_MAX_RETRIES, ret);
				continue;
			}

			/* Verify write */
			if (memcmp((void *)(SOC_NV_FLASH_ADDR + current_offset), src, chunk_size) ==
			    0) {
				write_verified = true;
				break;
			}

			LOG_WRN("Flash write verification failed at offset 0x%lx (attempt %d/%d)",
				(long)current_offset, retry_count + 1, FLASH_OPERATION_MAX_RETRIES);
		}

		if (!write_verified) {
			LOG_ERR("Flash write failed after %d attempts at offset 0x%lx",
				FLASH_OPERATION_MAX_RETRIES, (long)current_offset);
			ret = -EIO;
			break;
		}

		src += chunk_size;
		current_offset += chunk_size;
		remaining -= chunk_size;
	}

	FLASH_UNLOCK(dev_data);

	return ret;
}

static bool flash_ambiq_is_erased(off_t offset, size_t len)
{
	const uint8_t *flash_ptr = (const uint8_t *)(SOC_NV_FLASH_ADDR + offset);

	for (size_t i = 0; i < len; i++) {
		if (flash_ptr[i] != FLASH_ERASE_BYTE) {
			return false;
		}
	}

	return true;
}

static int flash_ambiq_erase(const struct device *dev, off_t offset, size_t len)
{
	struct flash_ambiq_data *dev_data = dev->data;
	int ret = 0;
	int retry_count;

	if (!flash_ambiq_valid_range(offset, len)) {
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

#if defined(AMBIQ_NOR_FLASH)
	if ((offset % FLASH_ERASE_BLOCK_SIZE) != 0) {
		LOG_ERR("offset 0x%lx is not on a page boundary", (long)offset);
		return -EINVAL;
	}

	if ((len % FLASH_ERASE_BLOCK_SIZE) != 0) {
		LOG_ERR("len %zu is not multiple of a page size", len);
		return -EINVAL;
	}
#else
	/* The erase address and length alignment check will be done in HAL.*/
#endif

	/* Lock device for erase operation - use mutex for priority inheritance */
	ret = FLASH_LOCK_TAKE(dev_data);
	if (ret != 0) {
		return ret;
	}

#if defined(AMBIQ_NOR_FLASH)
	/* Apollo2/3: erase each page individually with retry logic */
	size_t num_pages = len / FLASH_ERASE_BLOCK_SIZE;
	size_t current_offset = offset;

	for (size_t page = 0; page < num_pages; page++) {
		bool erase_verified = false;
		uint32_t page_inst =
			AM_HAL_FLASH_ADDR2INST(((uint32_t)SOC_NV_FLASH_ADDR + current_offset));
		uint32_t page_num =
			AM_HAL_FLASH_ADDR2PAGE(((uint32_t)SOC_NV_FLASH_ADDR + current_offset));

		/* Retry logic: attempt erase up to MAX_RETRIES times */
		for (retry_count = 0; retry_count < FLASH_OPERATION_MAX_RETRIES; retry_count++) {
			unsigned int key = irq_lock();

			ret = am_hal_flash_page_erase(AM_HAL_FLASH_PROGRAM_KEY, page_inst,
					     page_num);

			irq_unlock(key);

			/* Map HAL error code to errno */
			ret = flash_ambiq_hal_status_to_errno(ret);

			if (ret != 0) {
				LOG_WRN("Flash erase failed at offset 0x%lx (attempt %d/%d): %d",
					(long)current_offset, retry_count + 1,
					FLASH_OPERATION_MAX_RETRIES, ret);
				continue;
			}

			/* Verify erase - check if all bytes are 0xFF */
			if (flash_ambiq_is_erased(current_offset, FLASH_ERASE_BLOCK_SIZE)) {
				erase_verified = true;
				break;
			}

			LOG_WRN("Flash erase verification failed at offset 0x%lx (attempt %d/%d)",
			(long)current_offset, retry_count + 1, FLASH_OPERATION_MAX_RETRIES);
		}

		if (!erase_verified) {
			LOG_ERR("Flash erase failed after %d attempts at offset 0x%lx",
				FLASH_OPERATION_MAX_RETRIES, (long)current_offset);
			ret = -EIO;
			break;
		}

		current_offset += FLASH_ERASE_BLOCK_SIZE;
	}
#else
	/* Apollo4/5: use fill operation with retry logic */
	bool erase_verified = false;

	for (retry_count = 0; retry_count < FLASH_OPERATION_MAX_RETRIES; retry_count++) {
		ret = am_hal_mram_main_fill(AM_HAL_MRAM_PROGRAM_KEY, FLASH_ERASE_WORD,
					    (uint32_t *)(SOC_NV_FLASH_ADDR + offset),
					    (len / sizeof(uint32_t)));

		/*
		 * Invalidate instruction cache so an ISR cannot execute stale
		 * code from the just-erased flash region.
		 */
#if defined(CONFIG_SOC_SERIES_APOLLO4X)
		if (ret == AM_HAL_STATUS_SUCCESS) {
			am_hal_cachectrl_control(
				AM_HAL_CACHECTRL_CONTROL_MRAM_CACHE_INVALIDATE, NULL);
		}
#else
		if (ret == AM_HAL_STATUS_SUCCESS) {
			sys_cache_data_invd_range((void *)(SOC_NV_FLASH_ADDR + offset), len);
			sys_cache_instr_flush_range((void *)(SOC_NV_FLASH_ADDR + offset), len);
		}
#endif

		/* Map HAL error code to errno */
		ret = flash_ambiq_hal_status_to_errno(ret);

		if (ret != 0) {
			LOG_WRN("Flash erase failed at offset 0x%lx (attempt %d/%d): %d",
				(long)offset, retry_count + 1, FLASH_OPERATION_MAX_RETRIES, ret);
			continue;
		}

		/* Verify erase */
		if (flash_ambiq_is_erased(offset, len)) {
			erase_verified = true;
			break;
		}

		LOG_WRN("Flash erase verification failed at offset 0x%lx (attempt %d/%d)",
			(long)offset, retry_count + 1, FLASH_OPERATION_MAX_RETRIES);
	}

	if (!erase_verified) {
		LOG_ERR("Flash erase failed after %d attempts at offset 0x%lx",
			FLASH_OPERATION_MAX_RETRIES, (long)offset);
		ret = -EIO;
	}
#endif

	FLASH_UNLOCK(dev_data);

	return ret;
}

static int flash_ambiq_get_size(const struct device *dev, uint64_t *size)
{
	ARG_UNUSED(dev);

	*size = SOC_NV_FLASH_SIZE;

	return 0;
}

static const struct flash_parameters *flash_ambiq_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_ambiq_parameters;
}

#if CONFIG_FLASH_PAGE_LAYOUT
static const struct flash_pages_layout pages_layout = {
	.pages_count = SOC_NV_FLASH_SIZE / FLASH_ERASE_BLOCK_SIZE,
	.pages_size = FLASH_ERASE_BLOCK_SIZE,
};

static void flash_ambiq_pages_layout(const struct device *dev,
				     const struct flash_pages_layout **layout, size_t *layout_size)
{
	ARG_UNUSED(dev);

	*layout = &pages_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static DEVICE_API(flash, flash_ambiq_driver_api) = {
	.read = flash_ambiq_read,
	.write = flash_ambiq_write,
	.erase = flash_ambiq_erase,
	.get_parameters = flash_ambiq_get_parameters,
	.get_size = flash_ambiq_get_size,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_ambiq_pages_layout,
#endif
};

static int flash_ambiq_init(const struct device *dev)
{
	struct flash_ambiq_data *data = dev->data;

	/* Initialize mutex - provides priority inheritance for flash operations */
	FLASH_LOCK_INIT(data);

	return 0;
}

static struct flash_ambiq_data flash_ambiq_data_0;

DEVICE_DT_INST_DEFINE(0, flash_ambiq_init, NULL, &flash_ambiq_data_0, NULL, POST_KERNEL,
		      CONFIG_FLASH_INIT_PRIORITY, &flash_ambiq_driver_api);
