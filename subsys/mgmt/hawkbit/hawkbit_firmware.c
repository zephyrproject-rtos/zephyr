/*
 * Copyright (c) 2020 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/printk.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/dfu/mcuboot_partitions.h>

#include "hawkbit_firmware.h"

bool hawkbit_get_firmware_version(char *version, int version_len)
{
	struct mcuboot_img_header header;

	if (boot_read_bank_header(ZEPHYR_MCUBOOT_APP_0_PRIMARY_SLOT_ID, &header,
		sizeof(header)) != 0) {
		return false;
	}

	if (header.mcuboot_version != 1) {
		return false;
	}

	snprintk(version, version_len, "%d.%d.%d", header.h.v1.sem_ver.major,
		 header.h.v1.sem_ver.minor, header.h.v1.sem_ver.revision);

	return true;
}
