/*
 * Copyright (c) 2020 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <storage/flash_map.h>

#include "hawkbit_firmware.h"

bool hawkbit_get_firmware_version(char *version, int version_len)
{
	struct mcuboot_img_header header;

	if (boot_read_bank_header(FLASH_AREA_ID(image_0), &header,
				  version_len) != 0) {
		return false;
	}

	snprintk(version, version_len, "%d.%d.%d", header.h.v1.sem_ver.major,
		 header.h.v1.sem_ver.minor, header.h.v1.sem_ver.revision);

	return true;
}
