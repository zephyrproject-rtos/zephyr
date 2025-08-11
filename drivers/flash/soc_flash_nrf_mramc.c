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

#define _ADD_SIZE(node_id) + DT_REG_SIZE(node_id)
#define _WBS(node_id)	   DT_PROP(node_id, write_block_size),
#define _EBS(node_id)	   DT_PROP(node_id, erase_block_size),

#define MRAM_SIZE   (0 DT_INST_FOREACH_CHILD_STATUS_OKAY(0, _ADD_SIZE))
#define ERASE_VALUE (uint8_t)NRFY_MRAMC_WORD_AFTER_ERASED

/* Use DT_FOREACH_CHILD_STATUS_OKAY of mramc with _FIRST() helper
 * macro to get the write block size and erase block size from
 * the first child node.
 */
#define WBS_LIST  DT_INST_FOREACH_CHILD_STATUS_OKAY(0, _WBS)
#define EBS_LIST  DT_INST_FOREACH_CHILD_STATUS_OKAY(0, _EBS)

#define _FIRST_HELPER(first, ...)  first
#define _FIRST(...)  _FIRST_HELPER(__VA_ARGS__)

#define WRITE_BLOCK_SIZE  _FIRST(WBS_LIST)
#define ERASE_BLOCK_SIZE  _FIRST(EBS_LIST)

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

	if (must_align && !(nrfx_is_word_aligned((void const *)addr))) {
		LOG_ERR("Address %x is not word aligned", addr);
		return false;
	}

	return true;
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
			 const void *data, size_t len)
{
	ARG_UNUSED(dev);
	uint32_t addr = NRFX_MRAMC_MAP_TO_ADDR(offset);

	if (data == NULL) {
		LOG_ERR("Data pointer is NULL");
		return -EINVAL;
	}

	/* Validate addr and length in the range */
	if (!validate_action(addr, len, true)) {
		return -EINVAL;
	}

	LOG_DBG("write: %x:%zu", addr, len);

	/* Words write function takes second argument as number of write blocks
	 * and not number of bytes
	 */
	nrfx_mramc_words_write(addr, data, len / WRITE_BLOCK_SIZE);

	return 0;
}

static int nrf_mramc_erase(const struct device *dev, off_t offset, size_t size)
{
	ARG_UNUSED(dev);
	uint32_t addr = NRFX_MRAMC_MAP_TO_ADDR(offset);

	/* Validate addr and length in the range */
	if (size == 0) {
		LOG_DBG("No data to erase");
		return 0;
	}

	if (!validate_action(addr, size, true)) {
		return -EINVAL;
	}

	LOG_DBG("erase: %p:%zu", (void *)addr, size);

	/* Erase function takes second argument as number of write blocks
	 * and not number of bytes
	 */
	nrfx_mramc_area_erase(addr, size / WRITE_BLOCK_SIZE);
#if defined(CONFIG_SOC_FLASH_NRF_MRAMC_FLUSH_CACHE)
	sys_cache_instr_invd_all();
#endif
	return 0;
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
		.erase_value = ERASE_VALUE,
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
	nrfx_err_t err = nrfx_mramc_init(&config, NULL);

	if (err != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize MRAMC: %d", err);
		return -EIO;
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
