/*
 * Copyright (c) 2022 Martin Jäger <martin@libre.solar>
 * Copyright (c) 2022 tado GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "frag_flash.h"

#include <zephyr/zephyr.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>

LOG_MODULE_REGISTER(lorawan_frag_flash, CONFIG_LORAWAN_SERVICES_LOG_LEVEL);

#define TARGET_IMAGE_AREA FLASH_AREA_ID(image_1)

static const struct flash_area *fa;

int frag_flash_init(void)
{
	int err;

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
	LOG_DBG("Writing %u bytes to addr 0x%x", size, addr);

	return flash_area_write(fa, addr, data, size);
}

int8_t frag_flash_read(uint32_t addr, uint8_t *data, uint32_t size)
{
	return flash_area_read(fa, addr, data, size);
}

void frag_flash_finish(void)
{
	int err;

	flash_area_close(fa);

	LOG_DBG("All fragments written to flash");

	err = boot_request_upgrade(BOOT_UPGRADE_TEST);
	if (err) {
		LOG_ERR("Failed to request upgrade (err %d)", err);
	}
}
