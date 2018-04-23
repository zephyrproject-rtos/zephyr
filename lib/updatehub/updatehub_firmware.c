/*
 * Copyright (c) 2018 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "updatehub_firmware.h"

bool updatehub_get_firmware_version(char *version, int version_len)
{
	struct mcuboot_img_header header;

	if (boot_read_bank_header(DT_FLASH_AREA_IMAGE_0_ID, &header,
				  version_len) != 0) {
		return false;
	}

	snprintk(version, version_len, "%d.%d.%d",
		 header.h.v1.sem_ver.major,
		 header.h.v1.sem_ver.minor,
		 header.h.v1.sem_ver.revision);

	return true;
}
