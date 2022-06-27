/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>

#include "img_mgmt/image.h"
#include "img_mgmt/img_mgmt.h"

int
img_mgmt_ver_str(const struct image_version *ver, char *dst)
{
	int rc = 0;
	int rc1 = 0;

	rc = snprintf(dst, IMG_MGMT_VER_MAX_STR_LEN, "%hu.%hu.%hu",
		(uint16_t)ver->iv_major, (uint16_t)ver->iv_minor,
		ver->iv_revision);

	if (rc >= 0 && ver->iv_build_num != 0) {
		rc1 = snprintf(&dst[rc], IMG_MGMT_VER_MAX_STR_LEN - rc, ".%u",
			ver->iv_build_num);
	}

	if (rc1 >= 0 && rc >= 0) {
		rc = rc + rc1;
	} else {
		/* If any failed then all failed */
		rc = -1;
	}

	return rc;
}
