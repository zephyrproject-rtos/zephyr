/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/barrier.h>

LOG_MODULE_REGISTER(flash_nrf_mram, CONFIG_FLASH_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_mram

#define MRAM_START DT_INST_REG_ADDR(0)
#define MRAM_SIZE  DT_INST_REG_SIZE(0)

#define MRAM_WORD_SIZE 16
#define MRAM_WORD_MASK 0xf

#define WRITE_BLOCK_SIZE DT_INST_PROP_OR(0, write_block_size, MRAM_WORD_SIZE)
#define ERASE_BLOCK_SIZE DT_INST_PROP_OR(0, erase_block_size, WRITE_BLOCK_SIZE)

#define ERASE_VALUE 0xff

#define SOC_NRF_MRAM_BANK_11_OFFSET  0x10000
#define SOC_NRF_MRAM_BANK_11_ADDRESS (MRAM_START + SOC_NRF_MRAM_BANK_11_OFFSET)
#define SOC_NRF_MRAMC_ADDR_0         0x5f092000
#define SOC_NRF_MRAMC_ADDR_1         0x5f093000
#define SOC_NRF_MRAMC_READY_REG_0    (SOC_NRF_MRAMC_ADDR_0 + 0x400)
#define SOC_NRF_MRAMC_READY_REG_1    (SOC_NRF_MRAMC_ADDR_1 + 0x400)

BUILD_ASSERT(MRAM_START > 0, "nordic,mram: start address expected to be non-zero");
BUILD_ASSERT((ERASE_BLOCK_SIZE % WRITE_BLOCK_SIZE) == 0,
	     "erase-block-size expected to be a multiple of write-block-size");

static inline uint32_t nrf_mram_ready(uint32_t addr)
{
	if (addr < SOC_NRF_MRAM_BANK_11_ADDRESS) {
		return sys_read32(SOC_NRF_MRAMC_READY_REG_0);
	} else {
		return sys_read32(SOC_NRF_MRAMC_READY_REG_1);
	}
}

/**
 * @param[in,out] offset      Relative offset into memory, from the driver API.
 * @param[in]     len         Number of bytes for the intended operation.
 * @param[in]     must_align  Require MRAM word alignment, if applicable.
 *
 * @return Absolute address in MRAM, or NULL if @p offset or @p len are not
 *         within bounds or appropriately aligned.
 */
static uintptr_t validate_and_map_addr(off_t offset, size_t len, bool must_align)
{
	if (unlikely(offset < 0 || offset >= MRAM_SIZE || len > MRAM_SIZE - offset)) {
		LOG_ERR("invalid offset: %ld:%zu", offset, len);
		return 0;
	}

	const uintptr_t addr = MRAM_START + offset;

	if (WRITE_BLOCK_SIZE > 1 && must_align &&
	    unlikely((addr % WRITE_BLOCK_SIZE) != 0 || (len % WRITE_BLOCK_SIZE) != 0)) {
		LOG_ERR("invalid alignment: %p:%zu", (void *)addr, len);
		return 0;
	}

	return addr;
}

/**
 * @param[in] addr_end  Last modified MRAM address (not inclusive).
 */
static void commit_changes(uintptr_t addr_end)
{
	/* Barrier following our last write. */
	barrier_dmem_fence_full();

	if ((WRITE_BLOCK_SIZE & MRAM_WORD_MASK) == 0 || (addr_end & MRAM_WORD_MASK) == 0) {
		/* Our last operation was MRAM word-aligned, so we're done.
		 * Note: if WRITE_BLOCK_SIZE is a multiple of MRAM_WORD_SIZE,
		 * then this was already checked in validate_and_map_addr().
		 */
		return;
	}

	/* Get the most significant byte (MSB) of the last MRAM word we were modifying.
	 * Writing to this byte makes the MRAM controller commit other pending writes to that word.
	 */
	addr_end |= MRAM_WORD_MASK;

	/* Issue a dummy write, since we didn't have anything to write here.
	 * Doing this lets us finalize our changes before we exit the driver API.
	 */
	sys_write8(sys_read8(addr_end), addr_end);
	while (!nrf_mram_ready(addr_end)) {
		/* Wait until MRAM controller is ready */
	}
}

static int nrf_mram_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	ARG_UNUSED(dev);

	const uintptr_t addr = validate_and_map_addr(offset, len, false);

	if (!addr) {
		return -EINVAL;
	}

	LOG_DBG("read: %p:%zu", (void *)addr, len);

	memcpy(data, (void *)addr, len);

	return 0;
}

static int nrf_mram_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	ARG_UNUSED(dev);

	const uintptr_t addr = validate_and_map_addr(offset, len, true);

	if (!addr) {
		return -EINVAL;
	}

	LOG_DBG("write: %p:%zu", (void *)addr, len);

	for (uint32_t i = 0; i < (len / MRAM_WORD_SIZE); i++) {
		memcpy((void *)(addr + (i * MRAM_WORD_SIZE)),
		       (void *)((uintptr_t)data + (i * MRAM_WORD_SIZE)), MRAM_WORD_SIZE);
		while (!nrf_mram_ready(addr + (i * MRAM_WORD_SIZE))) {
			/* Wait until MRAM controller is ready */
		}
	}
	if (len % MRAM_WORD_SIZE) {
		memcpy((void *)(addr + (len & ~MRAM_WORD_MASK)),
		       (void *)((uintptr_t)data + (len & ~MRAM_WORD_MASK)), len & MRAM_WORD_MASK);
		while (!nrf_mram_ready(addr + (len & ~MRAM_WORD_MASK))) {
			/* Wait until MRAM controller is ready */
		}
	}
	commit_changes(addr + len);

	return 0;
}

static int nrf_mram_erase(const struct device *dev, off_t offset, size_t size)
{
	ARG_UNUSED(dev);

	const uintptr_t addr = validate_and_map_addr(offset, size, true);

	if (!addr) {
		return -EINVAL;
	}

	LOG_DBG("erase: %p:%zu", (void *)addr, size);

	for (uint32_t i = 0; i < (size / MRAM_WORD_SIZE); i++) {
		memset((void *)(addr + (i * MRAM_WORD_SIZE)), ERASE_VALUE, MRAM_WORD_SIZE);
		while (!nrf_mram_ready(addr + (i * MRAM_WORD_SIZE))) {
			/* Wait until MRAM controller is ready */
		}
	}
	if (size % MRAM_WORD_SIZE) {
		memset((void *)(addr + (size & ~MRAM_WORD_MASK)), ERASE_VALUE,
		       size & MRAM_WORD_MASK);
		while (!nrf_mram_ready(addr + (size & ~MRAM_WORD_MASK))) {
			/* Wait until MRAM controller is ready */
		}
	}
	commit_changes(addr + size);

	return 0;
}

static int nrf_mram_get_size(const struct device *dev, uint64_t *size)
{
	ARG_UNUSED(dev);

	*size = MRAM_SIZE;

	return 0;
}

static const struct flash_parameters *nrf_mram_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	static const struct flash_parameters parameters = {
		.write_block_size = WRITE_BLOCK_SIZE,
		.erase_value = ERASE_VALUE,
		.caps =
			{
				.no_explicit_erase = true,
			},
	};

	return &parameters;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void nrf_mram_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
				 size_t *layout_size)
{
	ARG_UNUSED(dev);

	static const struct flash_pages_layout pages_layout = {
		.pages_count = (MRAM_SIZE) / (ERASE_BLOCK_SIZE),
		.pages_size = ERASE_BLOCK_SIZE,
	};

	*layout = &pages_layout;
	*layout_size = 1;
}
#endif

static DEVICE_API(flash, nrf_mram_api) = {
	.read = nrf_mram_read,
	.write = nrf_mram_write,
	.erase = nrf_mram_erase,
	.get_size = nrf_mram_get_size,
	.get_parameters = nrf_mram_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = nrf_mram_page_layout,
#endif
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,
		      &nrf_mram_api);
