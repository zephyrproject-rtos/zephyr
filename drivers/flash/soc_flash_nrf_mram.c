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

BUILD_ASSERT(MRAM_START > 0, "nordic,mram: start address expected to be non-zero");

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

	memcpy((void *)addr, data, len);
	commit_changes(addr + len);

	return 0;
}

static const struct flash_parameters *nrf_mram_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	static const struct flash_parameters parameters = {
		.write_block_size = WRITE_BLOCK_SIZE,
		.caps = {
			.explicit_erase,
		},
	};

	return &parameters;
}

static const struct flash_driver_api nrf_mram_api = {
	.read = nrf_mram_read,
	.write = nrf_mram_write,
	.get_parameters = nrf_mram_get_parameters,
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,
		      &nrf_mram_api);
