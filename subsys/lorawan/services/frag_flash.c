/*
 * Copyright (c) 2022-2024 Libre Solar Technologies GmbH
 * Copyright (c) 2022-2024 tado GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "frag_flash.h"

#include <FragDecoder.h>

#include <zephyr/dfu/mcuboot.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>

LOG_MODULE_REGISTER(lorawan_frag_flash, CONFIG_LORAWAN_SERVICES_LOG_LEVEL);

#define TARGET_IMAGE_AREA FIXED_PARTITION_ID(slot1_partition)

struct frag_cache_entry {
	uint32_t addr;
	uint8_t data[FRAG_MAX_SIZE];
};

static struct frag_cache_entry frag_cache[FRAG_MAX_REDUNDANCY];
static uint32_t frag_size;
static int cached_frags;
static bool use_cache;

static const struct flash_area *fa;

int frag_flash_init(uint32_t fragment_size)
{
	int err;

	if (fragment_size > FRAG_MAX_SIZE) {
		return -ENOSPC;
	}

	frag_size = fragment_size;
	cached_frags = 0;
	use_cache = false;

	err = flash_area_open(TARGET_IMAGE_AREA, &fa);
	if (err) {
		return err;
	}

	LOG_DBG("Starting to erase flash area");

	err = flash_area_erase(fa, 0, fa->fa_size);

	LOG_DBG("Finished erasing flash area");

	return err;
}

int8_t frag_flash_write(uint32_t addr, uint8_t *data, uint32_t size)
{
	int8_t err = 0;

	if (!use_cache) {
		LOG_DBG("Writing %u bytes to addr 0x%X", size, addr);

		err = flash_area_write(fa, addr, data, size);
	} else {
		LOG_DBG("Caching %u bytes for addr 0x%X", size, addr);

		if (size != frag_size) {
			LOG_ERR("Invalid fragment size %d", size);
			return -EINVAL;
		}

		/* overwrite fragment in cache if existing */
		for (int i = 0; i < cached_frags; i++) {
			if (frag_cache[i].addr == addr) {
				memcpy(frag_cache[i].data, data, size);
				return 0;
			}
		}

		/* otherwise create new cache entry */
		if (cached_frags < ARRAY_SIZE(frag_cache)) {
			frag_cache[cached_frags].addr = addr;
			memcpy(frag_cache[cached_frags].data, data, size);
			cached_frags++;
		} else {
			LOG_ERR("Fragment cache too small");
			err = -ENOSPC;
		}
	}

	return err == 0 ? 0 : -1;
}

int8_t frag_flash_read(uint32_t addr, uint8_t *data, uint32_t size)
{
	for (int i = 0; i < cached_frags; i++) {
		if (frag_cache[i].addr == addr) {
			memcpy(data, frag_cache[i].data, size);
			return 0;
		}
	}

	return flash_area_read(fa, addr, data, size) == 0 ? 0 : -1;
}

void frag_flash_use_cache(void)
{
	use_cache = true;
}

void frag_flash_finish(void)
{
	int err;

	for (int i = 0; i < cached_frags; i++) {
		LOG_DBG("Writing %u bytes to addr 0x%x", frag_size, frag_cache[i].addr);
		flash_area_write(fa, frag_cache[i].addr, frag_cache[i].data, frag_size);
	}

	flash_area_close(fa);

	err = boot_request_upgrade(BOOT_UPGRADE_TEST);
	if (err) {
		LOG_ERR("Failed to request upgrade (err %d)", err);
	}
}
