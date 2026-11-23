/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "ext2_bitmap.h"

LOG_MODULE_DECLARE(ext2);

/* NOTICE: Offsets in bitmap start with 0 */

int ext2_bitmap_set(uint8_t *bm, uint32_t index, uint32_t size)
{
	LOG_DBG("Setting %d bit in bitmap", index);

	uint32_t idx = index / 8;
	uint32_t off = index % 8;

	if (idx >= size) {
		LOG_ERR("Tried to set value outside of bitmap (%d)", index);
		return -EINVAL;
	}

	__ASSERT((bm[idx] & BIT(off)) == 0, "Bit %d set in bitmap", index);

	LOG_DBG("Bitmap %d: %x", idx, bm[idx]);
	bm[idx] |= BIT(off);
	LOG_DBG("Bitmap %d: %x", idx, bm[idx]);

	return 0;
}

int ext2_bitmap_unset(uint8_t *bm, uint32_t index, uint32_t size)
{
	LOG_DBG("Unsetting %d bit in bitmap", index);

	uint32_t idx = index / 8;
	uint32_t off = index % 8;

	if (idx >= size) {
		LOG_ERR("Tried to unset value outside of bitmap (%d)", index);
		return -EINVAL;
	}

	__ASSERT(bm[idx] & BIT(off), "Bit %d not set in bitmap", index);

	LOG_DBG("Bitmap %d: %x", idx, bm[idx]);
	bm[idx] &= ~BIT(off);
	LOG_DBG("Bitmap %d: %x", idx, bm[idx]);

	return 0;
}

int32_t ext2_bitmap_find_free(uint8_t *bm, uint32_t size)
{
	for (int i = 0; i < size; ++i) {
		LOG_DBG("Bitmap %d: %x (%x)", i, bm[i], ~bm[i]);
		if (bm[i] < UINT8_MAX) {
			/* not all bits are set here */
			int off = find_lsb_set(~bm[i]) - 1;

			LOG_DBG("off: %d", off);
			return off + i * 8;
		}
	}
	return -ENOSPC;
}

uint32_t ext2_bitmap_count_set(uint8_t *bm, uint32_t size)
{
	int32_t count = 0;

	for (uint32_t i = 0; i < size; i += 8) {
		uint8_t val = bm[i / 8];

		for (int b = 0; b < 8 && i + b < size; ++b) {
			count += (val >> b) & BIT(0);
		}

	}
	return count;
}
