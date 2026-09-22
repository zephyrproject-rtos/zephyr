/*
 * Copyright (c) 2018-2023 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(updatehub, CONFIG_UPDATEHUB_LOG_LEVEL);

#include <zephyr/dfu/mcuboot.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/printk.h>

#include "updatehub_firmware.h"

bool updatehub_get_firmware_version(const uint32_t partition_id,
				    char *version, int version_len)
{
	struct mcuboot_img_header header;

	if (boot_read_bank_header(partition_id, &header,
				  sizeof(struct mcuboot_img_header)) != 0) {
		LOG_DBG("Error when executing boot_read_bank_header function");
		return false;
	}

	if (header.mcuboot_version != 1) {
		LOG_DBG("MCUboot header version not supported!");
		return false;
	}

	snprintk(version, version_len, "%d.%d.%d",
		 header.h.v1.sem_ver.major,
		 header.h.v1.sem_ver.minor,
		 header.h.v1.sem_ver.revision);

	return true;
}
