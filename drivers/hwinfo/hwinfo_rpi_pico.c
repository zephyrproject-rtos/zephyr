/*
 * Copyright (c) 2022 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <drivers/hwinfo.h>
#include <hardware/flash.h>

#define FLASH_RUID_DATA_BYTES 8

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint8_t id[FLASH_RUID_DATA_BYTES];
	uint32_t key;

	/*
	 * flash_get_unique_id temporarily disables XIP to query the
	 * flash for its ID. If the CPU is interrupted while XIP is
	 * disabled, it will halt. Therefore, interrupts must be disabled
	 * before fetching the ID.
	 */
	key = irq_lock();
	flash_get_unique_id(id);
	irq_unlock(key);

	if (length > sizeof(id)) {
		length = sizeof(id);
	}
	memcpy(buffer, id, length);

	return length;
}
