/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>

#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
#include "tfm_ioctl_core_api.h"
#else
#include <nrfx_mramc.h>
#endif

LOG_MODULE_REGISTER(flash_nrf_mramc, CONFIG_FLASH_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_nrf_mramc

#define MRAM_NODE		  DT_NODELABEL(cpuapp_mram)
#define MRAM_BASE		 DT_REG_ADDR(MRAM_NODE)
#define MRAM_SIZE		  DT_REG_SIZE(MRAM_NODE)

BUILD_ASSERT(MRAM_BASE <= UINT32_MAX, "MRAM_BASE is not in size of uint32_t");
BUILD_ASSERT(MRAM_SIZE <= UINT32_MAX, "MRAM_SIZE is not in size of uint32_t");

#define WRITE_BLOCK_SIZE  DT_PROP(MRAM_NODE, write_block_size)
#define ERASE_BLOCK_SIZE  DT_PROP(MRAM_NODE, erase_block_size)

#define ERASE_VALUE				   0xFFFFFFFF
#define MRAM_WORD_SIZE	16
#define MRAMC_CONFIG_WEN_NORMAL	1
#define MRAMC_CONFIG_WEN_DISABLE   0

#define MAP_TO_ADDR(offset) (MRAM_BASE + offset)

BUILD_ASSERT((WRITE_BLOCK_SIZE % MRAM_WORD_SIZE) == 0,
		 "write-block-size expected to be a multiple of MRAM_WORD_SIZE");
BUILD_ASSERT((ERASE_BLOCK_SIZE % WRITE_BLOCK_SIZE) == 0,
		 "erase-block-size expected to be a multiple of write-block-size");

struct k_mutex nrf_mramc_mutex;

/**
 * @param[in] addr		 Address of mram memory.
 * @param[in] len		 Number of bytes for the intended operation.
 * @param[in] must_align Require MRAM word alignment, if applicable.
 *
 * @return true if the address and length are valid, false otherwise.
 */
static bool validate_action(uint32_t addr, size_t len, bool must_align)
{
	bool valid = true;

#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	valid = (addr >= MRAM_BASE && addr < (MRAM_BASE + MRAM_SIZE));
#else
	valid = nrfx_mramc_valid_address_check(addr, true);
#endif
	if (!valid) {
		LOG_ERR("Invalid address: %x", addr);
		return false;
	}

#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	valid = (((addr - (uint32_t)MRAM_BASE) < MRAM_SIZE) &&
		(len <= ((uint32_t)MRAM_BASE + MRAM_SIZE - addr)));
#else
	valid = nrfx_mramc_fits_memory_check(addr, true, len);
#endif
	if (!valid) {
		LOG_ERR("Address %x with length %zu exceeds MRAM size", addr, len);
		return false;
	}

	if (must_align && (((addr % MRAM_WORD_SIZE) != 0) ||
		 ((len % MRAM_WORD_SIZE) != 0))) {
		LOG_ERR("Address %x is not block aligned or data length is not multiple of %u",
			addr, MRAM_WORD_SIZE);
		return false;
	}

	return true;
}

/**
 * @brief Verifies the data written to MRAM.
 *
 * @param[in] offset   Offset of mram memory from base address.
 * @param[in] len	   Number of bytes for the intended operation.
 * @param[in] is_erase Whether is verifying erase operation.
 *
 * @return true if the address and length are valid, false otherwise.
 */
static int nrf_mramc_verify_data(off_t offset, const void *data, size_t size, bool is_erase)
{
	uint32_t *mram_start = (uint32_t *) MAP_TO_ADDR(offset);
	uint32_t *mram_end = mram_start + size / sizeof(uint32_t);

	if (is_erase) {
		while (mram_start < mram_end) {
			if (*mram_start++ != ERASE_VALUE) {
				return -EIO;
			}
		}
		return 0;
	}

	/* Use word comparison when the source buffer is word-aligned. */
	if (((uintptr_t)data % sizeof(uint32_t)) == 0) {
		const uint32_t *buf = (const uint32_t *)data;

		while (mram_start < mram_end) {
			if (*mram_start++ != *buf++) {
				return -EIO;
			}
		}
		return 0;
	}

	/* Otherwise, fall back to byte comparison to avoid unaligned
	 * uint32_t accesses from data.
	 */
	if (memcmp((const void *)mram_start, data, size) != 0) {
		return -EIO;
	}
	return 0;
}

/**
 * @brief Setting the write enable mode of MRAM controller.
 *
 * @param[in] mode  Write enable mode to set, either normal(1) or disable(0).
 *
 * @return true if mode change is successful, false otherwise.
 */
static int nrf_mramc_set_wen(uint32_t mode)
{
#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	return tfm_platform_mramc_set_wen(mode);
#else
	nrfx_mramc_config_write_mode_set(mode);

	while (!nrfx_mramc_ready_check()) {
	}

	return 0;
#endif
}

static int nrf_mramc_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	ARG_UNUSED(dev);
	uint32_t addr = MAP_TO_ADDR(offset);

	if (data == NULL) {
		LOG_ERR("Data pointer is NULL");
		return -EINVAL;
	}

	/* Validate addr and length in the range */
	if (!validate_action(addr, len, false)) {
		return -EINVAL;
	}

	LOG_DBG("read: %x:%zu", addr, len);

	/* Buffer read number of bytes and store in data pointer.
	 */
#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	memcpy(data, (void *)addr, len);
#else
	nrfx_mramc_buffer_read(data, addr, len);
#endif
	return 0;
}

/**
 * @brief Writes a word with MRAM_WORD_SIZE to MRAM and verifies the written data.
 *
 * @param addr  Destination address. Must be aligned to MRAM_WORD_SIZE (16 bytes).
 * @param data  Pointer to the source data buffer.
 *
 * @retval 0	   Write succeeded.
 * @retval -EIO	Write failed after all retries.
 */
static int nrf_mram_write_and_verify_word(uint32_t addr, void *data)
{
	uint8_t retries = CONFIG_NRF_MRAMC_MAX_RETRIES;

	while (retries--) {
		/* Check if data address is aligned to 4 bytes */
		if (((uintptr_t)data % 4) == 0) {
			uint32_t *mram_start = (uint32_t *)addr;
			uint32_t *mram_end = mram_start + sizeof(uint32_t);
			uint32_t *data_ptr = (uint32_t *)data;

			/* Aligned: use fast uint32_t copies */
			while (mram_start < mram_end) {
				*mram_start++ = *data_ptr++;
			}
		} else {
			/* Not aligned: use memcpy */
			memcpy((void *)addr, data, MRAM_WORD_SIZE);
		}

		if (!nrf_mramc_verify_data(addr, data, MRAM_WORD_SIZE, false)) {
			return 0;
		}
		LOG_ERR("MRAM write verification failed at address 0x%x, retrying... (%u retries "
			"left)",
			addr, retries);
	}

	LOG_ERR("MRAM write failed at address 0x%x, after %u retries ",
		addr, retries);
	return -EIO;
}

/**
 * @brief Erase a word with MRAM_WORD_SIZE to MRAM and verifies the written data.
 *
 * @param addr		Destination address. Must be aligned to MRAM_WORD_SIZE (16 bytes).
 *
 * @retval 0		Erase succeeded and no corruption was detected.
 * @retval -EIO		Erase failed after all retries.
 */
static int nrf_mram_erase_and_verify_word(uint32_t addr)
{
	uint8_t retries = CONFIG_NRF_MRAMC_MAX_RETRIES;

	while (retries--) {
		uint32_t *mram_start = (uint32_t *)addr;
		uint32_t *mram_end = mram_start + sizeof(uint32_t);

		/* Check if data address is aligned to 4 bytes */
		while (mram_start < mram_end) {
			*mram_start++ = 0xFFFFFFFFu;
		}

		if (!nrf_mramc_verify_data(addr, NULL, MRAM_WORD_SIZE, true)) {
			return 0;
		}
		LOG_ERR("MRAM erase verification failed at address 0x%x, retrying... (%u retries "
			"left)",
			addr, retries);
	}

	LOG_ERR("MRAM erase failed at address 0x%x, after %u retries ",
		addr, retries);
	return -EIO;
}

static int nrf_mramc_write(const struct device *dev, off_t offset,
			 const void *data, size_t size)
{
	ARG_UNUSED(dev);
	uint32_t addr = MAP_TO_ADDR(offset);
	uint32_t ret = 0;

	if (data == NULL) {
		LOG_ERR("Data pointer is NULL");
		return -EINVAL;
	}

	/* Validate addr and length in the range */
	if (!validate_action(addr, size, true)) {
		return -EINVAL;
	}

	LOG_DBG("write: %x:%zu", addr, size);

	k_mutex_lock(&nrf_mramc_mutex, K_FOREVER);

	for (uint32_t i = 0; i < size; i += MRAM_WORD_SIZE) {
		/* Write full 16 bytes word for N iterations */
		nrf_mramc_set_wen(MRAMC_CONFIG_WEN_NORMAL);
		ret = nrf_mram_write_and_verify_word(addr + i, (void *)((uintptr_t)data + i));
		nrf_mramc_set_wen(MRAMC_CONFIG_WEN_DISABLE);

		if (ret) {
			k_mutex_unlock(&nrf_mramc_mutex);
			return ret;
		}
	}

	k_mutex_unlock(&nrf_mramc_mutex);
	return 0;
}

static int nrf_mramc_erase(const struct device *dev, off_t offset, size_t size)
{
	ARG_UNUSED(dev);
	uint32_t addr = MAP_TO_ADDR(offset);
	uint32_t ret = 0;

	/* Validate addr and length in the range */
	if (size == 0) {
		LOG_DBG("No data to erase");
		return 0;
	}

	if (!validate_action(addr, size, true)) {
		return -EINVAL;
	}

	LOG_DBG("erase: %p:%zu", (void *)addr, size);

	k_mutex_lock(&nrf_mramc_mutex, K_FOREVER);

	for (uint32_t i = 0; i < size; i += MRAM_WORD_SIZE) {
		/* Erase full 16 bytes word for N iterations */
		nrf_mramc_set_wen(MRAMC_CONFIG_WEN_NORMAL);
		ret = nrf_mram_erase_and_verify_word(addr + i);
		nrf_mramc_set_wen(MRAMC_CONFIG_WEN_DISABLE);

		if (ret) {
			k_mutex_unlock(&nrf_mramc_mutex);
			return ret;
		}
	}

	k_mutex_unlock(&nrf_mramc_mutex);
	return 0;
}

static int nrf_mramc_get_size(const struct device *dev, uint64_t *size)
{
	ARG_UNUSED(dev);
#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	*size = MRAM_SIZE;
#else
	*size = nrfx_mramc_memory_size_get();
#endif
	return 0;
}

static const struct flash_parameters *nrf_mramc_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	static const struct flash_parameters parameters = {
		.write_block_size = WRITE_BLOCK_SIZE,
		.erase_value = (uint8_t) ERASE_VALUE,
		.caps = {
			.no_explicit_erase = true,
		},
	};

	return &parameters;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void nrf_mramc_page_layout(const struct device *dev,
				 const struct flash_pages_layout **layout, size_t *layout_size)
{
	ARG_UNUSED(dev);

	static const struct flash_pages_layout pages_layout = {
		.pages_count = MRAM_SIZE / (ERASE_BLOCK_SIZE),
		.pages_size = ERASE_BLOCK_SIZE,
	};

	*layout = &pages_layout;
	*layout_size = 1;
}
#endif

static int mramc_sys_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	int ret = 0;

	k_mutex_init(&nrf_mramc_mutex);

#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	/* Initialization of MRAMC driver via secure processing environment */
	enum tfm_platform_err_t status = tfm_platform_mramc_init();

	if (status != TFM_PLATFORM_ERR_SUCCESS) {
		LOG_ERR("Failed to initialize MRAMC via TF-M service: %d", status);
		ret = -EIO;
	}
#else
	nrfx_mramc_config_t config = NRFX_MRAMC_DEFAULT_CONFIG();

	ret = nrfx_mramc_init(&config, NULL);
#endif

	if (ret != 0) {
		LOG_ERR("Failed to initialize MRAMC: %d", ret);
		return ret;
	}
	LOG_DBG("MRAMC initialized successfully");
	return 0;
}

static DEVICE_API(flash, nrf_mram_api) = {
	.read = nrf_mramc_read,
	.write = nrf_mramc_write,
	.erase = nrf_mramc_erase,
	.get_size = nrf_mramc_get_size,
	.get_parameters = nrf_mramc_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = nrf_mramc_page_layout,
#endif
};

DEVICE_DT_INST_DEFINE(0, mramc_sys_init, NULL, NULL, NULL, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,
			  &nrf_mram_api);
