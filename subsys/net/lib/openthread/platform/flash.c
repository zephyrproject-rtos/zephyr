/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <flash.h>

#include "platform-zephyr.h"

static struct device *flash_dev;
static size_t ot_flash_size;
static size_t ot_flash_offset;

static inline u32_t mapAddress(u32_t aAddress)
{
	return aAddress + ot_flash_offset;
}

otError utilsFlashInit(void)
{
	int i;
	struct flash_pages_info info;
	u32_t ot_flash_index;

	flash_dev = device_get_binding(DT_FLASH_DEV_NAME);

	if (!flash_dev) {
		return OT_ERROR_NOT_IMPLEMENTED;
	}

	if (flash_get_page_info_by_offs(flash_dev, FLASH_AREA_STORAGE_OFFSET,
					&info)) {
		return OT_ERROR_FAILED;
	}

	ot_flash_index = info.index;
	ot_flash_offset = info.start_offset;
	ot_flash_size = 0;

	for (i = 0; i < CONFIG_OT_PLAT_FLASH_PAGES_COUNT; i++) {
		if (flash_get_page_info_by_idx(flash_dev,
			ot_flash_index + i, &info)) {

			return OT_ERROR_FAILED;
		}
		ot_flash_size += info.size;
	}

	if (ot_flash_size > FLASH_AREA_STORAGE_SIZE) {
		return OT_ERROR_FAILED;
	}

	return OT_ERROR_NONE;
}

u32_t utilsFlashGetSize(void)
{
	return ot_flash_size;
}

otError utilsFlashErasePage(u32_t aAddress)
{
	struct flash_pages_info info;
	u32_t address;

	address = mapAddress(aAddress);
	if (flash_get_page_info_by_offs(flash_dev, address, &info)) {
		return OT_ERROR_FAILED;
	}

	if (flash_erase(flash_dev, address, info.size)) {
		return OT_ERROR_FAILED;
	}

	return OT_ERROR_NONE;
}

otError utilsFlashStatusWait(u32_t aTimeout)
{
	ARG_UNUSED(aTimeout);

	return OT_ERROR_NONE;
}

u32_t utilsFlashWrite(u32_t aAddress, uint8_t *aData, u32_t aSize)
{
	u32_t index = 0U;

	flash_write_protection_set(flash_dev, false);
	if (!flash_write(flash_dev, mapAddress(aAddress), aData, aSize)) {
		index = aSize;
	}
	flash_write_protection_set(flash_dev, true);

	return index;
}

u32_t utilsFlashRead(u32_t aAddress, uint8_t *aData, u32_t aSize)
{
	u32_t index = 0U;

	if (!flash_read(flash_dev, mapAddress(aAddress), aData, aSize)) {
		index = aSize;
	}

	return index;
}
