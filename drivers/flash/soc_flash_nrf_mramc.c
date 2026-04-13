/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <nrfx_mramc.h>
#if defined(CONFIG_SOC_FLASH_NRF_MRAMC_FLUSH_CACHE)
#include <zephyr/cache.h>
#endif

LOG_MODULE_REGISTER(flash_nrf_mramc, CONFIG_FLASH_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_nrf_mramc

#define MRAM_NODE         DT_CHILD(DT_DRV_INST(0), mram_0)
#define MRAM_SIZE         DT_REG_SIZE(MRAM_NODE)
#define WRITE_BLOCK_SIZE  DT_PROP(MRAM_NODE, write_block_size)
#define ERASE_BLOCK_SIZE  DT_PROP(MRAM_NODE, erase_block_size)

#define ERASE_VALUE                NRFY_MRAMC_WORD_AFTER_ERASED
#define MRAMC_MIN_BYTES_DATA_WIDTH NRFY_MRAMC_BYTES_IN_WORD

BUILD_ASSERT((ERASE_BLOCK_SIZE % WRITE_BLOCK_SIZE) == 0,
		 "erase-block-size expected to be a multiple of write-block-size");

/**
 * @param[in] addr       Address of mram memory.
 * @param[in] len        Number of bytes for the intended operation.
 * @param[in] must_align Require MRAM word alignment, if applicable.
 *
 * @return true if the address and length are valid, false otherwise.
 */
static bool validate_action(uint32_t addr, size_t len, bool must_align)
{
	if (!nrfx_mramc_valid_address_check(addr, true)) {
		LOG_ERR("Invalid address: %x", addr);
		return false;
	}

	if (!nrfx_mramc_fits_memory_check(addr, true, len)) {
		LOG_ERR("Address %x with length %zu exceeds MRAM size", addr, len);
		return false;
	}

	if (must_align && (!(nrfx_is_word_aligned((void const *)addr)) ||
		 ((len % MRAMC_MIN_BYTES_DATA_WIDTH) != 0))) {
		LOG_ERR("Address %x is not word aligned", addr);
		return false;
	}

	return true;
}

/**
 * @brief Verifies the data written to MRAM.
 *
 * @param[in] addr     Address of mram memory.
 * @param[in] len      Number of bytes for the intended operation.
 * @param[in] is_erase Whether is verifying erase operation.
 *
 * @return true if the address and length are valid, false otherwise.
 */
static int nrf_mramc_verify_data(off_t offset, const void *data, size_t size, bool is_erase)
{
	uint32_t addr = NRFX_MRAMC_MAP_TO_ADDR(offset);
	uint32_t num_words = size / MRAMC_MIN_BYTES_DATA_WIDTH;

	const uint32_t *mram = (const uint32_t *)addr;
	const uint32_t *buf  = (const uint32_t *)data;

	/* Compare and verify written data */
	for (size_t i = 0; i < num_words; i++) {
		uint32_t w = mram[i];

		if (is_erase) {
			if (w != ERASE_VALUE) {
				return -EIO;
			}
		} else {
			if (w != buf[i]) {
				return -EIO;
			}
		}
	}

	return 0;
}

static int nrf_mramc_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	ARG_UNUSED(dev);
	uint32_t addr = NRFX_MRAMC_MAP_TO_ADDR(offset);

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
	nrfx_mramc_buffer_read(data, addr, len);
	return 0;
}

static int nrf_mramc_write(const struct device *dev, off_t offset,
			 const void *data, size_t size)
{
	ARG_UNUSED(dev);
	uint32_t addr = NRFX_MRAMC_MAP_TO_ADDR(offset);
	uint8_t  retries = CONFIG_NRF_MRAMC_MAX_RETRIES;

	if (data == NULL) {
		LOG_ERR("Data pointer is NULL");
		return -EINVAL;
	}

	/* Validate addr and length in the range */
	if (!validate_action(addr, size, true)) {
		return -EINVAL;
	}

	LOG_DBG("write: %x:%zu", addr, size);

	while(retries--)
	{
		/* Words write function takes second argument as number of write blocks
		 * and not number of bytes
		 */
		nrfx_mramc_words_write(addr, data, size / MRAMC_MIN_BYTES_DATA_WIDTH);

		if (!nrf_mramc_verify_data(offset, data, size, false))
		{
			return 0;
		}
		LOG_ERR("MRAM write verification failed at address 0x%x, retrying... (%u retries "
			"left)",
			addr, retries);
	}

	return -EIO;
}

static int nrf_mramc_erase(const struct device *dev, off_t offset, size_t size)
{
	ARG_UNUSED(dev);
	uint32_t addr = NRFX_MRAMC_MAP_TO_ADDR(offset);
	uint8_t  retries = CONFIG_NRF_MRAMC_MAX_RETRIES;

	/* Validate addr and length in the range */
	if (size == 0) {
		LOG_DBG("No data to erase");
		return 0;
	}

	if (!validate_action(addr, size, true)) {
		return -EINVAL;
	}

	LOG_DBG("erase: %p:%zu", (void *)addr, size);

	while(retries--)
	{
		/* Erase function takes second argument as number of 32-bit words
		 * and not number of bytes
		 */
		nrfx_mramc_erase(addr, size / MRAMC_MIN_BYTES_DATA_WIDTH);

#if defined(CONFIG_SOC_FLASH_NRF_MRAMC_FLUSH_CACHE)
	    sys_cache_instr_invd_all();
#endif
		if (!nrf_mramc_verify_data(offset, NULL, size, true))
		{
			return 0;
		}
		LOG_ERR("MRAM Erase verification failed at address 0x%x, retrying... (%u retries "
			"left)",
			addr, retries);
	}

	return -EIO;
}

static int nrf_mramc_get_size(const struct device *dev, uint64_t *size)
{
	ARG_UNUSED(dev);
	*size = nrfx_mramc_memory_size_get();
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

	nrfx_mramc_config_t config = NRFX_MRAMC_DEFAULT_CONFIG();
	int ret = nrfx_mramc_init(&config, NULL);

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
